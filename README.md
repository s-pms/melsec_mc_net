# 程序整体介绍

- 项目名称：melsec_mc_net
- 开发语言：C 语言
- 支持操作系统：windows/linux
- 测试设备：QJ71E71 网络扩展卡/FX5U

目前实现功能，实现三菱 PLC 通讯类，采用 Qna 兼容 3E 帧协议实现，需要在 PLC 侧先的以太网模块先进行配置，必须为二进制通讯。

## 头文件

```c
#include "melsec_mc_bin.h"  //协议提供方法接口
#include "typedef.h"   //部分类型宏定义
```

## 三菱 PLC 地址说明

## 连接属性

- network_addr 网络号，通常为 0
- station_addr 网络站号，通常为 0

### PLC 地址分类

类型的代号值（软元件代码，用于区分软元件类型，如：D，R）

| 序号 | 描述               | 地址类型 |
| :--: | :----------------- | -------- |
|  1   | 中间继电器         | M        |
|  2   | 输入继电器         | X        |
|  3   | 输出继电器         | Y        |
|  4   | 数据寄存器         | D        |
|  5   | 链接寄存器         | W        |
|  6   | 锁存继电器         | L        |
|  7   | 报警器             | F        |
|  8   | 边沿继电器         | V        |
|  9   | 链接继电器         | B        |
|  10  | 文件寄存器         | R        |
|  11  | 累计定时器的当前值 | SN       |
|  12  | 累计定时器的触点   | SS       |
|  13  | 累计定时器的线圈   | S        |
|  14  | 文件寄存器 ZR 区   | ZR       |
|  15  | 变址寄存器         | Z        |
|  16  | 定时器的线圈       | TC       |
|  17  | 定时器的触点       | TS       |
|  18  | 定时器的当前值     | TN       |
|  19  | 计数器的当前值     | CN       |
|  20  | 计数器的触点       | CS       |
|  21  | 计数器的线圈       | CC       |

## 实现方法

### 1.连接 PLC 设备

```c
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr); //使用前先通过connect通过socket连接到设备，返回socket文件句柄
bool mc_disconnect(int fd);
```

### 2.读取数据

```c
mc_error_code_e mc_read_bool(int fd, const char* address, bool *val);  //读取设备的bool类型的数据
mc_error_code_e mc_read_short(int fd, const char* address, short *val);  //读取设备的short类型的数据
mc_error_code_e mc_read_ushort(int fd, const char* address, ushort *val); //读取设备的ushort类型的数据
mc_error_code_e mc_read_int32(int fd, const char* address, int32 *val);  //读取设备的int32类型的数据
mc_error_code_e mc_read_uint32(int fd, const char* address, uint32 *val); //读取设备的uint32类型的数据
mc_error_code_e mc_read_int64(int fd, const char* address, int64 *val);  //读取设备的int64类型的数据
mc_error_code_e mc_read_uint64(int fd, const char* address, uint64 *val); //读取设备的uint64类型的数据
mc_error_code_e mc_read_float(int fd, const char* address, float *val);  //读取设备的float类型的数据
mc_error_code_e mc_read_double(int fd, const char* address, double *val); ////读取设备的double类型的数据
mc_error_code_e mc_read_string(int fd, const char* address, int length, char **val); //读取设备的字符串数据，编码为ASCII 需要调用free方法释放
```

### 3.写入数据

```c
mc_error_code_e mc_write_bool(int fd, const char* address, bool val);
mc_error_code_e mc_write_short(int fd, const char* address, short val);
mc_error_code_e mc_write_ushort(int fd, const char* address, ushort val);
mc_error_code_e mc_write_int32(int fd, const char* address, int32 val);
mc_error_code_e mc_write_uint32(int fd, const char* address, uint32 val);
mc_error_code_e mc_write_int64(int fd, const char* address, int64 val);
mc_error_code_e mc_write_uint64(int fd, const char* address, uint64 val);
mc_error_code_e mc_write_float(int fd, const char* address, float val);
mc_error_code_e mc_write_double(int fd, const char* address, double val);
mc_error_code_e mc_write_string(int fd, const char* address, int length, const char *val);
```

