#ifndef __H_COMM_CONFIG_H__
#define __H_COMM_CONFIG_H__

#include "typedef.h"

// 通信配置结构体
typedef struct {
	int send_timeout_ms;     // 发送超时时间(毫秒)
	int recv_timeout_ms;     // 接收超时时间(毫秒)
	int retry_count;         // 重试次数
	int retry_interval_ms;   // 重试间隔(毫秒)
} mc_comm_config_t;

// 默认通信配置
extern mc_comm_config_t default_comm_config;

// 设置通信超时配置
mc_error_code_e mc_set_comm_config(int fd, mc_comm_config_t config);

// 获取当前通信超时配置
mc_error_code_e mc_get_comm_config(int fd, mc_comm_config_t* config);

// 使用指定配置创建连接
int mc_open_tcp_client_socket_with_config(char* destIp, short destPort, mc_comm_config_t config);

void mc_remove_connection_config(int fd);

#endif // !__H_COMM_CONFIG_H__