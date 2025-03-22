#include "melsec_helper.h"
#include "melsec_mc_bin.h"
#include "melsec_mc_bin_private.h"

#include "socket.h"
#include "error_handler.h"
#include "thread_safe.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#pragma warning(disable:4996)
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

plc_network_address g_network_address;

/**
 * @brief Connect to PLC device
 * @param ip_addr PLC's IP address
 * @param port PLC's port number
 * @param network_addr Network address
 * @param station_addr Station number
 * @param config Optional communication configuration, NULL means use default configuration
 * @return Returns connection descriptor (>0) on success, -1 on failure
 */
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr)
{
	if (ip_addr == NULL || port <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "PLC connection parameter error: IP address is empty or port number is invalid");
		return -1;
	}

	// Initialize thread-safe environment (if not already initialized)
	static bool thread_safe_initialized = false;
	if (!thread_safe_initialized) {
		if (mc_thread_safe_init() != MC_ERROR_CODE_SUCCESS) {
			mc_log_error(MC_ERROR_CODE_FAILED, "Failed to initialize thread-safe environment");
			return -1;
		}
		thread_safe_initialized = true;
	}

	// Lock protection for global network address settings
	mc_mutex_lock(&g_connection_mutex);
	g_network_address.network_number = network_addr;
	g_network_address.station_number = station_addr;
	mc_mutex_unlock(&g_connection_mutex);

	// Create connection using default configuration
	int fd = mc_open_tcp_client_socket(ip_addr, port);
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "Failed to connect to PLC: Unable to establish TCP connection");
		return -1;
	}

	return fd;
}

/**
 * @brief Disconnect from PLC
 * @param fd Connection descriptor
 * @return Returns true on success, false on failure
 */
bool mc_disconnect(int fd)
{
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Disconnect parameter error: Invalid connection descriptor");
		return false;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Close socket and clean up resources
	mc_close_tcp_socket(fd);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);

	return true;
}

//////////////////////////////////////////////////////////////////////////
/**
 * @brief Read boolean array values
 * @param fd Connection descriptor
 * @param address Address string
 * @param length Length to read
 * @param out_bytes Output byte array
 * @return Error code
 */
mc_error_code_e read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || out_bytes == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Read boolean value parameter error: Invalid parameters");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	melsec_mc_address_data address_data;

	// Parse address
	if (!mc_analysis_address(address, length, &address_data)) {
		mc_log_error(MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "Parse address failed: Invalid address format");
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	// Build read core command
	byte_array_info core_cmd = build_read_core_command(address_data, true);
	if (core_cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Build core command failed: Memory allocation error");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Pack MC command failed: Memory allocation error");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Send message with retry mechanism
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// Wait for retry interval
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Send message failed: Network error or connection disconnected");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_read_response(response, out_bytes);
	if (ret == MC_ERROR_CODE_SUCCESS) {
		extract_actual_bool_data(out_bytes);
	}
	else {
		mc_log_error(ret, "Parse read response failed");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief Read word data
 * @param fd Connection descriptor
 * @param address Address string
 * @param length Length to read
 * @param out_bytes Output byte array
 * @return Error code
 */
mc_error_code_e read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || out_bytes == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Read word data parameter error: Invalid parameters");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data)) {
		mc_log_error(MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "Parse address failed: Invalid address format");
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	return read_address_data(fd, address_data, out_bytes);
}

/**
 * @brief Read data from specified address
 * @param fd Connection descriptor
 * @param address_data Parsed address data
 * @param out_bytes Output byte array
 * @return Error code
 */
mc_error_code_e read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes)
{
	if (fd <= 0 || out_bytes == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Read address data parameter error: Invalid parameters");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

	// Build read core command
	byte_array_info core_cmd = build_read_core_command(address_data, false);
	if (core_cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Build core command failed: Memory allocation error");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Pack MC command failed: Memory allocation error");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Send message with retry mechanism
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// Wait for retry interval
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Send message failed: Network error or connection disconnected");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_read_response(response, out_bytes);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Parse read response failed");
	}

	RELEASE_DATA(response.data);
	return ret;
}

