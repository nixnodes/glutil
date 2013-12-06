/*
 * lref_nukelog.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LREF_NUKELOG_H_
#define LREF_NUKELOG_H_

#include <m_comp.h>

#define _MC_NUKELOG_NUKER               "nuker"
#define _MC_NUKELOG_UNNUKER             "unnuker"
#define _MC_NUKELOG_NUKEE               "nukee"
#define _MC_NUKELOG_REASON              "reason"
#define _MC_NUKELOG_MULT                "mult"

__d_ref_to_pval ref_to_val_ptr_nukelog;
_d_rtv_lk ref_to_val_lk_nukelog;

__g_proc_rv dt_rval_nukelog_size, dt_rval_nukelog_time, dt_rval_nukelog_status,
    dt_rval_nukelog_mult, dt_rval_nukelog_mode_e, dt_rval_nukelog_dir,
    dt_rval_nukelog_basedir_e, dt_rval_nukelog_nuker, dt_rval_nukelog_nukee,
    dt_rval_nukelog_unnuker, dt_rval_nukelog_reason, dt_rval_x_nukelog, dt_rval_xg_nukelog;


__d_format_block nukelog_format_block, nukelog_format_block_batch, nukelog_format_block_exp;

int
gcb_nukelog(void *buffer, char *key, char *val);

#endif /* LREF_NUKELOG_H_ */
