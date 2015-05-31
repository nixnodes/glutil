/*
 * init.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"

#include <l_error.h>
#include <fp_types.h>
#include <m_general.h>
#include <exec_t.h>

#include <unistd.h>
#include <errno.h>

#include <stdlib.h>

struct d_stats dl_stats =
  { 0 };
struct d_stats nl_stats =
  { 0 };

_g_handle g_act_1 =
  { 0 };
_g_handle g_act_2 =
  { 0 };

mda glconf =
  { 0 };

char MACRO_ARG1[4096] =
  { 0 };
char MACRO_ARG2[4096] =
  { 0 };
char MACRO_ARG3[4096] =
  { 0 };

char *g_mroot = NULL;

void *_p_macro_argv = NULL;
char **_p_argv = NULL;

int _p_macro_argc = 0;

uint32_t g_sleep = 0;
uint32_t g_usleep = 0;

void *p_argv_off = NULL;
void *prio_argv_off = NULL;

mda cfg_rf =
  { 0 };

uint32_t flags_udcfg = 0;

off_t max_hits = 0, max_results = 0;

int g_regex_flags = REG_EXTENDED;

size_t max_datain_f = MAX_DATAIN_F;

key_t SHM_IPC = (key_t) shm_ipc;

#include <sys/stat.h>

int g_shmcflags = S_IRUSR | S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;
;

char GLROOT[PATH_MAX] =
  { glroot };
char SITEROOT_N[PATH_MAX] =
  { siteroot };
char SITEROOT[PATH_MAX] =
  { 0 };
char DIRLOG[PATH_MAX] =
  { dir_log };
char NUKELOG[PATH_MAX] =
  { nuke_log };
char DU_FLD[PATH_MAX] =
  { du_fld };
char DUPEFILE[PATH_MAX] =
  { dupe_file };
char LASTONLOG[PATH_MAX] =
  { last_on_log };
char ONELINERS[PATH_MAX] =
  { oneliner_file };
char FTPDATA[PATH_MAX] =
  { ftp_data };
char IMDBLOG[PATH_MAX] =
  { imdb_file };
char GAMELOG[PATH_MAX] =
  { game_log };
char TVLOG[PATH_MAX] =
  { tv_log };
char GE1LOG[PATH_MAX] =
  { ge1_log };
char GE2LOG[PATH_MAX] =
  { ge2_log };
char GE3LOG[PATH_MAX] =
  { ge3_log };
char GE4LOG[PATH_MAX] =
  { ge4_log };
char SCONFLOG[PATH_MAX] =
  { sconf_log };
char GCONFLOG[PATH_MAX] =
  { gconf_log };
char ALTLOG[PATH_MAX] =
  { alt_log };
char XLOG[PATH_MAX] =
  { x_log };

#ifdef GLCONF
char GLCONF_I[PATH_MAX] =
  { GLCONF };
#else
char GLCONF_I[PATH_MAX] =
  { 0};
#endif

char b_spec1[PATH_MAX];

char LOGFILE[PATH_MAX];

FILE *fd_log = NULL;

char *LOOPEXEC = NULL;

char *NUKESTR_d[255] =
  { NUKESTR_DEF };
char *NUKESTR = (char*) NUKESTR_d;

int updmode = 0;
char *argv_off = NULL;

sigjmp g_sigjmp =
  {
    {
      {
        { 0 } } } };

mda _match_rr =
  { 0 };

pmda _match_clvl = &_match_rr;

_l_match _match_rr_l =
  { 0 };

_g_handle _dummy_hio =
  { 0 };

char *GLOBAL_PREEXEC = NULL;
char *GLOBAL_POSTEXEC = NULL;

uint64_t glob_ui64_stor[MAX_GLOB_STOR_AR_COUNT] =
  { 0 };
int64_t glob_si64_stor[MAX_GLOB_STOR_AR_COUNT] =
  { 0 };
float glob_float_stor[MAX_GLOB_STOR_AR_COUNT] =
  { 0 };

uint64_t gfl0 = 0, gfl = F_OPT_WBUFFER, status = 0;
uint32_t ofl = 0;

uint8_t l_sfo = 0;

uint32_t xref_flags = 0;

char *spec_p1 = NULL;
char *spec_p2 = NULL;
char *spec_p3 = NULL;
char *spec_p4 = NULL;

#ifndef _MAKE_SBIN
__dt_set pdt_set_dirlog = dt_set_dirlog, pdt_set_nukelog = dt_set_nukelog,
    pdt_set_lastonlog = dt_set_lastonlog, pdt_set_dupefile = dt_set_dupefile,
    pdt_set_oneliners = dt_set_oneliners, pdt_set_imdb = dt_set_imdb,
    pdt_set_game = dt_set_game, pdt_set_tvrage = dt_set_tvrage, pdt_set_gen1 =
        dt_set_gen1, pdt_set_gen2 = dt_set_gen2, pdt_set_gen3 = dt_set_inetobj,
    pdt_set_gen4 = dt_set_gen4, pdt_set_gconf = dt_set_gconf, pdt_set_sconf =
        dt_set_sconf, pdt_set_altlog = dt_set_altlog, pdt_set_online =
        dt_set_online, pdt_set_x = dt_set_x;
;
#endif
int
(*__pf_eof)(void *p);

char G_USER[128] =
  { 0 };
char G_GROUP[128] =
  { 0 };

#ifdef _G_SSYS_THREAD

#include <pthread.h>
#include <thread.h>

pthread_mutex_t mutex_glob00 =
  {
    { 0 } };
#endif

void
g_send_gkill(void)
{
#ifdef _G_SSYS_THREAD
  mutex_lock(&mutex_glob00);
#endif
  gfl |= F_OPT_KILL_GLOBAL;
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock(&mutex_glob00);
#endif
}

char
g_get_gkill(void)
{
#ifdef _G_SSYS_THREAD
  mutex_lock(&mutex_glob00);
#endif
  int ret;
  if (gfl & F_OPT_KILL_GLOBAL)
    {
      ret = 0;
    }
  else
    {
      ret = 1;
    }
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock(&mutex_glob00);
#endif
  return ret;
}
