#ifndef __H_MELSC_HELPER_H__
#define __H_MELSC_HELPER_H__
#include "melsec_mc_comm.h"

byte_array_info  build_read_core_command(melsec_mc_address_data address_data, bool is_bit);
byte_array_info  build_ascii_read_core_command(melsec_mc_address_data address_data, bool is_bit);
byte_array_info  build_write_word_core_command(melsec_mc_address_data address_data, byte_array_info value);
byte_array_info  build_ascii_write_word_core_command(melsec_mc_address_data address_data, byte_array_info value);
byte_array_info  build_write_bit_core_command(melsec_mc_address_data address_data, bool_array_info value);
byte_array_info  build_ascii_write_bit_core_command(melsec_mc_address_data address_data, bool_array_info value);

byte_array_info  build_bytes_from_address(int address, melsec_mc_data_type type);
byte_array_info  build_ascii_bytes_from_ushort(unsigned short value);
byte_array_info  build_ascii_bytes_from_int(int value);
byte_array_info  build_ascii_bytes_from_byte_array(const byte* value, int length);
byte_array_info  build_ascii_bytes_from_bool_array(const bool* value, int length);

byte_array_info  trans_bool_array_to_byte_data(bool_array_info value);

// Calculate checksum information for Fx protocol commands
byte_array_info  calculate_CRC(byte_array_info  data);
// Check if the specified checksum is correct
bool check_CRC(byte_array_info  data);

//
mc_error_code_e mc_parse_read_response(byte_array_info response, byte_array_info* data);
mc_error_code_e mc_parse_write_response(byte_array_info response, byte_array_info* data);

bool mc_try_send_msg(int fd, byte_array_info* in_bytes);

#endif//__H_MELSC_HELPER_H__