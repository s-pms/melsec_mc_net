#ifndef __H_NETWORK_INIT_H__
#define __H_NETWORK_INIT_H__

#include "typedef.h"

// Initialize network environment
mc_error_code_e mc_network_init(void);

// Clean up network environment
mc_error_code_e mc_network_cleanup(void);

#endif // !__H_NETWORK_INIT_H__