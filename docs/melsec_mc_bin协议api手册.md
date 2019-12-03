## 程序整体介绍

- 项目名称：melsec_mc_net

- 开发语言：C语言

- 支持操作系统：windows/linux

目前实现功能，实现三菱PLC通讯类，采用Qna兼容3E帧协议实现，需要在PLC侧先的以太网模块先进行配置，必须为二进制通讯。

## 实现方法

#### 1.连接PLC设备

`c
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);
`
#### 2.读取数据
`c
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
`

#### 3.写入数据

`c
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
`

#### 4.控制方法




