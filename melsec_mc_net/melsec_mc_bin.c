#include "melsec_helper.h"
#include "melsec_mc_bin.h"
#include "melsec_mc_bin_private.h"

#include "socket.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#pragma warning(disable:4996)
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

#define RELEASE_DATA(addr) { if(addr != NULL) { free(addr);} }

int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr)
{
	int fd = -1;
	g_network_address.network_number = network_addr;
	g_network_address.station_number = station_addr;

	fd = mc_open_tcp_client_socket(ip_addr, port);
	return fd;
}

bool mc_disconnect(int fd)
{
	mc_close_tcp_socket(fd);
	return true;
}

//////////////////////////////////////////////////////////////////////////
mc_error_code_e read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data))
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;

	byte_array_info core_cmd = build_read_core_command(address_data, true);
	if (core_cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd.data);

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_read_response(response, out_bytes);
	if (ret == MC_ERROR_CODE_SUCCESS) {
		extract_actual_bool_data(out_bytes);
	}

	RELEASE_DATA(response.data);
	return ret;
}

mc_error_code_e read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data))
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;

	ushort already_finished = 0;
	return read_address_data(fd, address_data, out_bytes);
}

mc_error_code_e read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info core_cmd = build_read_core_command(address_data, false);
	if (core_cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd.data);

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_read_response(response, out_bytes);
	RELEASE_DATA(response.data);

	return ret;
}

//////////////////////////////////////////////////////////////////////////
mc_error_code_e write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data))
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;

	byte_array_info core_cmd = build_write_bit_core_command(address_data, in_bytes);
	if (core_cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd.data);

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_write_response(response, NULL);
	RELEASE_DATA(response.data);
	return ret;
}

mc_error_code_e write_word_value(int fd, const char* address, int length, byte_array_info in_bytes)
{
	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data))
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;

	return write_address_data(fd, address_data, in_bytes);
}

mc_error_code_e write_address_data(int fd, melsec_mc_address_data address_data, byte_array_info in_bytes)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info core_cmd = build_write_word_core_command(address_data, in_bytes);
	if (core_cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd.data);

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_write_response(response, NULL);
	RELEASE_DATA(response.data);
	return ret;
}

mc_error_code_e mc_remote_run(int fd)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte core_cmd_temp[] = { 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };

	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;
	byte_array_info  cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd);

	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_read_response(response, NULL);
	RELEASE_DATA(response.data);

	return ret;
}

mc_error_code_e mc_remote_stop(int fd)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte core_cmd_temp[] = { 0x02, 0x10, 0x00, 0x00, 0x01, 0x00 };

	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;
	byte_array_info  cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd);
	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_read_response(response, NULL);
	RELEASE_DATA(response.data);

	return ret;
}

mc_error_code_e mc_remote_reset(int fd)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte core_cmd_temp[] = { 0x06, 0x10, 0x00, 0x00, 0x01, 0x00 };

	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;
	byte_array_info  cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd);

	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_read_response(response, NULL);
	RELEASE_DATA(response.data);

	return ret;
}

mc_error_code_e mc_read_plc_type(int fd, char** type)
{
	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info out_bytes;
	memset(&out_bytes, 0, sizeof(out_bytes));

	byte core_cmd_temp[] = { 0x01, 0x01, 0x00, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;
	byte_array_info  cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	free(core_cmd);

	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	if (!mc_try_send_msg(fd, &cmd))
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;

	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS)
	{
		RELEASE_DATA(response.data);
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		free(response.data);
		return MC_ERROR_CODE_RESPONSE_HEADE_FAILED;
	}

	ret = mc_parse_read_response(response, &out_bytes);
	RELEASE_DATA(response.data);

	if (ret == MC_ERROR_CODE_SUCCESS && out_bytes.length > 0)
		*type = (char*)out_bytes.data;

	return ret;
}

byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number)
{
	int core_len = mc_core->length;
	int cmd_len = core_len + 11;
	byte* command = (byte*)malloc(cmd_len);//core command + header command
	memset(command, 0, core_len + 11);

	command[0] = 0x50;					//副标题
	command[1] = 0x00;
	command[2] = network_number;		// 网络号
	command[3] = 0xFF;					// PLC编号
	command[4] = 0xFF;					// 目标模块IO编号
	command[5] = 0x03;
	command[6] = station_number;		// 目标模块站号
	command[7] = (byte)(cmd_len - 9);	// 请求数据长度
	command[8] = (byte)((cmd_len - 9) >> 8);
	command[9] = 0x0A;					// CPU监视定时器
	command[10] = 0x00;
	memcpy(command + 11, mc_core->data, mc_core->length);

	byte_array_info ret;
	ret.data = command;
	ret.length = cmd_len;

	return ret;
}

void extract_actual_bool_data(byte_array_info* response)
{
	// 位读取
	int resp_len = response->length * 2;
	byte* content = (byte*)malloc(resp_len);
	memset(content, 0, resp_len);

	for (int i = 0; i < response->length; i++)
	{
		if ((response->data[i] & 0x10) == 0x10)
			content[i * 2 + 0] = 0x01;

		if ((response->data[i] & 0x01) == 0x01)
			content[i * 2 + 1] = 0x01;
	}
	free(response->data);
	response->data = content;
	response->length = resp_len;
}

