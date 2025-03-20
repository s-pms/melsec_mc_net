#include "socket.h"
#include <stdio.h>
#include <string.h>
#include "error_handler.h"
#include "thread_safe.h"
#include "comm_config.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// 默认通信配置
mc_comm_config_t default_comm_config = {
	.send_timeout_ms = 5000,    // 默认发送超时5秒
	.recv_timeout_ms = 5000,    // 默认接收超时5秒
	.retry_count = 3,           // 默认重试3次
	.retry_interval_ms = 500    // 默认重试间隔500毫秒
};

// 存储每个连接的配置信息
#define MAX_CONNECTIONS 32
static struct {
	int fd;                    // 连接描述符
	mc_comm_config_t config;    // 连接配置
	bool in_use;                // 是否在使用
} connection_configs[MAX_CONNECTIONS] = { 0 };

int mc_write_msg(int fd, void* buf, int nbytes) {
	if (fd <= 0 || buf == NULL || nbytes <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "发送消息参数错误");
		return -1;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	int nleft, nwritten;
	char* ptr = (char*)buf;
	nleft = nbytes;

	while (nleft > 0) {
		nwritten = (int)send(fd, (char*)ptr, (size_t)nleft, 0);
		if (nwritten <= 0) {
#ifdef _WIN32
			if (WSAGetLastError() == WSAEINTR)
				continue;
			else {
				mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败");
				mc_mutex_unlock(&g_connection_mutex);
				return -1;
			}
#else
			if (errno == EINTR)
				continue;
			else {
				mc_log_error(MC_ERROR_CODE_SOCKET_SEND_FAILED, "发送消息失败");
				mc_mutex_unlock(&g_connection_mutex);
				return -1;
			}
#endif
		}
		else {
			nleft -= nwritten;
			ptr += nwritten;
		}
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return (nbytes - nleft);
}

int mc_read_msg(int fd, void* buf, int nbytes) {
	if (fd <= 0 || buf == NULL || nbytes <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "接收消息参数错误");
		return -1;
	}

	// 加锁保护
	mc_mutex_lock(&g_connection_mutex);

	int nleft, nread;
	char* ptr = (char*)buf;
	nleft = nbytes;

	while (nleft > 0) {
		nread = (int)recv(fd, ptr, nleft, 0);
		if (nread == 0) {
			mc_log_error(MC_ERROR_CODE_FAILED, "连接已关闭");
			break;
		}
		else if (nread < 0) {
#ifdef _WIN32
			if (WSAGetLastError() == WSAEINTR) {
				continue;
			}
			else {
				mc_log_error(MC_ERROR_CODE_FAILED, "接收消息失败");
				mc_mutex_unlock(&g_connection_mutex);
				return -1;
			}
#else
			if (errno == EINTR) {
				continue;
			}
			else {
				mc_log_error(MC_ERROR_CODE_FAILED, "接收消息失败");
				mc_mutex_unlock(&g_connection_mutex);
				return -1;
			}
#endif
		}
		else {
			nleft -= nread;
			ptr += nread;
		}
	}

	// 解锁
	mc_mutex_unlock(&g_connection_mutex);
	return (nbytes - nleft);
}

int mc_open_tcp_client_socket_with_config2(char* destIp, short destPort, mc_comm_config_t config) {
	if (destIp == NULL || destPort <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "创建TCP连接参数错误");
		return -1;
	}

	int sockFd = 0;
	struct sockaddr_in serverAddr;
	int ret;

	sockFd = (int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sockFd < 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "创建套接字失败");
		return -1;
#pragma warning(disable:4996)
	}

	memset((char*)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(destIp);
	serverAddr.sin_port = (uint16_t)htons((uint16_t)destPort);

	ret = connect(sockFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

	if (ret != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "连接服务器失败");
		mc_close_tcp_socket(sockFd);
		return -1;
	}

	// 设置套接字选项
#ifdef _WIN32
	int send_timeout = config.send_timeout_ms;
	int recv_timeout = config.recv_timeout_ms;

	ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout));
	if (ret != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置发送超时失败");
	}

	ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout));
	if (ret != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置接收超时失败");
	}
