/*
 * lref_dirlog.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LREF_DIRLOG_H_
#define LREF_DIRLOG_H_

#include <glutil.h>

#include <fp_types.h>

#define _MC_DIRLOG_FILES        "files"


__g_proc_rv dt_rval_dirlog_user, dt_rval_dirlog_group, dt_rval_dirlog_files,
    dt_rval_dirlog_size, dt_rval_dirlog_status, dt_rval_dirlog_time,
    dt_rval_dirlog_mode_e, dt_rval_dirlog_dir, dt_rval_xg_dirlog,
    dt_rval_x_dirlog, dt_rval_dirlog_basedir;

__d_ref_to_pval ref_to_val_ptr_dirlog;
_d_rtv_lk ref_to_val_lk_dirlog;

__d_format_block dirlog_format_block,dirlog_format_block_exp,  dirlog_format_block_batch;

int
gcb_dirlog(void *buffer, char *key, char *val);

#endif /* LREF_DIRLOG_H_ */
