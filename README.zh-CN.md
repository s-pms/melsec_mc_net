# melsec_mc_net

[English](README.md) | [简体中文](README.zh-CN.md)

一个基于 C 语言的三菱 PLC 以太网通信库，采用 MC 协议（QnA 兼容 3E 帧，支持 Binary / ASCII 两种模式）。

## 项目概览

- 语言：C
- 平台：Windows / Linux
- 协议：MC Protocol 3E Frame（Binary / ASCII）
- 常见测试设备：QJ71E71 网络模块、FX5U

当前代码已提供类型化读写接口、批量读写接口、PLC 远程控制接口，以及通信超时/重试配置能力。并且已增强同一连接上的事务级串行化，避免多线程并发导致响应串包。

## 核心能力

- PLC 连接与断开（Binary / ASCII）
- 多类型单值读写
- 常用类型批量读写
- 远程运行/停止/复位与 PLC 型号读取
- 发送/接收超时与重试参数配置
- 同一 fd 的请求-响应事务串行化
- `main.c` 支持按协议切换的同流程压测（same-fd / multi-fd）

## 目录结构

- `melsec_mc_net/`：核心源码与头文件
- `docs/`：协议/API 说明
- `app/`：Make 流程相关中间产物目录
- `melsec_mc_net/main.c`：示例与测试入口

## 构建方式

### Linux / WSL（Make）

在仓库根目录执行：

```bash
make clean
make
```

默认会在仓库根目录生成 `mc_bin_test`。

### Windows（Visual Studio）

使用 Visual Studio 打开 `melsec_mc_net/melsec_mc_net.sln` 并编译对应工程。

## 基础使用

头文件：

```c
#include "melsec_mc_bin.h"
#include "melsec_mc_ascii.h"
#include "typedef.h"
```

推荐先初始化网络环境：

```c
#include "network_init.h"

if (mc_network_init() != MC_ERROR_CODE_SUCCESS) {
    return -1;
}
```

连接与断开（Binary）：

```c
int fd = mc_connect("192.168.1.10", 6000, 0, 0);
if (fd < 0) {
    return -1;
}

mc_disconnect(fd);
mc_network_cleanup();
```

连接与断开（ASCII）：

```c
int fd = mc_ascii_connect("192.168.1.10", 6000, 0, 0);
if (fd < 0) {
    return -1;
}

mc_ascii_disconnect(fd);
```

读写示例：

```c
short s = 0;
mc_read_short(fd, "D100", &s);

mc_write_int32(fd, "D200", 12345);

char* plc_type = NULL;
if (mc_read_plc_type(fd, &plc_type) == MC_ERROR_CODE_SUCCESS) {
    /* 使用 plc_type */
    free(plc_type);
}
```

## 公开 API

### 连接接口

```c
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);
```

### 单值读取

```c
mc_error_code_e mc_read_bool(int fd, const char* address, bool* val);
mc_error_code_e mc_read_short(int fd, const char* address, short* val);
mc_error_code_e mc_read_ushort(int fd, const char* address, ushort* val);
mc_error_code_e mc_read_int32(int fd, const char* address, int32* val);
mc_error_code_e mc_read_uint32(int fd, const char* address, uint32* val);
mc_error_code_e mc_read_int64(int fd, const char* address, int64* val);
mc_error_code_e mc_read_uint64(int fd, const char* address, uint64* val);
mc_error_code_e mc_read_float(int fd, const char* address, float* val);
mc_error_code_e mc_read_double(int fd, const char* address, double* val);
mc_error_code_e mc_read_string(int fd, const char* address, int length, char** val);
```

### 单值写入

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
mc_error_code_e mc_write_string(int fd, const char* address, int length, const char* val);
```

### 批量接口

头文件：`melsec_mc_bin_batch.h`

```c
mc_error_code_e mc_read_bool_batch(int fd, const char* address, int length, bool* values);
mc_error_code_e mc_read_short_batch(int fd, const char* address, int length, short* values);
mc_error_code_e mc_read_ushort_batch(int fd, const char* address, int length, ushort* values);
mc_error_code_e mc_read_int32_batch(int fd, const char* address, int length, int32* values);
mc_error_code_e mc_read_uint32_batch(int fd, const char* address, int length, uint32* values);
mc_error_code_e mc_read_float_batch(int fd, const char* address, int length, float* values);

