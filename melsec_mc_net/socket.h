#ifndef __SOCKET_H_
#define __SOCKET_H_

#include "utill.h"

int mc_write_msg(int fd, void* ptr, int nbytes);
int mc_read_msg(int fd, void* ptr, int nbytes);
int mc_open_tcp_client_socket(char* ip, short port);
void mc_close_tcp_socket(int sockFd);

#endif //__SOCKET_H_
