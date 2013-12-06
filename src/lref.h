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

void *
as_ref_to_val_lk(char *match, void *c, __d_drt_h mppd, char *defdc);
char *
g_get_stf(char *match);

void *
ref_to_val_af(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd);

int
rtv_q(void *query, char *output, size_t max_size);

#endif /* LREF_H_ */
