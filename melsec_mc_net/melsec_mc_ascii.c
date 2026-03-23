/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2022-2026 wqliceman. All rights reserved.
 * Maintainer: iceman <wqliceman@gmail.com>
 */

#include "melsec_helper.h"
#include "melsec_mc_ascii.h"

#include "error_handler.h"
#include "socket.h"
#include "thread_safe.h"

#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#else
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static plc_network_address g_ascii_network_address = { 0 };

static int mc_ascii_hex_to_nibble(byte c);
static bool mc_ascii_hex_pair_to_byte(const byte* pair, byte* out);

#define ASCII_MIN_RESPONSE_HEADER_SIZE 22
#define ASCII_MIN_RECV_HEADER_SIZE 18

#define MAX_TX_CONNECTIONS 64
typedef struct {
	int fd;
	bool in_use;
	mc_mutex_t tx_mutex;
	bool tx_mutex_initialized;
} mc_tx_connection_lock_t;

static mc_tx_connection_lock_t g_tx_connection_locks[MAX_TX_CONNECTIONS] = { 0 };

static mc_mutex_t* mc_get_or_create_fd_tx_mutex_locked(int fd)
{
	for (int i = 0; i < MAX_TX_CONNECTIONS; i++) {
		if (g_tx_connection_locks[i].in_use && g_tx_connection_locks[i].fd == fd) {
			return &g_tx_connection_locks[i].tx_mutex;
		}
	}

	for (int i = 0; i < MAX_TX_CONNECTIONS; i++) {
		if (!g_tx_connection_locks[i].in_use) {
			g_tx_connection_locks[i].fd = fd;
			g_tx_connection_locks[i].in_use = true;
			if (!g_tx_connection_locks[i].tx_mutex_initialized) {
				if (mc_mutex_init(&g_tx_connection_locks[i].tx_mutex) != MC_ERROR_CODE_SUCCESS) {
					g_tx_connection_locks[i].in_use = false;
					return NULL;
				}
				g_tx_connection_locks[i].tx_mutex_initialized = true;
			}

			return &g_tx_connection_locks[i].tx_mutex;
		}
	}

	return NULL;
}

static mc_error_code_e mc_lock_fd_transaction(int fd)
{
	if (fd <= 0) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_mutex_t* tx_mutex = NULL;
	mc_mutex_lock(&g_connection_mutex);
	tx_mutex = mc_get_or_create_fd_tx_mutex_locked(fd);
	mc_mutex_unlock(&g_connection_mutex);
	if (tx_mutex == NULL) {
		return MC_ERROR_CODE_FAILED;
	}

	return mc_mutex_lock(tx_mutex);
}

static mc_error_code_e mc_unlock_fd_transaction(int fd)
{
	if (fd <= 0) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_mutex_t* tx_mutex = NULL;
	mc_mutex_lock(&g_connection_mutex);
	for (int i = 0; i < MAX_TX_CONNECTIONS; i++) {
		if (g_tx_connection_locks[i].in_use && g_tx_connection_locks[i].fd == fd) {
			tx_mutex = &g_tx_connection_locks[i].tx_mutex;
			break;
		}
	}
	mc_mutex_unlock(&g_connection_mutex);

	if (tx_mutex == NULL) {
		return MC_ERROR_CODE_FAILED;
	}

	return mc_mutex_unlock(tx_mutex);
}

static byte mc_ascii_to_hex_char(byte nibble)
{
	nibble &= 0x0F;
	return (nibble < 10) ? (byte)('0' + nibble) : (byte)('A' + (nibble - 10));
}

static void mc_ascii_write_hex_byte(byte* out2, byte value)
{
	out2[0] = mc_ascii_to_hex_char((byte)(value >> 4));
	out2[1] = mc_ascii_to_hex_char((byte)(value & 0x0F));
}

static void mc_ascii_write_hex_u16_be(byte* out4, ushort value)
{
	byte hi = (byte)((value >> 8) & 0xFF);
	byte lo = (byte)(value & 0xFF);
	mc_ascii_write_hex_byte(out4 + 0, hi);
	mc_ascii_write_hex_byte(out4 + 2, lo);
}