mc_error_code_e mc_read_response(int fd, byte_array_info* response, int* read_count)
{
	if (fd < 0 || read_count == 0 || response == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	byte* temp = malloc(BUFFER_SIZE); // 动态分配缓冲区
	if (temp == NULL)
		return MC_ERROR_CODE_MALLOC_FAILED;

	memset(temp, 0, BUFFER_SIZE);
	response->data = temp;
	response->length = BUFFER_SIZE;

	*read_count = 0;
	char* ptr = (char*)response->data;
	if (fd < 0 || response->length <= 0) return -1;
	*read_count = (int)recv(fd, ptr, response->length, 0);
	if (*read_count < 0) {
		return MC_ERROR_CODE_FAILED;
	}
	response->length = *read_count;
	return MC_ERROR_CODE_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

mc_error_code_e mc_read_bool(int fd, const char* address, bool* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_bool_value(fd, address, 1, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length > 0)
	{
		*val = (bool)read_data.data[0];
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_short(int fd, const char* address, short* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 1, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length > 0)
	{
		*val = bytes2short(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_ushort(int fd, const char* address, ushort* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 1, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 2)
	{
		*val = bytes2ushort(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_int32(int fd, const char* address, int32* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 4)
	{
		*val = bytes2int32(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_uint32(int fd, const char* address, uint32* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 2)
	{
		*val = bytes2uint32(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_int64(int fd, const char* address, int64* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 4, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 8)
	{
		*val = bytes2bigInt(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_uint64(int fd, const char* address, uint64* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 4, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 8)
	{
		*val = bytes2ubigInt(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_float(int fd, const char* address, float* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 4)
	{
		*val = bytes2float(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_double(int fd, const char* address, double* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 4, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= 8)
	{
		*val = bytes2double(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

mc_error_code_e mc_read_string(int fd, const char* address, int length, char** val)
{
	if (fd <= 0 || address == NULL || length <= 0)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	int read_len = (length % 2) == 1 ? length + 1 : length;
	ret = read_word_value(fd, address, length / 2, &read_data);
	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= read_len)
	{
		char* ret_str = (char*)malloc(read_len);
		if (ret_str != NULL)
		{
			memset(ret_str, 0, read_len);
			memcpy(ret_str, read_data.data, read_len);
			RELEASE_DATA(read_data.data);
			*val = ret_str;
		}
		else
			ret = MC_ERROR_CODE_MALLOC_FAILED;
	}
	return ret;
}

mc_error_code_e mc_write_bool(int fd, const char* address, bool val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 1;
	bool_array_info write_data;
	bool* data = (bool*)malloc(write_len);
	if (data == NULL)
		return MC_ERROR_CODE_MALLOC_FAILED;

	data[0] = val;

	write_data.data = data;
	write_data.length = write_len;
	return write_bool_value(fd, address, 1, write_data);
}

mc_error_code_e mc_write_short(int fd, const char* address, short val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 2;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	short2bytes(val, write_data.data);
	return write_word_value(fd, address, 1, write_data);
}

mc_error_code_e mc_write_ushort(int fd, const char* address, ushort val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 2;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	ushort2bytes(val, write_data.data);
	return write_word_value(fd, address, 1, write_data);
}

mc_error_code_e mc_write_int32(int fd, const char* address, int32 val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 4;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	int2bytes(val, write_data.data);
	return write_word_value(fd, address, 2, write_data);
}

mc_error_code_e mc_write_uint32(int fd, const char* address, uint32 val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 4;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	uint2bytes(val, write_data.data);
	return write_word_value(fd, address, 2, write_data);
}

mc_error_code_e mc_write_int64(int fd, const char* address, int64 val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;
	int write_len = 8;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	bigInt2bytes(val, write_data.data);
	return write_word_value(fd, address, 4, write_data);
}

mc_error_code_e mc_write_uint64(int fd, const char* address, uint64 val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 8;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	ubigInt2bytes(val, write_data.data);
	return write_word_value(fd, address, 4, write_data);
}

mc_error_code_e mc_write_float(int fd, const char* address, float val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 4;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	float2bytes(val, write_data.data);
	return write_word_value(fd, address, 2, write_data);
}

mc_error_code_e mc_write_double(int fd, const char* address, double val)
{
	if (fd <= 0 || address == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = 8;
	byte_array_info write_data;
	memset(&write_data, 0, sizeof(write_data));
	write_data.data = (byte*)malloc(write_len);
	write_data.length = write_len;

	double2bytes(val, write_data.data);
	return  write_word_value(fd, address, 4, write_data);
}

mc_error_code_e mc_write_string(int fd, const char* address, int length, const char* val)
{
	if (fd <= 0 || address == NULL || val == NULL)
		return MC_ERROR_CODE_INVALID_PARAMETER;

	int write_len = (length % 2) == 1 ? length + 1 : length;
	byte_array_info write_data = { 0 };
	write_data.data = (byte*)malloc(write_len);
	if (write_data.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	memset(write_data.data, 0, write_len);
	memcpy(write_data.data, val, length);
	write_data.length = write_len;

	return write_word_value(fd, address, write_len / 2, write_data);
}