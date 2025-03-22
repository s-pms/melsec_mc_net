#ifndef __SOCKET_H_
#define __SOCKET_H_

#include "utill.h"

// Communication configuration structure
typedef struct {
	int send_timeout_ms;     // Send timeout (milliseconds)
	int recv_timeout_ms;     // Receive timeout (milliseconds)
	int retry_count;         // Retry count
	int retry_interval_ms;   // Retry interval (milliseconds)
} mc_comm_config_t;

// Default communication configuration
extern mc_comm_config_t default_comm_config;

// Set communication timeout configuration
mc_error_code_e mc_set_comm_config(int fd, mc_comm_config_t config);

// Get current communication timeout configuration
mc_error_code_e mc_get_comm_config(int fd, mc_comm_config_t* config);

// Create connection with specified configuration
int mc_open_tcp_client_socket_with_config(char* destIp, short destPort, mc_comm_config_t config);

void mc_remove_connection_config(int fd);

int mc_write_msg(int fd, void* ptr, int nbytes);
int mc_read_msg(int fd, void* ptr, int nbytes);
int mc_open_tcp_client_socket(char* ip, short port);
void mc_close_tcp_socket(int sockFd);

#endif //__SOCKET_H_
