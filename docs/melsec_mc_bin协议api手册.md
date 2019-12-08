## 程序整体介绍

- 项目名称：melsec_mc_net

- 开发语言：C语言

- 支持操作系统：windows/linux

目前实现功能，实现三菱PLC通讯类，采用Qna兼容3E帧协议实现，需要在PLC侧先的以太网模块先进行配置，必须为二进制通讯。

#### 头文件

```c
#include "melsec_mc_bin.h" 	//协议提供方法接口
#include "typedef.h" 		//部分类型宏定义
```

## 三菱PLC地址说明
####  连接属性
- network_addr 网络号，通常为0 
- station_addr 网络站号，通常为0 
#### PLC地址分类
类型的代号值（软元件代码，用于区分软元件类型，如：D，R） 

| 序号 | 描述       | 地址类型 |
| :--: | :--------- | -------- |
| 1     | 中间继电器 | M        |
| 2     | 输入继电器 | X        |
| 3     | 输出继电器 | Y        |
|4|	数据寄存器| D|
|5 |链接寄存器|W	|
|6 |锁存继电器|L	|
|7 |报警器|F	|
|8 |边沿继电器|V	|
|9 |链接继电器|B	|
|10 |文件寄存器|R	|
|11 |累计定时器的当前值|SN |
|12 |累计定时器的触点|SS |
|13 |累计定时器的线圈|S |
|14 |文件寄存器ZR区|ZR |
|15 |变址寄存器|Z |
|16|定时器的线圈|TC |
|17 |定时器的触点|TS |
|18 |定时器的当前值|TN |
|19 |计数器的当前值|CN |
|20 |计数器的触点|CS |
|21 |计数器的线圈|CC |


## 实现方法

#### 1.连接PLC设备

```c
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);
```

#### 2.读取数据
```c
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
```

#### 3.写入数据

```c
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
```

#### 4.控制方法

```c
bool mc_remote_run(int fd);		//远程Stop操作
bool mc_remote_stop(int fd);	//远程Run操作
bool mc_remote_reset(int fd);	//远程Reset操作 
char* mc_read_plc_type(int fd);	//读取PLC的型号信息 
```





