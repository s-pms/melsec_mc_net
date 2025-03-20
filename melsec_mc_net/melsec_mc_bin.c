#include "melsec_helper.h"
#include "melsec_mc_bin.h"
#include "melsec_mc_bin_private.h"

#include "socket.h"
#include "error_handler.h"
#include "thread_safe.h"
#include "comm_config.h"
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
 * @brief 连接到PLC设备
 * @param ip_addr PLC的IP地址
 * @param port PLC的端口号
 * @param network_addr 网络地址
 * @param station_addr 站号
 * @param config 可选的通信配置，NULL表示使用默认配置
 * @return 成功返回连接描述符(>0)，失败返回-1
 */
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr)
{
	if (ip_addr == NULL || port <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "连接PLC参数错误: IP地址为空或端口号无效");
		return -1;
	}

	// 初始化线程安全环境（如果尚未初始化）
	static bool thread_safe_initialized = false;
	if (!thread_safe_initialized) {
		if (mc_thread_safe_init() != MC_ERROR_CODE_SUCCESS) {
			mc_log_error(MC_ERROR_CODE_FAILED, "初始化线程安全环境失败");
			return -1;
		}
		thread_safe_initialized = true;
	}

	// 加锁保护全局网络地址设置
	mc_mutex_lock(&g_connection_mutex);
	g_network_address.network_number = network_addr;
	g_network_address.station_number = station_addr;
	mc_mutex_unlock(&g_connection_mutex);

	// 使用默认配置创建连接
	int fd = mc_open_tcp_client_socket(ip_addr, port);
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "连接PLC失败: 无法建立TCP连接");
		return -1;
	}

	return fd;
}

/**
 * @brief 断开与PLC的连接
 * @param fd 连接描述符
 * @return 成功返回true，失败返回false
 */
bool mc_disconnect(int fd)
{
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "断开连接参数错误: 无效的连接描述符");
		return false;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 关闭套接字并清理资源
	mc_close_tcp_socket(fd);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);

	return true;
}

//////////////////////////////////////////////////////////////////////////
/**
 * @brief 读取布尔数组值
 * @param fd 连接描述符
 * @param address 地址字符串
 * @param length 要读取的长度
 * @param out_bytes 输出的字节数组
 * @return 错误码
 */
mc_error_code_e read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || out_bytes == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "读取布尔值参数错误: 无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	melsec_mc_address_data address_data;

	// 解析地址
	if (!mc_analysis_address(address, length, &address_data)) {
		mc_log_error(MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "解析地址失败: 无效的地址格式");
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	// 构建读取核心命令
	byte_array_info core_cmd = build_read_core_command(address_data, true);
	if (core_cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "构建核心命令失败: 内存分配错误");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "打包MC命令失败: 内存分配错误");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 发送消息，带重试机制
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// 等待重试间隔
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败: 网络错误或连接断开");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_read_response(response, out_bytes);
	if (ret == MC_ERROR_CODE_SUCCESS) {
		extract_actual_bool_data(out_bytes);
	}
	else {
		mc_log_error(ret, "解析读取响应失败");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief 读取字数据
 * @param fd 连接描述符
 * @param address 地址字符串
 * @param length 要读取的长度
 * @param out_bytes 输出的字节数组
 * @return 错误码
 */
mc_error_code_e read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || out_bytes == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "读取字数据参数错误: 无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	melsec_mc_address_data address_data;
	if (!mc_analysis_address(address, length, &address_data)) {
		mc_log_error(MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "解析地址失败: 无效的地址格式");
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	return read_address_data(fd, address_data, out_bytes);
}

/**
 * @brief 从指定地址读取数据
 * @param fd 连接描述符
 * @param address_data 已解析的地址数据
 * @param out_bytes 输出的字节数组
 * @return 错误码
 */
mc_error_code_e read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes)
{
	if (fd <= 0 || out_bytes == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "读取地址数据参数错误: 无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

	// 构建读取核心命令
	byte_array_info core_cmd = build_read_core_command(address_data, false);
	if (core_cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "构建核心命令失败: 内存分配错误");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "打包MC命令失败: 内存分配错误");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 发送消息，带重试机制
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// 等待重试间隔
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败: 网络错误或连接断开");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_read_response(response, out_bytes);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "解析读取响应失败");
	}

	RELEASE_DATA(response.data);
	return ret;
}

//////////////////////////////////////////////////////////////////////////
/**
 * @brief 写入布尔数组值
 * @param fd 连接描述符
 * @param address 地址字符串
 * @param length 要写入的长度
 * @param in_bytes 输入的布尔数组
 * @return 错误码
 */
