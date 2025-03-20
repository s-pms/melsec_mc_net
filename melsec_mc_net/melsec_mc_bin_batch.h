#ifndef __H_BATCH_OPERATION_H__
#define __H_BATCH_OPERATION_H__

#include "typedef.h"

// Batch read boolean values
mc_error_code_e mc_read_bool_batch(int fd, const char* address, int length, bool* values);

// Batch read short integers
mc_error_code_e mc_read_short_batch(int fd, const char* address, int length, short* values);

// Batch read unsigned short integers
mc_error_code_e mc_read_ushort_batch(int fd, const char* address, int length, ushort* values);

// Batch read 32-bit integers
mc_error_code_e mc_read_int32_batch(int fd, const char* address, int length, int32* values);

// Batch read unsigned 32-bit integers
mc_error_code_e mc_read_uint32_batch(int fd, const char* address, int length, uint32* values);

// Batch read floating-point numbers
mc_error_code_e mc_read_float_batch(int fd, const char* address, int length, float* values);

// Batch write boolean values
mc_error_code_e mc_write_bool_batch(int fd, const char* address, int length, const bool* values);

// Batch write short integers
mc_error_code_e mc_write_short_batch(int fd, const char* address, int length, const short* values);

// Batch write unsigned short integers
mc_error_code_e mc_write_ushort_batch(int fd, const char* address, int length, const ushort* values);

// Batch write 32-bit integers
mc_error_code_e mc_write_int32_batch(int fd, const char* address, int length, const int32* values);

// Batch write unsigned 32-bit integers
mc_error_code_e mc_write_uint32_batch(int fd, const char* address, int length, const uint32* values);

// Batch write floating-point numbers
mc_error_code_e mc_write_float_batch(int fd, const char* address, int length, const float* values);

#endif // !__H_BATCH_OPERATION_H__