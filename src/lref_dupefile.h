/*
 * lref_dupefile.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LREF_DUPEFILE_H_
#define LREF_DUPEFILE_H_

#include <glutil.h>

#include <fp_types.h>

#define _MC_GLOB_FILE   "file"

__d_ref_to_pval ref_to_val_ptr_dupefile;
_d_rtv_lk ref_to_val_lk_dupefile;

__g_proc_rv dt_rval_dupefile_time, dt_rval_dupefile_file, dt_rval_dupefile_user;

__d_format_block dupefile_format_block, dupefile_format_block_batch, dupefile_format_block_exp;

int
gcb_dupefile(void *buffer, char *key, char *val);

#endif /* LREF_DUPEFILE_H_ */