static bool mc_ascii_parse_u16_be(const byte* ascii4, ushort* out)
{
	if (ascii4 == NULL || out == NULL) {
		return false;
	}

	byte b0 = 0;
	byte b1 = 0;
	if (!mc_ascii_hex_pair_to_byte(ascii4 + 0, &b0)) {
		return false;
	}
	if (!mc_ascii_hex_pair_to_byte(ascii4 + 2, &b1)) {
		return false;
	}

	*out = (ushort)(((ushort)b0 << 8) | b1);
	return true;
}

// Convert binary MC core words (little-endian per word) to ASCII MC core words.
// Example: [0x01,0x04,0x00,0x00] -> "04010000".
static byte_array_info mc_ascii_build_core_from_le_words(const byte* raw_cmd, int raw_cmd_len)
{
	byte_array_info ret = { 0 };
	if (raw_cmd == NULL || raw_cmd_len <= 0 || (raw_cmd_len % 2) != 0) {
		return ret;
	}

	int ascii_len = raw_cmd_len * 2;
	byte* out = (byte*)malloc((size_t)ascii_len + 1);
	if (out == NULL) {
		return ret;
	}

	memset(out, 0, (size_t)ascii_len + 1);
	for (int i = 0; i < raw_cmd_len; i += 2) {
		// A word is [low, high] in binary command payload.
		// ASCII field should be high-byte then low-byte.
		mc_ascii_write_hex_byte(out + i * 2 + 0, raw_cmd[i + 1]);
		mc_ascii_write_hex_byte(out + i * 2 + 2, raw_cmd[i + 0]);
	}

	ret.data = out;
	ret.length = ascii_len;
	return ret;
}

static bool mc_ascii_format_device_number(const melsec_mc_address_data* address_data, byte* out6)
{
	if (address_data == NULL || out6 == NULL) {
		return false;
	}

	if (address_data->address_start < 0) {
		return false;
	}

	if (address_data->data_type.from_base == 16) {
		snprintf((char*)out6, 7, "%06X", (unsigned int)address_data->address_start);
	}
	else {
		snprintf((char*)out6, 7, "%06u", (unsigned int)address_data->address_start);
	}

	return true;
}

static byte_array_info mc_ascii_build_read_core_command(melsec_mc_address_data address_data, bool is_bit)
{
	byte_array_info ret = { 0 };
	byte* command = (byte*)malloc(21);
	if (command == NULL) {
		return ret;
	}

	memset(command, 0, 21);
	memcpy(command + 0, "0401", 4);
	memcpy(command + 4, is_bit ? "0001" : "0000", 4);
	command[8] = (byte)address_data.data_type.ascii_code[0];
	command[9] = (byte)address_data.data_type.ascii_code[1];
	if (!mc_ascii_format_device_number(&address_data, command + 10)) {
		RELEASE_DATA(command);
		return ret;
	}
	mc_ascii_write_hex_u16_be(command + 16, (ushort)address_data.length);

	ret.data = command;
	ret.length = 20;
	return ret;
}

static byte_array_info mc_ascii_build_write_word_core_command(melsec_mc_address_data address_data, byte_array_info value)
{
	byte_array_info ret = { 0 };
	if (value.data == NULL || value.length <= 0) {
		return ret;
	}
	if ((value.length % 2) != 0) {
		return ret;
	}

	byte* payload_bytes = (byte*)malloc((size_t)value.length);
	if (payload_bytes == NULL) {
		return ret;
	}
	memcpy(payload_bytes, value.data, (size_t)value.length);

	// ASCII 3E word data uses high-byte then low-byte for each word.
	for (int i = 0; i < value.length; i += 2) {
		byte t = payload_bytes[i];
		payload_bytes[i] = payload_bytes[i + 1];
		payload_bytes[i + 1] = t;
	}

	byte_array_info payload = build_ascii_bytes_from_byte_array(payload_bytes, value.length);
	RELEASE_DATA(payload_bytes);
	if (payload.data == NULL || payload.length <= 0) {
		return ret;
	}

	int cmd_len = 20 + payload.length;
	byte* command = (byte*)malloc((size_t)cmd_len + 1);
	if (command == NULL) {
		RELEASE_DATA(payload.data);
		return ret;
	}

	memset(command, 0, (size_t)cmd_len + 1);
	memcpy(command + 0, "1401", 4);
	memcpy(command + 4, "0000", 4);
	command[8] = (byte)address_data.data_type.ascii_code[0];
	command[9] = (byte)address_data.data_type.ascii_code[1];
	if (!mc_ascii_format_device_number(&address_data, command + 10)) {
		RELEASE_DATA(payload.data);
		RELEASE_DATA(command);
		return ret;
	}
	mc_ascii_write_hex_u16_be(command + 16, (ushort)address_data.length);
	memcpy(command + 20, payload.data, (size_t)payload.length);
	RELEASE_DATA(payload.data);

	ret.data = command;
	ret.length = cmd_len;
	return ret;
}

