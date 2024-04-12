#ifndef __H_MELSEC_MC_BIN_PRIVATE_H__
#define __H_MELSEC_MC_BIN_PRIVATE_H__

#include "melsec_mc_comm.h"

mc_error_code_e mc_read_response(int fd, byte_array_info* response, int* read_count);

//////////////////////////////////////////////////////////////////////////
mc_error_code_e read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes);
mc_error_code_e read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes);
mc_error_code_e read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes);

mc_error_code_e write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes);
mc_error_code_e write_word_value(int fd, const char* address, int length, byte_array_info in_bytes);
mc_error_code_e write_address_data(int fd, melsec_mc_address_data address_data, byte_array_info in_bytes);

//////////////////////////////////////////////////////////////////////////

//将MC协议的核心报文打包成一个可以直接对PLC进行发送的原始报文
byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number);

//从PLC反馈的数据中提取出实际的数据内容，需要传入反馈数据，是否位读取
void extract_actual_bool_data(byte_array_info* response);

plc_network_address g_network_address;

#endif//__H_MELSEC_MC_BIN_PRIVATE_H__
