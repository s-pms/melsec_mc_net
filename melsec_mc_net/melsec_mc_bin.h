#ifndef	__H_MELSECMCNET_H__
#define __H_MELSECMCNET_H__

#include "typedef.h"

// Connection functions
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);

// Read functions for different data types
mc_error_code_e mc_read_bool(int fd, const char* address, bool* val);
mc_error_code_e mc_read_short(int fd, const char* address, short* val);
mc_error_code_e mc_read_ushort(int fd, const char* address, ushort* val);
mc_error_code_e mc_read_int32(int fd, const char* address, int32* val);
mc_error_code_e mc_read_uint32(int fd, const char* address, uint32* val);
mc_error_code_e mc_read_int64(int fd, const char* address, int64* val);
mc_error_code_e mc_read_uint64(int fd, const char* address, uint64* val);
mc_error_code_e mc_read_float(int fd, const char* address, float* val);
mc_error_code_e mc_read_double(int fd, const char* address, double* val);
mc_error_code_e mc_read_string(int fd, const char* address, int length, char** val); //val needs to be freed manually

// Write functions for different data types
mc_error_code_e mc_write_bool(int fd, const char* address, bool val);
mc_error_code_e mc_write_short(int fd, const char* address, short val);
mc_error_code_e mc_write_ushort(int fd, const char* address, ushort val);
mc_error_code_e mc_write_int32(int fd, const char* address, int32 val);
mc_error_code_e mc_write_uint32(int fd, const char* address, uint32 val);
mc_error_code_e mc_write_int64(int fd, const char* address, int64 val);
mc_error_code_e mc_write_uint64(int fd, const char* address, uint64 val);
mc_error_code_e mc_write_float(int fd, const char* address, float val);
mc_error_code_e mc_write_double(int fd, const char* address, double val);
mc_error_code_e mc_write_string(int fd, const char* address, int length, const char* val);

// PLC control and information functions
mc_error_code_e mc_remote_run(int fd);   // Run PLC remotely
mc_error_code_e mc_remote_stop(int fd);  // Stop PLC remotely
mc_error_code_e mc_remote_reset(int fd); // Reset PLC remotely
mc_error_code_e mc_read_plc_type(int fd, char** type); // Read PLC type, type needs to be freed manually

#endif //__H_MELSECMCNET_H__