#else
	struct timeval send_timeout = { config.send_timeout_ms / 1000, (config.send_timeout_ms % 1000) * 1000 };
	struct timeval recv_timeout = { config.recv_timeout_ms / 1000, (config.recv_timeout_ms % 1000) * 1000 };

	ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout));
	if (ret != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置发送超时失败");
	}

	ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout));
	if (ret != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置接收超时失败");
	}
#endif

	// 保存连接配置
	mc_set_comm_config(sockFd, config);

	return sockFd;
}

int mc_open_tcp_client_socket(char* destIp, short destPort) {
	// 使用默认配置创建连接
	return mc_open_tcp_client_socket_with_config2(destIp, destPort, default_comm_config);
}

void mc_close_tcp_socket(int sockFd) {
	if (sockFd <= 0) {
		return;
	}

	// 移除连接配置
	mc_remove_connection_config(sockFd);

	// 关闭套接字
#ifdef _WIN32
	closesocket(sockFd);
#else
	close(sockFd);
#endif

	mc_log_error(MC_ERROR_CODE_SUCCESS, "成功关闭连接");
}

void tinet_ntoa(char* ipstr, unsigned int ip) {
	sprintf(ipstr, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
}

// 查找连接配置
static int find_connection_config(int fd) {
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (connection_configs[i].in_use && connection_configs[i].fd == fd) {
			return i;
		}
	}
	return -1;
}

// 添加连接配置
static int add_connection_config(int fd, mc_comm_config_t config) {
	int index = find_connection_config(fd);
	if (index >= 0) {
		// 更新已有配置
		connection_configs[index].config = config;
		return index;
	}

	// 查找空闲位置
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (!connection_configs[i].in_use) {
			connection_configs[i].fd = fd;
			connection_configs[i].config = config;
			connection_configs[i].in_use = true;
			return i;
		}
	}

	return -1; // 没有空闲位置
}

// 移除连接配置
void mc_remove_connection_config(int fd) {
	int index = find_connection_config(fd);
	if (index >= 0) {
		connection_configs[index].in_use = false;
	}
}

// 设置通信超时配置
mc_error_code_e mc_set_comm_config(int fd, mc_comm_config_t config) {
	if (fd <= 0) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "无效的连接描述符");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	// 保存配置
	if (add_connection_config(fd, config) < 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "保存连接配置失败，连接数量已达上限");
		return MC_ERROR_CODE_FAILED;
	}

	// 设置套接字选项
#ifdef _WIN32
	int send_timeout = config.send_timeout_ms;
	int recv_timeout = config.recv_timeout_ms;

	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout)) < 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置发送超时失败");
		return MC_ERROR_CODE_FAILED;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout)) < 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置接收超时失败");
		return MC_ERROR_CODE_FAILED;
	}
#else
	struct timeval send_timeout = {
		.tv_sec = config.send_timeout_ms / 1000,
		.tv_usec = (config.send_timeout_ms % 1000) * 1000
	};

	struct timeval recv_timeout = {
		.tv_sec = config.recv_timeout_ms / 1000,
		.tv_usec = (config.recv_timeout_ms % 1000) * 1000
	};

	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout)) < 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置发送超时失败");
		return MC_ERROR_CODE_FAILED;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout)) < 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "设置接收超时失败");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// 获取当前通信超时配置
mc_error_code_e mc_get_comm_config(int fd, mc_comm_config_t* config) {
	if (fd <= 0 || config == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "无效的参数");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

	int index = find_connection_config(fd);
	if (index >= 0) {
		*config = connection_configs[index].config;
		return MC_ERROR_CODE_SUCCESS;
	}
	else {
		// 如果没有找到配置，返回默认配置
		*config = default_comm_config;
		return MC_ERROR_CODE_SUCCESS;
	}
}

// 使用指定配置创建连接
int mc_open_tcp_client_socket_with_config(char* destIp, short destPort, mc_comm_config_t config) {
	int sockFd = mc_open_tcp_client_socket(destIp, destPort);
	if (sockFd <= 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "创建TCP连接失败");
		return sockFd;
	}

	// 设置通信配置
	if (mc_set_comm_config(sockFd, config) != MC_ERROR_CODE_SUCCESS) {
		mc_close_tcp_socket(sockFd);
		mc_log_error(MC_ERROR_CODE_FAILED, "设置通信配置失败");
		return -1;
	}

	return sockFd;
}