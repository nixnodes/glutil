/*
 * log_op.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LOG_OP_H_
#define LOG_OP_H_


#include <fp_types.h>
#include <im_hdr.h>

char *
g_dgetf(char *str);

typedef int
_d_ag_handle_i(__g_handle);

_d_ag_handle_i determine_datatype;
int
g_proc_mr(__g_handle hdl);
int
find_absolute_path(char *exec, char *output);

char *_print_ptr;

int
online_format_block_comp(void *iarg, char *output);
int
data_backup_records(char *file);
int
rebuild(void *arg);
int
d_gen_dump(char *arg);

#endif /* LOG_OP_H_ */
