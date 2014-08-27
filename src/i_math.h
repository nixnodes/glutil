/*
 * i_math.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef I_MATH_H_
#define I_MATH_H_

#include <memory_t.h>
#include <im_hdr.h>

#define F_MATH_INT                      (a32 << 1)
#define F_MATH_INT_S                    (a32 << 2)
#define F_MATH_FLOAT                    (a32 << 3)

#define F_MATH_TYPES                    (F_MATH_INT|F_MATH_INT_S|F_MATH_FLOAT)

#define F_MATH_VAR_KNOWN                (a32 << 4)
#define F_MATH_NITEM                    (a32 << 5)

#define F_MATH_IS_SQRT                  (a32 << 10)
#define F_MATH_IS_GLOB                  (a32 << 11)

#define MLIB_TEST_MACRO                 _BSD_SOURCE || _SVID_SOURCE || _XOPEN_SOURCE >= 600 || _ISOC99_SOURCE || _POSIX_C_SOURCE >= 200112L

typedef uint64_t
(*g_op_tu)(uint64_t s, uint64_t d);
typedef int64_t
(*g_op_ts)(int64_t s, int64_t d);
typedef float
(*g_op_tf)(float s, float d);

typedef void *
(*g_t_pg)(void *base, size_t offset);
typedef void
(*g_op_tg)(void *s, void *d, void *o);

typedef struct ___g_math
{
  uint32_t flags;
  //g_t_pg g_t_ptr;
  void **_m_p;
  void *next;
  g_op_tg op_t;
  size_t l_off;
  uint8_t vstor[8];
  int vb;
  void *_glob_p;
} _g_math, *__g_math;

int
g_math_res(void *d_ptr, pmda mdm, void *res);

int
g_process_math_string(__g_handle hdl, char *string, pmda mdm, pmda chain,
    int *ret, void **p_ret, uint32_t flags, uint32_t int_flags);
int
g_build_math_packet(__g_handle hdl, char *field, char oper, pmda mdm,
    __g_math *ret, uint32_t flags);
int
g_get_math_g_t_ptr(__g_handle hdl, char *field, __g_math math, uint32_t flags);

typedef void
g_op_t(void *s, void *d, void *o);

g_op_t g_arith_add_u64, g_arith_rem_u64, g_arith_mult_u64, g_arith_div_u64,
    g_arith_mod_u64, g_arith_bin_and_u64, g_arith_bin_or_u64,
    g_arith_bin_xor_u64, g_arith_bin_lshift_u64, g_arith_bin_rshift_u64;

static void *_m_u64[];

g_op_t g_arith_add_s64, g_arith_rem_s64, g_arith_mult_s64, g_arith_div_s64,
    g_arith_mod_s64, g_arith_bin_and_s64, g_arith_bin_or_s64,
    g_arith_bin_xor_s64, g_arith_bin_lshift_s64, g_arith_bin_rshift_s64;

static void *_m_s64[];

g_op_t g_arith_add_f, g_arith_rem_f, g_arith_mult_f, g_arith_div_f, g_arith_dummy;

static void *_m_f[];

int
is_ascii_arith_bin_oper(char c);
__g_math
m_get_def_val(pmda math);

#endif /* I_MATH_H_ */
