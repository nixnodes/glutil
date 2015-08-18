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
(*__opt_cptr) (void *arg, int m, void *opt);

#define F_PARSE_ARG_IGNORE_ERRORS      (a32 << 1)
#define F_PARSE_ARG_SILENT             (a32 << 2)
#define F_PARSE_ARG_IGNORE_NOT_FOUND   (a32 << 3)
#define F_PARSE_ARG_BREAK_NOT_FOUND    (a32 << 4)

typedef struct ___gg_opt
{
  uint32_t id;
  char *on;             // option name
  uint8_t ac;           // arg count
  __opt_cptr op;        // option proc funct
} _gg_opt, *__gg_opt;

typedef struct ___opt_dbent
{
  _gg_opt opt;
  uint16_t l;
  void *n_dbent;
} _o_dbent, *__o_dbent;

typedef struct ___g_vop
{
  uint8_t ac_s;
} _g_vop, *__g_vop;

void *
g_pg (void *arg, int m);
char *
g_pd (void *arg, int m, size_t l);
int
g_cpg (void *arg, void *out, int m, size_t sz);

char **
build_argv (char *args, size_t max, int *c);
int
opt_execv_stdout_rd (void *arg, int m, void *opt);
int
parse_args (int argc, char **argv, _gg_opt fref_t[], char ***la, uint32_t flags);

int
register_option (__opt_cptr func, uint8_t argc, char *match, __o_dbent pe);
__gg_opt
proc_option (char *option);
int
parse_args_n (int argc, char **argv, _gg_opt fref_t[], char ***la,
	      uint32_t flags);

int
default_determine_negated (void);

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
     if (check_path(b_in) == -1) \
       { \
         print_str("ERROR: could not open '%s' [%s]\n", b_in, strerror(errno)); \
         return code2; \
       } \
     fn_in = b_in; \
     fh_in = NULL; \
   } \
}

_gg_opt gg_f_ref[1024];
_gg_opt gg_prio_f_ref[128];

_o_dbent glob_dbe[UCHAR_MAX];

typedef struct ___ar_vrp
{
  int ttl;
  uint32_t opt;
  void *arg;
} _ar_vrp, *__ar_vrp;

__ar_vrp
ar_find (pmda md, uint32_t opt);
__ar_vrp
ar_add (pmda md, uint32_t opt, int ttl, void *arg);
void
ar_mod_ttl (pmda md, int by);
int
ar_remove (pmda md, uint32_t opt);

#include <memory_t.h>

mda ar_vref;

#define P_OPT_DL_O      0x7C
#define P_OPT_DL_V      0x3D

typedef int
(*_p_opt_cb) (pmda opt, void *arg);

int
g_parse_opts (char *input, _p_opt_cb _proc, void *arg, int dl_o, int dl_v);

#endif /* ARG_PROC_H_ */
