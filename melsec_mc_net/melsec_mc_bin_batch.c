#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "melsec_mc_bin_private.h"
#include "melsec_mc_bin_batch.h"
#include "melsec_mc_bin.h"
#include "melsec_helper.h"
#include "thread_safe.h"
#include "error_handler.h"


extern plc_network_address g_network_address;

// 批量读取布尔值
mc_error_code_e mc_read_bool_batch(int fd, const char* address, int length, bool* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量读取布尔值参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// 调用底层读取函数
	ret = read_bool_value(fd, address, length, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length) {
		// 复制数据到输出缓冲区
		for (int i = 0; i < length && i < read_data.length; i++) {
			values[i] = (bool)read_data.data[i];
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "批量读取布尔值失败");
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量读取短整型
mc_error_code_e mc_read_short_batch(int fd, const char* address, int length, short* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量读取短整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// 调用底层读取函数
	ret = read_word_value(fd, address, length, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 2) {
		// 复制数据到输出缓冲区
		for (int i = 0; i < length; i++) {
			values[i] = bytes2short(read_data.data + i * 2);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "批量读取短整型失败");
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量读取无符号短整型
mc_error_code_e mc_read_ushort_batch(int fd, const char* address, int length, ushort* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量读取无符号短整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// 调用底层读取函数
	ret = read_word_value(fd, address, length, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 2) {
		// 复制数据到输出缓冲区
		for (int i = 0; i < length; i++) {
			values[i] = bytes2ushort(read_data.data + i * 2);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "批量读取无符号短整型失败");
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量读取32位整型
mc_error_code_e mc_read_int32_batch(int fd, const char* address, int length, int32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量读取32位整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// 调用底层读取函数
	ret = read_word_value(fd, address, length * 2, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 4) {
		// 复制数据到输出缓冲区
		for (int i = 0; i < length; i++) {
			values[i] = bytes2int32(read_data.data + i * 4);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "批量读取32位整型失败");
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量读取无符号32位整型
mc_error_code_e mc_read_uint32_batch(int fd, const char* address, int length, uint32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量读取无符号32位整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// 调用底层读取函数
	ret = read_word_value(fd, address, length * 2, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 4) {
		// 复制数据到输出缓冲区
		for (int i = 0; i < length; i++) {
			values[i] = bytes2uint32(read_data.data + i * 4);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "批量读取无符号32位整型失败");
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量读取浮点数
mc_error_code_e mc_read_float_batch(int fd, const char* address, int length, float* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量读取浮点数参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// 调用底层读取函数
	ret = read_word_value(fd, address, length * 2, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 4) {
		// 复制数据到输出缓冲区
		for (int i = 0; i < length; i++) {
			values[i] = bytes2float(read_data.data + i * 4);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "批量读取浮点数失败");
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量写入布尔值
mc_error_code_e mc_write_bool_batch(int fd, const char* address, int length, const bool* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量写入布尔值参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 创建布尔数组
	bool_array_info bool_array;
	bool_array.data = (bool*)malloc(length);
	if (bool_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "批量写入布尔值内存分配失败");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	bool_array.length = length;

	// 复制数据
	memcpy(bool_array.data, values, length * sizeof(bool));

	// 调用底层写入函数
	mc_error_code_e ret = write_bool_value(fd, address, length, bool_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "批量写入布尔值失败");
	}

	// 释放内存
	RELEASE_DATA(bool_array.data);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量写入短整型
mc_error_code_e mc_write_short_batch(int fd, const char* address, int length, const short* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量写入短整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 创建字节数组
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 2);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "批量写入短整型内存分配失败");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 2;

	// 转换数据
	for (int i = 0; i < length; i++) {
		short2bytes(values[i], byte_array.data + i * 2);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "批量写入短整型失败");
	}

	// 释放内存
	RELEASE_DATA(byte_array.data);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量写入32位整型
mc_error_code_e mc_write_int32_batch(int fd, const char* address, int length, const int32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量写入32位整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 创建字节数组
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 4);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "批量写入32位整型内存分配失败");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 4;

	// 转换数据
	for (int i = 0; i < length; i++) {
		int2bytes(values[i], byte_array.data + i * 4);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length * 2, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "批量写入32位整型失败");
	}

	// 释放内存
	RELEASE_DATA(byte_array.data);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量写入无符号32位整型
mc_error_code_e mc_write_uint32_batch(int fd, const char* address, int length, const uint32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量写入无符号32位整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 创建字节数组
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 4);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "批量写入无符号32位整型内存分配失败");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 4;

	// 转换数据
	for (int i = 0; i < length; i++) {
		uint2bytes(values[i], byte_array.data + i * 4);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length * 2, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "批量写入无符号32位整型失败");
	}

	// 释放内存
	RELEASE_DATA(byte_array.data);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量写入浮点数
mc_error_code_e mc_write_float_batch(int fd, const char* address, int length, const float* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量写入浮点数参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 创建字节数组
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 4);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "批量写入浮点数内存分配失败");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 4;

	// 转换数据
	for (int i = 0; i < length; i++) {
		float2bytes(values[i], byte_array.data + i * 4);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length * 2, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "批量写入浮点数失败");
	}

	// 释放内存
	RELEASE_DATA(byte_array.data);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// 批量写入无符号短整型
mc_error_code_e mc_write_ushort_batch(int fd, const char* address, int length, const ushort* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "批量写入无符号短整型参数错误");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	// 创建字节数组
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 2);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "批量写入无符号短整型内存分配失败");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 2;

	// 转换数据
	for (int i = 0; i < length; i++) {
		ushort2bytes(values[i], byte_array.data + i * 2);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "批量写入无符号短整型失败");
	}

	// 释放内存
	RELEASE_DATA(byte_array.data);

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}