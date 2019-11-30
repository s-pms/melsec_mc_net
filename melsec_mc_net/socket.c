#include "socket.h"
#include <stdio.h>

#ifdef WIN32
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

unsigned int ip2uint(const char *const ip_addr);

// Function converting an IP address string to an unsigned int.
unsigned int ip2uint(const char *const ip_addr) {
	char ip_addr_cpy[20];
	char ip[5];

	strcpy(ip_addr_cpy, ip_addr);

	char *s_start, *s_end;
	s_start = ip_addr_cpy;
	s_end = ip_addr_cpy;

	int k;

	for (k = 0; *s_start != '\0'; s_start = s_end) {
		for (s_end = s_start; *s_end != '.' && *s_end != '\0'; s_end++) {
		}
		if (*s_end == '.') {
			*s_end = '\0';
			s_end++;
		}
		ip[k++] = (char)atoi(s_start);
	}

	ip[k] = '\0';

	return *((unsigned int *)ip);
}

int mc_write_msg(int fd, void *buf, int nbytes) {
	int   nleft, nwritten;
	char *ptr = (char *)buf;

	nleft = nbytes;

	while (nleft > 0) {
#ifdef WIN32
		nwritten = (int)send(fd, (char*)ptr, (size_t)nleft, 0);
#else
		nwritten = (int)write(fd, (char *)ptr, (size_t)nleft);
#endif
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

int mc_read_msg(int fd, void *buf, int nbytes) {
	int   nleft, nread;
	char *ptr = (char *)buf;

	nleft = nbytes;

	if (fd < 0) return -1;

	while (nleft > 0) {
#ifdef WIN32
		nread = (int)recv(fd, ptr, nleft, 0);
#else
		nread = (int)read(fd, ptr, (size_t)nleft);
#endif
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

int mc_open_tcp_client_socket(char *destIp, short destPort) {
	int                sockFd = 0;
	struct sockaddr_in serverAddr;
	int                ret;

	sockFd = (int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sockFd < 0) {
		return -1;
#pragma warning(disable:4996)
	}

	memset((char *)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(destIp);
	serverAddr.sin_port = (uint16_t)htons((uint16_t)destPort);

	ret = connect(sockFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

	if (ret != 0) {
		mc_close_tcp_socket(sockFd);
		sockFd = -1;
	}

#ifdef WIN32
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
#ifdef WIN32
		closesocket(sockFd);
#else
		close(sockFd);
#endif
	}
}

void tinet_ntoa(char *ipstr, unsigned int ip) {
	sprintf(ipstr, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
}