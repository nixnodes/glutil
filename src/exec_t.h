/*
 * exec.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef EXEC_H_
#define EXEC_H_

#include <memory_t.h>
#include <fp_types.h>



typedef struct ___d_argv_ch
{
  int cindex;
  mda mech;
} _d_argv_ch, *__d_argv_ch;

int
g_build_argv_c(__g_handle hdl);

int
g_do_exec_v(void *buffer, void *callback, char *ex_str, void * p_hdl);
int
process_execv_args(void *data, __g_handle hdl);
int
prep_for_exec(void);

int
l_execv(char *exec, char **argv);

char *exec_str;
char **exec_v;
int exec_vc;

int
g_do_exec_fb(void *buffer, void *callback, char *ex_str, void *hdl);

__d_exec exc;

long amax;

int
opt_execv(void *arg, int m);

int
process_exec_string(char *input, char *output, size_t max_size, void *callback,
    void *data);
int
g_do_exec(void *buffer, void *callback, char *ex_str, void *hdl);

#endif /* EXEC_H_ */
