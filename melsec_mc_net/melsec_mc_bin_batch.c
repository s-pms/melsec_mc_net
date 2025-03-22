#include <string.h>

#include "melsec_mc_bin_private.h"
#include "melsec_mc_bin_batch.h"
#include "melsec_mc_bin.h"
#include "melsec_helper.h"
#include "thread_safe.h"
#include "error_handler.h"

extern plc_network_address g_network_address;

// Batch read boolean values
mc_error_code_e mc_read_bool_batch(int fd, const char* address, int length, bool* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch read boolean values parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// Call underlying read function
	ret = read_bool_value(fd, address, length, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length) {
		// Copy data to output buffer
		for (int i = 0; i < length && i < read_data.length; i++) {
			values[i] = (bool)read_data.data[i];
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "Batch read boolean values failed");
	}

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch read short integers
mc_error_code_e mc_read_short_batch(int fd, const char* address, int length, short* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch read short integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// Call underlying read function
	ret = read_word_value(fd, address, length, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 2) {
		// Copy data to output buffer
		for (int i = 0; i < length; i++) {
			values[i] = bytes2short(read_data.data + i * 2);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "Batch read short integers failed");
	}

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch read unsigned short integers
mc_error_code_e mc_read_ushort_batch(int fd, const char* address, int length, ushort* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch read unsigned short integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// Call underlying read function
	ret = read_word_value(fd, address, length, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 2) {
		// Copy data to output buffer
		for (int i = 0; i < length; i++) {
			values[i] = bytes2ushort(read_data.data + i * 2);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "Batch read unsigned short integers failed");
	}

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch read 32-bit integers
mc_error_code_e mc_read_int32_batch(int fd, const char* address, int length, int32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch read 32-bit integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// Call underlying read function
	ret = read_word_value(fd, address, length * 2, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 4) {
		// Copy data to output buffer
		for (int i = 0; i < length; i++) {
			values[i] = bytes2int32(read_data.data + i * 4);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "Batch read 32-bit integers failed");
	}

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch read unsigned 32-bit integers
mc_error_code_e mc_read_uint32_batch(int fd, const char* address, int length, uint32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch read unsigned 32-bit integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// Call underlying read function
	ret = read_word_value(fd, address, length * 2, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 4) {
		// Copy data to output buffer
		for (int i = 0; i < length; i++) {
			values[i] = bytes2uint32(read_data.data + i * 4);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "Batch read unsigned 32-bit integers failed");
	}

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch read floating point numbers
mc_error_code_e mc_read_float_batch(int fd, const char* address, int length, float* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch read floating point numbers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;
	byte_array_info read_data = { 0 };

	// Call underlying read function
	ret = read_word_value(fd, address, length * 2, &read_data);

	if (ret == MC_ERROR_CODE_SUCCESS && read_data.length >= length * 4) {
		// Copy data to output buffer
		for (int i = 0; i < length; i++) {
			values[i] = bytes2float(read_data.data + i * 4);
		}
		RELEASE_DATA(read_data.data);
	}
	else {
		mc_log_error(ret, "Batch read floating point numbers failed");
	}

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch write boolean values
mc_error_code_e mc_write_bool_batch(int fd, const char* address, int length, const bool* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch write boolean values parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Create boolean array
	bool_array_info bool_array;
	bool_array.data = (bool*)malloc(length);
	if (bool_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Batch write boolean values memory allocation failed");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	bool_array.length = length;

	// Copy data
	memcpy(bool_array.data, values, length * sizeof(bool));

	// 调用底层写入函数
	mc_error_code_e ret = write_bool_value(fd, address, length, bool_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Batch write boolean values failed");
	}

	// Free memory
	RELEASE_DATA(bool_array.data);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch write short integers
mc_error_code_e mc_write_short_batch(int fd, const char* address, int length, const short* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch write short integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Create byte array
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 2);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Batch write short integers memory allocation failed");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 2;

	// Convert data
	for (int i = 0; i < length; i++) {
		short2bytes(values[i], byte_array.data + i * 2);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Batch write short integers failed");
	}

	// Free memory
	RELEASE_DATA(byte_array.data);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch write 32-bit integers
mc_error_code_e mc_write_int32_batch(int fd, const char* address, int length, const int32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch write 32-bit integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Create byte array
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 4);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Batch write 32-bit integers memory allocation failed");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 4;

	// Convert data
	for (int i = 0; i < length; i++) {
		int2bytes(values[i], byte_array.data + i * 4);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length * 2, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Batch write 32-bit integers failed");
	}

	// Free memory
	RELEASE_DATA(byte_array.data);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch write unsigned 32-bit integers
mc_error_code_e mc_write_uint32_batch(int fd, const char* address, int length, const uint32* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch write unsigned 32-bit integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Create byte array
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 4);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Batch write unsigned 32-bit integers memory allocation failed");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 4;

	// Convert data
	for (int i = 0; i < length; i++) {
		uint2bytes(values[i], byte_array.data + i * 4);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length * 2, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Batch write unsigned 32-bit integers failed");
	}

	// Free memory
	RELEASE_DATA(byte_array.data);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch write floating point numbers
mc_error_code_e mc_write_float_batch(int fd, const char* address, int length, const float* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch write floating point numbers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Create byte array
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 4);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Batch write floating point numbers memory allocation failed");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 4;

	// Convert data
	for (int i = 0; i < length; i++) {
		float2bytes(values[i], byte_array.data + i * 4);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length * 2, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Batch write floating point numbers failed");
	}

	// Free memory
	RELEASE_DATA(byte_array.data);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}

// Batch write unsigned short integers
mc_error_code_e mc_write_ushort_batch(int fd, const char* address, int length, const ushort* values) {
	if (fd <= 0 || address == NULL || values == NULL || length <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Batch write unsigned short integers parameter error");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// Lock protection
	mc_mutex_lock(&g_connection_mutex);

	// Create byte array
	byte_array_info byte_array;
	byte_array.data = (byte*)malloc(length * 2);
	if (byte_array.data == NULL) {
		mc_log_error(MC_ERROR_CODE_MALLOC_FAILED, "Batch write unsigned short integers memory allocation failed");
		mc_mutex_unlock(&g_connection_mutex);
		return MC_ERROR_CODE_MALLOC_FAILED;
	}
	byte_array.length = length * 2;

	// Convert data
	for (int i = 0; i < length; i++) {
		ushort2bytes(values[i], byte_array.data + i * 2);
	}

	// 调用底层写入函数
	mc_error_code_e ret = write_word_value(fd, address, length, byte_array);
	if (ret != MC_ERROR_CODE_SUCCESS) {
		mc_log_error(ret, "Batch write unsigned short integers failed");
	}

	// Free memory
	RELEASE_DATA(byte_array.data);

	// Unlock
	mc_mutex_unlock(&g_connection_mutex);
	return ret;
}