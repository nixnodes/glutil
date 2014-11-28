/*
 * lref_dirlog.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_gen.h"

#include <l_sb.h>
#include <m_comp.h>
#include <lref.h>
#include <str.h>
#include <x_f.h>

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

char *l_av_st[L_AV_MAX] =
  { 0 };

static char *
dt_legacy_gg_int(char *match, char *output, size_t max_size)
{
  int idx;
  DT_GG_GIDX(match)
  snprintf(output, max_size, "%llu",
      (unsigned long long int) glob_ui64_stor[idx]);
  return output;
}

static char *
dt_legacy_gg_sint(char *match, char *output, size_t max_size)
{
  int idx;
  DT_GG_GIDX(match)
  snprintf(output, max_size, "%lld", (long long int) glob_si64_stor[idx]);
  return output;
}

static char *
dt_legacy_gg_float(char *match, char *output, size_t max_size)
{
  int idx;
  DT_GG_GIDX(match)
  snprintf(output, max_size, "%f", glob_float_stor[idx]);
  return output;
}

int
ref_to_val_generic(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strncmp(match, "nukestr", 7))
    {
      if (NUKESTR)
        {
          snprintf(output, max_size, NUKESTR, "");
        }
    }
  else if (!strncmp(match, "procid", 6))
    {
      snprintf(output, max_size, "%d", getpid());
    }
  else if (!strncmp(match, "ipc", 3))
    {
      snprintf(output, max_size, "%.8X", (uint32_t) SHM_IPC);
    }
  else if (!strncmp(match, "usroot", 6))
    {
      snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
      DEFPATH_USERS);
      remove_repeating_chars(output, 0x2F);
    }
  else if (!strncmp(match, "logroot", 7))
    {
      snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
      DEFPATH_LOGS);
      remove_repeating_chars(output, 0x2F);
    }
  else if (!strncmp(match, "memlimit", 8))
    {
      snprintf(output, max_size, "%llu", db_max_size);
    }
  else if (!strncmp(match, "curtime", 7))
    {
      snprintf(output, max_size, "%d", (int32_t) time(NULL));
    }
  else if (!strncmp(match, "q:", 2))
    {
      return rtv_q(&match[2], output, max_size);
    }
  else if (!strncmp(match, "?q:", 3))
    {
      return rtv_q(&match[3], output, max_size);
    }
  else if (!strncmp(match, "exe", 3))
    {
      if (self_get_path(output))
        {
          output[0x0] = 0;
        }
    }
  else if (!strncmp(match, "glroot", 6))
    {
      strcp_s(output, max_size, GLROOT);
    }
  else if (!strncmp(match, "siterootb", 9))
    {
      strcp_s(output, max_size, SITEROOT_N);
    }
  else if (!strncmp(match, "siteroot", 8))
    {
      strcp_s(output, max_size, SITEROOT);
    }
  else if (!strncmp(match, "siterootn", 9))
    {
      strcp_s(output, max_size, SITEROOT);
    }
  else if (!strncmp(match, "ftpdata", 7))
    {
      strcp_s(output, max_size, FTPDATA);
    }
  else if (!strncmp(match, "logfile", 7))
    {
      strcp_s(output, max_size, LOGFILE);
    }
  else if (!strncmp(match, "imdbfile", 8))
    {
      strcp_s(output, max_size, IMDBLOG);
    }
  else if (!strncmp(match, "gamefile", 8))
    {
      strcp_s(output, max_size, GAMELOG);
    }
  else if (!strncmp(match, "tvragefile", 10))
    {
      strcp_s(output, max_size, TVLOG);
    }
  else if (!strncmp(match, "spec1", 5))
    {
      strcp_s(output, max_size, b_spec1);
    }
  else if (!strncmp(match, "pspec1", 6))
    {
      strcp_s(output, max_size, spec_p1);
    }
  else if (!strncmp(match, "pspec2", 6))
    {
      strcp_s(output, max_size, spec_p2);
    }
  else if (!strncmp(match, "pspec3", 6))
    {
      strcp_s(output, max_size, spec_p3);
    }
  else if (!strncmp(match, "pspec4", 6))
    {
      strcp_s(output, max_size, spec_p4);
    }
  else if (!strncmp(match, "glconf", 6))
    {
      strcp_s(output, max_size, GLCONF_I);
    }
  else if (!strncmp(match, _MC_GLOB_U64G, 7))
    {
      dt_legacy_gg_int(match, output, max_size);
    }
  else if (!strncmp(match, _MC_GLOB_S64G, 7))
    {
      dt_legacy_gg_sint(match, output, max_size);
    }
  else if (!strncmp(match, _MC_GLOB_F32G, 7))
    {
      dt_legacy_gg_float(match, output, max_size);
    }
  else
    {
      return 1;
    }
  return 0;
}

char *
dt_rval_generic_nukestr(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (NUKESTR)
    {
      snprintf(output, max_size, NUKESTR, "");
    }
  return output;
}

char *
dt_rval_generic_procid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, getpid());
  return output;
}

char *
dt_rval_generic_ipc(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((__d_drt_h ) mppd)->hdl->ipc_key);
  return output;
}

char *
dt_rval_generic_usroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
  DEFPATH_USERS);
  remove_repeating_chars(output, 0x2F);
  return output;
}

char *
dt_rval_generic_logroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
  DEFPATH_LOGS);
  remove_repeating_chars(output, 0x2F);
  return output;
}

char *
dt_rval_generic_memlimit(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, db_max_size);
  return output;
}

char *
dt_rval_generic_curtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) time(NULL));
  return output;
}

char *
dt_rval_q(void *arg, char *match, char *output, size_t max_size, void *mppd)
{

  while (match[0] != 0x3A && match[0])
    {
      match++;
    }

  match++;

  if (!match[0])
    {
      return output;
    }

  if (rtv_q(match, output, max_size))
    {
      output[0] = 0x0;
    }
  return output;
}

char *
dt_rval_gg_int(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (unsigned long long int) glob_ui64_stor[((__d_drt_h) mppd)->uc_1]);

  return output;
}

char *
dt_rval_gg_sint(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (long long int) glob_si64_stor[((__d_drt_h) mppd)->uc_1]);

  return output;
}

char *
dt_rval_gg_float(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      glob_float_stor[((__d_drt_h) mppd)->uc_1]);

  return output;
}

char *
dt_rval_generic_exe(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (self_get_path(output))
    {
      output[0x0] = 0;
    }
  return output;
}

char *
dt_rval_generic_glroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return GLROOT;
}

char *
dt_rval_generic_siteroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return SITEROOT;
}

char *
dt_rval_generic_siterootn(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return SITEROOT_N;
}

char *
dt_rval_generic_ftpdata(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return FTPDATA;
}

char *
dt_rval_generic_imdbfile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return IMDBLOG;
}

char *
dt_rval_generic_tvfile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return TVLOG;
}

char *
dt_rval_generic_gamefile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return GAMELOG;
}

char *
dt_rval_generic_spec1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return b_spec1;
}

char *
dt_rval_generic_glconf(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return GLCONF_I;
}

char *
dt_rval_generic_logfile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return LOGFILE;
}

char *
dt_rval_generic_newline(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return MSG_NL;
}

char *
dt_rval_generic_cr(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return MSG_CR;
}

char *
dt_rval_generic_tab(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return MSG_TAB;
}

char *
dt_rval_generic_pspec1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (spec_p1)
    {
      return spec_p1;
    }
  return "";
}

char *
dt_rval_generic_pspec2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (spec_p2)
    {
      return spec_p2;
    }
  return "";
}

char *
dt_rval_generic_pspec3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (spec_p3)
    {
      return spec_p3;
    }
  return "";
}

char *
dt_rval_generic_pspec4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (spec_p4)
    {
      return spec_p4;
    }
  return "";
}

static char *
dt_rval_generic_lav(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char *ptr = l_av_st[((__d_drt_h ) mppd)->v_ui0];

  if (NULL != ptr)
    {
      snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
          l_av_st[((__d_drt_h ) mppd)->v_ui0]);
    }
  else
    {
      output[0] = 0x0;
    }
  return output;
}

static void *
rt_af_gen_lav(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  while (is_ascii_numeric(match[0]) && 0 != match[0])
    {
      match++;
    }

  if (match[0] == 0x0)
    {
      print_str("ERROR: rt_af_gen_lav: invalid 'arg' variable name: '%s'\n",
          match);
      return NULL;
    }

  errno = 0;
  uint32_t lav_idx = (uint32_t) strtoul(match, NULL, 10);

  if (errno == EINVAL || errno == ERANGE)
    {
      print_str("ERROR: rt_af_gen_lav: could not get index: '%s'\n", match);
      return NULL;
    }

  uint32_t max_index = sizeof(l_av_st) / sizeof(void*);

  if (lav_idx > max_index)
    {
      print_str("ERROR: rt_af_gen_lav: index out of range: %u, max: %u\n",
          lav_idx, max_index);
      return NULL;
    }

  __d_drt_h _mppd = (__d_drt_h) mppd;

  _mppd->v_ui0 = lav_idx;

  return as_ref_to_val_lk(match, dt_rval_generic_lav, (__d_drt_h ) mppd, "%s");
}

#define MSG_GENERIC_BS         0x3A

void *
ref_to_val_lk_generic(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{

  if (!strncmp(match, "nukestr", 7))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_nukestr, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "procid", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_procid, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, "ipc", 3))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_ipc, (__d_drt_h ) mppd,
          "%X");
    }
  else if (!strncmp(match, "usroot", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_usroot, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "logroot", 7))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_logroot, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "memlimit", 8))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_memlimit,
          (__d_drt_h ) mppd, "%llu");
    }
  else if (!strncmp(match, "curtime", 7))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_curtime, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, "q:", 2))
    {
      return as_ref_to_val_lk(&match[4], dt_rval_q, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_U64G, 7))
    {
      int idx;
      DT_GG_GIDX(match)
      ((__d_drt_h ) mppd)->uc_1 = (uint8_t) idx;

      return as_ref_to_val_lk(match, dt_rval_gg_int, (__d_drt_h ) mppd, "%llu");
    }
  else if (!strncmp(match, _MC_GLOB_S64G, 7))
    {
      int idx;
      DT_GG_GIDX(match)
      ((__d_drt_h ) mppd)->uc_1 = (uint8_t) idx;

      return as_ref_to_val_lk(match, dt_rval_gg_sint, (__d_drt_h ) mppd, "%lld");
    }
  else if (!strncmp(match, _MC_GLOB_F32G, 7))
    {
      int idx;
      DT_GG_GIDX(match)
      ((__d_drt_h ) mppd)->uc_1 = (uint8_t) idx;

      return as_ref_to_val_lk(match, dt_rval_gg_float, (__d_drt_h ) mppd, "%f");
    }
  else if (match[0] == 0x3A)
    {
      switch (match[1])
        {
      case 0x6E:
        return as_ref_to_val_lk(match, dt_rval_generic_newline,
            (__d_drt_h ) mppd, "%s");
      case 0x74:
        return as_ref_to_val_lk(match, dt_rval_generic_tab, (__d_drt_h ) mppd,
            "%s");
      case 0x72:
        return as_ref_to_val_lk(match, dt_rval_generic_cr, (__d_drt_h ) mppd,
            "%s");
      case 0x52:
        return as_ref_to_val_lk(match, dt_rval_generic_cr, (__d_drt_h ) mppd,
            "%s");
      case 0x4E:
        return as_ref_to_val_lk(match, dt_rval_generic_newline,
            (__d_drt_h ) mppd, "%s");
      case 0x54:
        return as_ref_to_val_lk(match, dt_rval_generic_tab, (__d_drt_h ) mppd,
            "%s");
        }

    }
  else if (!strncmp(match, "exe", 3))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_exe, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "glroot", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_glroot, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "siterootb", 9))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_siterootn,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, "siterootn", 9))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_siteroot,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, "siteroot", 8))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_siteroot,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, "ftpdata", 7))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_ftpdata, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "logfile", 7))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_logfile, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "imdbfile", 8))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_imdbfile,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, "gamefile", 8))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_gamefile,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, "tvragefile", 10))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_tvfile, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "spec1", 5))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_spec1, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "glconf", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_glconf, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "pspec1", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_pspec1, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "pspec2", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_pspec2, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "pspec3", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_pspec3, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "pspec4", 6))
    {
      return as_ref_to_val_lk(match, dt_rval_generic_pspec4, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, "arg", 3))
    {
      return rt_af_gen_lav(arg, match, output, max_size, (__d_drt_h ) mppd);
    }
  else if (match[0] == 0x3F)
    {
      ((__d_drt_h ) mppd)->st_ptr0 = dt_rval_q;
      return ref_to_val_af(arg, match, output, max_size, (__d_drt_h ) mppd);
    }

  return NULL;
}
