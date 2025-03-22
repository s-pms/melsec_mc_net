#ifndef __H_TYPEDEF_H__
#define __H_TYPEDEF_H__

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

typedef unsigned char byte;
typedef unsigned short ushort;
typedef signed int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

#define RELEASE_DATA(addr) { if(addr != NULL) { free(addr);} }

typedef enum _tag_mc_error_code {
	MC_ERROR_CODE_SUCCESS = 0, // Success
	MC_ERROR_CODE_FAILED = 1, // Error
	MC_ERROR_CODE_MALLOC_FAILED = 2, // Memory allocation error
	MC_ERROR_CODE_INVALID_PARAMETER = 3, // Parameter error
	MC_ERROR_CODE_PARSE_ADDRESS_FAILED, // Address parsing error
	MC_ERROR_CODE_BUILD_CORE_CMD_FAILED, // Core command building error
	MC_ERROR_CODE_SOCKET_SEND_FAILED, // Command sending error
	MC_ERROR_CODE_SOCKET_RECV_FAILED, // Command receiving error
	MC_ERROR_CODE_RESPONSE_HEADER_FAILED, // Incomplete response header error
	MC_ERROR_CODE_CONNECTION_CLOSED,	// Connection closed
	MC_ERROR_CODE_UNKOWN = 99, // Unknown error
} mc_error_code_e;

#endif // !__H_TYPEDEF_H__
