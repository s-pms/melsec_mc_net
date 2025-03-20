#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "error_handler.h"

// 错误信息表
static const mc_error_info_t error_info_table[] = {
	{MC_ERROR_CODE_SUCCESS, "操作成功", "无需恢复"},
	{MC_ERROR_CODE_FAILED, "操作失败", "请检查PLC连接状态和通信参数"},
	{MC_ERROR_CODE_MALLOC_FAILED, "内存分配错误", "请检查系统内存使用情况"},
	{MC_ERROR_CODE_INVALID_PARAMETER, "参数错误", "请检查输入参数是否正确"},
	{MC_ERROR_CODE_PARSE_ADDRESS_FAILED, "地址解析错误", "请检查PLC地址格式是否正确"},
	{MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, "构建核心命令错误", "请检查命令格式是否正确"},
	{MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送命令错误", "请检查网络连接是否正常"},
	{MC_ERROR_CODE_SOCKET_RECV_FAILED, "接收命令错误", "请检查网络连接是否正常"},
	{MC_ERROR_CODE_RESPONSE_HEADER_FAILED, "响应包头不完整错误", "请检查PLC响应数据是否正确"},
	{MC_ERROR_CODE_CONNECTION_CLOSED, "连接已关闭", "请检查网络连接是否正常"},
	{MC_ERROR_CODE_UNKOWN, "未知错误", "请联系技术支持"}
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

// 记录错误日志
void mc_log_error(mc_error_code_e error_code, const char* custom_message) {
	time_t now;
	time(&now);
	char time_str[64];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

	const char* error_msg = mc_get_error_message(error_code);
	const char* recovery_hint = mc_get_error_recovery_hint(error_code);

	fprintf(stderr, "[%s] 错误码: %d, 错误信息: %s, 恢复提示: %s, 自定义信息: %s\n",
		time_str, error_code, error_msg, recovery_hint,
		custom_message ? custom_message : "无");
}