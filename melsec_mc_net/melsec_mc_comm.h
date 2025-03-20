#ifndef __H_MELSC_MC_COMM_H__
#define __H_MELSC_MC_COMM_H__
#include "utill.h"

#define BUFFER_SIZE 512
#define MIN_RESPONSE_HEADER_SIZE 11 // Minimum response header size, replacing magic number 11
#define MAX_RETRY_TIMES 3 // Maximum retry count

typedef struct _tag_melsec_mc_data_type {
	byte data_code; // Type code value
	byte data_type; // Data type, 0 represents by word, 1 represents by bit
	char ascii_code[10]; // Type description when communicating in ASCII format
	int from_base; // Indicates whether the address is decimal or hexadecimal
} melsec_mc_data_type;

typedef struct _tag_melsec_mc_address_data {
	melsec_mc_data_type data_type; // Mitsubishi data address information
	int address_start; // Starting address of the number, i.e., the offset address
	int length; // Length of data to read
} melsec_mc_address_data;

int mc_convert_string_to_int(const char* address, int frombase);
melsec_mc_data_type mc_create_data_type(byte code, byte type, const char* ascii_code, int from_base);
bool mc_analysis_address(const char* address, int length, melsec_mc_address_data* out_address_data);

#endif // __H_MELSC_MC_COMM_H__