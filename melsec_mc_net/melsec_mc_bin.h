#ifndef	__H_MELSECMCNET_H__
#define __H_MELSECMCNET_H__

#include "typedef.h"

int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);

//read
bool mc_read_bool(int fd, const char* address, bool* val);
bool mc_read_short(int fd, const char* address, short* val);
bool mc_read_ushort(int fd, const char* address, ushort* val);
bool mc_read_int32(int fd, const char* address, int32* val);
bool mc_read_uint32(int fd, const char* address, uint32* val);
bool mc_read_int64(int fd, const char* address, int64* val);
bool mc_read_uint64(int fd, const char* address, uint64* val);
bool mc_read_float(int fd, const char* address, float* val);
bool mc_read_double(int fd, const char* address, double* val);
bool mc_read_string(int fd, const char* address, int length, char** val); //need free val

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
bool mc_write_string(int fd, const char* address, int length, const char* val);

//
bool mc_remote_run(int fd);
bool mc_remote_stop(int fd);
bool mc_remote_reset(int fd);
char* mc_read_plc_type(int fd);

#endif //__H_MELSECMCNET_H__