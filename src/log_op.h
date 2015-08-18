/*
 * log_op.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef LOG_OP_H_
#define LOG_OP_H_

#include <glutil.h>

#include <fp_types.h>
#include <im_hdr.h>

char *
g_dgetf(char *str);

typedef int
_d_ag_handle_i(__g_handle);

int
determine_datatype(__g_handle hdl, char *file);
int
g_proc_mr(__g_handle hdl);

char *_print_ptr, *_print_ptr_post, *_print_ptr_pre, *_cl_print_ptr;

int
data_backup_records(char *file);
int
rebuild(void *arg);
int
d_gen_dump(char *arg);


#endif /* LOG_OP_H_ */
