/*
 * lref.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LREF_H_
#define LREF_H_

#include <t_glob.h>
#include <l_error.h>

#include <m_comp.h>

#include <mc_glob.h>
#include <m_general.h>

#define MAX_SHARG_SZ    4096

void *
as_ref_to_val_lk(char *match, void *c, __d_drt_h mppd, char *defdc);
char *
g_get_stf(char *match);

typedef void*
rtv_af(void *arg, char *match, char *output, size_t max_size, __d_drt_h mppd);

rtv_af ref_to_val_af, ref_to_val_af_math;

int
rtv_q(void *query, char *output, size_t max_size);

char*
l_mppd_shell_ex(char *input, char *output, size_t max_size, __d_drt_h mppd);

#define PROC_SH_EX(match) \
{ \
char m_b[MAX_SHARG_SZ]; \
  if (NULL == (match = l_mppd_shell_ex(match, m_b, MAX_SHARG_SZ, mppd))) \
    { \
      return NULL; \
    } \
};

#endif /* LREF_H_ */
