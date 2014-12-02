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
#include <lref_gen.h>
#include <omfp.h>
#include <m_string.h>
#include <m_lom.h>
#include <lc_oper.h>
#include <exec_t.h>
#include <exech.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <regex.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

_g_handle h_gconf =
  { 0 };
__d_gconf gconf = NULL;

uint32_t g_hflags = F_DL_FOPEN_BUFFER;

char *cl_g_sub = cl_sub;

char *post_m_exec_str = NULL;

char *pp_msg_00[MAX_EXEC_STR + 1];

int
pce_proc(char *path, char *dir)
{
  gfl |= F_OPT_PS_LOGGING;
  pce_enable_logging();

  if (!path || !dir)
    {
      print_str("ERROR: insufficient arguments from ftpd\n");
      return 0;
    }
  gfl |= G_HFLAGS | F_OPT_PS_SILENT | F_OPT_VERBMAX;
  //char *subject_b = strdup(subject);
  int r;

  h_gconf.shmatflags |= SHM_RDONLY;

  if ((r = g_fopen(GCONFLOG, "r", F_DL_FOPEN_BUFFER, &h_gconf)))
    {
      print_str("ERROR: failed opening '%s', code %d\n", GCONFLOG, r);
      goto aft_end;
    }

  gconf = (__d_gconf) h_gconf.buffer.pos->ptr;

  if (NULL == gconf)
    {
      print_str("ERROR: %s: no global configuration was loaded\n", GCONFLOG);
      goto aft_end;
    }

  if (gconf->o_logging == 0)
    {
      gfl ^= F_OPT_PS_LOGGING;
      if (fd_log)
        {
          fclose(fd_log);
        }
      fd_log = NULL;
    }

  gfl ^= G_HFLAGS;

  char s_b[255], *s_b_p = s_b;

  //g_zerom_r(subject_b, 0x2F);

  if (!(s_b_p = reg_getsubm(path, gconf->r_sects, REG_EXTENDED, s_b, 255)))
    {
      print_str("ERROR: %s: reg_getsubm failed\n", path);
      goto aft_end;
    }

  if (!s_b_p[0])
    {
      print_str("ERROR: %s: unconfigured section ( %s )\n", path, dir);
      goto aft_end;
    }

  char r_sb[GCONF_MAX_REG_EXPR];

  snprintf(r_sb, GCONF_MAX_REG_EXPR, "%s%s", SITEROOT_N, gconf->r_sects);

  if (pce_do_regex_match(r_sb, path,
  REG_EXTENDED, 0))
    {
      print_str("ERROR: no path '%s' was defined (%s doesn't match input)\n",
          s_b_p, r_sb);
      goto aft_end;
    }

  s_b_p = g_zerom(s_b_p, 0x2F);
  //g_zerom_r(s_b_p, 0x2F);

  cl_dir = cl_dir_b = spec_p1 = dir;

  if (pce_g_skip_proc())
    {
      goto aft_end;
    }

  char s_lp[PATH_MAX];

  snprintf(s_lp, PATH_MAX, "%s/%s", pce_data_path, s_b_p);
  remove_repeating_chars(s_lp, 0x2F);

  if (file_exists(s_lp))
    {
      print_str("ERROR: %s - no section configuration exists at '%s'\n", s_b_p,
          s_lp);
      goto aft_end;
    }

  _g_handle h_sconf =
    { 0 };
  mda lh_ref =
    { 0 };

  h_sconf.shmatflags |= SHM_RDONLY;

  if (determine_datatype(&h_sconf, SCONFLOG))
    {
      print_str("ERROR: SCONF: determine_datatype failed\n");
      goto end;
    }

  if ((r = g_fopen(s_lp, "r", F_DL_FOPEN_BUFFER, &h_sconf)))
    {
      print_str("ERROR: failed opening '%s', code %d\n", s_lp, r);
      goto end;
    }

  if (gconf->o_use_shared_mem)
    {
      gfl |= F_OPT_SHAREDMEM;
    }

  gfl |= F_OPT_PROCREV;

  md_init(&lh_ref, 4);

  off_t nres;

  char b_sp2[4096];
  snprintf(b_sp2, 4096, "%s/%s", path, dir);
  spec_p2 = (char*) b_sp2;
  spec_p3 = path;

  if ((r = g_enum_log(pce_match_build, &h_sconf, &nres, &lh_ref)))
    {
      print_str("ERROR: failed processing records in '%s', code %d\n", s_lp, r);
      goto end;
    }

  if ((pce_f & F_PCE_FORKED) && gconf->o_exec_on_lookup_fail == 2
      && EXITVAL == 2 && gconf->e_match[0])
    {
      print_str("NOTICE: executing: '%s'\n", gconf->e_match);
      _g_handle t_h =
        { 0 };
      t_h.g_proc1_lookup = ref_to_val_lk_generic;

      if (pce_f & F_PCE_HAS_PP_MSG)
        {
          spec_p4 = (char*) pp_msg_00;
        }
      else
        {
          spec_p4 = "Denied";
        }
      pce_process_execv(&t_h, gconf->e_match, pce_prep_for_exec_r);
      g_cleanup(&t_h);
    }

  end:

  pce_lh_ref_clean(&lh_ref);

  aft_end:

  g_cleanup(&h_gconf);
  g_cleanup(&h_sconf);

  //free(subject_b);

  return EXITVAL;
}

