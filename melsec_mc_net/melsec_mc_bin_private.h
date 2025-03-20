#ifndef __H_MELSEC_MC_BIN_PRIVATE_H__
#define __H_MELSEC_MC_BIN_PRIVATE_H__

#include "melsec_mc_comm.h"

extern plc_network_address g_network_address;

mc_error_code_e mc_read_response(int fd, byte_array_info* response, int* read_count);

//////////////////////////////////////////////////////////////////////////
mc_error_code_e read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes);
mc_error_code_e read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes);
mc_error_code_e read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes);

mc_error_code_e write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes);
mc_error_code_e write_word_value(int fd, const char* address, int length, byte_array_info in_bytes);
mc_error_code_e write_address_data(int fd, melsec_mc_address_data address_data, byte_array_info in_bytes);

//////////////////////////////////////////////////////////////////////////

//Package the MC protocol core message into a raw message that can be sent directly to the PLC
byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number);

//Extract the actual data content from the PLC feedback data, requires passing in feedback data, whether it is bit reading
void extract_actual_bool_data(byte_array_info* response);

#endif//__H_MELSEC_MC_BIN_PRIVATE_H__
