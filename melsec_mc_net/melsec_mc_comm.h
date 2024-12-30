#ifndef __H_MELSC_MC_COMM_H__
#define __H_MELSC_MC_COMM_H__
#include "utill.h"

#define BUFFER_SIZE 512
#define MIN_RESPONSE_HEADER_SIZE 11 // 响应最小报头大小，替换魔术数字11
#define MAX_RETRY_TIMES 3 // 最大重试次数

typedef struct _tag_melsec_mc_data_type {
	byte data_code; // 类型的代号值
	byte data_type; // 数据的类型，0代表按字，1代表按位
	char ascii_code[10]; // 当以ASCII格式通讯时的类型描述
	int from_base; // 指示地址是10进制，还是16进制的
} melsec_mc_data_type;

typedef struct _tag_melsec_mc_address_data {
	melsec_mc_data_type data_type; // 三菱的数据地址信息
	int address_start; // 数字的起始地址，也就是偏移地址
	int length; // 读取的数据长度
} melsec_mc_address_data;

int mc_convert_string_to_int(const char* address, int frombase);
melsec_mc_data_type mc_create_data_type(byte code, byte type, const char* ascii_code, int from_base);
bool mc_analysis_address(const char* address, int length, melsec_mc_address_data* out_address_data);

#endif // __H_MELSC_MC_COMM_H__