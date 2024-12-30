#ifndef __H_TYPEDEF_H__
#define __H_TYPEDEF_H__

#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte;
typedef unsigned short ushort;
typedef signed int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

#define RELEASE_DATA(addr) { if(addr != NULL) { free(addr);} }

typedef enum _tag_mc_error_code {
	MC_ERROR_CODE_SUCCESS = 0, // 成功
	MC_ERROR_CODE_FAILED = 1, // 错误
	MC_ERROR_CODE_MALLOC_FAILED = 2, // 内存分配错误
	MC_ERROR_CODE_INVALID_PARAMETER = 3, // 参数错误
	MC_ERROR_CODE_PARSE_ADDRESS_FAILED, // 地址解析错误
	MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, // 构建核心命令错误
	MC_ERROR_CODE_SOCKET_SEND_FAILED, // 发送命令错误
	MC_ERROR_CODE_RESPONSE_HEADER_FAILED, // 响应包头不完整错误
	MC_ERROR_CODE_UNKOWN = 99, // 未知错误
} mc_error_code_e;

#endif // !__H_TYPEDEF_H__
