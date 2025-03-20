#include "network_init.h"
#include "error_handler.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#endif

static int g_network_initialized = 0;

mc_error_code_e mc_network_init(void) {
#ifdef _WIN32
	if (g_network_initialized) {
		return MC_ERROR_CODE_SUCCESS; // 已经初始化过
	}

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "初始化Winsock库失败");
		return MC_ERROR_CODE_FAILED;
	}

	g_network_initialized = 1;
#endif
	return MC_ERROR_CODE_SUCCESS;
}

mc_error_code_e mc_network_cleanup(void) {
#ifdef _WIN32
	if (!g_network_initialized) {
		return MC_ERROR_CODE_SUCCESS; // 未初始化，无需清理
	}

	if (WSACleanup() != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "清理Winsock库失败");
		return MC_ERROR_CODE_FAILED;
	}

	g_network_initialized = 0;
#endif
	return MC_ERROR_CODE_SUCCESS;
}