/*
 * m_general.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef M_GENERAL_H_
#define M_GENERAL_H_

#include <glutil.h>

#include <m_comp.h>
#include <fp_types.h>
#include <stdio.h>


#define G_MATCH         ((int)0)
#define G_NOMATCH       ((int)1)

#define F_GM_ISREGEX                    (a32 << 1)
#define F_GM_ISMATCH                    (a32 << 2)
#define F_GM_ISLOM                      (a32 << 3)
#define F_GM_IMATCH                     (a32 << 4)
#define F_GM_NAND                       (a32 << 5)
#define F_GM_NOR                        (a32 << 6)

#define F_GM_TYPES                      (F_GM_ISREGEX|F_GM_ISMATCH|F_GM_ISLOM)



typedef float
(*g_tf_p)(void *base, size_t offset);
typedef uint64_t
(*g_t_p)(void *base, size_t offset);
typedef int64_t
(*g_ts_p)(void *base, size_t offset);
typedef int
(*g_op)(int s, int d);
void
g_ipcbm(void *ptr, pmda md, int *r_p);
int
g_filter(__g_handle hdl, pmda md);
int
g_bmatch(void *, __g_handle, pmda md);
int
opt_g_operator_or(void *arg, int m);
int
opt_g_operator_and(void *arg, int m);

typedef struct ___g_lom
{
  int result;
  /* --- */
  uint32_t flags;
  g_t_p g_t_ptr_left;
  g_ts_p g_ts_ptr_left;
  g_t_p g_t_ptr_right;
  g_ts_p g_ts_ptr_right;
  g_tf_p g_tf_ptr_left;
  g_tf_p g_tf_ptr_right;
  int
  (*g_icomp_ptr)(uint64_t s, uint64_t d);
  int
  (*g_iscomp_ptr)(int64_t s, int64_t d);
  int
  (*g_fcomp_ptr)(float s, float d);
  int
  (*g_lom_vp)(void *d_ptr, void * lom);
  g_op g_oper_ptr;
  uint64_t t_left, t_right;
  int64_t ts_left, ts_right;
  float tf_left, tf_right;
  /* --- */
  size_t t_l_off, t_r_off;
} _g_lom, *__g_lom;

typedef struct ___g_match_h
{
  uint32_t flags;
  char *match, *field;
  int reg_i_m, match_i_m, regex_flags;
  regex_t preg;
  mda lom;
  g_op g_oper_ptr;
  __g_proc_v pmstr_cb;
  _d_drt_h dtr;
  char *data;
  void *hdl_ref;
} _g_match, *__g_match;

mda _match_rr;

typedef struct ___last_match
{
  uint32_t flags;
  void *ptr;
} _l_match, *__l_match;

__g_match
g_global_register_match(void);
int
do_match(__g_handle hdl, void *d_ptr, __g_match _gm);

_l_match _match_rr_l;


#endif /* M_GENERAL_H_ */