//////////////////////////////////////////////////////////////////////////
/**
 * @brief Write boolean array values
 * @param fd Connection descriptor
 * @param address Address string
 * @param length Length to write
 * @param in_bytes Input boolean array
 * @return Error code
 */
mc_error_code_e write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || in_bytes.data == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Write boolean value parameter error: Invalid parameters");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	melsec_mc_address_data address_data;

	// Parse address
	if (!mc_analysis_address(address, length, &address_data)) {
		mc_log_error(MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "Parse address failed: Invalid address format");
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	// Build write core command
	byte_array_info core_cmd = build_write_bit_core_command(address_data, in_bytes);
	RELEASE_DATA(in_bytes.data); // Release input data
	if (core_cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Build core command failed: Memory allocation error");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Pack MC command failed: Memory allocation error");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Send message with retry mechanism
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// Wait for retry interval
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Send message failed: Network error or connection disconnected");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Parse write response failed");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief Write word data
 * @param fd Connection descriptor
 * @param address Address string
 * @param length Length to write
 * @param in_bytes Input byte array
 * @return Error code
 */
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

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);
	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	bool send_ret = mc_try_send_msg(fd, &cmd);
	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Send message failed: Network error or connection disconnected");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Remote PLC run failed: Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Parse write response failed");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief Remote run PLC
 * @param fd Connection descriptor
 * @return Error code
 */
mc_error_code_e mc_remote_run(int fd)
{
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Remote run PLC parameter error: Invalid connection descriptor");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

	// Prepare remote run command
	byte core_cmd_temp[] = { 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);

	// Allocate memory
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	if (core_cmd == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Remote run PLC failed: Memory allocation error");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	// Copy command data
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	// Prepare command information
	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Remote PLC run failed: Command packaging failed");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Send message with retry mechanism
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// Wait for retry interval
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Remote PLC run failed: Command sending failed");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Remote run PLC failed: Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Remote PLC run failed: Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Parse write response failed");
	}

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
	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	bool send_ret = mc_try_send_msg(fd, &cmd);
	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Send message failed: Network error or connection disconnected");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Remote PLC run failed: Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Parse write response failed");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief Remote reset PLC
 * @param fd Connection descriptor
 * @return Error code
 */
