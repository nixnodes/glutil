/*
 * lc_oper.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LC_OPER_H_
#define LC_OPER_H_

#include <glutil.h>

#include <stdio.h>
#include <fp_types.h>

int
g_oper_and(int s, int d);
int
g_oper_or(int s, int d);

typedef int
__d_icomp(uint64_t s, uint64_t d);
typedef int
__d_iscomp(int64_t s, int64_t d);
typedef int
__d_fcomp(float s, float d);
typedef int
__d_comp(void *s, void *d);

typedef int
(*gs_cmp_p)(void *s, void *d, void * t_ptr);

__d_iscomp g_is_higher_s, g_is_lower_s, g_is_higherorequal_s, g_is_equal_S,
    g_is_not_equal_s, g_is_lowerorequal_s, g_is_not_s, g_is_s, g_is_lower_2_s,
    g_is_higher_2_s, g_is_equal_s;

__d_icomp g_is_higher, g_is_lower, g_is_higherorequal, g_is_equal,
    g_is_not_equal, g_is_lowerorequal, g_is_not, g_is, g_is_lower_2,
    g_is_higher_2, g_is_equal;

__d_fcomp g_is_lower_f, g_is_higher_f, g_is_higherorequal_f, g_is_equal_f,
    g_is_lower_f_2, g_is_higher_f_2, g_is_not_equal_f, g_is_lowerorequal_f,
    g_is_f, g_is_not_f, g_is_equal_f, g_is_notequal_f;

#endif /* LC_OPER_H_ */
