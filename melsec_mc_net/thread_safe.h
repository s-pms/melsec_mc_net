#ifndef __H_THREAD_SAFE_H__
#define __H_THREAD_SAFE_H__

#include "typedef.h"

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION mc_mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mc_mutex_t;
#endif

// Initialize mutex lock
mc_error_code_e mc_mutex_init(mc_mutex_t* mutex);

// Destroy mutex lock
mc_error_code_e mc_mutex_destroy(mc_mutex_t* mutex);

// Lock
mc_error_code_e mc_mutex_lock(mc_mutex_t* mutex);

// Try to lock, non-blocking
mc_error_code_e mc_mutex_trylock(mc_mutex_t* mutex);

// Unlock
mc_error_code_e mc_mutex_unlock(mc_mutex_t* mutex);

// Global connection lock, used to protect connection-related operations
extern mc_mutex_t g_connection_mutex;

// Initialize thread-safe environment
mc_error_code_e mc_thread_safe_init(void);

// Clean up thread-safe environment
mc_error_code_e mc_thread_safe_cleanup(void);

#endif // !__H_THREAD_SAFE_H__