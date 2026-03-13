#ifndef __H_MELSECMCASCII_H__
#define __H_MELSECMCASCII_H__

#include "typedef.h"

// Connection functions
int mc_ascii_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_ascii_disconnect(int fd);

// Read functions for different data types
mc_error_code_e mc_ascii_read_bool(int fd, const char* address, bool* val);
mc_error_code_e mc_ascii_read_short(int fd, const char* address, short* val);
mc_error_code_e mc_ascii_read_ushort(int fd, const char* address, ushort* val);
mc_error_code_e mc_ascii_read_int32(int fd, const char* address, int32* val);
mc_error_code_e mc_ascii_read_uint32(int fd, const char* address, uint32* val);
mc_error_code_e mc_ascii_read_int64(int fd, const char* address, int64* val);
mc_error_code_e mc_ascii_read_uint64(int fd, const char* address, uint64* val);
mc_error_code_e mc_ascii_read_float(int fd, const char* address, float* val);
mc_error_code_e mc_ascii_read_double(int fd, const char* address, double* val);
mc_error_code_e mc_ascii_read_string(int fd, const char* address, int length, char** val);

// Write functions for different data types
mc_error_code_e mc_ascii_write_bool(int fd, const char* address, bool val);
mc_error_code_e mc_ascii_write_short(int fd, const char* address, short val);
mc_error_code_e mc_ascii_write_ushort(int fd, const char* address, ushort val);
mc_error_code_e mc_ascii_write_int32(int fd, const char* address, int32 val);
mc_error_code_e mc_ascii_write_uint32(int fd, const char* address, uint32 val);
mc_error_code_e mc_ascii_write_int64(int fd, const char* address, int64 val);
mc_error_code_e mc_ascii_write_uint64(int fd, const char* address, uint64 val);
mc_error_code_e mc_ascii_write_float(int fd, const char* address, float val);
mc_error_code_e mc_ascii_write_double(int fd, const char* address, double val);
mc_error_code_e mc_ascii_write_string(int fd, const char* address, int length, const char* val);

// PLC control and information functions
mc_error_code_e mc_ascii_remote_run(int fd);
mc_error_code_e mc_ascii_remote_stop(int fd);
mc_error_code_e mc_ascii_remote_reset(int fd);
mc_error_code_e mc_ascii_read_plc_type(int fd, char** type);

#endif // __H_MELSECMCASCII_H__
