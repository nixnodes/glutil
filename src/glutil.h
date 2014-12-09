/*
 * glutil.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef GLUTIL_H_
#define GLUTIL_H_

#include <glc.h>

#define DL_SZ                           sizeof(struct dirlog)
#define NL_SZ                           sizeof(struct nukelog)
#define DF_SZ                           sizeof(struct dupefile)
#define LO_SZ                           sizeof(struct lastonlog)
#define OL_SZ                           sizeof(struct oneliner)
#define ON_SZ                           sizeof(struct ONLINE)

#include <g_conf.h>

#include <t_glob.h>
#include <l_sb.h>
#include <im_hdr.h>

#include <stdint.h>
#include <inttypes.h>

#define DEFF_DIRLOG                     "dirlog"
#define DEFF_NUKELOG                    "nukelog"
#define DEFF_LASTONLOG                  "laston.log"
#define DEFF_DUPEFILE                   "dupefile"
#define DEFF_ONELINERS                  "oneliners.log"
#define DEFF_DULOG                      "glutil.log"
#define DEFF_IMDB                       "imdb.log"
#define DEFF_GAMELOG                    "game.log"
#define DEFF_TV                         "tv.log"
#define DEFF_GEN1                       "gen1.log"
#define DEFF_GEN2                       "gen2.log"
#define DEFF_GEN3                       "gen3.log"
#define DEFF_GEN4                       "gen4.log"

struct d_stats
{
  uint64_t bw, br, rw;
};

struct d_stats dl_stats;
struct d_stats nl_stats;

_g_handle g_act_1;
_g_handle g_act_2;

mda dirlog_buffer;
mda nukelog_buffer;

mda glconf;

int EXITVAL;

uint64_t loop_max;

int shmatf;

char MACRO_ARG1[4096];
char MACRO_ARG2[4096];
char MACRO_ARG3[4096];

char *g_mroot;

void *_p_macro_argv;
char **_p_argv;
int _p_macro_argc;

uint32_t g_sleep;
uint32_t g_usleep;

void *p_argv_off;
void *prio_argv_off;

mda cfg_rf;

uint32_t flags_udcfg;

off_t max_hits;
off_t max_results;

int execv_stdout_redir;

int g_regex_flags;

size_t max_datain_f;

char infile_p[PATH_MAX];

off_t max_depth, min_depth;

char b_glob[MAX_EXEC_STR + 4];

int g_shmcflags;

#define MAX_GLOB_STOR_AR_COUNT          255

uint64_t glob_ui64_stor[MAX_GLOB_STOR_AR_COUNT];
int64_t glob_si64_stor[MAX_GLOB_STOR_AR_COUNT];
float glob_float_stor[MAX_GLOB_STOR_AR_COUNT];
int64_t glob_curtime;

char *GLOBAL_PREEXEC;
char *GLOBAL_POSTEXEC;

char *spec_p1, *spec_p2, *spec_p3, *spec_p4;

uint32_t g_omfp_sto, g_omfp_suto;

uint32_t xref_flags;

#ifdef _G_SSYS_THREAD

#include <pthread.h>

pthread_mutex_t mutex_glob00;
#endif

int
g_print_str(const char * volatile buf, ...);

int
(*__pf_eof)(void *p);

#include <signal.h>

int
setup_sighandlers(void);
void
sig_handler(int);
void
child_sig_handler(int, siginfo_t*, void*);
int
g_shutdown(void *arg);
int
g_init(int argc, char **argv, char **l_arg);

#endif /* GLUTIL_H_ */