### 4.控制方法

```c
mc_error_code_e mc_remote_run(int fd);  //远程Stop操作
mc_error_code_e mc_remote_stop(int fd); //远程Run操作
mc_error_code_e mc_remote_reset(int fd); //远程Reset操作
mc_error_code_e mc_read_plc_type(int fd, char** type); //读取PLC的型号信息
```

## 使用样例

完整样例参见代码中**main.c**文件，如下提供主要代码和使用方法：

_读取地址，格式为"**M100**","**D100**","**W1A0**"_

```c
#ifdef _WIN32
#include <WinSock2.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#pragma warning(disable : 4996)

#define GET_RESULT(ret)  { if (!ret) failed_count++; }

// #define USE_SO
#ifndef USE_SO
#include "melsec_mc_bin.h"
#include "melsec_mc_ascii.h"
#endif

#ifdef USE_SO
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <memory.h>
#include <dlfcn.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

#include "typedef.h"

typedef bool (*pmc_mc_connect)(char *ip_addr, int port, byte network_addr, byte station_addr);
typedef bool (*pmc_mc_disconnect)(int fd);

typedef bool (*pmc_write_bool)(int fd, const char *address, bool val); // write
typedef bool (*pmc_write_short)(int fd, const char *address, short val);
typedef bool (*pmc_write_ushort)(int fd, const char *address, ushort val);
typedef bool (*pmc_write_int32)(int fd, const char *address, int32 val);
typedef bool (*pmc_write_uint32)(int fd, const char *address, uint32 val);
typedef bool (*pmc_write_int64)(int fd, const char *address, int64 val);
typedef bool (*pmc_write_uint64)(int fd, const char *address, uint64 val);
typedef bool (*pmc_write_float)(int fd, const char *address, float val);
typedef bool (*pmc_write_double)(int fd, const char *address, double val);
typedef bool (*pmc_write_string)(int fd, const char *address, int length, const char *val);

typedef bool (*pmc_read_bool)(int fd, const char *address, bool *val); // read
typedef bool (*pmc_read_short)(int fd, const char *address, short *val);
typedef bool (*pmc_read_ushort)(int fd, const char *address, ushort *val);
typedef bool (*pmc_read_int32)(int fd, const char *address, int32 *val);
typedef bool (*pmc_read_uint32)(int fd, const char *address, uint32 *val);
typedef bool (*pmc_read_int64)(int fd, const char *address, int64 *val);
typedef bool (*pmc_read_uint64)(int fd, const char *address, uint64 *val);
typedef bool (*pmc_read_float)(int fd, const char *address, float *val);
typedef bool (*pmc_read_double)(int fd, const char *address, double *val);
typedef bool (*pmc_read_string)(int fd, const char *address, int length, const char *val);

pmc_mc_connect mc_connect;
pmc_mc_disconnect mc_disconnect;

pmc_write_bool mc_write_bool;
pmc_write_short mc_write_short;
pmc_write_ushort mc_write_ushort;
pmc_write_int32 mc_write_int32;
pmc_write_uint32 mc_write_uint32;
pmc_write_int64 mc_write_int64;
pmc_write_uint64 mc_write_uint64;
pmc_write_float mc_write_float;
pmc_write_double mc_write_double;
pmc_write_string mc_write_string;

pmc_read_bool mc_read_bool;
pmc_read_short mc_read_short;
pmc_read_ushort mc_read_ushort;
pmc_read_int32 mc_read_int32;
pmc_read_uint32 mc_read_uint32;
pmc_read_int64 mc_read_int64;
pmc_read_uint64 mc_read_uint64;
pmc_read_float mc_read_float;
pmc_read_double mc_read_double;
pmc_read_string mc_read_string;

void libso_fun(char *szdllpath)
{
 void *handle_so;
 handle_so = dlopen(szdllpath, RTLD_NOW);
 if (!handle_so)
 {
  printf("%s\n", dlerror());
 }

 mc_connect = (pmc_mc_connect)dlsym(handle_so, "mc_connect");
 mc_disconnect = (pmc_mc_disconnect)dlsym(handle_so, "mc_disconnect");

 mc_write_bool = (pmc_write_bool)dlsym(handle_so, "mc_write_bool");
 mc_write_short = (pmc_write_short)dlsym(handle_so, "mc_write_short");
 mc_write_ushort = (pmc_write_ushort)dlsym(handle_so, "mc_write_ushort");
 mc_write_int32 = (pmc_write_int32)dlsym(handle_so, "mc_write_int32");
 mc_write_uint32 = (pmc_write_uint32)dlsym(handle_so, "mc_write_uint32");
 mc_write_int64 = (pmc_write_int64)dlsym(handle_so, "mc_write_int64");
 mc_write_uint64 = (pmc_write_uint64)dlsym(handle_so, "mc_write_uint64");
 mc_write_float = (pmc_write_float)dlsym(handle_so, "mc_write_float");
 mc_write_double = (pmc_write_double)dlsym(handle_so, "mc_write_double");
 mc_write_string = (pmc_write_string)dlsym(handle_so, "mc_write_string");

 mc_read_bool = (pmc_read_bool)dlsym(handle_so, "mc_read_bool");
 mc_read_short = (pmc_read_short)dlsym(handle_so, "mc_read_short");
 mc_read_ushort = (pmc_read_ushort)dlsym(handle_so, "mc_read_ushort");
 mc_read_int32 = (pmc_read_int32)dlsym(handle_so, "mc_read_int32");
 mc_read_uint32 = (pmc_read_uint32)dlsym(handle_so, "mc_read_uint32");
 mc_read_int64 = (pmc_read_int64)dlsym(handle_so, "mc_read_int64");
 mc_read_uint64 = (pmc_read_uint64)dlsym(handle_so, "mc_read_uint64");
 mc_read_float = (pmc_read_float)dlsym(handle_so, "mc_read_float");
 mc_read_double = (pmc_read_double)dlsym(handle_so, "mc_read_double");
 mc_read_string = (pmc_read_string)dlsym(handle_so, "mc_read_string");
 return;
}
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
 WSADATA wsa;
 if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
 {
  return -1;
 }
#endif

#ifdef USE_SO
 char szdllpath[1024];
 strcpy(szdllpath, "../melsec_mc_net.so");
 libso_fun(szdllpath);
#endif

 char *plc_ip = "192.168.122.148";
 int plc_port = 6001;
 if (argc > 1)
 {
  plc_ip = argv[1];
  plc_port = atoi(argv[2]);
 }
 int fd = mc_connect(plc_ip, plc_port, 0, 0);
 if (fd < 0)
  goto label_end;

 mc_error_code_e ret = MC_ERROR_CODE_FAILED;

#if false
 char* type = NULL;
 ret = mc_read_plc_type(fd, &type);
 printf("plc type: %s\n", type);
 free(type);
#endif

 const int TEST_COUNT = 5000;
 const int TEST_SLEEP_TIME = 200;
 int failed_count = 0;

 for (int i = 0; i < TEST_COUNT; i++)
 {
  printf("==============Test count: %d==============\n", i + 1);
  bool all_success = false;
  //////////////////////////////////////////////////////////////////////////
  bool val = true;
  ret = mc_write_bool(fd, "X1", val);
  printf("Write\t X1 \tbool:\t %d, \tret: %d\n", val, ret);
  GET_RESULT(ret);

  val = false;
  ret = mc_read_bool(fd, "x1", &val);
  printf("Read\t X1 \tbool:\t %d\n", val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  short w_s_val = 23;
  ret = mc_write_short(fd, "D100", w_s_val);
  printf("Write\t D100 \tshort:\t %d, \tret: %d\n", w_s_val, ret);
  GET_RESULT(ret);

  short s_val = 0;
  ret = mc_read_short(fd, "D100", &s_val);
  printf("Read\t D100 \tshort:\t %d\n", s_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  ushort w_us_val = 255;
  ret = mc_write_ushort(fd, "D100", w_us_val);
  printf("Write\t D100 \tushort:\t %d, \tret: %d\n", w_us_val, ret);
  GET_RESULT(ret);

  ushort us_val = 0;
  ret = mc_read_ushort(fd, "D100", &us_val);
  printf("Read\t D100 \tushort:\t %d\n", us_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  int32 w_i_val = 12345;
  ret = mc_write_int32(fd, "D100", w_i_val);
  printf("Write\t D100 \tint32:\t %d, \tret: %d\n", w_i_val, ret);
  GET_RESULT(ret);

  int i_val = 0;
  ret = mc_read_int32(fd, "D100", &i_val);
  printf("Read\t D100 \tint32:\t %d\n", i_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  uint32 w_ui_val = 22345;
  ret = mc_write_uint32(fd, "D100", w_ui_val);
  printf("Write\t D100 \tuint32:\t %d, \tret: %d\n", w_ui_val, ret);
  GET_RESULT(ret);

  uint32 ui_val = 0;
  ret = mc_read_uint32(fd, "D100", &ui_val);
  printf("Read\t D100 \tuint32:\t %d\n", ui_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  int64 w_i64_val = 333334554;
  ret = mc_write_int64(fd, "D100", w_i64_val);
  printf("Write\t D100 \tuint64:\t %lld, \tret: %d\n", w_i64_val, ret);
  GET_RESULT(ret);

  int64 i64_val = 0;
  ret = mc_read_int64(fd, "D100", &i64_val);
  printf("Read\t D100 \tint64:\t %lld\n", i64_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  uint64 w_ui64_val = 4333334554;
  ret = mc_write_uint64(fd, "D100", w_ui64_val);
  printf("Write\t D100 \tuint64:\t %lld, \tret: %d\n", w_ui64_val, ret);
  GET_RESULT(ret);

  int64 ui64_val = 0;
  ret = mc_read_uint64(fd, "D100", &ui64_val);
  printf("Read\t D100 \tuint64:\t %lld\n", ui64_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  float w_f_val = 32.454f;
  ret = mc_write_float(fd, "D100", w_f_val);
  printf("Write\t D100 \tfloat:\t %f, \tret: %d\n", w_f_val, ret);
  GET_RESULT(ret);

  float f_val = 0;
  ret = mc_read_float(fd, "D100", &f_val);
  printf("Read\t D100 \tfloat:\t %f\n", f_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  double w_d_val = 12345.6789;
  ret = mc_write_double(fd, "D100", w_d_val);
  printf("Write\t D100 \tdouble:\t %lf, \tret: %d\n", w_d_val, ret);
  GET_RESULT(ret);

  double d_val = 0;
  ret = mc_read_double(fd, "D100", &d_val);
  printf("Read\t D100 \tdouble:\t %lf\n", d_val);
  GET_RESULT(ret);

  //////////////////////////////////////////////////////////////////////////
  const char sz_write[] = "wqliceman@gmail.com";
  int length = sizeof(sz_write) / sizeof(sz_write[0]);
  ret = mc_write_string(fd, "D100", length, sz_write);
  printf("Write\t D100 \tstring:\t %s, \tret: %d\n", sz_write, ret);
  GET_RESULT(ret);

  char *str_val = NULL;
  ret = mc_read_string(fd, "D100", length, &str_val);
  printf("Read\t D100 \tstring:\t %s\n", str_val);
  free(str_val);
  GET_RESULT(ret);

#ifdef _WIN32
  Sleep(TEST_SLEEP_TIME);
#else
  usleep(TEST_SLEEP_TIME * 1000);
#endif
 }

 printf("All Failed count: %d\n", failed_count);

 // mc_remote_run(fd);
 // mc_remote_stop(fd);
 // mc_remote_reset(fd);
 mc_disconnect(fd);

label_end:
#ifdef _WIN32
 WSACleanup();
#else
 ;
#endif
}

```
