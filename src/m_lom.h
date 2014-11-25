/*
 * m_lom.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <t_glob.h>
#include <stdint.h>

#include <m_general.h>

#ifndef M_LOM_H_
#define M_LOM_H_

#define F_LM_CPRG                       (a32 << 1)
#define F_LM_LOM                        (a32 << 2)

#define F_LM_TYPES                      (F_LM_CPRG|F_LM_LOM)

#define F_LOM_INT                       (a32 << 1)
#define F_LOM_INT_S                     (a32 << 2)
#define F_LOM_FLOAT                     (a32 << 3)
#define F_LOM_LVAR_KNOWN                (a32 << 4)
#define F_LOM_RVAR_KNOWN                (a32 << 5)
#define F_LOM_HASOPER                   (a32 << 6)
#define F_LOM_FLOAT_DBL                 (a32 << 7)
#define F_LOM_IS_LVAR_MATH              (a32 << 8)
#define F_LOM_IS_RVAR_MATH              (a32 << 9)

#define F_LOM_TYPES                     (F_LOM_FLOAT|F_LOM_INT|F_LOM_INT_S)
#define F_LOM_VAR_KNOWN                 (F_LOM_LVAR_KNOWN|F_LOM_RVAR_KNOWN)

#define F_GLT_LEFT                      (a32 << 1)
#define F_GLT_RIGHT                     (a32 << 2)

#define F_GLT_DIRECT                    (F_GLT_LEFT|F_GLT_RIGHT)

#define MAX_LOM_STRING                  4096

typedef struct ___lom_strings_header
{
  uint32_t flags;
  g_op g_oper_ptr;
  __g_match m_ref;
  char string[8192];
} _lom_s_h, *__lom_s_h;

#define G_LOM_VAR_L(x, y, d_ptr) { \
    if (lom->g_tf_ptr_left) \
      { \
        lom->y = (x)lom->g_tf_ptr_left(d_ptr, lom->t_l_off); \
      } \
    else if (lom->g_t_ptr_left) \
      { \
        lom->y = (x) lom->g_t_ptr_left(d_ptr, lom->t_l_off); \
      } \
    else if (lom->g_ts_ptr_left) \
      { \
        lom->y = (x) lom->g_ts_ptr_left(d_ptr, lom->t_l_off); \
      } \
    if ((lom->flags & F_LOM_IS_LVAR_MATH)) { \
        g_math_res(d_ptr, &lom->math_l, &lom->y);\
      } \
}

#define G_LOM_VAR_R(x,y, d_ptr) { \
    if (lom->g_tf_ptr_right) \
      { \
        lom->y = (x)lom->g_tf_ptr_right(d_ptr, lom->t_r_off); \
      } \
    else if (lom->g_t_ptr_right) \
      { \
        lom->y = (x) lom->g_t_ptr_right(d_ptr, lom->t_r_off); \
      } \
    else if (lom->g_ts_ptr_right) \
      { \
        lom->y = (x) lom->g_ts_ptr_right(d_ptr, lom->t_r_off); \
      } \
      if ((lom->flags & F_LOM_IS_RVAR_MATH)) { \
          g_math_res(d_ptr, &lom->math_r, &lom->y);\
      } \
}

#define G_LOM_VAR(x,y, d_ptr, g_ptr) { \
      lom->y = (x)lom->g_ptr(d_ptr, lom->t_r_off); \
}

typedef int
__d_lom_vp(void *d_ptr, void *_lom);

__d_lom_vp g_lom_var_int, g_lom_var_uint, g_lom_var, g_lom_var_math,
    g_lom_var_float, g_lom_var_accu_uint, g_lom_var_accu_int,
    g_lom_var_accu_float;
int
g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm);
int
g_lom_accu(__g_handle hdl, void *d_ptr, pmda _accumulator);
int
g_load_lom(__g_handle hdl);
int
g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
    uint32_t flags);
int
g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
    size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
    uint32_t flags);

int
g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags);
int
g_get_lom_alignment(__g_lom lom, uint32_t flags, int *vb, size_t off);

int
opt_g_lom(void *arg, int m, uint32_t flags);
int
g_build_lom_packet_bare(__g_handle hdl, __g_lom lom, char *field, void *right,
    void *comp_set[], g_op lop);
int
g_lom_match_bare(__g_handle hdl, void *d_ptr, __g_lom lom);

void *_lcs_isequal[3];
void *_lcs_ishigher[3];
void *_lcs_islower[3];
void *_lcs_islowerorequal[3];
void *_lcs_ishigherorequal[3];
void *_lcs_isnotequal[3];
void *_lcs_isnot[3];

#endif /* M_LOM_H_ */
