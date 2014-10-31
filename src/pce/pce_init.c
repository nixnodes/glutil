/*
 * pce_init.c

 *
 *  Created on: Dec 8, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "pce_config.h"
#include <config.h>
#include "pce_proc.h"
#include "pce_misc.h"

#include <l_error.h>
#include <misc.h>
#include <x_f.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

uint32_t pce_f = 0;
char *s_year = NULL;
int EXITVAL = 0;
uint32_t pce_gfl = 0;
int32_t pce_lm = 0;

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
  { dir_log};
char NUKELOG[PATH_MAX] =
  { nuke_log};
char DU_FLD[PATH_MAX] =
  { du_fld};
char DUPEFILE[PATH_MAX] =
  { dupe_file};
char LASTONLOG[PATH_MAX] =
  { last_on_log};
char ONELINERS[PATH_MAX] =
  { oneliner_file};
char FTPDATA[PATH_MAX] =
  { ftp_data };
char IMDBLOG[PATH_MAX] =
  { imdb_file};
char GAMELOG[PATH_MAX] =
  { game_log};
char TVLOG[PATH_MAX] =
  { tv_log};
char GE1LOG[PATH_MAX] =
  { ge1_log};
char GE2LOG[PATH_MAX] =
  { ge2_log};
char GE3LOG[PATH_MAX] =
  { ge3_log};
char GE4LOG[PATH_MAX] =
  { ge4_log};
char SCONFLOG[PATH_MAX] =
  { sconf_log };
char GCONFLOG[PATH_MAX] =
  { gconf_log};

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

_l_match _match_rr_l =
  { 0 };

char *GLOBAL_PREEXEC = NULL;
char *GLOBAL_POSTEXEC = NULL;

uint64_t gfl0 = 0x0, gfl = F_OPT_WBUFFER;
uint32_t ofl = 0;

char *spec_p1 = NULL;
char *spec_p2 = NULL;
char *spec_p3 = NULL;
char *spec_p4 = NULL;

#include <sys/stat.h>

int g_shmcflags = S_IRUSR | S_IRGRP | S_IROTH;

__dt_set pdt_set_dirlog = dt_set_dummy, pdt_set_nukelog = dt_set_dummy,
    pdt_set_lastonlog = dt_set_dummy, pdt_set_dupefile = dt_set_dummy,
    pdt_set_oneliners = dt_set_dummy, pdt_set_imdb = dt_set_imdb, pdt_set_game =
        dt_set_game, pdt_set_tvrage = dt_set_tvrage,
    pdt_set_gen1 = dt_set_dummy, pdt_set_gen2 = dt_set_dummy, pdt_set_gen3 =
        dt_set_dummy, pdt_set_gen4 = dt_set_dummy, pdt_set_gconf = dt_set_gconf,
    pdt_set_sconf = dt_set_sconf;

int
main(int argc, char *argv[])
{
  print_str = pce_log;
  LOGLVL = LOG_LEVEL;
  setup_sighandlers();

  //gfl |= F_OPT_PS_SILENT;
  pce_proc(argv[2], argv[1]);

  if (fd_log)
    {
      fclose(fd_log);
    }

  if ((pce_f & F_PCE_FORKED))
    {
      _exit(EXITVAL);
    }
  exit(EXITVAL);
}

void
pce_enable_logging(void)
{
  if ((gfl & F_OPT_PS_LOGGING) && !fd_log)
    {
      if (!(fd_log = fopen(pce_logfile, "a")))
        {
          gfl ^= F_OPT_PS_LOGGING;
        }
    }
  return;
}
