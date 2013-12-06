/*
 * arg_proc.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef ARG_PROC_H_
#define ARG_PROC_H_

#include <stdio.h>

void *
g_pg(void *arg, int m);
char *
g_pd(void *arg, int m, size_t l);
int
g_cpg(void *arg, void *out, int m, size_t sz);

char **
build_argv(char *args, size_t max, int *c);
int
opt_execv_stdout_redir(void *arg, int m);
int
parse_args(int argc, char **argv, void*fref_t[]);

typedef struct option_reference_array
{
  char *option;
  void *function, *arg_cnt;
}*p_ora;

#endif /* ARG_PROC_H_ */