int
pce_g_skip_proc(void)
{
  char *e_tmp;

  if (gconf->r_exclude_user[0] && (e_tmp = getenv("USER")))
    {
      if (!pce_do_regex_match(gconf->r_exclude_user, e_tmp,
      REG_EXTENDED, 0))
        {
          print_str("NOTICE: ignoring user '%s'\n", e_tmp);
          return 1;
        }
    }

  if (gconf->r_exclude_user_flags[0] && (e_tmp = getenv("FLAGS")))
    {
      if (!pce_do_regex_match(gconf->r_exclude_user_flags, e_tmp,
      REG_EXTENDED, 0))
        {
          print_str("NOTICE: ignoring user '%s'\n", e_tmp);
          return 1;
        }
    }

  if (gconf->r_skip_basedir[0]
      && !pce_do_regex_match(gconf->r_skip_basedir, cl_dir_b,
      REG_EXTENDED | REG_ICASE, 0))
    {
      print_str("NOTICE: skipping directory '%s'\n", cl_dir);
      return 1;
    }
  return 0;
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

  if (pce_lm && pce_lm != ptr->i32)
    {
      return 0;
    }

  char *log_s;

  if (ptr->i32)
    {
      _g_dgetr dgetr =
        { 0};

      log_s = pce_dgetlf(&dgetr, (int)ptr->i32);

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
          p_log->shmatflags = SHM_RDONLY;
          p_log->shmcflags = S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP
          | S_IWGRP;

          if ((r=g_fopen(log_s, "r", F_DL_FOPEN_BUFFER, p_log)))
            {
              print_str("ERROR: failed opening '%s', code %d\n", log_s, r);
              p_log->flags |= F_GH_LOCKED;
              return 0;
            }

          if ((r=pce_do_lookup(p_log, &dgetr, ptr, log_s) ) != 1)
            {
              if (gfl & F_OPT_KILL_GLOBAL)
                {
                  return -1;
                }
              return r;
            }
        }
      else
        {
          if ((p_log->flags & F_GH_LOCKED))
            {
              print_str("NOTICE: '%s': this log has been locked, ignoring rule\n", p_log->file );
              return 0;
            }
        }

      switch(ptr->type)
        {
          case 1:
          pce_process_string_match(p_log, ptr);
          break;
          case 2:
          pce_process_lom_match(p_log, ptr);
          break;
          case 3:

          if ((r = pce_process_execv(p_log, ptr->match, pce_prep_for_exec)))
            {
              print_str(
                  "WARNING: [%d] rule chain hit positive external match (%s), blocking..\n",
                  r, ptr->match);
              EXITVAL = r;
              if (ptr->message[0])
                {
                  pce_print_msg(ptr->message, p_log);
                }
            }
          else
            {
              pce_pcl_stat(r, ptr);
            }
          break;
          default:
          print_str("ERROR: '%s': (%s / %s): lookup requested but no match type given\n", p_log->file, ptr->field, ptr->match);
          return 0;
          break;
        }

      if (EXITVAL)
        {
          return -1;
        }

    }
  else
    {
      int r;
      switch(ptr->type)
        {
          case 1:;

          int i_m = 0;
          if (ptr->invert == 1)
            {
              i_m = REG_NOMATCH;
            }

          int cflags = REG_EXTENDED;

          if (ptr->icase)
            {
              cflags |= REG_ICASE;
            }

          if (!(r=pce_do_regex_match(ptr->match, cl_dir, cflags, i_m)))
            {
              print_str("WARNING: '%s': rule chain hit positive match (%s), blocking..\n", cl_dir, ptr->match);
              EXITVAL = 2;
              if (ptr->message[0])
                {
                  _g_handle t_h =
                    { 0};
                  t_h.g_proc1_lookup = ref_to_val_lk_generic;
                  pce_print_msg(ptr->message, &t_h);
                  g_cleanup(&t_h);
                }
            }
          else
            {
              pce_pcl_stat(r, ptr);
            }
          break;
          case 3:;
          _g_handle t_h =
            { 0};
          t_h.g_proc1_lookup = ref_to_val_lk_generic;
          if ((r = pce_process_execv(&t_h, ptr->match, pce_prep_for_exec)))
            {
              print_str(
                  "WARNING: [%d] rule chain hit positive external match (%s), blocking..\n",
                  r, ptr->match);
              EXITVAL = r;
              if (ptr->message[0])
                {
                  pce_print_msg(ptr->message, &t_h);
                }
            }
          else
            {
              pce_pcl_stat(r, ptr);
            }
          g_cleanup(&t_h);
          break;
        }

      if (EXITVAL)
        {
          return -1;
        }
    }

  return 0;
}

