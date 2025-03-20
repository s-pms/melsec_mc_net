#ifndef __H_ERROR_HANDLER_H__
#define __H_ERROR_HANDLER_H__

#include "typedef.h"

// Error information structure
typedef struct {
	mc_error_code_e code;       // Error code
	const char* message;        // Error message
	const char* recovery_hint;  // Recovery hint
} mc_error_info_t;

// Get error message
const char* mc_get_error_message(mc_error_code_e error_code);

// Get error recovery hint
const char* mc_get_error_recovery_hint(mc_error_code_e error_code);

// Get complete error information structure
mc_error_info_t mc_get_error_info(mc_error_code_e error_code);

// Record error log
void mc_log_error(mc_error_code_e error_code, const char* custom_message);

#endif // !__H_ERROR_HANDLER_H__