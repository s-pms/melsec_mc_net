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

// 初始化互斥锁
mc_error_code_e mc_mutex_init(mc_mutex_t* mutex);

// 销毁互斥锁
mc_error_code_e mc_mutex_destroy(mc_mutex_t* mutex);

// 加锁
mc_error_code_e mc_mutex_lock(mc_mutex_t* mutex);

// 尝试加锁，非阻塞
mc_error_code_e mc_mutex_trylock(mc_mutex_t* mutex);

// 解锁
mc_error_code_e mc_mutex_unlock(mc_mutex_t* mutex);

// 全局连接锁，用于保护连接相关操作
extern mc_mutex_t g_connection_mutex;

// 初始化线程安全环境
mc_error_code_e mc_thread_safe_init(void);

// 清理线程安全环境
mc_error_code_e mc_thread_safe_cleanup(void);

#endif // !__H_THREAD_SAFE_H__