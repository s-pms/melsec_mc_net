#include <stdio.h>
#include "thread_safe.h"
#include "error_handler.h"

// 全局连接锁
mc_mutex_t g_connection_mutex;

// 初始化互斥锁
mc_error_code_e mc_mutex_init(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Mutex pointer is NULL");
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
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Mutex pointer is NULL");
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
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Mutex pointer is NULL");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	EnterCriticalSection(mutex);
#else
	if (pthread_mutex_lock(mutex) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "Failed to lock mutex");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// Try to lock, non-blocking
mc_error_code_e mc_mutex_trylock(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Mutex pointer is NULL");
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

// Unlock
mc_error_code_e mc_mutex_unlock(mc_mutex_t* mutex) {
	if (mutex == NULL) {
		mc_log_error(MC_ERROR_CODE_INVALID_PARAMETER, "Mutex pointer is NULL");
		return MC_ERROR_CODE_INVALID_PARAMETER;
	}

#ifdef _WIN32
	LeaveCriticalSection(mutex);
#else
	if (pthread_mutex_unlock(mutex) != 0) {
		mc_log_error(MC_ERROR_CODE_FAILED, "Failed to unlock mutex");
		return MC_ERROR_CODE_FAILED;
	}
#endif

	return MC_ERROR_CODE_SUCCESS;
}

// Initialize thread-safe environment
mc_error_code_e mc_thread_safe_init(void) {
	// Initialize global connection lock
	return mc_mutex_init(&g_connection_mutex);
}

// Cleanup thread-safe environment
mc_error_code_e mc_thread_safe_cleanup(void) {
	// Destroy global connection lock
	return mc_mutex_destroy(&g_connection_mutex);
}