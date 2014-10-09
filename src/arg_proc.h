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

typedef int
(*__opt_cptr)(void *arg, int m);

typedef struct ___gg_opt
{
  uint32_t id;
  char *on;             // option name
  uint8_t ac;           // arg count
  __opt_cptr op;        // option proc funct
} _gg_opt, *__gg_opt;

void *
g_pg(void *arg, int m);
char *
g_pd(void *arg, int m, size_t l);
int
g_cpg(void *arg, void *out, int m, size_t sz);

char **
build_argv(char *args, size_t max, int *c);
int
opt_execv_stdout_rd(void *arg, int m);
int
parse_args(int argc, char **argv, _gg_opt fref_t[]);

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

_gg_opt gg_f_ref[1024];
_gg_opt gg_prio_f_ref[128];

#endif /* ARG_PROC_H_ */
