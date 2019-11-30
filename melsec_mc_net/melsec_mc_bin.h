#ifndef	__MELSECMCNET_H__
#define __MELSECMCNET_H__
#include "melsec_mc_comm.h"

int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);

//read
bool mc_read_bool(int fd, const char* address, bool *val);
bool mc_read_short(int fd, const char* address, short *val);
bool mc_read_ushort(int fd, const char* address, ushort *val);
bool mc_read_int32(int fd, const char* address, int32 *val);
bool mc_read_uint32(int fd, const char* address, uint32 *val);
bool mc_read_int64(int fd, const char* address, int64 *val);
bool mc_read_uint64(int fd, const char* address, uint64 *val);
bool mc_read_float(int fd, const char* address, float *val);
bool mc_read_double(int fd, const char* address, double *val);
bool mc_read_string(int fd, const char* address, int length, char **val); //need free val

//write
bool mc_write_bool(int fd, const char* address, bool val);
bool mc_write_short(int fd, const char* address, short val);
bool mc_write_ushort(int fd, const char* address, ushort val);
bool mc_write_int32(int fd, const char* address, int32 val);
bool mc_write_uint32(int fd, const char* address, uint32 val);
bool mc_write_int64(int fd, const char* address, int64 val);
bool mc_write_uint64(int fd, const char* address, uint64 val);
bool mc_write_float(int fd, const char* address, float val);
bool mc_write_double(int fd, const char* address, double val);
bool mc_write_string(int fd, const char* address, int length, const char *val);

//////////////////////////////////////////////////////////////////////////
int mc_read_response(int fd, byte_array_info *response);

//////////////////////////////////////////////////////////////////////////
bool read_bool_value(int fd, const char* address, int length, byte_array_info *out_bytes);
bool read_word_value(int fd, const char* address, int length, byte_array_info *out_bytes);
bool read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes);

bool write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes);
bool write_word_value(int fd, const char* address, int length, byte_array_info in_bytes);
bool write_address_data(int fd, melsec_mc_address_data address_data, byte_array_info in_bytes);

//////////////////////////////////////////////////////////////////////////
bool mc_remote_run(int fd);
bool mc_remote_stop(int fd);
bool mc_remote_reset(int fd);
char* mc_read_plc_type(int fd);

//将MC协议的核心报文打包成一个可以直接对PLC进行发送的原始报文
byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number);
//从PLC反馈的数据中提取出实际的数据内容，需要传入反馈数据，是否位读取
void extract_actual_bool_data(byte_array_info *response);

plc_network_address g_network_address;

//

#endif //__MELSECMCNET_H__