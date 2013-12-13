/*
 * glutil.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef GLUTIL_H_
#define GLUTIL_H_

#define _LARGEFILE64_SOURCE 1
#define _LARGEFILE_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <glconf.h>

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

int loop_interval;
uint64_t loop_max;

int shmatf;

char MACRO_ARG1[4096];
char MACRO_ARG2[4096];
char MACRO_ARG3[4096];

void *_p_macro_argv;
int _p_macro_argc ;

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

char b_glob[MAX_EXEC_STR + 1];

void *prio_f_ref[8192];
void *f_ref[8192];

char *GLOBAL_PREEXEC;
char *GLOBAL_POSTEXEC;

char *spec_p1;

int
g_print_str(const char * volatile buf, ...);

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
g_init(int argc, char **argv);

#endif /* GLUTIL_H_ */
