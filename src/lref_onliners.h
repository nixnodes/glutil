/*
 * lref_online.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_ONLINERS_H_
#define LREF_ONLINERS_H_

#include <fp_types.h>

#define _MC_ONELINERS_MSG       "msg"

__g_proc_rv dt_rval_oneliners_time, dt_rval_oneliners_user,
    dt_rval_oneliners_group, dt_rval_oneliners_tag, dt_rval_oneliners_msg;

_d_rtv_lk ref_to_val_lk_oneliners;

__d_ref_to_pval ref_to_val_ptr_oneliners;

__d_format_block oneliner_format_block, oneliner_format_block_batch,
    oneliner_format_block_exp;

int
gcb_oneliner (void *buffer, char *key, char *val);

#endif /* LREF_ONLINE_H_ */
