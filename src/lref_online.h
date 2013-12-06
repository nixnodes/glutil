/*
 * lref_online.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_ONLINE_H_
#define LREF_ONLINE_H_

#define _MC_ONLINE_BTXFER       "btxfer"
#define _MC_ONLINE_BXFER        "bxfer"
#define _MC_ONLINE_GROUP        "group"
#define _MC_ONLINE_SSL          "ssl"
#define _MC_ONLINE_LUPDT        "lupdtime"
#define _MC_ONLINE_LXFRT        "lxfertime"
#define _MC_ONLINE_HOST         "host"

#include <fp_types.h>

int
online_format_block_comp(void *iarg, char *output);

_d_rtv_lk ref_to_val_lk_online;

__g_proc_rv dt_rval_online_ssl, dt_rval_online_group, dt_rval_online_time,
    dt_rval_online_lupdt, dt_rval_online_lxfrt, dt_rval_online_bxfer,
    dt_rval_online_btxfer, dt_rval_online_pid, dt_rval_online_rate,
    dt_rval_online_basedir, dt_rval_online_ndir, dt_rval_online_user,
    dt_rval_online_tag, dt_rval_online_status, dt_rval_online_host,
    dt_rval_online_dir, dt_rval_online_config;

__d_ref_to_pval ref_to_val_ptr_online;

__d_format_block online_format_block, online_format_block_batch, online_format_block_exp;



#endif /* LREF_ONLINE_H_ */
