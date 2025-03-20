#include <stdio.h>
#include "thread_safe.h"
#include "error_handler.h"

// 全局连接锁
mc_mutex_t g_connection_mutex;

// 初始化互斥锁
mc_error_code_e mc_mutex_init(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "互斥锁指针为空");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	InitializeCriticalSection(mutex);
#else
	if (pthread_mutex_init(mutex, NULL) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "初始化互斥锁失败");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// 销毁互斥锁
mc_error_code_e mc_mutex_destroy(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "互斥锁指针为空");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	DeleteCriticalSection(mutex);
#else
	if (pthread_mutex_destroy(mutex) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "销毁互斥锁失败");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// 加锁
mc_error_code_e mc_mutex_lock(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "互斥锁指针为空");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	EnterCriticalSection(mutex);
#else
	if (pthread_mutex_lock(mutex) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "加锁失败");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// 尝试加锁，非阻塞
mc_error_code_e mc_mutex_trylock(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "互斥锁指针为空");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	if (!TryEnterCriticalSection(mutex)) {
		return MC_ERROR_CODE_FAILED;
	}
#else
	if (pthread_mutex_trylock(mutex) != 0) {
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// 解锁
mc_error_code_e mc_mutex_unlock(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "互斥锁指针为空");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	LeaveCriticalSection(mutex);
#else
	if (pthread_mutex_unlock(mutex) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "解锁失败");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// 初始化线程安全环境
mc_error_code_e mc_thread_safe_init(void) {
	// 初始化全局连接锁
	return mc_mutex_init(&g_connection_mutex);
}

// 清理线程安全环境
mc_error_code_e mc_thread_safe_cleanup(void) {
	// 销毁全局连接锁
	return mc_mutex_destroy(&g_connection_mutex);
}