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
#include <fnmatch.h>

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
opt_g_stdout_lvl(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (!buffer)
    {
      return 81192;
    }

  uint32_t stdout_lvl;

  int r;

  if (0 != (r = build_msg_reg(buffer, &stdout_lvl)))
    {
      return r;
    }

  STDOUT_LVL = stdout_lvl;

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

  if (buffer == NULL)
    {
      return 7190;
    }

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

  if (buffer == NULL)
    {
      return 7201;
    }

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
  print_str("ERROR: this executable was not compiled with zlib, compression is not available\n");
  return 41513;
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
opt_g_rec_off(void *arg, int m)
{
  if (flags_udcfg & F_PD_RECURSIVE)
    {
      flags_udcfg ^= F_PD_RECURSIVE;
    }
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

  if (buffer == NULL)
    {
      return 19022;
    }

  max_results = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_HASMAXRES;
  l_sfo = L_STFO_FILTER;
  return 0;
}

static int
opt_g_maxhits(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 19032;
    }

  max_hits = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_HASMAXHIT;
  l_sfo = L_STFO_FILTER;
  return 0;
}

int
opt_g_maxdepth(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 19042;
    }

  max_depth = ((off_t) strtoll(buffer, NULL, 10)) + 1;
  gfl |= F_OPT_MAXDEPTH;
  return 0;
}

int
opt_g_mindepth(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 19052;
    }

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
opt_bo_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_BATCH | F_OPT_PS_SILENT;
  return 0;
}

int
opt_ex_o_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_EXPORT | F_OPT_PS_SILENT;
  return 0;
}

int
opt_crof(void *arg, int m)
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

int
opt_g_loadq(void *arg, int m)
{
  gfl |= F_OPT_LOADQ;
  return 0;
}

int
opt_g_loadqa(void *arg, int m)
{
  gfl0 |= F_OPT_LOADQA;
  return 0;
}

int
opt_g_shmdestroy(void *arg, int m)
{
  gfl |= F_OPT_SHMDESTROY;
  return 0;
}

int
opt_g_shmdoex(void *arg, int m)
{
  gfl |= F_OPT_SHMDESTONEXIT;
  return 0;
}

int
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
  if (gfl & F_OPT_WBUFFER)
    {
      gfl ^= F_OPT_WBUFFER;
    }
  return 0;
}

int
opt_g_flinks(void *arg, int m)
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

static int
g_opt_shmflg(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
    {
      return 32141;
    }

  if (strlen(buffer) != 3)
    {
      return 32142;
    }

  int t_flg = (int) strtol(buffer, NULL, 8);

  g_shmcflags = t_flg;

  return 0;
}

