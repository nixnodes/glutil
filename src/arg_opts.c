/*
 * arg_opts.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "arg_opts.h"

#include <m_general.h>
#include <m_string.h>
#include <m_lom.h>

#include <g_modes.h>
#include <g_help.h>
#include <exec_t.h>
#include <log_op.h>
#include <exec_t.h>
#include <xref.h>
#include <arg_proc.h>
#include <misc.h>
#include <x_f.h>
#include <sort_hdr.h>
#include <exec_t.h>
#include <l_error.h>

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_ZLIB_H

uint8_t comp_level = 0;

#endif

int
prio_opt_g_macro(void *arg, int m)
{
  prio_argv_off = g_pg(arg, m);
  updmode = PRIO_UPD_MODE_MACRO;
  return 0;
}

int
prio_opt_g_pinfo(void *arg, int m)
{
  updmode = PRIO_UPD_MODE_INFO;
  return 0;
}

int
opt_g_loglvl(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  int lvl = atoi(buffer), i;
  uint32_t t_LOGLVL = 0;

  for (i = -1; i < lvl; i++)
    {
      t_LOGLVL <<= 1;
      t_LOGLVL |= 0x1;
    }

  LOGLVL = t_LOGLVL;
  gfl |= F_OPT_PS_LOGGING;

  return 0;
}

int
opt_g_verbose(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE;
  return 0;
}

int
opt_g_verbose2(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2;
  return 0;
}

int
opt_g_verbose3(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3;
  return 0;
}

int
opt_g_verbose4(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4;
  return 0;
}

int
opt_g_verbose5(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4
      | F_OPT_VERBOSE5;
  return 0;
}

int
opt_g_force(void *arg, int m)
{
  gfl |= F_OPT_FORCE;
  return 0;
}

int
opt_g_force2(void *arg, int m)
{
  gfl |= (F_OPT_FORCE2 | F_OPT_FORCE);
  return 0;
}

int
opt_g_update(void *arg, int m)
{
  gfl |= F_OPT_UPDATE;
  return 0;
}

int
opt_g_loop(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 7180;
    }

  g_sleep = (uint32_t) strtol(buffer, NULL, 10);
  gfl |= F_OPT_LOOP;

  return 0;
}

int
opt_g_loop_sleep(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  g_omfp_sto = (uint32_t) strtoul(buffer, NULL, 10);
  if (g_omfp_sto)
    {
      gfl0 |= F_OPT_LOOP_SLEEP;
    }

  return 0;
}

int
opt_g_loop_usleep(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  g_omfp_suto = (uint32_t) strtoul(buffer, NULL, 10);
  if (g_omfp_suto)
    {
      gfl0 |= F_OPT_LOOP_USLEEP;
    }

  return 0;
}

int
opt_g_nostats(void *arg, int m)
{
  gfl0 |= F_OPT_NOSTATS;
  return 0;
}

int
opt_g_stats(void *arg, int m)
{
  gfl0 |= F_OPT_STATS;
  return 0;
}

static int
opt_g_mlist(void *arg, int m)
{
  updmode = UPD_MODE_LIST_MACROS;
  return 0;
}

static int
opt_g_comp(void *arg, int m)
{
#ifdef HAVE_ZLIB_H

  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 9180;
    }

  int32_t t = (int32_t) strtol(buffer, NULL, 10);

  if (t < 0 || t > 9)
    {
      return 9181;
    }

  comp_level = (uint8_t) t;

  if (comp_level != 0)
    {
      gfl0 |= F_OPT_GZIP;
    }

  __pf_eof = gz_feof;
  return 0;
#else
  print_str("WARNING: compression not available, this executable was not compiled with zlib\n");
  return 0;
#endif
}

int
opt_loop_max(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  if (buffer == NULL)
    {
      return 9080;
    }
  errno = 0;
  loop_max = (uint64_t) strtol(buffer, NULL, 10);
  if ((errno == ERANGE && (loop_max == LONG_MAX || loop_max == LONG_MIN))
      || (errno != 0 && loop_max == 0))
    {
      return 4080;
    }

  return 0;
}

int
opt_g_udc(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_DUMP_GEN;
  return 0;
}

int
opt_g_dg(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_DUMP_GENERIC;
  return 0;
}

int
opt_g_recursive(void *arg, int m)
{
  flags_udcfg |= F_PD_RECURSIVE;
  return 0;
}

int
opt_g_udc_dir(void *arg, int m)
{
  flags_udcfg |= F_PD_MATCHDIR;
  return 0;
}

int
opt_g_udc_f(void *arg, int m)
{
  flags_udcfg |= F_PD_MATCHREG;
  return 0;
}

static int
opt_g_maxresults(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  max_results = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_HASMAXRES;
  l_sfo = L_STFO_FILTER;
  return 0;
}

static int
opt_g_maxhits(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  max_hits = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_HASMAXHIT;
  l_sfo = L_STFO_FILTER;
  return 0;
}

int
opt_g_maxdepth(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  max_depth = ((off_t) strtoll(buffer, NULL, 10)) + 1;
  gfl |= F_OPT_MAXDEPTH;
  return 0;
}

int
opt_g_mindepth(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  min_depth = ((off_t) strtoll(buffer, NULL, 10)) + 1;
  gfl |= F_OPT_MINDEPTH;
  return 0;
}

int
opt_g_daemonize(void *arg, int m)
{
  gfl |= F_OPT_DAEMONIZE;
  return 0;
}

int
opt_g_ifres(void *arg, int m)
{
  gfl |= F_OPT_IFIRSTRES;
  return 0;
}

int
opt_g_ifhit(void *arg, int m)
{
  gfl |= F_OPT_IFIRSTHIT;
  return 0;
}

int
opt_g_ifrh_e(void *arg, int m)
{
  gfl |= F_OPT_IFRH_E;
  return 0;
}

int
opt_g_nofq(void *arg, int m)
{
  gfl |= F_OPT_NOFQ;
  return 0;
}

int
opt_g_noereg(void *arg, int m)
{
  if (g_regex_flags & REG_EXTENDED)
    {
      g_regex_flags ^= REG_EXTENDED;
    }
  return 0;
}

int
opt_g_fd(void *arg, int m)
{
  gfl |= F_OPT_XFD;
  return 0;
}

int
opt_prune(void *arg, int m)
{
  gfl |= F_OPT_ZPRUNEDUP;
  return 0;
}

int
opt_g_noglconf(void *arg, int m)
{
  gfl |= F_OPT_NOGLCONF;
  return 0;
}

int
opt_g_fix(void *arg, int m)
{
  gfl |= F_OPT_FIX;
  return 0;
}

int
opt_g_sfv(void *arg, int m)
{
  gfl |= F_OPT_SFV;
  return 0;
}

int
opt_batch_output_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_BATCH | F_OPT_PS_SILENT;
  return 0;
}

int
opt_export_output_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_EXPORT | F_OPT_PS_SILENT;
  return 0;
}

int
opt_compact_output_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_COMP;
  return 0;
}

int
opt_g_shmem(void *arg, int m)
{
  gfl |= F_OPT_SHAREDMEM;
  return 0;
}

static int
opt_g_loadq(void *arg, int m)
{
  gfl |= F_OPT_LOADQ;
  return 0;
}

static int
opt_g_loadqa(void *arg, int m)
{
  gfl0 |= F_OPT_LOADQA;
  return 0;
}

static int
opt_g_shmdestroy(void *arg, int m)
{
  gfl |= F_OPT_SHMDESTROY;
  return 0;
}

static int
opt_g_shmdestroyonexit(void *arg, int m)
{
  gfl |= F_OPT_SHMDESTONEXIT;
  return 0;
}

static int
opt_g_shmreload(void *arg, int m)
{
  gfl |= F_OPT_SHMRELOAD;
  return 0;
}

int
opt_g_nowrite(void *arg, int m)
{
  gfl |= F_OPT_NOWRITE;
  return 0;
}

int
opt_g_nobuffering(void *arg, int m)
{
  gfl |= F_OPT_NOBUFFER;
  return 0;
}

int
opt_g_buffering(void *arg, int m)
{
  gfl ^= F_OPT_WBUFFER;
  return 0;
}

int
opt_g_followlinks(void *arg, int m)
{
  gfl |= F_OPT_FOLLOW_LINKS;
  return 0;
}

int
opt_g_ftime(void *arg, int m)
{
  gfl |= F_OPT_PS_TIME;
  return 0;
}

int
opt_g_matchq(void *arg, int m)
{
  gfl |= F_OPT_MATCHQ;
  return 0;
}

int
opt_g_imatchq(void *arg, int m)
{
  gfl |= F_OPT_IMATCHQ;
  return 0;
}

int
opt_update_single_record(void *arg, int m)
{
  argv_off = g_pg(arg, m);
  updmode = UPD_MODE_SINGLE;
  return 0;
}

int
opt_recursive_update_records(void *arg, int m)
{
  updmode = UPD_MODE_RECURSIVE;
  return 0;
}

int
opt_raw_dump(void *arg, int m)
{
  gfl |= F_OPT_MODE_RAWDUMP | F_OPT_PS_SILENT | F_OPT_NOWRITE;
  gfl0 |= F_OPT_PS_ABSSILENT;
  return 0;
}

int
opt_binary(void *arg, int m)
{
  gfl |= F_OPT_MODE_BINARY;
  return 0;
}

int
opt_g_reverse(void *arg, int m)
{
  gfl |= F_OPT_PROCREV;
  return 0;
}

int
opt_silent(void *arg, int m)
{
  gfl |= F_OPT_PS_SILENT;
  return 0;
}

int
opt_logging(void *arg, int m)
{
  gfl |= F_OPT_PS_LOGGING;
  return 0;
}

int
opt_nobackup(void *arg, int m)
{
  gfl |= F_OPT_NOBACKUP;
  return 0;
}

int
opt_backup(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_BACKUP;
  return 0;
}

int
opt_g_lom_match(void *arg, int m)
{
  return opt_g_lom(arg, m, 0);
}

int
opt_g_lom_imatch(void *arg, int m)
{
  return opt_g_lom(arg, m, F_GM_IMATCH);
}

int
opt_g_regexi(void *arg, int m)
{
  return g_cprg(arg, m, 0, 0, REG_ICASE, F_GM_ISREGEX);
}

int
opt_g_match(void *arg, int m)
{
  return g_cprg(arg, m, 0, 0, 0, F_GM_ISMATCH);
}

int
opt_g_imatch(void *arg, int m)
{
  return g_cprg(arg, m, 1, 0, 0, F_GM_ISMATCH);
}

int
opt_g_regex(void *arg, int m)
{
  return g_cprg(arg, m, 0, 0, 0, F_GM_ISREGEX);
}

int
opt_g_iregexi(void *arg, int m)
{
  return g_cprg(arg, m, 0, REG_NOMATCH, REG_ICASE, F_GM_ISREGEX);
}

int
opt_g_iregex(void *arg, int m)
{
  return g_cprg(arg, m, 0, REG_NOMATCH, 0, F_GM_ISREGEX);
}

int
opt_glconf_file(void *arg, int m)
{
  g_cpg(arg, GLCONF_I, m, PATH_MAX - 1);
  return 0;
}

int
opt_dirlog_sections_file(void *arg, int m)
{
  g_cpg(arg, DU_FLD, m, PATH_MAX - 1);
  return 0;
}

int
print_version(void *arg, int m)
{
  print_str("%s_%s-%s\n", PACKAGE_NAME, PACKAGE_VERSION, __STR_ARCH);
  updmode = UPD_MODE_NOOP;
  return 0;
}

int
g_opt_mode_noop(void *arg, int m)
{
  updmode = UPD_MODE_NOOP;
  return 0;
}

int
opt_dirlog_check(void *arg, int m)
{
  updmode = UPD_MODE_CHECK;
  return 0;
}

int
opt_check_ghost(void *arg, int m)
{
  gfl |= F_OPT_C_GHOSTONLY;
  return 0;
}

int
opt_g_xdev(void *arg, int m)
{
  gfl |= F_OPT_XDEV;
  return 0;
}

int
opt_g_xblk(void *arg, int m)
{
  gfl |= F_OPT_XBLK;
  return 0;
}

int
opt_dirlog_rebuild_full(void *arg, int m)
{
  gfl |= F_OPT_DIR_FULL_REBUILD;
  return 0;
}

int
opt_no_nuke_chk(void *arg, int m)
{
  gfl0 |= F_OPT_NO_CHECK_NUKED;
  return 0;
}

int
opt_arrange(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  if (!buffer)
    {
      return 5610;
    }

  if (!strncmp(buffer, "dist", 4))
    {
      gfl0 |= F_OPT_ARR_DIST;
    }
  else
    {
      return 5611;
    }

  return 0;
}

int
opt_g_indepth(void *arg, int m)
{
  gfl0 |= F_OPT_DRINDEPTH;
  return 0;
}

int
opt_g_cdironly(void *arg, int m)
{
  gfl |= F_OPT_CDIRONLY;
  return 0;
}

int
opt_dirlog_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP;
  return 0;
}

int
opt_dupefile_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_DUPEF;
  return 0;
}

int
opt_online_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_ONL;
  return 0;
}

int
opt_lastonlog_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_LON;
  return 0;
}

int
opt_dump_users(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_USERS;
  return 0;
}

int
opt_dump_grps(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_GRPS;
  return 0;
}

int
opt_dirlog_dump_nukelog(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_NUKE;
  return 0;
}

int
opt_g_write(void *arg, int m)
{
  updmode = UPD_MODE_WRITE;
  p_argv_off = g_pg(arg, m);
  return 0;
}

int
opt_g_dump_imdb(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_IMDB;
  return 0;
}

int
opt_g_dump_game(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_GAME;
  return 0;
}

int
opt_g_dump_tv(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_TV;
  return 0;
}

int
opt_oneliner_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_ONEL;
  return 0;
}

int
print_help(void *arg, int m)
{
  print_str(hpd_up, PACKAGE_VERSION, __STR_ARCH, PACKAGE_URL,
  PACKAGE_BUGREPORT);
  if (m != -1)
    {
      updmode = UPD_MODE_NOOP;
    }
  return 0;
}

int
opt_g_ex_fork(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_FORK;
  gfl |= F_OPT_DAEMONIZE;
  return 0;
}

int
opt_dirlog_chk_dupe(void *arg, int m)
{
  updmode = UPD_MODE_DUPE_CHK;
  return 0;
}

int
opt_membuffer_limit(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  if (!buffer)
    {
      return 3512;
    }
  long long int l_buffer = atoll(buffer);
  if (l_buffer > 1024)
    {
      db_max_size = l_buffer;
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: max memory buffer limit set to %lld bytes\n",
              l_buffer);
        }
    }
  else
    {
      print_str(
          "NOTICE: invalid memory buffer limit, using default (%lld bytes)\n",
          db_max_size);
    }
  return 0;
}

int
opt_membuffer_limit_in(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  if (!buffer)
    {
      return 3513;
    }
  long long int l_buffer = atoll(buffer);
  if (l_buffer > 8192)
    {
      max_datain_f = l_buffer;
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: ASCII input buffer limit set to %lld bytes\n",
              l_buffer);
        }
    }
  else
    {
      print_str(
          "NOTICE: invalid ASCII input buffer limit, using default (%lld bytes)\n",
          max_datain_f);
    }
  return 0;
}

int
opt_g_arg1(void *arg, int m)
{
  g_cpg(arg, MACRO_ARG1, m, 4095);
  gfl |= F_OPT_HAS_M_ARG1;
  return 0;
}

int
opt_g_arg2(void *arg, int m)
{
  g_cpg(arg, MACRO_ARG2, m, 4095);
  gfl |= F_OPT_HAS_M_ARG2;
  return 0;
}

int
opt_g_arg3(void *arg, int m)
{
  g_cpg(arg, MACRO_ARG3, m, 4095);
  gfl |= F_OPT_HAS_M_ARG3;
  return 0;
}

int
opt_g_preexec(void *arg, int m)
{
  GLOBAL_PREEXEC = g_pd(arg, m, MAX_EXEC_STR);
  if (GLOBAL_PREEXEC)
    {
      gfl |= F_OPT_PREEXEC;
    }
  return 0;
}

int
opt_g_postexec(void *arg, int m)
{
  GLOBAL_POSTEXEC = g_pd(arg, m, MAX_EXEC_STR);
  if (GLOBAL_POSTEXEC)
    {
      gfl |= F_OPT_POSTEXEC;
    }
  return 0;
}

void
l_opt_cstdin(char *arg)
{
  if (arg[0] == 0x2D && arg[1] == 0x0)
    {
      gfl0 |= F_OPT_STDIN;
    }
}

int
opt_dirlog_file(void *arg, int m)
{
  if (!(ofl & F_OVRR_DIRLOG))
    {
      g_cpg(arg, DIRLOG, m, PATH_MAX);
      ofl |= F_OVRR_DIRLOG;
      l_opt_cstdin(DIRLOG);
    }
  return 0;
}

int
opt_nukelog_file(void *arg, int m)
{
  if (!(ofl & F_OVRR_NUKELOG))
    {
      g_cpg(arg, NUKELOG, m, PATH_MAX - 1);
      ofl |= F_OVRR_NUKELOG;
      l_opt_cstdin(NUKELOG);
    }
  return 0;
}

int
opt_glroot(void *arg, int m)
{
  if (!(ofl & F_OVRR_GLROOT))
    {
      g_cpg(arg, GLROOT, m, PATH_MAX);
      ofl |= F_OVRR_GLROOT;
    }
  return 0;
}

int
opt_siteroot(void *arg, int m)
{
  if (!(ofl & F_OVRR_SITEROOT))
    {
      g_cpg(arg, SITEROOT_N, m, PATH_MAX);
      ofl |= F_OVRR_SITEROOT;
    }
  return 0;
}

int
opt_dupefile(void *arg, int m)
{
  if (!(ofl & F_OVRR_DUPEFILE))
    {
      g_cpg(arg, DUPEFILE, m, PATH_MAX);
      ofl |= F_OVRR_DUPEFILE;
      l_opt_cstdin(DUPEFILE);
    }
  return 0;
}

int
opt_lastonlog(void *arg, int m)
{
  if (!(ofl & F_OVRR_LASTONLOG))
    {
      g_cpg(arg, LASTONLOG, m, PATH_MAX);
      ofl |= F_OVRR_LASTONLOG;
      l_opt_cstdin(LASTONLOG);
    }
  return 0;
}

int
opt_sconf(void *arg, int m)
{
  if (!(ofl & F_OVRR_SCONF))
    {
      g_cpg(arg, SCONFLOG, m, PATH_MAX);
      ofl |= F_OVRR_SCONF;
      l_opt_cstdin(SCONFLOG);
    }
  return 0;
}

int
opt_gconf(void *arg, int m)
{
  if (!(ofl & F_OVRR_GCONF))
    {
      g_cpg(arg, GCONFLOG, m, PATH_MAX);
      ofl |= F_OVRR_GCONF;
      l_opt_cstdin(GCONFLOG);
    }
  return 0;
}

int
opt_oneliner(void *arg, int m)
{
  if (!(ofl & F_OVRR_ONELINERS))
    {
      g_cpg(arg, ONELINERS, m, PATH_MAX);
      ofl |= F_OVRR_ONELINERS;
      l_opt_cstdin(ONELINERS);
    }
  return 0;
}

int
opt_imdblog(void *arg, int m)
{
  if (!(ofl & F_OVRR_IMDBLOG))
    {
      g_cpg(arg, IMDBLOG, m, PATH_MAX);
      ofl |= F_OVRR_IMDBLOG;
      l_opt_cstdin(IMDBLOG);
    }
  return 0;
}

int
opt_tvlog(void *arg, int m)
{
  if (!(ofl & F_OVRR_TVLOG))
    {
      g_cpg(arg, TVLOG, m, PATH_MAX);
      ofl |= F_OVRR_TVLOG;
      l_opt_cstdin(TVLOG);
    }
  return 0;
}

int
opt_gamelog(void *arg, int m)
{
  if (!(ofl & F_OVRR_GAMELOG))
    {
      g_cpg(arg, GAMELOG, m, PATH_MAX);
      ofl |= F_OVRR_GAMELOG;
      l_opt_cstdin(GAMELOG);
    }
  return 0;
}

int
opt_GE1LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE1LOG))
    {
      g_cpg(arg, GE1LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE1LOG;
      l_opt_cstdin(GE1LOG);
    }
  return 0;
}

int
opt_GE2LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE2LOG))
    {
      g_cpg(arg, GE2LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE2LOG;
      l_opt_cstdin(GE2LOG);
    }
  return 0;
}

int
opt_GE3LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE3LOG))
    {
      g_cpg(arg, GE3LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE3LOG;
      l_opt_cstdin(GE3LOG);
    }
  return 0;
}

int
opt_GE4LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE4LOG))
    {
      g_cpg(arg, GE4LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE4LOG;
      l_opt_cstdin(GE4LOG);
    }
  return 0;
}

int
opt_altlog(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE4LOG))
    {
      g_cpg(arg, ALTLOG, m, PATH_MAX);
      ofl |= F_OVRR_GE4LOG;
      l_opt_cstdin(ALTLOG);
    }
  return 0;
}

int
opt_rebuild(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_REBUILD;
  return 0;
}

int
opt_g_xretry(void *arg, int m)
{
  gfl0 |= F_OPT_XRETRY;
  return 0;
}

int
opt_g_sleep(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  g_sleep = (uint32_t) strtoul(buffer, NULL, 10);

  return 0;
}

int
opt_g_usleep(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  g_usleep = (uint32_t) strtoul(buffer, NULL, 10);

  return 0;
}

int
opt_execv_stdout_redir(void *arg, int m)
{
  char *ptr = g_pg(arg, m);
  execv_stdout_redir = open(ptr, O_RDWR | O_CREAT,
      (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
  if (execv_stdout_redir == -1)
    {
      ofl |= F_ESREDIRFAILED;
    }

  return 0;
}

int
opt_g_infile(void *arg, int m)
{
  g_cpg(arg, infile_p, m, PATH_MAX);

  return 0;
}

int
opt_shmipc(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (!strlen(buffer))
    {
      return 53412;
    }

  SHM_IPC = (key_t) strtoul(buffer, NULL, 16);

  if (!SHM_IPC)
    {
      return 53413;
    }

  ofl |= F_OVRR_IPC;

  return 0;
}

int
opt_log_file(void *arg, int m)
{
  g_cpg(arg, LOGFILE, m, PATH_MAX);
  gfl |= F_OPT_PS_LOGGING;
  ofl |= F_OVRR_LOGFILE;
  return 0;
}

int
opt_print(void *arg, int m)
{
  if ((_print_ptr = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PRINT;
      return 0;
    }
  return 4250;
}

int
opt_printf(void *arg, int m)
{
  if ((_print_ptr = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PRINTF;
      return 0;
    }
  return 4251;
}

int
opt_stdin(void *arg, int m)
{
  gfl0 |= F_OPT_STDIN;
  return 0;
}

int
opt_exec(void *arg, int m)
{
  exec_str = g_pd(arg, m, MAX_EXEC_STR);
  exc = g_do_exec_fb;
  return 0;
}

int
option_crc32(void *arg, int m)
{
  g_setjmp(0, "option_crc32", NULL, NULL);
  char *buffer;
  if (m == 2)
    {
      buffer = (char *) arg;
    }
  else
    {
      buffer = ((char **) arg)[0];
    }

  updmode = UPD_MODE_NOOP;

  if (NULL == buffer)
    {
      return 55272;
    }

  uint32_t crc32;

  off_t read = file_crc32(buffer, &crc32);

  if (read > 0)
    print_str("%.8X\n", (uint32_t) crc32);
  else
    {
      print_str("ERROR: %s: [%d] could not get CRC32\n", buffer,
      errno);
      EXITVAL = 1;
    }

  return 0;
}

static int
opt_g_swapmode(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 140051;
    }

  size_t i_len = strlen(buffer);

  if (i_len > 4)
    {
      return 140052;
    }

  if (!strncmp(buffer, "swap", i_len))
    {
      gfl0 |= F_OPT_SMETHOD_SWAP;
    }
  else if (!strncmp(buffer, "heap", i_len))
    {
      gfl0 |= F_OPT_SMETHOD_HEAP;
    }
  else if (!strncmp(buffer, "q", i_len))
    {
      gfl0 |= F_OPT_SMETHOD_Q;
    }
  else
    {
      return 140059;
    }

  return 0;
}

void *prio_f_ref[] =
  { "noop", g_opt_mode_noop, (void*) 0, "--raw", opt_raw_dump, (void*) 0,
      "silent", opt_silent, (void*) 0, "--silent", opt_silent, (void*) 0,
      "-arg1", opt_g_arg1, (void*) 1, "--arg1", opt_g_arg1, (void*) 1, "-arg2",
      opt_g_arg2, (void*) 1, "--arg2", opt_g_arg2, (void*) 1, "-arg3",
      opt_g_arg3, (void*) 1, "--arg3", opt_g_arg3, (void*) 1, "-vvvvv",
      opt_g_verbose5, (void*) 0, "-vvvv", opt_g_verbose4, (void*) 0, "-vvv",
      opt_g_verbose3, (void*) 0, "-vv", opt_g_verbose2, (void*) 0, "-v",
      opt_g_verbose, (void*) 0, "-m", prio_opt_g_macro, (void*) 1, "--info",
      prio_opt_g_pinfo, (void*) 0, "--loglevel", opt_g_loglvl, (void*) 1,
      "--logfile", opt_log_file, (void*) 1, "--log", opt_logging, (void*) 0,
      "--dirlog", opt_dirlog_file, (void*) 1, "--ge1log", opt_GE1LOG, (void*) 1,
      "--ge2log", opt_GE2LOG, (void*) 1, "--ge3log", opt_GE3LOG, (void*) 1,
      "--ge4log", opt_GE4LOG, (void*) 1, "--altlog", opt_altlog, (void*) 1,
      "--gamelog", opt_gamelog, (void*) 1, "--tvlog", opt_tvlog, (void*) 1,
      "--imdblog", opt_imdblog, (void*) 1, "--oneliners", opt_oneliner,
      (void*) 1, "--lastonlog", opt_lastonlog, (void*) 1, "--nukelog",
      opt_nukelog_file, (void*) 1, "--sconf", opt_sconf, (void*) 1, "--gconf",
      opt_gconf, (void*) 1, "--siteroot", opt_siteroot, (void*) 1, "--glroot",
      opt_glroot, (void*) 1, "--noglconf", opt_g_noglconf, (void*) 0,
      "--glconf", opt_glconf_file, (void*) 1,
      NULL, NULL, NULL };

void *f_ref[] =
  { "noop", g_opt_mode_noop, (void*) 0, "and", opt_g_operator_and, (void*) 0,
      "or", opt_g_operator_or, (void*) 0, "--rev", opt_g_reverse, (void*) 0,
      "lom", opt_g_lom_match, (void*) 1, "--lom", opt_g_lom_match, (void*) 1,
      "ilom", opt_g_lom_imatch, (void*) 1, "--ilom", opt_g_lom_imatch,
      (void*) 1, "--info", prio_opt_g_pinfo, (void*) 0, "sort", opt_g_sort,
      (void*) 1, "--sort", opt_g_sort, (void*) 1, "-h", opt_g_dump_tv,
      (void*) 0, "-k", opt_g_dump_game, (void*) 0, "--cdir", opt_g_cdironly,
      (void*) 0, "--imatchq", opt_g_imatchq, (void*) 0, "--matchq",
      opt_g_matchq, (void*) 0, "-a", opt_g_dump_imdb, (void*) 0, "-z",
      opt_g_write, (void*) 1, "--infile", opt_g_infile, (void*) 1, "-xdev",
      opt_g_xdev, (void*) 0, "--xdev", opt_g_xdev, (void*) 0, "-xblk",
      opt_g_xblk, (void*) 0, "--xblk", opt_g_xblk, (void*) 0, "-file",
      opt_g_udc_f, (void*) 0, "--file", opt_g_udc_f, (void*) 0, "-dir",
      opt_g_udc_dir, (void*) 0, "--dir", opt_g_udc_dir, (void*) 0, "--loopmax",
      opt_loop_max, (void*) 1, "--ghost", opt_check_ghost, (void*) 0, "-q",
      opt_g_dg, (void*) 1, "-x", opt_g_udc, (void*) 1, "-R", opt_g_recursive,
      (void*) 0, "-recursive", opt_g_recursive, (void*) 0, "--recursive",
      opt_g_recursive, (void*) 0, "-g", opt_dump_grps, (void*) 0, "-t",
      opt_dump_users, (void*) 0, "--backup", opt_backup, (void*) 1, "-print",
      opt_print, (void*) 1, "-printf", opt_printf, (void*) 1, "-stdin",
      opt_stdin, (void*) 0, "--stdin", opt_stdin, (void*) 0, "--print",
      opt_print, (void*) 1, "--printf", opt_printf, (void*) 1, "-b", opt_backup,
      (void*) 1, "--postexec", opt_g_postexec, (void*) 1, "--preexec",
      opt_g_preexec, (void*) 1, "--usleep", opt_g_usleep, (void*) 1, "--sleep",
      opt_g_sleep, (void*) 1, "-arg1",
      NULL, (void*) 1, "--arg1", NULL, (void*) 1, "-arg2",
      NULL, (void*) 1, "--arg2", NULL, (void*) 1, "-arg3", NULL, (void*) 1,
      "--arg3", NULL, (void*) 1, "-m", NULL, (void*) 1, "--imatch",
      opt_g_imatch, (void*) 1, "imatch", opt_g_imatch, (void*) 1, "match",
      opt_g_match, (void*) 1, "--match", opt_g_match, (void*) 1, "--fork",
      opt_g_ex_fork, (void*) 1, "-vvvvv", opt_g_verbose5, (void*) 0, "-vvvv",
      opt_g_verbose4, (void*) 0, "-vvv", opt_g_verbose3, (void*) 0, "-vv",
      opt_g_verbose2, (void*) 0, "-v", opt_g_verbose, (void*) 0, "--loglevel",
      opt_g_loglvl, (void*) 1, "--ftime", opt_g_ftime, (void*) 0, "--logfile",
      opt_log_file, (void*) 0, "--log", opt_logging, (void*) 0, "silent",
      opt_silent, (void*) 0, "--silent", opt_silent, (void*) 0, "--loop",
      opt_g_loop, (void*) 1, "--daemon", opt_g_daemonize, (void*) 0, "-w",
      opt_online_dump, (void*) 0, "--ipc", opt_shmipc, (void*) 1, "-l",
      opt_lastonlog_dump, (void*) 0, "--ge1log", opt_GE1LOG, (void*) 1,
      "--ge2log", opt_GE2LOG, (void*) 1, "--ge3log", opt_GE3LOG, (void*) 1,
      "--gamelog", opt_gamelog, (void*) 1, "--tvlog", opt_tvlog, (void*) 1,
      "--imdblog", opt_imdblog, (void*) 1, "--oneliners", opt_oneliner,
      (void*) 1, "-o", opt_oneliner_dump, (void*) 0, "--lastonlog",
      opt_lastonlog, (void*) 1, "-i", opt_dupefile_dump, (void*) 0,
      "--dupefile", opt_dupefile, (void*) 1, "--sconf", opt_sconf, (void*) 1,
      "--gconf", opt_gconf, (void*) 1, "--nowbuffer", opt_g_buffering,
      (void*) 0, "--raw", opt_raw_dump, (void*) 0, "--binary", opt_binary,
      (void*) 0, "iregexi", opt_g_iregexi, (void*) 1, "--iregexi",
      opt_g_iregexi, (void*) 1, "iregex", opt_g_iregex, (void*) 1, "--iregex",
      opt_g_iregex, (void*) 1, "regexi", opt_g_regexi, (void*) 1, "--regexi",
      opt_g_regexi, (void*) 1, "regex", opt_g_regex, (void*) 1, "--regex",
      opt_g_regex, (void*) 1, "-e", opt_rebuild, (void*) 1, "--comp",
      opt_compact_output_formatting, (void*) 0, "--batch",
      opt_batch_output_formatting, (void*) 0, "-E",
      opt_export_output_formatting, (void*) 0, "--export",
      opt_export_output_formatting, (void*) 0, "-y", opt_g_followlinks,
      (void*) 0, "--allowsymbolic", opt_g_followlinks, (void*) 0,
      "--followlinks", opt_g_followlinks, (void*) 0, "--allowlinks",
      opt_g_followlinks, (void*) 0, "--execv", opt_execv, (void*) 1, "-execv",
      opt_execv, (void*) 1, "-exec", opt_exec, (void*) 1, "--exec", opt_exec,
      (void*) 1, "--fix", opt_g_fix, (void*) 0, "-u", opt_g_update, (void*) 0,
      "--memlimit", opt_membuffer_limit, (void*) 1, "--memlimita",
      opt_membuffer_limit_in, (void*) 1, "-p", opt_dirlog_chk_dupe, (void*) 0,
      "--dupechk", opt_dirlog_chk_dupe, (void*) 0, "--nobuffer",
      opt_g_nobuffering, (void*) 0, "-n", opt_dirlog_dump_nukelog, (void*) 0,
      "--help", print_help, (void*) 0, "--version", print_version, (void*) 0,
      "--folders", opt_dirlog_sections_file, (void*) 1, "--dirlog",
      opt_dirlog_file, (void*) 1, "--nukelog", opt_nukelog_file, (void*) 1,
      "--siteroot", opt_siteroot, (void*) 1, "--glroot", opt_glroot, (void*) 1,
      "--nowrite", opt_g_nowrite, (void*) 0, "--sfv", opt_g_sfv, (void*) 0,
      "--crc32", option_crc32, (void*) 1, "--nobackup", opt_nobackup, (void*) 0,
      "-c", opt_dirlog_check, (void*) 0, "--check", opt_dirlog_check, (void*) 0,
      "--dump", opt_dirlog_dump, (void*) 0, "-d", opt_dirlog_dump, (void*) 0,
      "-f", opt_g_force, (void*) 0, "-ff", opt_g_force2, (void*) 0, "-s",
      opt_update_single_record, (void*) 1, "-r", opt_recursive_update_records,
      (void*) 0, "--shmem", opt_g_shmem, (void*) 0, "--shmreload",
      opt_g_shmreload, (void*) 0, "--loadq", opt_g_loadq, (void*) 0, "--loadqa",
      opt_g_loadqa, (void*) 0, "--shmdestroy", opt_g_shmdestroy, (void*) 0,
      "--shmdestonexit", opt_g_shmdestroyonexit, (void*) 0, "--maxres",
      opt_g_maxresults, (void*) 1, "--maxhit", opt_g_maxhits, (void*) 1,
      "--ifres", opt_g_ifres, (void*) 0, "--ifhit", opt_g_ifhit, (void*) 0,
      "--ifrhe", opt_g_ifrh_e, (void*) 0, "--nofq", opt_g_nofq, (void*) 0,
      "--esredir", opt_execv_stdout_redir, (void*) 1, "--noglconf",
      opt_g_noglconf, (void*) 0, "--maxdepth", opt_g_maxdepth, (void*) 1,
      "-maxdepth", opt_g_maxdepth, (void*) 1, "--mindepth", opt_g_mindepth,
      (void*) 1, "-mindepth", opt_g_mindepth, (void*) 1, "--noereg",
      opt_g_noereg, (void*) 0, "--fd", opt_g_fd, (void*) 0, "-fd", opt_g_fd,
      (void*) 0, "--prune", opt_prune, (void*) 0, "--glconf", opt_glconf_file,
      (void*) 1, "--ge4log", opt_GE4LOG, (void*) 1, "--altlog", opt_altlog,
      (void*) 1, "--xretry", opt_g_xretry, (void*) 0, "--indepth",
      opt_g_indepth, (void*) 0, "--full", opt_dirlog_rebuild_full, (void*) 0,
      "--arr", opt_arrange, (void*) 1, "--nonukechk", opt_no_nuke_chk,
      (void*) 0, "--rsleep", opt_g_loop_sleep, (void*) 1, "--rusleep",
      opt_g_loop_usleep, (void*) 1, "--nostats", opt_g_nostats, (void*) 0,
      "--stats", opt_g_stats, (void*) 0, "-mlist", opt_g_mlist, (void*) 0,
      "--gz", opt_g_comp, (void*) 1, "--sortmethod", opt_g_swapmode, (void*) 1,
      NULL, NULL, NULL };
