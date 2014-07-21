/*
 * gv_off.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef GV_OFF_H_
#define GV_OFF_H_

float
g_tf_ptr(void *base, size_t offset);

g_sint_p g_ts8_ptr, g_ts16_ptr, g_ts32_ptr, g_ts64_ptr;
__g_t_ptr g_t8_ptr, g_t16_ptr, g_t32_ptr, g_t64_ptr;

#include <stdint.h>

typedef uint64_t
    (*g_t64_ptr_p)(void *base, size_t offset);

typedef int64_t
    (*g_ts64_ptr_p)(void *base, size_t offset);

typedef float
    (*g_tf_ptr_p)(void *base, size_t offset);

#endif /* GV_OFF_H_ */
