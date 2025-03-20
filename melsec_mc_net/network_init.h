#ifndef __H_NETWORK_INIT_H__
#define __H_NETWORK_INIT_H__

#include "typedef.h"

// 初始化网络环境
mc_error_code_e mc_network_init(void);

// 清理网络环境
mc_error_code_e mc_network_cleanup(void);

#endif // !__H_NETWORK_INIT_H__