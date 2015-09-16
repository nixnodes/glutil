/*
 * exec.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef EXEC_H_
#define EXEC_H_

#include <glutil.h>

#include <memory_t.h>
#include <fp_types.h>
#include <im_hdr.h>

typedef struct ___d_argv_ch
{
  int cindex;
  mda mech;
} _d_argv_ch, *__d_argv_ch;

int
g_build_argv_c (__g_handle hdl);

int
g_do_exec_v (void *buffer, void *callback, char *ex_str, void * p_hdl);
int
process_execv_args (void *data, __g_handle hdl);
int
process_execv_args_bare (void *data, __g_handle hdl, __execv exec_args);

int
l_execv (char *exec, char **argv, __g_handle hdl);
_d_wpid_cb l_waitpid_def;

void
l_post_exec_cleanup (__g_handle hdl);

char *exec_str;
char **exec_v;
int exec_vc;

int
g_do_exec_fb (void *buffer, void *callback, char *ex_str, void *hdl);

__d_exec exc;

long amax;

int
opt_execv (void *arg, int m, void *opt);
int
opt_execv_stdin (void *arg, int m, void *opt);

int
process_exec_string (char *input, char *output, size_t max_size, void *callback,
		     void *data);
int
g_do_exec (void *buffer, void *callback, char *ex_str, void *hdl);

int
g_init_execv_bare (__execv exec_args, __g_handle hdl, char *i_exec_str);
int
g_build_argv_c_bare (__execv exec_args, __g_handle hdl);
int
process_exec_string_n (char *input, char *output, size_t max_size,
		       _d_rtv_lk callback, void *data, void *mppd);

#endif /* EXEC_H_ */