int
pce_print_msg(char *input, __g_handle hdl)
{
  int r;
  md_g_free(&hdl->print_mech);
  if ((r = g_compile_exech(&hdl->print_mech, hdl, input)))
    {
      print_str("ERROR: %s: [%d]: could not compile print string\n", hdl->file,
          r);
      return 1;
    }

  char *s_ptr;
  if (!(s_ptr = g_exech_build_string(
      hdl->buffer.pos ? hdl->buffer.pos->ptr : NULL,
      &((__g_handle ) hdl)->print_mech, (__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      print_str("ERROR: could not assemble print string\n");
      return 1;
    }

  if (!(pce_f & F_PCE_FORKED))
    {
      fwrite(b_glob, strlen(b_glob), 1, stdout);
    }
  else
    {
      snprintf((char *) pp_msg_00, MAX_EXEC_STR, "%s", b_glob);
      pce_f |= F_PCE_HAS_PP_MSG;
    }

  return 0;
}

int
pce_dt(char *field, void *out)
{
  errno = 0;
  if (field[0] == 0x2D || field[0] == 0x2B)
    {
      *((int64_t*) out) = (int64_t) strtoll(field, NULL, 10);
    }
  else if (s_char(field, 0x2E, g_floatstrlen(field)))
    {
      *((float*) out) = (float) strtof(field, NULL);
    }
  else
    {
      *((uint64_t*) out) = (uint64_t) strtoull(field, NULL, 10);
    }
  if (errno == ERANGE)
    {
      return 1;
    }
  return 0;
}

void *
pce_rescomp(int m_i)
{
  switch (m_i)
    {
  case 1:
    return _lcs_isequal;
  case 2:
    return _lcs_ishigher;
  case 3:
    return _lcs_islower;
  case 4:
    return _lcs_islowerorequal;
  case 5:
    return _lcs_ishigherorequal;
  case 6:
    return _lcs_isnot;
    }
  return NULL;
}

int
pce_process_execv(__g_handle hdl, char *exec_str, __d_avoid_i ppfe)
{
  _execv exec_args =
    { 0 };
  int r;

  if ((r = g_init_execv_bare(&exec_args, hdl, exec_str)))
    {
      print_str("ERROR: [%d]: g_init_execv_bare failed: '%s'\n", r, exec_str);
      return 0;
    }

  if ((r = process_execv_args_bare(
      hdl->buffer.pos ? hdl->buffer.pos->ptr : NULL, hdl, &exec_args)))
    {
      print_str("ERROR: [%d]: process_execv_args failed: '%s'\n", r, exec_str);
      return 0;
    }

  return WEXITSTATUS(pce_l_execv(exec_args.exec_v_path, exec_args.argv_c, ppfe));
}

int
pce_process_exec(__g_handle hdl, char *exec_str)
{
  _d_exec_ch exech =
    { 0 };

  process_exec_string_n(exec_str, b_glob, MAX_EXEC_STR, hdl->g_proc1_lookup,
      hdl->buffer.pos ? hdl->buffer.pos->ptr : NULL, &exech.dtr);

  if (!exec_and_redirect_output(b_glob, stdout))
    {
      print_str("ERROR: pce_process_exec failed: '%s'\n", exec_str);
      return 0;
    }

  return 0;
}

static int
preproc_lom_rc(void *data, __d_sconf ptr)
{
  switch (ptr->i32)
    {
  case 1:
    ;
    if (gconf->o_imdb_skip_zero_score > 0 && !strncmp(ptr->field, "score", 5))
      {
        __d_imdb d_imdb = (__d_imdb) data;
        if (d_imdb->rating == (float)0)
          {
            print_str("WARNING: imdb score is 0, ignoring rule\n", ptr->field);
            return 1;
          }
      }
    break;
  }

return 0;
}

int
pce_process_lom_match(__g_handle hdl, __d_sconf ptr)
{
  int i_m = 1, r;

  if (preproc_lom_rc(hdl->buffer.pos->ptr, ptr))
    {
      return 0;
    }

  if (ptr->invert == 1)
    {
      i_m = 0;
    }

  _g_lom lom =
    { 0 };

  uint8_t match[8] =
    { 0 };

  if (pce_dt(ptr->match, &match))
    {
      print_str("ERROR: convert value %s to an integer type\n", ptr->match);
      return 1;
    }

  void *_lcs = pce_rescomp(ptr->lcomp);

  if (!_lcs)
    {
      print_str(
          "ERROR: LOM: %s = %s: unable to determine comparison match type (lcomp : %d)\n",
          ptr->field, ptr->match, ptr->lcomp);
      return 1;
    }

  if ((r = g_build_lom_packet_bare(hdl, &lom, ptr->field, &match, _lcs,
      g_oper_and)))
    {
      print_str("ERROR: LOM: %s = %s: unable to commit lom match : %d\n",
          ptr->field, ptr->match, r);
      return 1;
    }

  lom.g_lom_vp(hdl->buffer.pos->ptr, (void*) &lom);

  if (lom.result == i_m)
    {
      print_str(
          "WARNING: rule chain hit positive LOM match (%s [%d] %s), blocking..\n",
          ptr->field, ptr->lcomp, ptr->match);
      EXITVAL = 2;
      if (ptr->message[0])
        {
          pce_print_msg(ptr->message, hdl);
        }
      return -1;
    }
  else
    {
      pce_pcl_stat(lom.result, ptr);
    }

  return 0;
}

int
pce_process_string_match(__g_handle hdl, __d_sconf ptr)
{
  int i_m = 0, r;

  if (ptr->invert == 1)
    {
      i_m = REG_NOMATCH;
    }

  if (!(r = pce_match_log(hdl, ptr, i_m)))
    {
      print_str(
          "WARNING: '%s': rule chain hit positive REGEX match (pattern '%s' matches '%s' (%d)), blocking..\n",
          hdl->file, ptr->match, cl_g_sub, i_m);
      EXITVAL = 2;
      if (ptr->message[0])
        {
          pce_print_msg(ptr->message, hdl);
        }
      return -1;
    }
  else
    {
      pce_pcl_stat(r, ptr);
    }
  return 0;
}

void
pce_pcl_stat(int r, __d_sconf ptr)
{
  if (r < 0)
    {
      print_str("ERROR: %d processing chain link [%d][%d] (%s)\n", r, ptr->type,
          ptr->i32, ptr->match);
    }
  else
    {
      print_str("NOTICE: chain link passed [%d][%d] (%s)\n", ptr->type,
          ptr->i32, ptr->match);
    }
}

int
pce_do_lookup(__g_handle p_log, __d_dgetr dgetr, __d_sconf sconf, char *lp)
{
  int r;

  if (!(pce_f & F_PCE_DONE_STR_PREPROC))
    {
      if (!pce_do_str_preproc(cl_dir_b, dgetr))
        {
          print_str(
              "ERROR: unable to preprocess the directory string, aborting\n");
          return -1;
        }
      //printf("%s | %s\n", cl_g_sub, s_year);
      pce_f |= F_PCE_DONE_STR_PREPROC;
    }

  off_t nres;
  char did_retry = 0;

  retry: did_retry++;

  if (s_year && dgetr->d_yf)
    {
      uint64_t year = (uint64_t) strtoul(s_year, NULL, 10);
      md_init(&p_log->_match_rr, 2);
      __g_match tt_m = md_alloc(&p_log->_match_rr, sizeof(_g_match));
      md_init(&tt_m->lom, 2);
      __g_lom lom = (__g_lom ) md_alloc(&tt_m->lom, sizeof(_g_lom));
      tt_m->g_oper_ptr = g_oper_and;
      tt_m->flags |= F_GM_NAND;
      tt_m->flags |= F_GM_ISLOM;
      tt_m->match_i_m = G_MATCH;

      if ((r = g_build_lom_packet_bare(p_log, lom, dgetr->d_yf, &year,
          _lcs_isequal, g_oper_and)))
        {
          print_str("ERROR: unable to commit LOM match : %d\n", r);
          p_log->flags |= F_GH_LOCKED;
          return 0;
        }

    }

  if ((r = g_commit_strm_regex(p_log, dgetr->d_field, cl_g_sub, 0,
  REG_EXTENDED | REG_ICASE, F_GM_ISREGEX)))
    {
      print_str("ERROR: unable to commit regex match : %d\n", r);
      p_log->flags |= F_GH_LOCKED;
      return 0;
    }

  if ((r = g_enum_log(pce_run_log_match, p_log, &nres, NULL)))
    {
      print_str(
          "ERROR: could not find anything matching pattern '%s (%s)' in '%s', code %d, %llu\n",
          cl_g_sub, s_year ? s_year : "no year", p_log->file, r,
          (ulint64_t) nres);

      if (gconf->o_exec_on_lookup_fail && dgetr->e_lookup_fail[0]
          && did_retry == 1)
        {
          if (gconf->o_exec_on_lookup_fail == 2)
            {
              if (fork())
                {
                  print_str(
                      "NOTICE: o_exec_on_lookup_fail == 2, forked process\n");
                  //gfl |= F_OPT_KILL_GLOBAL;
                  p_log->flags |= F_GH_LOCKED;
                  return 0;
                }
              else
                {
                  setsid();
                  int pid2 = fork();
                  if (pid2 < 0)
                    {
                      print_str("ERROR: can't fork after releasing\n");
                      exit(1);
                    }
                  else if (pid2 > 0)
                    exit(0);
                  else
                    {
                      close(0);
                      close(1);
                      close(2);
                      umask(0);
                      chdir("/");
                    }

                  if (fd_log)
                    {
                      fclose(fd_log);
                    }

                  fd_log = NULL;

                  pce_enable_logging();

                  pce_f |= F_PCE_FORKED;
                  pce_lm = sconf->i32;
                }
            }

          _g_handle t_h =
            { 0 };

          t_h.g_proc1_lookup = ref_to_val_lk_generic;
          t_h.buffer.pos = p_log->buffer.objects;

          print_str("NOTICE: executing command on failed lookup: '%s'\n",
              dgetr->e_lookup_fail);

          if ((r = pce_process_execv(&t_h, dgetr->e_lookup_fail,
              pce_prep_for_exec_r)))
            {
              print_str(
                  "WARNING: %s: lookup retry call returned bad exit code: %d\n",
                  p_log->file, r);
            }

          print_str("NOTICE: %s: retrying lookup\n", p_log->file);

          g_cleanup(p_log);
          /*p_log->shmcflags = S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP
           | S_IWGRP;
           p_log->shmatflags = SHM_RDONLY;*/
          p_log->flags |= F_GH_HASMATCHES;
          uint64_t s_gfl = gfl;
          if (gfl & F_OPT_SHAREDMEM)
            {
              gfl ^= F_OPT_SHAREDMEM;
            }
          if ((r = g_fopen(lp, "r", F_DL_FOPEN_BUFFER, p_log)))
            {
              print_str("ERROR: failed re-opening '%s', code %d\n", lp, r);
              p_log->flags |= F_GH_LOCKED;
              return 0;
            }
          gfl = s_gfl;
          //nres = 0;

          goto retry;

          g_close(&t_h);
        }

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
  int cflags = REG_EXTENDED;

  if (sconf->icase)
    {
      cflags |= REG_ICASE;
    }

  return pce_do_regex_match(sconf->match, r_v, cflags, m_i_m);
}

int
pce_do_regex_match(char *pattern, char *match, int cflags, int m_i_m)
{
  regex_t preg;
  int r;

  if ((r = regcomp(&preg, pattern, cflags | REG_NOSUB)))
    {
      print_str("ERROR: could not compile pattern '%s'\n", pattern);
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

  if (g_bmatch(_ptr, hdl,&hdl->buffer ) == 0)
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
    dgetr->e_lookup_fail = gconf->e_lookup_fail_imdb;
    dgetr->o_lookup_strictness = gconf->o_lookup_strictness_imdb;
    return IMDBLOG;
    break;
  case 2:
    dgetr->pf |= F_GH_ISTVRAGE;
    dgetr->d_field = "name";
    dgetr->d_yf = "startyear";
    dgetr->e_lookup_fail = gconf->e_lookup_fail_tvrage;
    dgetr->o_lookup_strictness = gconf->o_lookup_strictness_tvrage;
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
pce_do_str_preproc(char *subject, __d_dgetr dgetr)
{

  if (gconf->r_clean[0])
    {
      if (!(cl_g_sub = reg_sub_d(subject, gconf->r_clean,
      REG_EXTENDED | REG_ICASE, cl_presub)))
        {
          print_str("ERROR: could not preprocess string (r_clean)\n");
          return NULL;
        }
    }
  else
    {
      cl_g_sub = subject;
    }

  if (gconf->r_postproc[0])
    {
      if (!(cl_g_sub = reg_sub_g(cl_g_sub, gconf->r_postproc,
      REG_EXTENDED | REG_ICASE, cl_sub, sizeof(cl_sub), ".*")))
        {
          print_str("ERROR: could not preprocess string (r_postproc)\n");
          return NULL;
        }
    }

  if (cl_g_sub == subject)
    {
      size_t s_l = strlen(subject);
      strncpy(cl_sub, subject,
          s_l > sizeof(cl_sub) - 1 ? sizeof(cl_sub) - 1 : s_l);
    }
  else if (cl_g_sub == cl_presub)
    {
      size_t s_l = strlen(cl_presub);
      strncpy(cl_sub, cl_presub,
          s_l > sizeof(cl_sub) - 1 ? sizeof(cl_sub) - 1 : s_l);
    }

  size_t cl_l = strlen(cl_sub);

  if (dgetr->o_lookup_strictness < 2)
    {

      if (cl_l >= sizeof(cl_sub) - 1)
        {
          print_str("ERROR: could not preprocess string (prepend ^)\n");
          return NULL;
        }

      memmove(&cl_sub[1], cl_sub, cl_l);
      cl_sub[0] = 0x5E;
      cl_l++;
    }

  if (dgetr->o_lookup_strictness < 1)
    {
      if (cl_l < sizeof(cl_sub) - 1)
        {
          strncpy(&cl_sub[cl_l], "(()|.)$", 7);
        }
    }

  cl_g_sub = cl_sub;

  s_year = pce_get_year_result(subject, cl_yr, sizeof(cl_yr));

  return cl_g_sub;
}

int
pce_pfe(void)
{
  const char inputfile[] = "/dev/null";

  if (close(0) < 0)
    {
      fprintf(stdout, "ERROR: could not close stdin\n");
      return 1;
    }
  else
    {
      if (open(inputfile, O_RDONLY
#if defined O_LARGEFILE
          | O_LARGEFILE
#endif
          ) < 0)
        {
          fprintf(stdout, "ERROR: could not open %s\n", inputfile);
        }
    }
  return 0;
}

void
pce_pfe_r(void)
{
  if (fd_log)
    {
      fclose(fd_log);
      if (NULL == (fd_log = fopen(pce_logfile, "a")))
        {
          print_str("ERROR: count not re-open log after fork: %s\n",
          pce_logfile);
        }
      dup2(fileno(fd_log), STDOUT_FILENO);
      dup2(fileno(fd_log), STDERR_FILENO);
    }
  else
    {
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
    }
}

int
pce_prep_for_exec(void)
{
  if (pce_pfe())
    {
      return 1;
    }
  return 0;
}

int
pce_prep_for_exec_r(void)
{
  if (pce_pfe())
    {
      return 1;
    }
  pce_pfe_r();
  return 0;
}

int
pce_l_execv(char *exec, char **argv, __d_avoid_i ppfe)
{
  pid_t c_pid;

  fflush(stdout);
  fflush(stderr);

  c_pid = fork();

  if (c_pid == (pid_t) -1)
    {
      print_str("ERROR: %s: fork failed\n", exec);
      return 0;
    }

  if (!c_pid)
    {
      if (ppfe())
        {
          _exit(0);
        }
      else
        {
          execv(exec, argv);
          fprintf(stderr, "ERROR: %s: execv failed to execute [%s]\n", exec,
              strerror(errno));
          _exit(0);
        }
    }
  int status;
  while (waitpid(c_pid, &status, 0) == (pid_t) -1)
    {
      if (errno != EINTR)
        {
          print_str(
              "ERROR: %s: failed waiting for child process to finish [%s]\n",
              exec, strerror(errno));
          return 0;
        }
    }

  return status;
}
