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
#define F_MATH_INT_TYPES                (F_MATH_INT|F_MATH_INT_S)

#define F_MATH_VAR_KNOWN                (a32 << 4)
#define F_MATH_NITEM                    (a32 << 5)
#define F_MATH_STRCONV                  ((uint32_t)1 << 6)

#define F_MATH_IS_SQRT                  (a32 << 10)
#define F_MATH_IS_GLOB                  (a32 << 11)
#define F_MATH_HAS_CT                   (a32 << 12)

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
  int m_p_i;
  void *next;
  g_op_tg op_t;
  size_t l_off;
  uint8_t vstor[8];
  int vb;
  void *_glob_p;
  __g_proc_v sconv_proc;
  void * misc0;
} _g_math, *__g_math;

int
g_math_res(void *d_ptr, pmda mdm, void *res);

int
g_process_math_string(__g_handle hdl, char *string, pmda mdm, pmda chain,
    int *ret, void **p_ret, uint32_t flags, uint32_t int_flags);
int
g_build_math_packet(__g_handle hdl, char *field, char oper, pmda mdm,
    __g_math *ret, uint32_t flags);
void*
g_get_math_m_p(__g_math math, __g_math p_math);
int
g_get_math_g_t_ptr(__g_handle hdl, char *field, __g_math math, uint32_t flags,
    __g_math p_math);

typedef void
g_op_t(void *s, void *d, void *o);

g_op_t g_arith_add_u64, g_arith_rem_u64, g_arith_mult_u64, g_arith_div_u64,
    g_arith_mod_u64, g_arith_bin_and_u64, g_arith_bin_or_u64,
    g_arith_bin_xor_u64, g_arith_bin_lshift_u64, g_arith_bin_rshift_u64;

static void *_m_u64[];

g_op_t g_arith_add_u64_f, g_arith_rem_u64_f, g_arith_mult_u64_f,
    g_arith_div_u64_f, g_arith_mod_u64_f, g_arith_bin_and_u64_f,
    g_arith_bin_or_u64_f, g_arith_bin_xor_u64_f, g_arith_bin_lshift_u64_f,
    g_arith_bin_rshift_u64_f;

static void *_m_u64_f[];

g_op_t g_arith_add_s64, g_arith_rem_s64, g_arith_mult_s64, g_arith_div_s64,
    g_arith_mod_s64, g_arith_bin_and_s64, g_arith_bin_or_s64,
    g_arith_bin_xor_s64, g_arith_bin_lshift_s64, g_arith_bin_rshift_s64;

static void *_m_s64[];

g_op_t g_arith_add_s64_f, g_arith_rem_s64_f, g_arith_mult_s64_f,
    g_arith_div_s64_f, g_arith_mod_s64_f, g_arith_bin_and_s64_f,
    g_arith_bin_or_s64_f, g_arith_bin_xor_s64_f, g_arith_bin_lshift_s64_f,
    g_arith_bin_rshift_s64_f;

static void *_m_s64_f[];

g_op_t g_arith_dummy, g_generic_dummy;

g_op_t g_arith_add_f, g_arith_rem_f, g_arith_mult_f, g_arith_div_f;

static void *_m_f[];

g_op_t g_arith_add_f_u64, g_arith_rem_f_u64, g_arith_mult_f_u64,
    g_arith_div_f_u64;

static void *_m_f_u64[];

g_op_t g_arith_add_f_s64, g_arith_rem_f_s64, g_arith_mult_f_s64,
    g_arith_div_f_s64;

static void *_m_f_s64[];

int
is_ascii_arith_bin_oper(char c);
__g_math
m_get_def_val(pmda math);

#define M_PROC_ONE() { \
  uint64_t *p_v_b = (uint64_t*)&v_b; \
  if (math->flags & F_MATH_NITEM) \
    {  \
      g_math_res(d_ptr, (pmda) math->next, v_b); \
      c_ptr = v_b; \
    } \
  else \
    { \
      if ((math->flags & F_MATH_VAR_KNOWN)) \
        { \
          c_ptr = math->vstor; \
        } \
      else if (math->flags & F_MATH_STRCONV) { \
          char m_str_b[32]; \
          char *m_str = (char*)math->sconv_proc(d_ptr, NULL, m_str_b , (size_t) sizeof(m_str_b), math->misc0); \
          uint8_t sconv_res[8]; \
          switch ( math->flags & F_MATH_TYPES ) { \
            case F_MATH_INT:; \
                uint64_t *sconv_t_u = (uint64_t*)sconv_res; \
                *sconv_t_u = (uint64_t)strtoull(m_str, NULL, 10); \
            break; \
            case F_MATH_INT_S:; \
                int64_t *sconv_t_su = (int64_t*)sconv_res; \
                *sconv_t_su = (int64_t)strtoll(m_str, NULL, 10); \
            break; \
            case F_MATH_FLOAT:; \
                float *sconv_t_f = (float*)sconv_res; \
                *sconv_t_f = (float)strtof(m_str, NULL); \
                math->vb = 4; \
            break; \
            default:; \
              abort(); \
          } \
          *p_v_b = 0; \
          memcpy((void*) v_b, (void*) sconv_res , math->vb); \
          c_ptr = (void*) v_b; \
        } \
      else \
        { \
          if (math->flags & F_MATH_HAS_CT) \
            { \
              int32_t *ct = (int32_t *) math->_glob_p; \
              *ct = (int32_t) time(NULL); \
            } \
          *p_v_b = 0; \
          memcpy((void*) v_b, \
              (math->flags & F_MATH_IS_GLOB) ? \
                  math->_glob_p : d_ptr + math->l_off, math->vb); \
          c_ptr = (void*) v_b; \
        } \
    } \
};

#endif /* I_MATH_H_ */