static byte_array_info mc_ascii_build_write_bit_core_command(melsec_mc_address_data address_data, bool_array_info value)
{
	byte_array_info ret = { 0 };
	if (value.data == NULL || value.length <= 0) {
		return ret;
	}

	byte_array_info payload = build_ascii_bytes_from_bool_array(value.data, value.length);
	if (payload.data == NULL || payload.length <= 0) {
		return ret;
	}

	int cmd_len = 20 + payload.length;
	byte* command = (byte*)malloc((size_t)cmd_len + 1);
	if (command == NULL) {
		RELEASE_DATA(payload.data);
		return ret;
	}

	memset(command, 0, (size_t)cmd_len + 1);
	memcpy(command + 0, "1401", 4);
	memcpy(command + 4, "0001", 4);
	command[8] = (byte)address_data.data_type.ascii_code[0];
	command[9] = (byte)address_data.data_type.ascii_code[1];
	if (!mc_ascii_format_device_number(&address_data, command + 10)) {
		RELEASE_DATA(payload.data);
		RELEASE_DATA(command);
		return ret;
	}
	mc_ascii_write_hex_u16_be(command + 16, (ushort)address_data.length);
	memcpy(command + 20, payload.data, (size_t)payload.length);
	RELEASE_DATA(payload.data);

	ret.data = command;
	ret.length = cmd_len;
	return ret;
}

static int mc_ascii_hex_to_nibble(byte c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}
	return -1;
}

static bool mc_ascii_hex_pair_to_byte(const byte* pair, byte* out)
{
	if (pair == NULL || out == NULL) {
		return false;
	}

	int hi = mc_ascii_hex_to_nibble(pair[0]);
	int lo = mc_ascii_hex_to_nibble(pair[1]);
	if (hi < 0 || lo < 0) {
		return false;
	}

	*out = (byte)((hi << 4) | lo);
	return true;
}

static mc_error_code_e mc_recv_exact(int fd, byte* buffer, int bytes_to_read)
{
	int total_read = 0;
	while (total_read < bytes_to_read) {
		int nread = (int)recv(fd, (char*)buffer + total_read, (size_t)(bytes_to_read - total_read), 0);
		if (nread == 0) {
			return MC_ERROR_CODE_CONNECTION_CLOSED;
		}
		if (nread < 0) {
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEINTR) {
				continue;
			}
#else
			if (errno == EINTR) {
				continue;
			}
#endif
			return MC_ERROR_CODE_SOCKET_RECV_FAILED;
		}

		total_read += nread;
	}

	return MC_ERROR_CODE_SUCCESS;
}

static byte_array_info pack_mc_ascii_command(byte_array_info* mc_core, byte network_number, byte station_number)
{
	byte_array_info empty_ret = { 0 };
	if (mc_core == NULL || mc_core->data == NULL || mc_core->length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Pack ASCII MC command parameter error");
		return empty_ret;
	}

	int cmd_len = 22 + mc_core->length;
	byte* command = (byte*)malloc((size_t)cmd_len + 1);
	if (command == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Pack ASCII MC command malloc failed");
		return empty_ret;
	}

	memset(command, 0, (size_t)cmd_len + 1);
	command[0] = '5';
	command[1] = '0';
	command[2] = '0';
	command[3] = '0';

	mc_ascii_write_hex_byte(command + 4, network_number);

	command[6] = 'F';
	command[7] = 'F';
	command[8] = '0';
	command[9] = '3';
	command[10] = 'F';
	command[11] = 'F';

	mc_ascii_write_hex_byte(command + 12, station_number);

	ushort data_len = (ushort)(mc_core->length + 4);
	mc_ascii_write_hex_u16_be(command + 14, data_len);

	command[18] = '0';
	command[19] = '0';
	command[20] = '1';
	command[21] = '0';

	memcpy(command + 22, mc_core->data, (size_t)mc_core->length);

	byte_array_info ret = { 0 };
	ret.data = command;
	ret.length = cmd_len;
	return ret;
}