mc_error_code_e write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes)
{
	if (fd <= 0 || address == NULL || length <= 0 || in_bytes.data == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "写入布尔值参数错误: 无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	melsec_mc_address_data address_data;

	// 解析地址
	if (!mc_analysis_address(address, length, &address_data)) {
		mc_log_error(MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "解析地址失败: 无效的地址格式");
		return MC_ERROR_CODE_PARSE_ADDRESS_FAILED;
	}

	// 构建写入核心命令
	byte_array_info core_cmd = build_write_bit_core_command(address_data, in_bytes);
	RELEASE_DATA(in_bytes.data); // 释放输入数据
	if (core_cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "构建核心命令失败: 内存分配错误");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "打包MC命令失败: 内存分配错误");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 发送消息，带重试机制
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// 等待重试间隔
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败: 网络错误或连接断开");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "解析写入响应失败");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief 写入字数据
 * @param fd 连接描述符
 * @param address 地址字符串
 * @param length 要写入的长度
 * @param in_bytes 输入的字节数组
 * @return 错误码
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

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);
	RELEASE_DATA(core_cmd.data);
	if (cmd.data == NULL)
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;

	bool send_ret = mc_try_send_msg(fd, &cmd);
	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败: 网络错误或连接断开");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "远程运行PLC失败: 响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "解析写入响应失败");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief 远程运行PLC
 * @param fd 连接描述符
 * @return 错误码
 */
mc_error_code_e mc_remote_run(int fd)
{
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "远程运行PLC参数错误: 无效的连接描述符");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

	// 准备远程运行命令
	byte core_cmd_temp[] = { 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);

	// 分配内存
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	if (core_cmd == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "远程运行PLC失败: 内存分配错误");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	// 复制命令数据
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	// 准备命令信息
	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "远程运行PLC失败: 打包命令失败");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 发送消息，带重试机制
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// 等待重试间隔
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "远程运行PLC失败: 发送命令失败");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "远程运行PLC失败: 读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "远程运行PLC失败: 响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "解析写入响应失败");
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
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败: 网络错误或连接断开");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "远程运行PLC失败: 响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "解析写入响应失败");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief 远程复位PLC
 * @param fd 连接描述符
 * @return 错误码
 */
mc_error_code_e mc_remote_reset(int fd)
{
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "远程复位PLC参数错误: 无效的连接描述符");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

	// 准备远程复位命令
	byte core_cmd_temp[] = { 0x06, 0x10, 0x00, 0x00, 0x01, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);

	// 分配内存
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	if (core_cmd == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "远程复位PLC失败: 内存分配错误");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	// 复制命令数据
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	// 准备命令信息
	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "远程复位PLC失败: 打包命令失败");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 发送消息，带重试机制
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// 等待重试间隔
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "远程复位PLC失败: 发送命令失败");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	// 接收响应
	byte_array_info response = { 0 };
	int recv_size = 0;
	ret = mc_read_response(fd, &response, &recv_size);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		RELEASE_DATA(response.data);
		mc_log_error(ret, "远程复位PLC失败: 读取响应失败");
		return ret;
	}

	if (recv_size < MIN_RESPONSE_HEADER_SIZE) {
		RELEASE_DATA(response.data);
		mc_log_error(MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "远程复位PLC失败: 响应头部长度不足");
		return MC_ERROR_CODE_RESPONSE_HEADER_FAILED;
	}

	// 解析响应
	ret = mc_parse_write_response(response, NULL);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "远程复位PLC失败: 解析响应失败");
	}

	RELEASE_DATA(response.data);
	return ret;
}

/**
 * @brief 读取PLC类型
 * @param fd 连接描述符
 * @param type 输出的PLC类型字符串
 * @return 错误码
 */
mc_error_code_e mc_read_plc_type(int fd, char** type)
{
	if (fd <= 0 || type == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "读取PLC类型参数错误: 无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info out_bytes;
	memset(&out_bytes, 0, sizeof(out_bytes));

	// 准备读取PLC类型命令
	byte core_cmd_temp[] = { 0x01, 0x01, 0x00, 0x00, 0x01, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);

	// 分配内存
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	if (core_cmd == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "读取PLC类型失败: 内存分配错误");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	// 复制命令数据
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	// 准备命令信息
	byte_array_info temp_info = { 0 };
	temp_info.data = core_cmd;
	temp_info.length = core_cmd_len;

	// 加锁保护全局网络地址
	mc_mutex_lock(&g_connection_mutex);
	byte_array_info cmd = pack_mc_command(&temp_info, g_network_address.network_number, g_network_address.station_number);
	mc_mutex_unlock(&g_connection_mutex);

	RELEASE_DATA(core_cmd);
	if (cmd.data == NULL) {
		mc_log_error(MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "读取PLC类型失败: 打包命令失败");
		return MC_ERROR_CODE_BUILD_CORE_CMD_FAILED;
	}

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 发送消息，带重试机制
	bool send_ret = false;
	for (int i = 0; i <= config.retry_count; i++) {
		send_ret = mc_try_send_msg(fd, &cmd);
		if (send_ret) break;

		if (i < config.retry_count) {
			// 等待重试间隔
#ifdef _WIN32
			Sleep(config.retry_interval_ms);
#else
			usleep(config.retry_interval_ms * 1000);
#endif
		}
	}

	RELEASE_DATA(cmd.data);
	if (!send_ret) {
		mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "读取PLC类型失败: 发送命令失败");
		return MC_ERROR_CODE_SOCKET_SEND_FAILED;
	}

	return MC_ERROR_CODE_SUCCESS;
}
//

