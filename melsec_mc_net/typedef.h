#ifndef __H_TYPEDEF_H__
#define __H_TYPEDEF_H__

#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte;
typedef unsigned short ushort;
typedef signed int	int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;


typedef enum _tag_mc_error_code
{
	MC_ERROR_CODE_SUCCESS = 0,				// �ɹ�
	MC_ERROR_CODE_FAILED = 1,				// ����
	MC_ERROR_CODE_MALLOC_FAILED = 2,		// �ڴ�������
	MC_ERROR_CODE_INVALID_PARAMETER = 3,	// ��������
	MC_ERROR_CODE_PARSE_ADDRESS_FAILED,		// ��ַ��������
	MC_ERROR_CODE_BUILD_CORE_CMD_FAILED,	// ���������������
	MC_ERROR_CODE_SOCKET_SEND_FAILED,		// �����������
	MC_ERROR_CODE_RESPONSE_HEADE_FAILED,	// ��Ӧ��ͷ����������
	MC_ERROR_CODE_UNKOWN = 99,				// δ֪����
} mc_error_code_e;


#endif // !__H_TYPEDEF_H__
