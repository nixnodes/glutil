/*
 * lref_gen.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LREF_GEN_H_
#define LREF_GEN_H_

#include <t_glob.h>
#include <fp_types.h>

#define _MC_GE_I1       "i1"
#define _MC_GE_I2       "i2"
#define _MC_GE_I3       "i3"
#define _MC_GE_I4       "i4"
#define _MC_GE_U1       "u1"
#define _MC_GE_U2       "u2"
#define _MC_GE_U3       "u3"
#define _MC_GE_U4       "u4"
#define _MC_GE_F1       "f1"
#define _MC_GE_F2       "f2"
#define _MC_GE_F3       "f3"
#define _MC_GE_F4       "f4"
#define _MC_GE_UL1      "ul1"
#define _MC_GE_UL2      "ul2"
#define _MC_GE_UL3      "ul3"
#define _MC_GE_UL4      "ul4"
#define _MC_GE_GE1      "ge1"
#define _MC_GE_GE2      "ge2"
#define _MC_GE_GE3      "ge3"
#define _MC_GE_GE4      "ge4"
#define _MC_GE_GE5      "ge5"
#define _MC_GE_GE6      "ge6"
#define _MC_GE_GE7      "ge7"
#define _MC_GE_GE8      "ge8"

#define DT_GG_GIDX(m) { \
  idx = (int) strtol(&m[7], NULL, 10);  \
  if (!(idx >= 0 && idx <= MAX_GLOB_STOR_AR_COUNT))  \
    {  \
      return NULL;  \
    } \
}

_d_rtv_lk ref_to_val_lk_generic;

int
ref_to_val_generic(void *arg, char *match, char *output, size_t max_size,
    void *mppd);

__g_proc_rv dt_rval_generic_nukestr, dt_rval_generic_procid,
    dt_rval_generic_ipc, dt_rval_generic_usroot, dt_rval_generic_logroot,
    dt_rval_generic_memlimit, dt_rval_generic_curtime, dt_rval_q,
    dt_rval_generic_exe, dt_rval_generic_glroot, dt_rval_generic_siteroot,
    dt_rval_generic_siterootn, dt_rval_generic_ftpdata,
    dt_rval_generic_imdbfile, dt_rval_generic_tvfile, dt_rval_generic_gamefile,
    dt_rval_generic_spec1, dt_rval_generic_glconf, dt_rval_generic_logfile,
    dt_rval_gg_int, dt_rval_gg_float;

#define L_AV_MAX        256

char *l_av_st[L_AV_MAX];

#endif /* LREF_GEN_H_ */
