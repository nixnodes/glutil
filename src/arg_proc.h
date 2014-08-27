/*
 * arg_proc.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef ARG_PROC_H_
#define ARG_PROC_H_

#include <glutil.h>

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

#define O_FI_STDIN(arg, m, code, code2) { \
 b_in = g_pg(arg, m); \
 if (NULL == b_in) { \
     return code; \
 } \
 if (b_in[0] == 0x2D) \
   { \
     fh_in = stdin; \
     fn_in = NULL; \
   } \
 else \
   { \
     if (file_exists(b_in)) \
       { \
         return code2; \
       } \
     fn_in = b_in; \
     fh_in = NULL; \
   } \
}


#endif /* ARG_PROC_H_ */
