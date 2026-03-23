# melsec_mc_net

[English](README.md) | [简体中文](README.zh-CN.md)

A C library for Mitsubishi PLC communication over Ethernet using MC Protocol (QnA-compatible 3E frame), with both binary and ASCII modes.

## Overview

- Language: C
- Platforms: Windows and Linux
- Protocol: MC Protocol 3E frame (binary and ASCII)
- Typical tested hardware: QJ71E71 network module, FX5U

The library exposes typed read/write APIs, PLC remote control APIs, and batch APIs. It also includes communication timeout/retry configuration and thread-safety improvements for request/response transaction serialization on the same socket.

## Key Features

- PLC connect/disconnect (binary and ASCII)
- Typed single-value read/write
- Batch read/write for common scalar types
- Remote run/stop/reset and PLC type query
- Configurable send/receive timeout and retry strategy
- Request-response transaction serialization per file descriptor
- Protocol-switchable stress test in `main.c` (same-fd and multi-fd)

## Project Layout

- `melsec_mc_net/`: core library source and headers
- `docs/`: protocol/API notes
- `app/`: build artifacts/dependency files for the Make-based flow
- `main.c` in `melsec_mc_net/`: example and test entry

## Build

### Linux / WSL (Make)

From repository root:

```bash
make clean
make
```

The default Make flow produces `mc_bin_test` at repository root.

### Windows (Visual Studio)

Open `melsec_mc_net/melsec_mc_net.sln` in Visual Studio and build the target project.

## Basic Usage

Include headers:

```c
#include "melsec_mc_bin.h"
#include "melsec_mc_ascii.h"
#include "typedef.h"
```

Initialize network stack (recommended cross-platform entry):

```c
#include "network_init.h"

if (mc_network_init() != MC_ERROR_CODE_SUCCESS) {
    return -1;
}
```

Connect and disconnect (binary):

```c
int fd = mc_connect("192.168.1.10", 6000, 0, 0);
if (fd < 0) {
    return -1;
}

mc_disconnect(fd);
mc_network_cleanup();
```

Connect and disconnect (ASCII):

```c
int fd = mc_ascii_connect("192.168.1.10", 6000, 0, 0);
if (fd < 0) {
    return -1;
}

mc_ascii_disconnect(fd);
```

Read and write examples:

```c
short s = 0;
mc_read_short(fd, "D100", &s);

mc_write_int32(fd, "D200", 12345);

char* plc_type = NULL;
if (mc_read_plc_type(fd, &plc_type) == MC_ERROR_CODE_SUCCESS) {
    /* use plc_type */
    free(plc_type);
}
```

## Public APIs

### Connection

```c
int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_disconnect(int fd);
```

### Single-value Read

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

### Single-value Write

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

### Batch APIs

Header: `melsec_mc_bin_batch.h`

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

### PLC Control / Information

```c
mc_error_code_e mc_remote_run(int fd);
mc_error_code_e mc_remote_stop(int fd);
mc_error_code_e mc_remote_reset(int fd);
mc_error_code_e mc_read_plc_type(int fd, char** type);
```

### ASCII APIs

ASCII mode uses the same semantic API set with `mc_ascii_` prefix, for example:

```c
int mc_ascii_connect(char* ip_addr, int port, byte network_addr, byte station_addr);
bool mc_ascii_disconnect(int fd);

mc_error_code_e mc_ascii_read_int32(int fd, const char* address, int32* val);
mc_error_code_e mc_ascii_write_double(int fd, const char* address, double val);
mc_error_code_e mc_ascii_read_string(int fd, const char* address, int length, char** val);
```

Header: `melsec_mc_ascii.h`

### Communication Configuration

Header: `socket.h`

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

## Address Format

The API accepts address strings such as:

- `M100`
- `X1`
- `D100`
- `W1A0`
- `ZR200`
- `TN10`

Supported device addresses include M/X/Y/D/W/L/F/V/B/R/SN/SS/SC/ZR/Z/TC/TS/TN/CN/CS/CC.

For easier migration and one-to-one mapping, the full address list is kept below:

1. Internal relay: M
2. Input relay: X
3. Output relay: Y
4. Data register: D
5. Link register: W
6. Latch relay: L
7. Annunciator: F
8. Edge relay: V
9. Link relay: B
10. File register: R
11. Accumulative timer current value: SN
12. Accumulative timer contact: SS
13. Accumulative timer coil: SC
14. File register ZR area: ZR
15. Index register: Z
16. Timer coil: TC
17. Timer contact: TS
18. Timer current value: TN
19. Counter current value: CN
20. Counter contact: CS
21. Counter coil: CC

## Thread-Safety Notes

- Requests on the same file descriptor are serialized at transaction level (send + receive) to prevent response interleaving across threads.
- You can still run parallel workloads across different connections.

## Stress Test Entry

`melsec_mc_net/main.c` supports stress testing with protocol selection:

- Set `test_protocol` to `"bin"` or `"ascii"`
- Same test workflow is used for both modes:
    - `run_same_fd_stress`
    - `run_multi_fd_stress`

## Limitations

- Remote control commands (run/stop/reset/type) may be unsupported by some simulators or restricted by PLC-side permissions/configuration.
- Please configure the PLC Ethernet module and MC communication parameters before use.

## Troubleshooting

- Connection fails: verify IP, port, PLC network/station number, firewall, and PLC Ethernet module settings.
- Timeout or intermittent errors: tune `send_timeout_ms`, `recv_timeout_ms`, and retry parameters.
- Incorrect data: verify device address type and base (decimal/hex by device family).

## Copyright

- Copyright (c) 2022-2026 wqliceman. All rights reserved.
- GitHub: [iceman](https://github.com/iceman)
- Email: wqliceman@gmail.com

## License

See `LICENSE`.