mc_error_code_e mc_remote_reset(int fd)
{
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Remote reset PLC parameter error: Invalid connection descriptor");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

	// Prepare remote reset command
	byte core_cmd_temp[] = { 0x06, 0x10, 0x00, 0x00, 0x01, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);

	// Allocate memory
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	if (core_cmd == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Remote reset PLC failed: Memory allocation error");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	// Copy command data
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	// Prepare command information
	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Remote PLC reset failed: Command packaging failed");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Send message with retry mechanism
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// Wait for retry interval
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Remote PLC reset failed: Command sending failed");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// Receive response
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "Remote reset PLC failed: Read response failed");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Remote PLC reset failed: Response header length insufficient");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// Parse response
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Remote PLC reset failed: Response parsing failed");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief Read PLC type
 * @param fd Connection descriptor
 * @param type Output PLC type string
 * @return Error code
 */
mc_error_code_e mc_read_plc_type(int fd, char** type)
{
	if (fd <= 0 || type == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Read PLC type parameter error: Invalid parameters");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info out_bytes;
	memset(&out_bytes, 0, sizeof(out_bytes));

	// Prepare read PLC type command
	byte core_cmd_temp[] = { 0x01, 0x01, 0x00, 0x00, 0x01, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);

	// Allocate memory
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	if (core_cmd == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Read PLC type failed: Memory allocation error");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	// Copy command data
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	// Prepare command information
	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;

	// Lock protection for global network address
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Read PLC type failed: Pack command failed");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Send message with retry mechanism
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// Wait for retry interval
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "Read PLC type failed: Send command failed");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	return MC_ERROR_CODE_SUCCESS;
}
//

/**
 * @brief Package MC protocol command
 * @param mc_core Core command data
 * @param network_number Network number
 * @param station_number Station number
 * @return Packaged command data
 */
byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number)
{
	if (mc_core == NULL || mc_core->data == NULL || mc_core->length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Pack MC command parameter error: Invalid core command data");
		byte_array_info empty_ret = { 0 };
		return empty_ret;
	}

	int core_len = mc_core->length;
	int cmd_len = core_len + 11; // Core command + header command

	// Allocate memory
	byte* command = (byte*)malloc(cmd_len);
	if (command == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Pack MC command failed: Memory allocation error");
		byte_array_info empty_ret = { 0 };
		return empty_ret;
	}

	// Initialize memory
	memset(command, 0, cmd_len);

	// Fill command header
	command[0] = 0x50; // Fixed header
	command[1] = 0x00; // Fixed header
	command[2] = network_number; // Network number
	command[3] = 0xFF; // PLC number
	command[4] = 0xFF; // Request target module IO number
	command[5] = 0x03; // Request target module station number
	command[6] = station_number; // Request data length
	command[7] = (byte)(cmd_len - 9); // Data length low byte
	command[8] = (byte)((cmd_len - 9) >> 8); // Data length high byte
	command[9] = 0x0A; // CPU monitoring timer
	command[10] = 0x00; // CPU monitoring timer

	// Copy core command data
	memcpy(command + 11, mc_core->data, mc_core->length);

	// Return result
	byte_array_info ret = { 0 };
	ret.data = command;
	ret.length = cmd_len;

	return ret;
}

/**
 * @brief Extract actual boolean data
 * @param response Response data, will be modified to actual boolean data
 */
void extract_actual_bool_data(byte_array_info* response)
{
	if (response == NULL || response->data == NULL || response->length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Extract boolean data parameter error: Invalid response data");
		return;
	}

	// Calculate result length and allocate memory
	int resp_len = response->length * 2;
	byte* content = (byte*)malloc(resp_len);
	if (content == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Extract boolean data failed: Memory allocation error");
		// Do not release original data, return as is
		return;
	}

	// Initialize memory
	memset(content, 0, resp_len);

	// Extract boolean values
	for (int i = 0; i < response->length; i++) {
		// Check high bits
		if ((response->data[i] & 0x10) == 0x10) {
			content[i * 2 + 0] = 0x01;
		}

		// Check low bits
		if ((response->data[i] & 0x01) == 0x01) {
			content[i * 2 + 1] = 0x01;
		}
	}

	// Release original data and update response
	RELEASE_DATA(response->data);
	response->data = content;
	response->length = resp_len;
}

/**
 * @brief Read PLC response
 * @param fd Connection descriptor
 * @param response Response data
 * @param read_count Actual bytes read
 * @return Error code
 */
mc_error_code_e mc_read_response(int fd, byte_array_info* response, int* read_count)
{
	if (fd <= 0 || read_count == NULL || response == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Read response parameter error: Invalid parameters");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Allocate memory
	byte* temp = (byte*)malloc(BUFFER_SIZE);
	if (temp == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Memory allocation failed: Unable to allocate memory for response data");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	memset(temp, 0, BUFFER_SIZE);
	response->data = temp;
	response->length = BUFFER_SIZE;

	*read_count = 0;
	char* ptr = (char*)response->data;

	// Get communication configuration
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// Receive data
	*read_count = (int)recv(fd, ptr, response->length, 0);
	if (*read_count < 0) {
		RELEASE_DATA(response->data);
		mc_log_error(MC_ERROR_CODE_SOCKET_RECV_FAILED, "Receive data failed: Network error or connection disconnected");
		return MC_ERROR_CODE_SOCKET_RECV_FAILED;
	}
	else if (*read_count == 0) {
		RELEASE_DATA(response->data);
		mc_log_error(MC_ERROR_CODE_CONNECTION_CLOSED, "Connection closed: Remote device closed the connection");
		return MC_ERROR_CODE_CONNECTION_CLOSED;
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
	ret = read_word_value(fd, address, read_len / 2, &read_data);
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