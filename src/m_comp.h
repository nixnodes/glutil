/*
 * m_comp.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef M_COMP_H_
#define M_COMP_H_

#include <glutil.h>
#include <t_glob.h>
#include <im_hdr.h>
#include <regex.h>

#define _D_DRT_HASREGEX         (a32 << 10)


typedef struct ___d_drt_h
{
  uint32_t flags;
  char direc[1024];
  __g_proc_v fp_rval1;
  __g_proc_v fp_rval2;
  uint32_t t_1;
  time_t ts_1;
  char c_1;
  size_t vp_off1;
  size_t vp_off2;
  __g_handle hdl;
  char *match;
  mda math;
  mda chains;
  regex_t preg;
  int regex_flags;
  char r_rep[10000];
  uint16_t r_rep_l;
  uint8_t uc_1;
  void *st_ptr0;
  void *rt_cond;
  char *st_p;
  char *st_p0;
  void *st_p1;
  void *v_p0;
  struct ___d_drt_h *mppd_next;
  uint64_t mppd_depth;
  struct ___d_drt_h *mppd_aux_next;
  char tp_b0[10000];
  char *varg_l;
  mda sub_mech;
  uint32_t v_ui0;
} _d_drt_h, *__d_drt_h;


#endif /* M_COMP_H_ */
