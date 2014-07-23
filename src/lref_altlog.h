/*
 * lref_altlog.h
 *
 *  Created on: Jul 22, 2014
 *      Author: reboot
 */

#ifndef LREF_ALTLOG_H_
#define LREF_ALTLOG_H_

#include <glutil.h>

#include <fp_types.h>

#define _MC_ALTLOG_FILES        "files"

struct altlog {
    uint16_t    status;          /* 0 = NEWDIR, 1 = NUKE, 2 = UNNUKE, 3 = DELETED */
    time32_t    uptime;          /* Creation time since epoch (man 2 time) */
    //uint16_t    uploader;        /* The userid of the creator */
    //uint16_t    group;           /* The groupid of the primary group of the creator */
    uint16_t    files;           /* The number of files inside the dir */
    uint64_t    bytes;           /* The number of bytes in the dir */
    char        user[255];
    char        groupn[255];
    char        dirname[4096];    /* The name of the dir (fullpath) */
};

#define AL_SZ                           sizeof(struct altlog)

__g_proc_rv dt_rval_altlog_user, dt_rval_altlog_group, dt_rval_altlog_files,
    dt_rval_altlog_size, dt_rval_altlog_status, dt_rval_altlog_time,
    dt_rval_altlog_mode_e, dt_rval_altlog_dir, dt_rval_xg_altlog,
    dt_rval_x_altlog, dt_rval_altlog_basedir;

__d_ref_to_pval ref_to_val_ptr_altlog;
_d_rtv_lk ref_to_val_lk_altlog;

__d_format_block altlog_format_block,altlog_format_block_exp,  altlog_format_block_batch;

int
gcb_altlog(void *buffer, char *key, char *val);

#endif /* LREF_ALTLOG_H_ */
