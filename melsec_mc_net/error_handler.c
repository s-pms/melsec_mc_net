#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "error_handler.h"

// Error information table
static const mc_error_info_t error_info_table[] = {
	{MC_ERROR_CODE_SUCCESS, "Operation successful", "No recovery needed"},
	{MC_ERROR_CODE_FAILED, "Operation failed", "Please check PLC connection status and communication parameters"},
	{MC_ERROR_CODE_MALLOC_FAILED, "Memory allocation error", "Please check system memory usage"},
	{MC_ERROR_CODE_INVALID_PARAMETER, "Parameter error", "Please check if input parameters are correct"},
	{MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "Address parsing error", "Please check if PLC address format is correct"},
	{MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "Core command building error", "Please check if command format is correct"},
	{MC_ERROR_CODE_SOCKET_SEND_FAILED, "Command sending error", "Please check if network connection is normal"},
	{MC_ERROR_CODE_SOCKET_RECV_FAILED, "Command receiving error", "Please check if network connection is normal"},
	{MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "Incomplete response header error", "Please check if PLC response data is correct"},
	{MC_ERROR_CODE_CONNECTION_CLOSED, "Connection closed", "Please check if network connection is normal"},
	{MC_ERROR_CODE_UNKOWN, "Unknown error", "Please contact technical support"}
};

// 获取错误信息
const char* mc_get_error_message(mc_error_code_e error_code) {
	for (int i = 0; i < sizeof(error_info_table) / sizeof(error_info_table[0]); i++) {
		if (error_info_table[i].code == error_code) {
			return error_info_table[i].message;
		}
	}
	return "未定义的错误";
}

// 获取错误恢复提示
const char* mc_get_error_recovery_hint(mc_error_code_e error_code) {
	for (int i = 0; i < sizeof(error_info_table) / sizeof(error_info_table[0]); i++) {
		if (error_info_table[i].code == error_code) {
			return error_info_table[i].recovery_hint;
		}
	}
	return "请联系技术支持";
}

// 获取完整错误信息结构体
mc_error_info_t mc_get_error_info(mc_error_code_e error_code) {
	for (int i = 0; i < sizeof(error_info_table) / sizeof(error_info_table[0]); i++) {
		if (error_info_table[i].code == error_code) {
			return error_info_table[i];
		}
	}

	mc_error_info_t unknown_error = { MC_ERROR_CODE_UNKOWN, "未定义的错误", "请联系技术支持" };
	return unknown_error;
}

// Record error log
void mc_log_error(mc_error_code_e error_code, const char* custom_message) {
	time_t now;
	time(&now);
	char time_str[64];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

	const char* error_msg = mc_get_error_message(error_code);
	const char* recovery_hint = mc_get_error_recovery_hint(error_code);

	fprintf(stderr, "[%s] Error code: %d, Error message: %s, Recovery hint: %s, Custom message: %s\n",
		time_str, error_code, error_msg, recovery_hint,
		custom_message ? custom_message : "None");
}