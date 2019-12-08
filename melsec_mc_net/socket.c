#include "socket.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

int mc_write_msg(int fd, void* buf, int nbytes) {
	int   nleft, nwritten;
	char* ptr = (char*)buf;

	nleft = nbytes;

	while (nleft > 0) {
		nwritten = (int)send(fd, (char*)ptr, (size_t)nleft, 0);
		if (nwritten <= 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		else {
			nleft -= nwritten;
			ptr += nwritten;
		}
	}

	return (nbytes - nleft);
}

int mc_read_msg(int fd, void* buf, int nbytes) {
	int   nleft, nread;
	char* ptr = (char*)buf;

	nleft = nbytes;

	if (fd < 0) return -1;

	while (nleft > 0) {
		nread = (int)recv(fd, ptr, nleft, 0);
		if (nread == 0) {
			break;
		}
		else if (nread < 0) {
			if (errno == EINTR) {
				continue;
			}
			else {
				return -1;
			}
		}
		else {
			nleft -= nread;
			ptr += nread;
		}
	}

	return (nbytes - nleft);
}

int mc_open_tcp_client_socket(char* destIp, short destPort) {
	int                sockFd = 0;
	struct sockaddr_in serverAddr;
	int                ret;

	sockFd = (int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sockFd < 0) {
		return -1;
#pragma warning(disable:4996)
	}

	memset((char*)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(destIp);
	serverAddr.sin_port = (uint16_t)htons((uint16_t)destPort);

	ret = connect(sockFd, (struct sockaddr*) & serverAddr, sizeof(serverAddr));

	if (ret != 0) {
		mc_close_tcp_socket(sockFd);
		sockFd = -1;
	}

#ifdef _WIN32
	int timeout = 5000; //5s
	ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
	ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
	struct timeval timeout = { 5,0 };//3s
	ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
	ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#endif

	return sockFd;
}

void mc_close_tcp_socket(int sockFd) {
	if (sockFd > 0)
	{
#ifdef _WIN32
		closesocket(sockFd);
#else
		close(sockFd);
#endif
	}
}

void tinet_ntoa(char* ipstr, unsigned int ip) {
	sprintf(ipstr, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
}