/**
 * @brief 打包MC协议命令
 * @param mc_core 核心命令数据
 * @param network_number 网络号
 * @param station_number 站号
 * @return 打包后的命令数据
 */
byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number)
{
	if (mc_core == NULL || mc_core->data == NULL || mc_core->length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "打包MC命令参数错误: 无效的核心命令数据");
		byte_array_info empty_ret = { 0 };
		return empty_ret;
	}

	int core_len = mc_core->length;
	int cmd_len = core_len + 11; // 核心命令 + 头部命令

	// 分配内存
	byte* command = (byte*)malloc(cmd_len);
	if (command == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "打包MC命令失败: 内存分配错误");
		byte_array_info empty_ret = { 0 };
		return empty_ret;
	}

	// 初始化内存
	memset(command, 0, cmd_len);

	// 填充命令头部
	command[0] = 0x50; // 固定头
	command[1] = 0x00; // 固定头
	command[2] = network_number; // 网络号
	command[3] = 0xFF; // PLC编号
	command[4] = 0xFF; // 请求目标模块IO编号
	command[5] = 0x03; // 请求目标模块站号
	command[6] = station_number; // 请求数据长度
	command[7] = (byte)(cmd_len - 9); // 数据长度低字节
	command[8] = (byte)((cmd_len - 9) >> 8); // 数据长度高字节
	command[9] = 0x0A; // CPU监视定时器
	command[10] = 0x00; // CPU监视定时器

	// 复制核心命令数据
	memcpy(command + 11, mc_core->data, mc_core->length);

	// 返回结果
	byte_array_info ret = { 0 };
	ret.data = command;
	ret.length = cmd_len;

	return ret;
}

/**
 * @brief 提取实际的布尔数据
 * @param response 响应数据，将被修改为实际的布尔数据
 */
void extract_actual_bool_data(byte_array_info* response)
{
	if (response == NULL || response->data == NULL || response->length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "提取布尔数据参数错误: 无效的响应数据");
		return;
	}

	// 计算结果长度并分配内存
	int resp_len = response->length * 2;
	byte* content = (byte*)malloc(resp_len);
	if (content == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "提取布尔数据失败: 内存分配错误");
		// 不释放原始数据，保持原样返回
		return;
	}

	// 初始化内存
	memset(content, 0, resp_len);

	// 提取布尔值
	for (int i = 0; i < response->length; i++) {
		// 检查高位
		if ((response->data[i] & 0x10) == 0x10) {
			content[i * 2 + 0] = 0x01;
		}

		// 检查低位
		if ((response->data[i] & 0x01) == 0x01) {
			content[i * 2 + 1] = 0x01;
		}
	}

	// 释放原始数据并更新响应
	RELEASE_DATA(response->data);
	response->data = content;
	response->length = resp_len;
}

/**
 * @brief 读取PLC响应
 * @param fd 连接描述符
 * @param response 响应数据缓冲区
 * @param read_count 实际读取的字节数
 * @return 错误码
 */
mc_error_code_e mc_read_response(int fd, byte_array_info* response, int* read_count)
{
	if (fd <= 0 || read_count == NULL || response == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "读取响应参数错误: 无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 分配内存
	byte* temp = (byte*)malloc(BUFFER_SIZE);
	if (temp == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "分配内存失败: 无法为响应数据分配内存");
		return MC_ERROR_CODE_MALLOC_FAILED;
	}

	memset(temp, 0, BUFFER_SIZE);
	response->data = temp;
	response->length = BUFFER_SIZE;

	*read_count = 0;
	char* ptr = (char*)response->data;

	// 获取通信配置
	mc_comm_config_t config;
	mc_get_comm_config(fd, &config);

	// 接收数据
	*read_count = (int)recv(fd, ptr, response->length, 0);
	if (*read_count < 0) {
		RELEASE_DATA(response->data);
		mc_log_error(MC_ERROR_CODE_SOCKET_RECV_FAILED, "接收数据失败: 网络错误或连接断开");
		return MC_ERROR_CODE_SOCKET_RECV_FAILED;
	}
	else if (*read_count == 0) {
		RELEASE_DATA(response->data);
		mc_log_error(MC_ERROR_CODE_CONNECTION_CLOSED, "连接已关闭: 远程设备关闭了连接");
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