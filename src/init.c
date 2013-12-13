/*
 * init.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <l_error.h>
#include <im_hdr.h>
#include <t_glob.h>
#include <l_sb.h>
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

void *_p_macro_argv = NULL;
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

char *NUKESTR_d[255] = { NUKESTR_DEF };
char *NUKESTR = (char*)NUKESTR_d;

int updmode = 0;
char *argv_off = NULL;

sigjmp g_sigjmp =
  {
    {
      {
        { 0 } } } };

mda _match_rr =
  { 0 };

_l_match _match_rr_l =
  { 0 };

char *GLOBAL_PREEXEC = NULL;
char *GLOBAL_POSTEXEC = NULL;

uint64_t gfl0 = 0x0, gfl = F_OPT_WBUFFER;
uint32_t ofl = 0;

char *spec_p1 = NULL;