static mc_error_code_e mc_ascii_read_response(int fd, byte_array_info* response, int* read_count)
{
	if (fd <= 0 || response == NULL || read_count == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte header[ASCII_MIN_RECV_HEADER_SIZE] = { 0 };
	mc_error_code_e ret = mc_recv_exact(fd, header, ASCII_MIN_RECV_HEADER_SIZE);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Receive ASCII response header failed");
		return ret;
	}

	ushort payload_with_end_code = 0;
	if (!mc_ascii_parse_u16_be(header + 14, &payload_with_end_code) || payload_with_end_code < 4) {
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Invalid ASCII response length field");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	int total_len = ASCII_MIN_RECV_HEADER_SIZE + (int)payload_with_end_code;
	if (total_len < ASCII_MIN_RESPONSE_HEADER_SIZE) {
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Calculated ASCII response length invalid");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	byte* temp = (byte*)malloc((size_t)total_len + 1);
	if (temp == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "ASCII response malloc failed");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	memset(temp, 0, (size_t)total_len + 1);
	memcpy(temp, header, ASCII_MIN_RECV_HEADER_SIZE);

	int remain = total_len - ASCII_MIN_RECV_HEADER_SIZE;
	if (remain > 0) {
		ret = mc_recv_exact(fd, temp + ASCII_MIN_RECV_HEADER_SIZE, remain);
		if (ret != MC_ERROR_CODE_SUCCESS) {
			RELEASE_DATA(temp);
			return ret;
		}
	}

	response->data = temp;
	response->length = total_len;
	*read_count = total_len;
	return MC_ERROR_CODE_SUCCESS;
}

static mc_error_code_e mc_ascii_parse_response_end_code(byte_array_info response)
{
	if (response.data == NULL || response.length < ASCII_MIN_RESPONSE_HEADER_SIZE) {
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	ushort end_code = 0;
	if (!mc_ascii_parse_u16_be(response.data + 18, &end_code)) {
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	return (end_code == 0) ? MC_ERROR_CODE_SUCCESS : MC_ERROR_CODE_FAILED;
}

static mc_error_code_e mc_ascii_parse_read_response(byte_array_info response, byte_array_info* out_data, bool is_bit)
{
	if (out_data == NULL || response.data == NULL || response.length < ASCII_MIN_RESPONSE_HEADER_SIZE) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	out_data->data = NULL;
	out_data->length = 0;

	mc_error_code_e ret = mc_ascii_parse_response_end_code(response);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		return ret;
	}

	int payload_ascii_len = response.length - ASCII_MIN_RESPONSE_HEADER_SIZE;
	if (payload_ascii_len <= 0) {
		return MC_ERROR_CODE_SUCCESS;
	}

	if (is_bit) {
		byte* bits = (byte*)malloc((size_t)payload_ascii_len);
		if (bits == NULL) {
			return MC_ERROR_CODE_MALLOC_FAILED;
		}

		for (int i = 0; i < payload_ascii_len; i++) {
			bits[i] = (response.data[ASCII_MIN_RESPONSE_HEADER_SIZE + i] == '1') ? 0x01 : 0x00;
		}

		out_data->data = bits;
		out_data->length = payload_ascii_len;
		return MC_ERROR_CODE_SUCCESS;
	}

	if ((payload_ascii_len % 2) != 0) {
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	int byte_len = payload_ascii_len / 2;
	byte* data = (byte*)malloc((size_t)byte_len);
	if (data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	for (int i = 0; i < byte_len; i++) {
		if (!mc_ascii_hex_pair_to_byte(response.data + ASCII_MIN_RESPONSE_HEADER_SIZE + i * 2, &data[i])) {
			RELEASE_DATA(data);
			return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
		}
	}

	// Convert from ASCII word byte order (high,low) to native little-endian bytes.
	if ((byte_len % 2) == 0) {
		for (int i = 0; i < byte_len; i += 2) {
			byte t = data[i];
			data[i] = data[i + 1];
			data[i + 1] = t;
		}
	}

	out_data->data = data;
	out_data->length = byte_len;
	return MC_ERROR_CODE_SUCCESS;
}

static mc_error_code_e mc_ascii_parse_write_response(byte_array_info response, byte_array_info* out_data)
{
	mc_error_code_e ret = mc_ascii_parse_response_end_code(response);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		return ret;
	}

	if (out_data == NULL) {
		return MC_ERROR_CODE_SUCCESS;
	}

	out_data->data = NULL;
	out_data->length = 0;
	int payload_ascii_len = response.length - ASCII_MIN_RESPONSE_HEADER_SIZE;
	if (payload_ascii_len <= 0) {
		return MC_ERROR_CODE_SUCCESS;
	}

	if ((payload_ascii_len % 2) != 0) {
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	int byte_len = payload_ascii_len / 2;
	byte* data = (byte*)malloc((size_t)byte_len);
	if (data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	for (int i = 0; i < byte_len; i++) {
		if (!mc_ascii_hex_pair_to_byte(response.data + ASCII_MIN_RESPONSE_HEADER_SIZE + i * 2, &data[i])) {
			RELEASE_DATA(data);
			return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
		}
	}

	out_data->data = data;
	out_data->length = byte_len;
	return MC_ERROR_CODE_SUCCESS;
}

static mc_error_code_e mc_send_and_receive_transaction(int fd, byte_array_info* cmd, byte_array_info* response, int* recv_size)
{
	if (fd <= 0 || cmd == NULL || cmd->data == NULL || cmd->length <= 0 || response == NULL || recv_size == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = mc_lock_fd_transaction(fd);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		return ret;
	}

	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, cmd);
		if (send_ret) {
			break;
		}

		if (i < config.retry_count) {
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	if (!send_ret) {
		ret = MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}
	else {
		ret = mc_ascii_read_response(fd, response, recv_size);
	}

	mc_error_code_e unlock_ret = mc_unlock_fd_transaction(fd);
	if (ret == MC_ERROR_CODE_SUCCESS && unlock_ret != MC_ERROR_CODE_SUCCESS) {
		ret = unlock_ret;
	}

	return ret;
}

static mc_error_code_e read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || out_bytes == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data)) {
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	byte_array_info core_cmd = mc_ascii_build_read_core_command(address_data, true);
	if (core_cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_ascii_command(&core_cmd, g_ascii_network_address.network_number, g_ascii_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	byte_array_info response = { 0 };
	int recv_size = 0;
	mc_error_code_e ret = mc_send_and_receive_transaction(fd, &cmd, &response, &recv_size);
	RELEASE_DATA(cmd.data);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < ASCII_MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	ret = mc_ascii_parse_read_response(response, out_bytes, true);
	RELEASE_DATA(response.data);
	return ret;
}

static mc_error_code_e read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || out_bytes == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data)) {
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	byte_array_info core_cmd = mc_ascii_build_read_core_command(address_data, false);
	if (core_cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_ascii_command(&core_cmd, g_ascii_network_address.network_number, g_ascii_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	byte_array_info response = { 0 };
	int recv_size = 0;
	mc_error_code_e ret = mc_send_and_receive_transaction(fd, &cmd, &response, &recv_size);
	RELEASE_DATA(cmd.data);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < ASCII_MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	ret = mc_ascii_parse_read_response(response, out_bytes, false);
	RELEASE_DATA(response.data);
	return ret;
}

static mc_error_code_e write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || in_bytes.data == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data)) {
		RELEASE_DATA(in_bytes.data);
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	byte_array_info core_cmd = mc_ascii_build_write_bit_core_command(address_data, in_bytes);
	RELEASE_DATA(in_bytes.data);
	if (core_cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_ascii_command(&core_cmd, g_ascii_network_address.network_number, g_ascii_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	byte_array_info response = { 0 };
	int recv_size = 0;
	mc_error_code_e ret = mc_send_and_receive_transaction(fd, &cmd, &response, &recv_size);
	RELEASE_DATA(cmd.data);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		return ret;
	}

	ret = mc_ascii_parse_write_response(response, NULL);
	RELEASE_DATA(response.data);
	return ret;
}

static mc_error_code_e write_word_value(int fd, const char* address, int length, byte_array_info in_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || in_bytes.data == NULL) {
		RELEASE_DATA(in_bytes.data);
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data)) {
		RELEASE_DATA(in_bytes.data);
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	byte_array_info core_cmd = mc_ascii_build_write_word_core_command(address_data, in_bytes);
	RELEASE_DATA(in_bytes.data);
	if (core_cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_ascii_command(&core_cmd, g_ascii_network_address.network_number, g_ascii_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	byte_array_info response = { 0 };
	int recv_size = 0;
	mc_error_code_e ret = mc_send_and_receive_transaction(fd, &cmd, &response, &recv_size);
	RELEASE_DATA(cmd.data);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		return ret;
	}

	ret = mc_ascii_parse_write_response(response, NULL);
	RELEASE_DATA(response.data);
	return ret;
}

static mc_error_code_e mc_ascii_execute_raw_command(int fd, const byte* raw_cmd, int raw_cmd_len, byte_array_info* out_payload)
{
	if (fd <= 0 || raw_cmd == NULL || raw_cmd_len <= 0) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info core_cmd = mc_ascii_build_core_from_le_words(raw_cmd, raw_cmd_len);
	if (core_cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_ascii_command(&core_cmd, g_ascii_network_address.network_number, g_ascii_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	byte_array_info response = { 0 };
	int recv_size = 0;
	mc_error_code_e ret = mc_send_and_receive_transaction(fd, &cmd, &response, &recv_size);
	RELEASE_DATA(cmd.data);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		return ret;
	}

	ret = mc_ascii_parse_write_response(response, out_payload);
	RELEASE_DATA(response.data);
	return ret;
}

int mc_ascii_connect(char* ip_addr, int port, byte network_addr, byte station_addr)
{
	if (ip_addr == NULL || port <= 0) {
		return -1;
	}

	static bool thread_safe_initialized = false;
	if (!thread_safe_initialized) {
		if (mc_thread_safe_init() != MC_ERROR_CODE_SUCCESS) {
			return -1;
		}
		thread_safe_initialized = true;
	}

	mc_mutex_lock(&g_connection_mutex);
	g_ascii_network_address.network_number = network_addr;
	g_ascii_network_address.station_number = station_addr;
	mc_mutex_unlock(&g_connection_mutex);

	return mc_open_tcp_client_socket(ip_addr, port);
}

bool mc_ascii_disconnect(int fd)
{
	if (fd <= 0) {
		return false;
	}

	mc_mutex_lock(&g_connection_mutex);
	mc_close_tcp_socket(fd);
	mc_mutex_unlock(&g_connection_mutex);
	return true;
}

mc_error_code_e mc_ascii_read_bool(int fd, const char* address, bool* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_bool_value(fd, address, 1, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length > 0) {
		*val = (read_data.data[0] != 0);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_short(int fd, const char* address, short* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 1, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 2) {
		*val = bytes2short(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_ushort(int fd, const char* address, ushort* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 1, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 2) {
		*val = bytes2ushort(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_int32(int fd, const char* address, int32* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 4) {
		*val = bytes2int32(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_uint32(int fd, const char* address, uint32* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 4) {
		*val = bytes2uint32(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_int64(int fd, const char* address, int64* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 4, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 8) {
		*val = bytes2bigInt(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_uint64(int fd, const char* address, uint64* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 4, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 8) {
		*val = bytes2ubigInt(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_float(int fd, const char* address, float* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 4) {
		*val = bytes2float(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_double(int fd, const char* address, double* val)
{
	if (fd <= 0 || address == NULL || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, 4, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 8) {
		*val = bytes2double(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_ascii_read_string(int fd, const char* address, int length, char** val)
{
	if (fd <= 0 || address == NULL || length <= 0 || val == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	*val = NULL;
	int read_len = ((length % 2) == 1) ? (length + 1) : length;
	byte_array_info read_data = { 0 };
	mc_error_code_e ret = read_word_value(fd, address, read_len / 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= read_len) {
		char* out = (char*)malloc((size_t)read_len + 1);
		if (out == NULL) {
			RELEASE_DATA(read_data.data);
			return MC_ERROR_CODE_MALLOC_FAILED;
		}

		memset(out, 0, (size_t)read_len + 1);
		memcpy(out, read_data.data, (size_t)length);
		RELEASE_DATA(read_data.data);
		*val = out;
	}
	return ret;
}

mc_error_code_e mc_ascii_write_bool(int fd, const char* address, bool val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	bool* data = (bool*)malloc(1);
	if (data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	data[0] = val;
	bool_array_info write_data = { 0 };
	write_data.data = data;
	write_data.length = 1;
	return write_bool_value(fd, address, 1, write_data);
}

mc_error_code_e mc_ascii_write_short(int fd, const char* address, short val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(2);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 2;
	short2bytes(val, write_data.data);
	return write_word_value(fd, address, 1, write_data);
}

mc_error_code_e mc_ascii_write_ushort(int fd, const char* address, ushort val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(2);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 2;
	ushort2bytes(val, write_data.data);
	return write_word_value(fd, address, 1, write_data);
}

mc_error_code_e mc_ascii_write_int32(int fd, const char* address, int32 val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(4);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 4;
	int2bytes(val, write_data.data);
	return write_word_value(fd, address, 2, write_data);
}

mc_error_code_e mc_ascii_write_uint32(int fd, const char* address, uint32 val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(4);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 4;
	uint2bytes(val, write_data.data);
	return write_word_value(fd, address, 2, write_data);
}

mc_error_code_e mc_ascii_write_int64(int fd, const char* address, int64 val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(8);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 8;
	bigInt2bytes(val, write_data.data);
	return write_word_value(fd, address, 4, write_data);
}

mc_error_code_e mc_ascii_write_uint64(int fd, const char* address, uint64 val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(8);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 8;
	ubigInt2bytes(val, write_data.data);
	return write_word_value(fd, address, 4, write_data);
}

mc_error_code_e mc_ascii_write_float(int fd, const char* address, float val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(4);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 4;
	float2bytes(val, write_data.data);
	return write_word_value(fd, address, 2, write_data);
}

mc_error_code_e mc_ascii_write_double(int fd, const char* address, double val)
{
	if (fd <= 0 || address == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(8);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	write_data.length = 8;
	double2bytes(val, write_data.data);
	return write_word_value(fd, address, 4, write_data);
}

mc_error_code_e mc_ascii_write_string(int fd, const char* address, int length, const char* val)
{
	if (fd <= 0 || address == NULL || val == NULL || length <= 0) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	int write_len = ((length % 2) == 1) ? (length + 1) : length;
	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc((size_t)write_len);
	if (write_data.data == NULL) {
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	memset(write_data.data, 0, (size_t)write_len);
	memcpy(write_data.data, val, (size_t)length);
	write_data.length = write_len;
	return write_word_value(fd, address, write_len / 2, write_data);
}

mc_error_code_e mc_ascii_remote_run(int fd)
{
	byte raw_cmd[] = { 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
	return mc_ascii_execute_raw_command(fd, raw_cmd, (int)sizeof(raw_cmd), NULL);
}

mc_error_code_e mc_ascii_remote_stop(int fd)
{
	byte raw_cmd[] = { 0x02, 0x10, 0x00, 0x00, 0x01, 0x00 };
	return mc_ascii_execute_raw_command(fd, raw_cmd, (int)sizeof(raw_cmd), NULL);
}

mc_error_code_e mc_ascii_remote_reset(int fd)
{
	byte raw_cmd[] = { 0x06, 0x10, 0x00, 0x00, 0x01, 0x00 };
	return mc_ascii_execute_raw_command(fd, raw_cmd, (int)sizeof(raw_cmd), NULL);
}

mc_error_code_e mc_ascii_read_plc_type(int fd, char** type)
{
	if (type == NULL) {
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	*type = NULL;
	byte raw_cmd[] = { 0x01, 0x01, 0x00, 0x00, 0x01, 0x00 };
	byte_array_info payload = { 0 };
	mc_error_code_e ret = mc_ascii_execute_raw_command(fd, raw_cmd, (int)sizeof(raw_cmd), &payload);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(payload.data);
		return ret;
	}

	char* out = (char*)malloc((size_t)payload.length + 1);
	if (out == NULL) {
		RELEASE_DATA(payload.data);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	memset(out, 0, (size_t)payload.length + 1);
	if (payload.length > 0) {
		memcpy(out, payload.data, (size_t)payload.length);
	}

	*type = out;
	RELEASE_DATA(payload.data);
	return MC_ERROR_CODE_SUCCESS;
}