mc_error_code_e mc_write_bool_batch(int fd, const char* address, int length, const bool* values);
mc_error_code_e mc_write_short_batch(int fd, const char* address, int length, const short* values);
mc_error_code_e mc_write_ushort_batch(int fd, const char* address, int length, const ushort* values);
mc_error_code_e mc_write_int32_batch(int fd, const char* address, int length, const int32* values);
mc_error_code_e mc_write_uint32_batch(int fd, const char* address, int length, const uint32* values);
mc_error_code_e mc_write_float_batch(int fd, const char* address, int length, const float* values);
```

### PLC 控制与信息

```c
mc_error_code_e mc_remote_run(int fd);
mc_error_code_e mc_remote_stop(int fd);
mc_error_code_e mc_remote_reset(int fd);
mc_error_code_e mc_read_plc_type(int fd, char** type);
```

### ASCII 接口

ASCII 模式接口与 binary 语义一致，函数名前缀为 `mc_ascii_`，例如：

```c
int mc_ascii_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_ascii_disconnect(int fd);

mc_error_code_e mc_ascii_read_int32(int fd, const char* address, int32* val);
mc_error_code_e mc_ascii_write_double(int fd, const char* address, double val);
mc_error_code_e mc_ascii_read_string(int fd, const char* address, int length, char** val);
```

头文件：`melsec_mc_ascii.h`

### 通信参数配置

头文件：`socket.h`

```c
typedef struct {
    int send_timeout_ms;
    int recv_timeout_ms;
    int retry_count;
    int retry_interval_ms;
} mc_comm_config_t;

mc_error_code_e mc_set_comm_config(int fd, mc_comm_config_t config);
mc_error_code_e mc_get_comm_config(int fd, mc_comm_config_t* config);
```

## 地址格式

支持如下一类地址字符串：

- `M100`
- `X1`
- `D100`
- `W1A0`
- `ZR200`
- `TN10`

支持的设备地址包含：M/X/Y/D/W/L/F/V/B/R/SN/SS/SC/ZR/Z/TC/TS/TN/CN/CS/CC。

为便于对照与迁移，保留完整地址列表如下：

| 序号 | 描述               | 地址类型 |
| :--: | :----------------- | :------: |
|  1   | 中间继电器         |    M     |
|  2   | 输入继电器         |    X     |
|  3   | 输出继电器         |    Y     |
|  4   | 数据寄存器         |    D     |
|  5   | 链接寄存器         |    W     |
|  6   | 锁存继电器         |    L     |
|  7   | 报警器             |    F     |
|  8   | 边沿继电器         |    V     |
|  9   | 链接继电器         |    B     |
|  10  | 文件寄存器         |    R     |
|  11  | 累计定时器的当前值 |    SN    |
|  12  | 累计定时器的触点   |    SS    |
|  13  | 累计定时器的线圈   |    SC    |
|  14  | 文件寄存器 ZR 区   |    ZR    |
|  15  | 变址寄存器         |    Z     |
|  16  | 定时器的线圈       |    TC    |
|  17  | 定时器的触点       |    TS    |
|  18  | 定时器的当前值     |    TN    |
|  19  | 计数器的当前值     |    CN    |
|  20  | 计数器的触点       |    CS    |
|  21  | 计数器的线圈       |    CC    |

## 线程安全说明

- 同一 fd 上的请求按事务级串行化（发送 + 接收在同一锁中完成），可避免多线程响应交叉。
- 不同连接间可并发执行。

## 压测入口

`melsec_mc_net/main.c` 已支持按协议切换压测：

- 在 `test_protocol` 中设置 `"bin"` 或 `"ascii"`
- 两种协议均走同一压测流程：
    - `run_same_fd_stress`
    - `run_multi_fd_stress`

## 使用限制

- 部分仿真器或 PLC 侧配置可能不支持远程控制命令（run/stop/reset/type）。
- 使用前需在 PLC 侧正确配置以太网模块与 MC 协议参数。

## 常见问题

- 无法连接：检查 IP、端口、network/station、PLC 侧参数和防火墙。
- 超时/偶发失败：根据网络质量调整 timeout 和 retry 参数。
- 数据异常：检查软元件地址类型与地址进制是否匹配。

## 版权信息

- Copyright (c) 2022-2026 wqliceman. All rights reserved.
- GitHub 用户名：iceman
- 邮箱：wqliceman@gmail.com

## 许可证

见 `LICENSE`。
