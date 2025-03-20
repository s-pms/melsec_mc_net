#ifndef __H_ERROR_HANDLER_H__
#define __H_ERROR_HANDLER_H__

#include "typedef.h"

// 错误信息结构体
typedef struct {
	mc_error_code_e code;       // 错误码
	const char* message;        // 错误信息
	const char* recovery_hint;  // 恢复提示
} mc_error_info_t;

// 获取错误信息
const char* mc_get_error_message(mc_error_code_e error_code);

// 获取错误恢复提示
const char* mc_get_error_recovery_hint(mc_error_code_e error_code);

// 获取完整错误信息结构体
mc_error_info_t mc_get_error_info(mc_error_code_e error_code);

// 记录错误日志
void mc_log_error(mc_error_code_e error_code, const char* custom_message);

#endif // !__H_ERROR_HANDLER_H__