static int
g_opt_shmro(void *arg, int m)
{
  gfl0 |= F_OPT_SHMRO;
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
opt_rec_upd_records(void *arg, int m)
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

static int
opt_g_negate(void *arg, int m)
{
  ar_add(&ar_vref, AR_VRP_OPT_NEGATE_MATCH, -1);

  ar_vref.flags |= F_MDA_ST_MISC00;

  return 0;
}

static int
opt_g_tfd(void *arg, int m)
{
  ar_add(&ar_vref, AR_VRP_OPT_TARGET_FD, -1);

  ar_vref.flags |= F_MDA_ST_MISC00;

  return 0;
}

static int
regex_determine_negated(void)
{
  if ( NULL != ar_find(&ar_vref, AR_VRP_OPT_NEGATE_MATCH))
    {
      return REG_NOMATCH;
    }
  else
    {
      return 0;
    }
}

static int
lom_determine_negated(void)
{
  if ( NULL != ar_find(&ar_vref, AR_VRP_OPT_NEGATE_MATCH))
    {
      return F_GM_IMATCH;
    }
  else
    {
      return 0;
    }

}

static int
fname_determine_negated(void)
{
  if ( NULL != ar_find(&ar_vref, AR_VRP_OPT_NEGATE_MATCH))
    {
      return FNM_NOMATCH;
    }
  else
    {
      return 0;
    }
}

int
opt_g_d_lom_match(void *arg, int m)
{
  return opt_g_lom(arg, m, lom_determine_negated());
}

int
opt_g_d_regex(void *arg, int m)
{
  return g_cprg(arg, m, regex_determine_negated(), 0, 0, F_GM_ISREGEX);
}

int
opt_g_d_regexi(void *arg, int m)
{
  return g_cprg(arg, m, regex_determine_negated(), 0, REG_ICASE, F_GM_ISREGEX);
}

int
opt_g_d_match(void *arg, int m)
{
  return g_cprg(arg, m, default_determine_negated(), 0, 0, F_GM_ISMATCH);
}

int
opt_g_d_fname(void *arg, int m)
{
  return g_cprg(arg, m, default_determine_negated(), 0, 0, F_GM_ISFNAME);
}

int
opt_g_d_fnamei(void *arg, int m)
{
#if defined FNM_CASEFOLD
  return g_cprg(arg, m, fname_determine_negated(), FNM_CASEFOLD, 0, F_GM_ISFNAME);
#else
  return 9910;
#endif
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
opt_dirlog_sect_fl(void *arg, int m)
{
  g_cpg(arg, DU_FLD, m, PATH_MAX - 1);
  return 0;
}

int
print_version(void *arg, int m)
{
  print_str("%s-%s-%s\n", PACKAGE_NAME, PACKAGE_VERSION, __STR_ARCH);
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
opt_dirlog_rb_full(void *arg, int m)
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
  if (NULL == buffer)
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
opt_memb_limit(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
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
opt_memb_limit_in(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
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
g_opt_mroot(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
    {
      return 25173;
    }

  g_mroot = buffer;

  gfl0 |= F_OPT_MROOT;
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
opt_g_xloop(void *arg, int m)
{
  gfl0 |= F_OPT_XLOOP;
  return 0;
}

int
opt_g_sleep(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
    {
      return 25511;
    }

  if (buffer[0] == 0x0)
    {
      return 25512;
    }

  g_sleep = (uint32_t) strtoul(buffer, NULL, 10);

  return 0;
}

int
opt_g_usleep(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
    {
      return 25611;
    }

  if (buffer[0] == 0x0)
    {
      return 25612;
    }

  g_usleep = (uint32_t) strtoul(buffer, NULL, 10);

  return 0;
}

int
opt_execv_stdout_rd(void *arg, int m)
{
  char *ptr = g_pg(arg, m);

  if (NULL == ptr)
    {
      return 28621;
    }

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

  if (NULL == buffer)
    {
      return 53411;
    }

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

#define MAX_PRINT_LINE_SIZE     ((1024*1024) / 4)

int
opt_print_stdin(void *arg, int m)
{
  char *b_in, *fn_in;
  FILE *fh_in;

  O_FI_STDIN(arg, m, 4273, 4274)

  _cl_print_ptr = _print_ptr = calloc(MAX_PRINT_LINE_SIZE, 1);
  if (read_file(fn_in, _print_ptr, MAX_PRINT_LINE_SIZE, 0, fh_in))
    {
      gfl0 |= F_OPT_PRINT;
      return 0;
    }

  return 4280;
}

int
opt_printf(void *arg, int m)
{
  if (NULL != (_print_ptr = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PRINTF;
      return 0;
    }
  return 4251;
}

int
opt_printf_stdin(void *arg, int m)
{
  char *b_in, *fn_in;
  FILE *fh_in;

  O_FI_STDIN(arg, m, 4323, 4324)

  _cl_print_ptr = _print_ptr = calloc(MAX_PRINT_LINE_SIZE, 1);
  if (read_file(fn_in, _print_ptr, MAX_PRINT_LINE_SIZE, 0, fh_in))
    {
      gfl0 |= F_OPT_PRINTF;
      return 0;
    }
  return 4281;
}

int
opt_postprintf(void *arg, int m)
{
  if ((_print_ptr_post = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_POSTPRINTF;
      return 0;
    }
  return 4251;
}

int
opt_postprint(void *arg, int m)
{
  if (NULL != (_print_ptr_post = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_POSTPRINT;
      return 0;
    }
  return 4250;
}

int
opt_preprintf(void *arg, int m)
{
  if (NULL != (_print_ptr_pre = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PREPRINTF;
      return 0;
    }
  return 4251;
}

int
opt_preprint(void *arg, int m)
{
  if (NULL != (_print_ptr_pre = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PREPRINT;
      return 0;
    }
  return 4250;
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
opt_exec_stdin(void *arg, int m)
{
  char *b_in, *fn_in;
  FILE *fh_in;

  O_FI_STDIN(arg, m, 6838, 6839)

  exec_str = calloc(MAX_EXEC_STR, 1);
  if (read_file(fn_in, exec_str, MAX_EXEC_STR, 0, fh_in))
    {
      exc = g_do_exec_fb;
      return 0;
    }

  return 6898;
}

int
opt_g_progress(void *arg, int m)
{
  gfl0 |= F_OPT_PROGRESS;
  return 0;
}

int
opt_g_fsroot(void *arg, int m)
{
  gfl0 |= F_OPT_FSROOT;
  return 0;
}

int
option_crc32(void *arg, int m)
{
  g_setjmp(0, "option_crc32", NULL, NULL);
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
    {
      return 55272;
    }

  updmode = UPD_MODE_NOOP;

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

int
opt_g_swapmode(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (buffer == NULL)
    {
      return 140051;
    }

  if (!strncmp(buffer, "swap", 4))
    {
      gfl0 |= F_OPT_SMETHOD_SWAP;
    }
  else if (!strncmp(buffer, "heap", 4))
    {
      gfl0 |= F_OPT_SMETHOD_HEAP;
    }
  else if (!strncmp(buffer, "qsort", 5))
    {
      gfl0 |= F_OPT_SMETHOD_Q;
    }
  else if (!strncmp(buffer, "insert", 5))
    {
      gfl0 |= F_OPT_SMETHOD_INSSORT;
    }
  else if (!strncmp(buffer, "select", 6))
    {
      gfl0 |= F_OPT_SMETHOD_SELECT;
    }
  else
    {
      return 140059;
    }

  return 0;
}

_gg_opt gg_prio_f_ref[] =
  {
    { .id = 0x0003, .on = "silent", .ac = 0, .op = opt_silent },
    { .id = 0x0004, .on = "--silent", .ac = 0, .op = opt_silent },
    { .id = 0x000B, .on = "-vvvvv", .ac = 0, .op = opt_g_verbose5 },
    { .id = 0x000C, .on = "-vvvv", .ac = 0, .op = opt_g_verbose4 },
    { .id = 0x000D, .on = "-vvv", .ac = 0, .op = opt_g_verbose3 },
    { .id = 0x000E, .on = "-vv", .ac = 0, .op = opt_g_verbose2 },
    { .id = 0x000F, .on = "-v", .ac = 0, .op = opt_g_verbose },
    { .id = 0x0014, .on = "--log", .ac = 0, .op = opt_logging },
    { .id = 0x0012, .on = "--loglevel", .ac = 1, .op = opt_g_loglvl },
    { .id = 0x0013, .on = "--logfile", .ac = 1, .op = opt_log_file },
    { .id = 0x0011, .on = "--info", .ac = 0, .op = prio_opt_g_pinfo },
    { .id = 0x0082, .on = "--help", .ac = 0, .op = print_help },
    { .id = 0x0083, .on = "--version", .ac = 0, .op = print_version },
    { .id = 0x00A1, .on = "--esredir", .ac = 1, .op = opt_execv_stdout_rd },
#ifndef _MAKE_SBIN
        { .id = 0x0005, .on = "-arg1", .ac = 1, .op = opt_g_arg1 },
        { .id = 0x0006, .on = "--arg1", .ac = 1, .op = opt_g_arg1 },
        { .id = 0x0007, .on = "-arg2", .ac = 1, .op = opt_g_arg2 },
        { .id = 0x0008, .on = "--arg2", .ac = 1, .op = opt_g_arg2 },
        { .id = 0x0009, .on = "-arg3", .ac = 1, .op = opt_g_arg3 },
        { .id = 0x000A, .on = "--arg3", .ac = 1, .op = opt_g_arg3 },
        { .id = 0x0002, .on = "--raw", .ac = 0, .op = opt_raw_dump },
        { .id = 0x0015, .on = "--dirlog", .ac = 1, .op = opt_dirlog_file },
        { .id = 0x0016, .on = "--ge1log", .ac = 1, .op = opt_GE1LOG },
        { .id = 0x0017, .on = "--ge2log", .ac = 1, .op = opt_GE2LOG },
        { .id = 0x0018, .on = "--ge3log", .ac = 1, .op = opt_GE3LOG },
        { .id = 0x0019, .on = "--ge4log", .ac = 1, .op = opt_GE4LOG },
        { .id = 0x001A, .on = "--altlog", .ac = 1, .op = opt_altlog },
        { .id = 0x001B, .on = "--gamelog", .ac = 1, .op = opt_gamelog },
        { .id = 0x001C, .on = "--tvlog", .ac = 1, .op = opt_tvlog },
        { .id = 0x001D, .on = "--imdblog", .ac = 1, .op = opt_imdblog },
        { .id = 0x001E, .on = "--oneliners", .ac = 1, .op = opt_oneliner },
        { .id = 0x001F, .on = "--lastonlog", .ac = 1, .op = opt_lastonlog },
        { .id = 0x0020, .on = "--nukelog", .ac = 1, .op = opt_nukelog_file },
        { .id = 0x0021, .on = "--sconf", .ac = 1, .op = opt_sconf },
        { .id = 0x0022, .on = "--gconf", .ac = 1, .op = opt_gconf },
        { .id = 0x0023, .on = "--siteroot", .ac = 1, .op = opt_siteroot },
        { .id = 0x0024, .on = "--glroot", .ac = 1, .op = opt_glroot },
        { .id = 0x0025, .on = "--noglconf", .ac = 0, .op = opt_g_noglconf },
        { .id = 0x0026, .on = "--glconf", .ac = 1, .op = opt_glconf_file },
        { .id = 0x0010, .on = "-m", .ac = 1, .op = prio_opt_g_macro },
        { .id = 0x1282, .on = "--mroot", .ac = 1, .op = g_opt_mroot },
        { .id = 0x0014, .on = "-xdev", .ac = 0, .op = opt_g_xdev },
        { .id = 0x004F, .on = "--daemon", .ac = 0, .op = opt_g_daemonize },
#endif
        { 0x0 } };

_gg_opt gg_f_ref[] =
  {
    { .id = 0x0001, .on = "noop", .ac = 0, .op = g_opt_mode_noop },
    { .id = 0x0004, .on = "--rev", .ac = 0, .op = opt_g_reverse },
    { .id = 0x0009, .on = "--info", .ac = 0, .op = prio_opt_g_pinfo },
    { .id = 0x000B, .on = "sort", .ac = 1, .op = opt_g_sort },
    { .id = 0x000B, .on = "--sort", .ac = 1, .op = opt_g_sort },
    { .id = 0x000B, .on = "-sort", .ac = 1, .op = opt_g_sort },
    { .id = 0x000E, .on = "--cdir", .ac = 0, .op = opt_g_cdironly },
    { .id = 0x000E, .on = "-prune", .ac = 0, .op = opt_g_cdironly },
    { .id = 0x000F, .on = "--imatchq", .ac = 0, .op = opt_g_imatchq },
    { .id = 0x0010, .on = "--matchq", .ac = 0, .op = opt_g_matchq },
    { .id = 0x0013, .on = "--infile", .ac = 1, .op = opt_g_infile },
    { .id = 0x0014, .on = "-xdev", .ac = 0, .op = opt_g_xdev },
    { .id = 0x0015, .on = "--xdev", .ac = 0, .op = opt_g_xdev },
    { .id = 0x0016, .on = "-xblk", .ac = 0, .op = opt_g_xblk },
    { .id = 0x0017, .on = "--xblk", .ac = 0, .op = opt_g_xblk },
    { .id = 0x0018, .on = "-file", .ac = 0, .op = opt_g_udc_f },
    { .id = 0x0019, .on = "--file", .ac = 0, .op = opt_g_udc_f },
    { .id = 0x001A, .on = "-dir", .ac = 0, .op = opt_g_udc_dir },
    { .id = 0x001B, .on = "--dir", .ac = 0, .op = opt_g_udc_dir },
    { .id = 0x001C, .on = "--loopmax", .ac = 1, .op = opt_loop_max },
    { .id = 0x001D, .on = "--ghost", .ac = 0, .op = opt_check_ghost },
    { .id = 0x0020, .on = "-R", .ac = 0, .op = opt_g_recursive },
    { .id = 0x0021, .on = "-recursive", .ac = 0, .op = opt_g_recursive },
    { .id = 0x0022, .on = "--recursive", .ac = 0, .op = opt_g_recursive },
    { .id = 0x0020, .on = "--no-recursive", .ac = 0, .op = opt_g_rec_off },
    { .id = 0x0025, .on = "--backup", .ac = 1, .op = opt_backup },
    { .id = 0x0026, .on = "-preprint", .ac = 1, .op = opt_preprint },
    { .id = 0x0027, .on = "-preprintf", .ac = 1, .op = opt_preprintf },
    { .id = 0x0028, .on = "-postprint", .ac = 1, .op = opt_postprint },
    { .id = 0x0029, .on = "-postprintf", .ac = 1, .op = opt_postprintf },
    { .id = 0x002A, .on = "-print", .ac = 1, .op = opt_print },
    { .id = 0x002B, .on = "-print-", .ac = 1, .op = opt_print_stdin },
    { .id = 0x002C, .on = "-printf-", .ac = 1, .op = opt_printf_stdin },
    { .id = 0x002D, .on = "-printf", .ac = 1, .op = opt_printf },
    { .id = 0x002E, .on = "-stdin", .ac = 0, .op = opt_stdin },
    { .id = 0x002F, .on = "--stdin", .ac = 0, .op = opt_stdin },
    { .id = 0x0030, .on = "--print", .ac = 1, .op = opt_print },
    { .id = 0x0031, .on = "--printf", .ac = 1, .op = opt_printf },
    { .id = 0x0033, .on = "--postexec", .ac = 1, .op = opt_g_postexec },
    { .id = 0x0034, .on = "--preexec", .ac = 1, .op = opt_g_preexec },
    { .id = 0x0035, .on = "--usleep", .ac = 1, .op = opt_g_usleep },
    { .id = 0x0036, .on = "--sleep", .ac = 1, .op = opt_g_sleep },
    { .id = 0x0037, .on = "-arg1", .ac = 1, .op = NULL },
    { .id = 0x0038, .on = "--arg1", .ac = 1, .op = NULL },
    { .id = 0x0039, .on = "-arg2", .ac = 1, .op = NULL },
    { .id = 0x003A, .on = "--arg2", .ac = 1, .op = NULL },
    { .id = 0x003B, .on = "-arg3", .ac = 1, .op = NULL },
    { .id = 0x003C, .on = "--arg3", .ac = 1, .op = NULL },
    { .id = 0x003D, .on = "-m", .ac = 1, .op = NULL },
    { .id = 0x0042, .on = "--fork", .ac = 1, .op = opt_g_ex_fork },
    { .id = 0x0043, .on = "-vvvvv", .ac = 0, .op = opt_g_verbose5 },
    { .id = 0x0044, .on = "-vvvv", .ac = 0, .op = opt_g_verbose4 },
    { .id = 0x0045, .on = "-vvv", .ac = 0, .op = opt_g_verbose3 },
    { .id = 0x0046, .on = "-vv", .ac = 0, .op = opt_g_verbose2 },
    { .id = 0x0047, .on = "-v", .ac = 0, .op = opt_g_verbose },
    { .id = 0x004C, .on = "silent", .ac = 0, .op = opt_silent },
    { .id = 0x004D, .on = "--silent", .ac = 0, .op = opt_silent },
    { .id = 0x004A, .on = "--logfile", .ac = 0, .op = opt_log_file },
    { .id = 0x0048, .on = "--loglevel", .ac = 1, .op = opt_g_loglvl },
    { .id = 0x0049, .on = "--ftime", .ac = 0, .op = opt_g_ftime },
    { .id = 0x004B, .on = "--log", .ac = 0, .op = opt_logging },
    { .id = 0x004E, .on = "--loop", .ac = 1, .op = opt_g_loop },
    { .id = 0x004F, .on = "--daemon", .ac = 0, .op = opt_g_daemonize },
#ifdef _GL_DUMMY_NONE
        { .id = 0x0063, .on = "regexi", .ac = 1, .op = opt_g_iregexi},
        { .id = 0x0064, .on = "--regexi", .ac = 1, .op = opt_g_iregexi},
        { .id = 0x0065, .on = "regex", .ac = 1, .op = opt_g_iregex},
        { .id = 0x0066, .on = "--regex", .ac = 1, .op = opt_g_iregex},
        { .id = 0x0067, .on = "iregexi", .ac = 1, .op = opt_g_regexi},
        { .id = 0x0068, .on = "--iregexi", .ac = 1, .op = opt_g_regexi},
        { .id = 0x0069, .on = "iregex", .ac = 1, .op = opt_g_regex},
        { .id = 0x006A, .on = "--iregex", .ac = 1, .op = opt_g_regex},
        { .id = 0x003E, .on = "--match", .ac = 1, .op = opt_g_imatch},
        { .id = 0x003F, .on = "match", .ac = 1, .op = opt_g_imatch},
        { .id = 0x0040, .on = "imatch", .ac = 1, .op = opt_g_match},
        { .id = 0x0041, .on = "--imatch", .ac = 1, .op = opt_g_match},
        { .id = 0x0005, .on = "ilom", .ac = 1, .op = opt_g_lom_match},
        { .id = 0x0006, .on = "--ilom", .ac = 1, .op = opt_g_lom_match},
        { .id = 0x0007, .on = "lom", .ac = 1, .op = opt_g_lom_imatch},
        { .id = 0x0008, .on = "--lom", .ac = 1, .op = opt_g_lom_imatch},
#endif
        { .id = 0x0067, .on = "regexi", .ac = 1, .op = opt_g_d_regexi },
        { .id = 0x0068, .on = "--regexi", .ac = 1, .op = opt_g_d_regexi },
        { .id = 0x0468, .on = "-regexi", .ac = 1, .op = opt_g_d_regexi },
        { .id = 0x0069, .on = "regex", .ac = 1, .op = opt_g_d_regex },
        { .id = 0x006A, .on = "--regex", .ac = 1, .op = opt_g_d_regex },
        { .id = 0x046A, .on = "-regex", .ac = 1, .op = opt_g_d_regex },
        { .id = 0x0040, .on = "match", .ac = 1, .op = opt_g_d_match },
        { .id = 0x0041, .on = "--match", .ac = 1, .op = opt_g_d_match },
        { .id = 0x0441, .on = "-match", .ac = 1, .op = opt_g_d_match },
        { .id = 0x1040, .on = "name", .ac = 1, .op = opt_g_d_fname },
        { .id = 0x1041, .on = "--name", .ac = 1, .op = opt_g_d_fname },
        { .id = 0x1441, .on = "-name", .ac = 1, .op = opt_g_d_fname },
        { .id = 0x1042, .on = "namei", .ac = 1, .op = opt_g_d_fnamei },
        { .id = 0x1043, .on = "--namei", .ac = 1, .op = opt_g_d_fnamei },
        { .id = 0x1443, .on = "-namei", .ac = 1, .op = opt_g_d_fnamei },
        { .id = 0x0005, .on = "lom", .ac = 1, .op = opt_g_d_lom_match },
        { .id = 0x0006, .on = "--lom", .ac = 1, .op = opt_g_d_lom_match },
        { .id = 0x0406, .on = "-lom", .ac = 1, .op = opt_g_d_lom_match },
        { .id = 0x1000, .on = "!", .ac = 0, .op = opt_g_negate },
        { .id = 0x1001, .on = "-fd:", .ac = 0, .op = opt_g_tfd },
        { .id = 0x1001, .on = "-prune:", .ac = 0, .op = opt_g_tfd },
        { .id = 0x0002, .on = "and", .ac = 0, .op = opt_g_operator_and },
        { .id = 0x0003, .on = "or", .ac = 0, .op = opt_g_operator_or },
        { .id = 0x0004, .on = "(", .ac = 0, .op = opt_g_m_raise_level },
        { .id = 0x0005, .on = ")", .ac = 0, .op = opt_g_m_lower_level },
        { .id = 0x0070, .on = "-y", .ac = 0, .op = opt_g_flinks },
        { .id = 0x0071, .on = "--allowsymbolic", .ac = 0, .op = opt_g_flinks },
        { .id = 0x0072, .on = "--followlinks", .ac = 0, .op = opt_g_flinks },
        { .id = 0x0073, .on = "--allowlinks", .ac = 0, .op = opt_g_flinks },
        { .id = 0x0074, .on = "--execv", .ac = 1, .op = opt_execv },
        { .id = 0x0075, .on = "-execv", .ac = 1, .op = opt_execv },
        { .id = 0x0076, .on = "-execv-", .ac = 1, .op = opt_execv_stdin },
        { .id = 0x0077, .on = "-exec", .ac = 1, .op = opt_exec },
        { .id = 0x0078, .on = "-exec-", .ac = 1, .op = opt_exec_stdin },
        { .id = 0x0079, .on = "--exec", .ac = 1, .op = opt_exec },
        { .id = 0x007A, .on = "--fix", .ac = 0, .op = opt_g_fix },
        { .id = 0x007C, .on = "--memlimit", .ac = 1, .op = opt_memb_limit },
        { .id = 0x007D, .on = "--memlimita", .ac = 1, .op = opt_memb_limit_in },
        { .id = 0x007F, .on = "--dupechk", .ac = 0, .op = opt_dirlog_chk_dupe },
        { .id = 0x0080, .on = "--nobuffer", .ac = 0, .op = opt_g_nobuffering },
        { .id = 0x0082, .on = "--help", .ac = 0, .op = print_help },
        { .id = 0x0083, .on = "--version", .ac = 0, .op = print_version },
        { .id = 0x008A, .on = "--sfv", .ac = 0, .op = opt_g_sfv },
        { .id = 0x008B, .on = "--crc32", .ac = 1, .op = option_crc32 },
        { .id = 0x008C, .on = "--nobackup", .ac = 0, .op = opt_nobackup },
        { .id = 0x008E, .on = "--check", .ac = 0, .op = opt_dirlog_check },
        { .id = 0x008F, .on = "--dump", .ac = 0, .op = opt_dirlog_dump },
        { .id = 0x0091, .on = "-f", .ac = 0, .op = opt_g_force },
        { .id = 0x0092, .on = "-ff", .ac = 0, .op = opt_g_force2 },
        { .id = 0x0097, .on = "--loadq", .ac = 0, .op = opt_g_loadq },
        { .id = 0x0098, .on = "--loadqa", .ac = 0, .op = opt_g_loadqa },
        { .id = 0x009B, .on = "--maxres", .ac = 1, .op = opt_g_maxresults },
        { .id = 0x009C, .on = "--maxhit", .ac = 1, .op = opt_g_maxhits },
        { .id = 0x009D, .on = "--ifres", .ac = 0, .op = opt_g_ifres },
        { .id = 0x009E, .on = "--ifhit", .ac = 0, .op = opt_g_ifhit },
        { .id = 0x009F, .on = "--ifrhe", .ac = 0, .op = opt_g_ifrh_e },
        { .id = 0x00A1, .on = "--esredir", .ac = 1, .op = opt_execv_stdout_rd },
        { .id = 0x00A2, .on = "--noglconf", .ac = 0, .op = opt_g_noglconf },
        { .id = 0x00A3, .on = "--maxdepth", .ac = 1, .op = opt_g_maxdepth },
        { .id = 0x00A4, .on = "-maxdepth", .ac = 1, .op = opt_g_maxdepth },
        { .id = 0x00A5, .on = "--mindepth", .ac = 1, .op = opt_g_mindepth },
        { .id = 0x00A6, .on = "-mindepth", .ac = 1, .op = opt_g_mindepth },
        { .id = 0x00A7, .on = "--noereg", .ac = 0, .op = opt_g_noereg },
        { .id = 0x00AA, .on = "--prune", .ac = 0, .op = opt_prune },
        { .id = 0x00AE, .on = "--xretry", .ac = 0, .op = opt_g_xretry },
        { .id = 0x22AE, .on = "--xloop", .ac = 0, .op = opt_g_xloop },
        { .id = 0x00AF, .on = "--indepth", .ac = 0, .op = opt_g_indepth },
        { .id = 0x00B0, .on = "--full", .ac = 0, .op = opt_dirlog_rb_full },
        { .id = 0x00B1, .on = "--arr", .ac = 1, .op = opt_arrange },
        { .id = 0x00B2, .on = "--nonukechk", .ac = 0, .op = opt_no_nuke_chk },
        { .id = 0x00B3, .on = "--rsleep", .ac = 1, .op = opt_g_loop_sleep },
        { .id = 0x00B4, .on = "--rusleep", .ac = 1, .op = opt_g_loop_usleep },
        { .id = 0x00B5, .on = "--nostats", .ac = 0, .op = opt_g_nostats },
        { .id = 0x00B6, .on = "--stats", .ac = 0, .op = opt_g_stats },
        { .id = 0x00B7, .on = "-mlist", .ac = 0, .op = opt_g_mlist },
        { .id = 0x00B8, .on = "--gz", .ac = 1, .op = opt_g_comp },
        { .id = 0x00BA, .on = "--progress", .ac = 0, .op = opt_g_progress },
        { .id = 0x2512, .on = "--fsrec", .ac = 0, .op = opt_g_fsroot },
        { .id = 0x5591, .on = "--stdlog", .ac = 1, .op = opt_g_stdout_lvl },
        { .id = 0x5512, .on = "--xflags", .ac = 1, .op = opt_xref_sl_dat },
        { .id = 0x5513, .on = "--depth", .ac = 0, .op = opt_xref_depth },
#ifndef _MAKE_SBIN
        { .id = 0x1282, .on = "--mroot", .ac = 1, .op = g_opt_mroot },
        { .id = 0x00A0, .on = "--nofq", .ac = 0, .op = opt_g_nofq },
        { .id = 0x006C, .on = "--comp", .ac = 0, .op = opt_crof },
        { .id = 0x006D, .on = "--batch", .ac = 0, .op = opt_bo_formatting },
        { .id = 0x006E, .on = "-E", .ac = 0, .op = opt_ex_o_formatting },
        { .id = 0x006F, .on = "--export", .ac = 0, .op = opt_ex_o_formatting },
        { .id = 0x0051, .on = "--ipc", .ac = 1, .op = opt_shmipc },
        { .id = 0x0060, .on = "--nowbuffer", .ac = 0, .op = opt_g_buffering },
        { .id = 0x0089, .on = "--nowrite", .ac = 0, .op = opt_g_nowrite },
        { .id = 0x0061, .on = "--raw", .ac = 0, .op = opt_raw_dump },
        { .id = 0x0062, .on = "--binary", .ac = 0, .op = opt_binary },
        { .id = 0x0095, .on = "--shmem", .ac = 0, .op = opt_g_shmem },
        { .id = 0x0096, .on = "--shmreload", .ac = 0, .op = opt_g_shmreload },
        { .id = 0x0099, .on = "--shmdestroy", .ac = 0, .op = opt_g_shmdestroy },
        { .id = 0x009A, .on = "--shmdestonexit", .ac = 0, .op = opt_g_shmdoex },
        { .id = 0x00B9, .on = "--sortmethod", .ac = 1, .op = opt_g_swapmode },
        { .id = 0x0053, .on = "--ge1log", .ac = 1, .op = opt_GE1LOG },
        { .id = 0x0054, .on = "--ge2log", .ac = 1, .op = opt_GE2LOG },
        { .id = 0x0055, .on = "--ge3log", .ac = 1, .op = opt_GE3LOG },
        { .id = 0x0056, .on = "--gamelog", .ac = 1, .op = opt_gamelog },
        { .id = 0x0057, .on = "--tvlog", .ac = 1, .op = opt_tvlog },
        { .id = 0x0058, .on = "--imdblog", .ac = 1, .op = opt_imdblog },
        { .id = 0x0059, .on = "--oneliners", .ac = 1, .op = opt_oneliner },
        { .id = 0x005B, .on = "--lastonlog", .ac = 1, .op = opt_lastonlog },
        { .id = 0x005D, .on = "--dupefile", .ac = 1, .op = opt_dupefile },
        { .id = 0x005E, .on = "--sconf", .ac = 1, .op = opt_sconf },
        { .id = 0x005F, .on = "--gconf", .ac = 1, .op = opt_gconf },
        { .id = 0x00AB, .on = "--glconf", .ac = 1, .op = opt_glconf_file },
        { .id = 0x00AC, .on = "--ge4log", .ac = 1, .op = opt_GE4LOG },
        { .id = 0x00AD, .on = "--altlog", .ac = 1, .op = opt_altlog },
        { .id = 0x0084, .on = "--folders", .ac = 1, .op = opt_dirlog_sect_fl },
        { .id = 0x0085, .on = "--dirlog", .ac = 1, .op = opt_dirlog_file },
        { .id = 0x0086, .on = "--nukelog", .ac = 1, .op = opt_nukelog_file },
        { .id = 0x0087, .on = "--siteroot", .ac = 1, .op = opt_siteroot },
        { .id = 0x0088, .on = "--glroot", .ac = 1, .op = opt_glroot },
        { .id = 0x1288, .on = "--shmcflags", .ac = 1, .op = g_opt_shmflg },
        { .id = 0x1288, .on = "--shmro", .ac = 0, .op = g_opt_shmro },
        { .id = 0x0023, .on = "-g", .ac = 0, .op = opt_dump_grps },
        { .id = 0x0024, .on = "-t", .ac = 0, .op = opt_dump_users },
        { .id = 0x0032, .on = "-b", .ac = 1, .op = opt_backup },
        { .id = 0x0052, .on = "-l", .ac = 0, .op = opt_lastonlog_dump },
        { .id = 0x0081, .on = "-n", .ac = 0, .op = opt_dirlog_dump_nukelog },
        { .id = 0x007E, .on = "-p", .ac = 0, .op = opt_dirlog_chk_dupe },
        { .id = 0x001E, .on = "-q", .ac = 1, .op = opt_g_dg },
        { .id = 0x001F, .on = "-x", .ac = 1, .op = opt_g_udc },
        { .id = 0x000C, .on = "-h", .ac = 0, .op = opt_g_dump_tv },
        { .id = 0x000D, .on = "-k", .ac = 0, .op = opt_g_dump_game },
        { .id = 0x0011, .on = "-a", .ac = 0, .op = opt_g_dump_imdb },
        { .id = 0x0012, .on = "-z", .ac = 1, .op = opt_g_write },
        { .id = 0x005A, .on = "-o", .ac = 0, .op = opt_oneliner_dump },
        { .id = 0x0050, .on = "-w", .ac = 0, .op = opt_online_dump },
        { .id = 0x005C, .on = "-i", .ac = 0, .op = opt_dupefile_dump },
        { .id = 0x006B, .on = "-e", .ac = 1, .op = opt_rebuild },
        { .id = 0x008D, .on = "-c", .ac = 0, .op = opt_dirlog_check },
        { .id = 0x0090, .on = "-d", .ac = 0, .op = opt_dirlog_dump },
        { .id = 0x007B, .on = "-u", .ac = 0, .op = opt_g_update },
        { .id = 0x0093, .on = "-s", .ac = 1, .op = opt_update_single_record },
        { .id = 0x0094, .on = "-r", .ac = 0, .op = opt_rec_upd_records },
#endif
        { 0x0 } };
