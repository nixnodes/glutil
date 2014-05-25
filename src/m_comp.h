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
  char direc[128];
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
  regex_t preg;
  int regex_flags;
  char r_rep[2048];
  uint16_t r_rep_l;
  uint64_t *p_accu;
  uint8_t uc_1;
} _d_drt_h, *__d_drt_h;

#endif /* M_COMP_H_ */
