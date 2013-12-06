/*
 * lref_lastonlog.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LREF_LASTONLOG_H_
#define LREF_LASTONLOG_H_

#define _MC_LASTONLOG_STATS     "stats"

__d_ref_to_pval ref_to_val_ptr_lastonlog;

_d_rtv_lk ref_to_val_lk_lastonlog;


__d_format_block lastonlog_format_block, lastonlog_format_block_batch, lastonlog_format_block_exp;

__g_proc_rv dt_rval_lastonlog_logon, dt_rval_lastonlog_logoff,
    dt_rval_lastonlog_upload, dt_rval_lastonlog_download,
    dt_rval_lastonlog_config, dt_rval_lastonlog_user, dt_rval_lastonlog_user,
    dt_rval_lastonlog_group, dt_rval_lastonlog_stats, dt_rval_lastonlog_tag;

int
gcb_lastonlog(void *buffer, char *key, char *val);

#endif /* LREF_LASTONLOG_H_ */
