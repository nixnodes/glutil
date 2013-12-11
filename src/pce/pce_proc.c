/*
 * pce_proc.c
 *
 *  Created on: Dec 7, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "pce_config.h"
#include "pce_proc.h"
#include "pce_misc.h"

#include <str.h>
#include <x_f.h>
#include <log_io.h>
#include <log_op.h>
#include <lref_sconf.h>
#include <lref_gconf.h>
#include <lref_imdb.h>
#include <omfp.h>
#include <m_string.h>
#include <m_lom.h>
#include <lc_oper.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <regex.h>

_g_handle h_gconf =
  { 0 };
__d_gconf gconf = NULL;

uint32_t g_hflags = F_DL_FOPEN_BUFFER;

int
pce_proc(char *subject)
{
  gfl |= G_HFLAGS | F_OPT_PS_SILENT;

  int r;

  if ((r = g_fopen(GCONFLOG, "r", F_DL_FOPEN_BUFFER, &h_gconf)))
    {
      pce_log("ERROR: failed opening '%s', code %d\n", GCONFLOG, r);
      goto end;
    }

  gconf = (__d_gconf) h_gconf.buffer.pos->ptr;

  gfl ^= G_HFLAGS;

  char s_b[255], *s_b_p = s_b;

  if (!(s_b_p = reg_getsubm(subject, gconf->r_sects, REG_EXTENDED, s_b, 255)))
    {
      goto end;
    }

  if (gconf->o_use_shared_mem)
    {
      gfl |= F_OPT_SHAREDMEM;
    }

  s_b_p = g_zerom(s_b_p, 0x2F);
  g_zerom_r(s_b_p, 0x2F);
  cl_dir = subject;

  char s_lp[PATH_MAX];

  snprintf(s_lp, PATH_MAX, "%s/%s/%s/%s", GLROOT, FTPDATA, DEFPATH_LOGS, s_b_p);
  remove_repeating_chars(s_lp, 0x2F);

  if (file_exists(s_lp))
    {
      pce_log("ERROR: %s - no section configuration exists at '%s'\n", s_b_p,
          s_lp);
      goto end;
    }

  _g_handle h_sconf =
    { 0 };
  mda lh_ref =
    { 0 };

  if (determine_datatype(&h_sconf, SCONFLOG))
    {
      pce_log("ERROR: SCONF: determine_datatype failed\n");
      goto end;
    }

  if ((r = g_fopen(s_lp, "r", F_DL_FOPEN_BUFFER, &h_sconf)))
    {
      pce_log("ERROR: failed opening '%s', code %d\n", s_lp, r);
      goto end;
    }

  md_init(&lh_ref, 4);

  off_t nres;

  if ((r = g_enum_log(pce_match_build, &h_sconf, &nres, &lh_ref)))
    {
      pce_log("ERROR: failed processing records in '%s', code %d\n", s_lp, r);
      goto end;
    }

  end:

  g_cleanup(&h_gconf);
  g_cleanup(&h_sconf);
  pce_lh_ref_clean(&lh_ref);

  if (fd_log)
    {
      fclose(fd_log);
    }

  return EXITVAL;
}

int
pce_lh_ref_clean(pmda lh_ref)
{
  p_md_obj ptr = md_first(lh_ref);
  __g_handle hdl;

  while (ptr)
    {
      hdl = (__g_handle) ptr->ptr;
      if (!hdl)
        {
          break;
        }
      p_md_obj l_ptr = ptr;
      ptr = ptr->next;
      g_cleanup(hdl);
      free(l_ptr->ptr);
    }

  free(lh_ref->objects);

  return 0;
}

int
pce_match_build(void *_hdl, void *_ptr, void *arg)
{
  __d_sconf ptr = (__d_sconf) _ptr;
  pmda lh_ref = (pmda) arg;

  char *log_s;

  if (ptr->i32)
    {
      _g_dgetr dgetr =
        { 0};

      log_s = pce_dgetlf(&dgetr, ptr->i32);

      if ( !log_s )
        {
          return 1;
        }

      __g_handle p_log;
      int r;

      if (!(p_log=pce_lhref_fst(lh_ref, &dgetr.pf)))
        {
          p_log = md_alloc(lh_ref, sizeof(_g_handle));
          p_log->flags |= F_GH_HASMATCHES;

          if ((r=g_fopen(log_s, "r", F_DL_FOPEN_BUFFER, p_log)))
            {
              pce_log("ERROR: failed opening '%s', code %d\n", log_s, r);
              p_log->flags |= F_GH_LOCKED;
              return 0;
            }

          if ((r=pce_do_lookup(p_log, &dgetr) ) != 1)
            {
              return r;
            }
        }
      else
        {
          if ((p_log->flags & F_GH_LOCKED))
            {
              pce_log("NOTICE: '%s': this log has been locked\n", p_log->file );
              return 0;
            }
        }

      int i_m = 0;

      if (ptr->i32_1 == 1)
        {
          i_m = 0;
        }
      else if (ptr->i32_1 == 2)
        {
          i_m = REG_NOMATCH;
        }

      if (!(r=pce_match_log(p_log, ptr, i_m)))
        {
          pce_log("WARNING: '%s': rule chain hit positive match (%s) (%s), blocking..\n", cl_sub, p_log->file, ptr->match);
          EXITVAL = 1;
          if (ptr->message[0])
            {
              printf("200 %s\n", ptr->message);
            }
          return -1;
        }
      else
        {
          pce_pcl_stat(r, ptr);
        }
    }
  else
    {
      int i_m = 0;
      if (ptr->i32_1 == 1)
        {
          i_m = 0;
        }
      else if (ptr->i32_1 == 2)
        {
          i_m = REG_NOMATCH;
        }

      int r;
      if (!(r=pce_do_regex_match(ptr->match, cl_dir, REG_EXTENDED, i_m)))
        {
          pce_log("WARNING: '%s': rule chain hit positive match (%s), blocking..\n", cl_sub, ptr->match);
          EXITVAL = 1;
          if (ptr->message[0])
            {
              printf("200 %s\n", ptr->message);
            }
          return -1;
        }
      else
        {
          pce_pcl_stat(r, ptr);
        }
    }

  return 0;
}

void
pce_pcl_stat(int r, __d_sconf ptr)
{
  if (r < 0)
    {
      pce_log("ERROR: %d processing chain link (%s) (%s)\n", r, ptr->field,
          ptr->match);
    }
  else
    {
      pce_log("NOTICE: chain link passed (%s) (%s)\n", ptr->field, ptr->match);
    }
}

int
pce_do_lookup(__g_handle p_log, __d_dgetr dgetr)
{
  int r;

  if (!(pce_f & F_PCE_DONE_STR_PREPROC))
    {
      if (!pce_do_str_preproc(g_basename(cl_dir)))
        {
          pce_log(
              "ERROR: unable to preprocess the directory string, aborting\n");
          return -1;
        }
      //pce_log("%s | %s\n", cl_sub, s_year);
      pce_f |= F_PCE_DONE_STR_PREPROC;
    }

  if (s_year && dgetr->d_yf)
    {
      uint64_t year = (uint64_t) strtoul(s_year, NULL, 10);
      md_init(&p_log->_match_rr, 2);
      __g_match tt_m = md_alloc(&p_log->_match_rr, sizeof(_g_match));
      tt_m->g_oper_ptr = g_oper_and;
      tt_m->flags |= F_GM_NAND;
      tt_m->match_i_m = 0;

      if ((r = g_build_lom_packet_bare(p_log, tt_m, dgetr->d_yf, &year,
          _lcs_isequal, g_oper_and)))
        {
          pce_log(" ERROR: unable to commit lom match : %d\n", r);
          p_log->flags |= F_GH_LOCKED;
          return 0;
        }
    }

  if ((r = g_commit_strm_regex(p_log, dgetr->d_field, cl_sub, 0,
  REG_EXTENDED | REG_ICASE, F_GM_ISREGEX)))
    {
      pce_log("ERROR: unable to commit regex match : %d\n", r);
      p_log->flags |= F_GH_LOCKED;
      return 0;
    }

  off_t nres;

  if ((r = g_enum_log(pce_run_log_match, p_log, &nres, NULL)))
    {
      pce_log("ERROR: could not find match '%s (%s)' in '%s', code %d, %llu\n",
          cl_sub, s_year, p_log->file, r, (ulint64_t) nres);
      p_log->flags |= F_GH_LOCKED;
      return 0;
    }
  return 1;
}

int
pce_match_log(__g_handle hdl, __d_sconf sconf, int m_i_m)
{
  _d_drt_h dtr =
    { 0 };

  __g_proc_v h_lk = hdl->g_proc1_lookup(hdl->buffer.pos->ptr, sconf->field,
      hdl->mv1_b,
      MAX_VAR_LEN, &dtr);

  if (!h_lk)
    {
      return -12;
    }

  char *r_v = h_lk(hdl->buffer.pos->ptr, sconf->field, hdl->mv1_b,
  MAX_VAR_LEN, &dtr);

  if (!r_v)
    {
      return -14;
    }
  // pce_log("%s : %s\n", r_v, sconf->match);
  return pce_do_regex_match(sconf->match, r_v, REG_EXTENDED | REG_ICASE, m_i_m);
}

int
pce_do_regex_match(char *pattern, char *match, int cflags, int m_i_m)
{
  regex_t preg;
  int r;

  if ((r = regcomp(&preg, pattern, cflags | REG_NOSUB)))
    {
      pce_log("ERROR: could not compile pattern '%s'\n", pattern);
      return 0;
    }

  if (regexec(&preg, match, 0, NULL, 0) == m_i_m)
    {
      regfree(&preg);
      return 0;
    }

  regfree(&preg);
  return 1;
}

int
pce_run_log_match(void *_hdl, void *_ptr, void *arg)
{
  __g_handle hdl = (__g_handle) _hdl;

  if (g_bmatch(_ptr, hdl,&hdl->buffer ))
    {
      return -1;
    }

  return 1;
}

__g_handle
pce_lhref_fst(pmda lh_ref, uint64_t *pf)
{
  p_md_obj ptr = md_first(lh_ref);
  __g_handle hdl;

  while (ptr)
    {
      hdl = (__g_handle) ptr->ptr;
      if (!hdl)
        {
          break;
        }
      if ((hdl->flags & *pf))
        {
          return hdl;
        }
      ptr = ptr->next;
    }

  return NULL;
}

char *
pce_dgetlf(__d_dgetr dgetr, int logc)
{
  switch (logc)
    {
  case 1:
    dgetr->pf |= F_GH_ISIMDB;
    dgetr->d_field = "title";
    dgetr->d_yf = "year";
    return IMDBLOG;
    break;
  case 2:
    dgetr->pf |= F_GH_ISTVRAGE;
    dgetr->d_field = "name";
    dgetr->d_yf = "startyear";
    return TVLOG;
    break;
    }
  return NULL;
}

char *
pce_get_year_result(char *subject, char *output, size_t max_size)
{
  char *syr_p;

  if ((syr_p = reg_getsubm(subject, gconf->r_yearm,
  REG_EXTENDED, output, max_size)))
    {
      while (is_ascii_numeric((uint8_t) syr_p[0]) && syr_p[0])
        {
          syr_p++;
        }

      if (!syr_p[0])
        {
          return NULL;
        }

      char *syr_pt;

      syr_pt = syr_p;

      while (!is_ascii_numeric((uint8_t) syr_pt[0]) && syr_pt[0])
        {
          syr_pt++;
        }

      syr_pt[0] = 0x0;

    }

  return syr_p;
}

char*
pce_do_str_preproc(char *subject)
{
  char *s_rs;
  if (!(s_rs = reg_sub_d(subject, gconf->r_clean,
  REG_EXTENDED | REG_ICASE, cl_sub)))
    {
      return NULL;
    }

  s_year = pce_get_year_result(subject, cl_yr, sizeof(cl_yr));

  return s_rs;
}

