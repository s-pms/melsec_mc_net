#ifndef __H_BATCH_OPERATION_H__
#define __H_BATCH_OPERATION_H__

#include "typedef.h"

// 批量读取布尔值
mc_error_code_e mc_read_bool_batch(int fd, const char* address, int length, bool* values);

// 批量读取短整型
mc_error_code_e mc_read_short_batch(int fd, const char* address, int length, short* values);

// 批量读取无符号短整型
mc_error_code_e mc_read_ushort_batch(int fd, const char* address, int length, ushort* values);

// 批量读取32位整型
mc_error_code_e mc_read_int32_batch(int fd, const char* address, int length, int32* values);

// 批量读取无符号32位整型
mc_error_code_e mc_read_uint32_batch(int fd, const char* address, int length, uint32* values);

// 批量读取浮点数
mc_error_code_e mc_read_float_batch(int fd, const char* address, int length, float* values);

// 批量写入布尔值
mc_error_code_e mc_write_bool_batch(int fd, const char* address, int length, const bool* values);

// 批量写入短整型
mc_error_code_e mc_write_short_batch(int fd, const char* address, int length, const short* values);

// 批量写入无符号短整型
mc_error_code_e mc_write_ushort_batch(int fd, const char* address, int length, const ushort* values);

// 批量写入32位整型
mc_error_code_e mc_write_int32_batch(int fd, const char* address, int length, const int32* values);

// 批量写入无符号32位整型
mc_error_code_e mc_write_uint32_batch(int fd, const char* address, int length, const uint32* values);

// 批量写入浮点数
mc_error_code_e mc_write_float_batch(int fd, const char* address, int length, const float* values);

#endif // !__H_BATCH_OPERATION_H__