/*
 * m_general.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include "m_general.h"
#include <glutil.h>
#include <m_lom.h>
#include <m_string.h>
#include <l_error.h>
#include <log_op.h>
#include <lc_oper.h>
#include <arg_proc.h>
#include <arg_opts.h>

#include <fnmatch.h>
#ifdef _G_SSYS_THREAD
#include <pthread.h>
#include <thread.h>
#endif

void
g_ipcbm(void *phdl, pmda md, int *r_p, void *ptr)
{
  __g_handle hdl = (__g_handle) phdl;

  if (*r_p)
    {
      if (hdl->flags & F_GH_IFRES)
        {
          hdl->flags ^= F_GH_IFRES;
          *r_p = 0;
        }
      md->rescnt++;
      /*if (!(hdl->flags & F_GH_NO_ACCU))
       {
       g_lom_accu(hdl, ptr, &hdl->_accumulator);
       }*/
    }
  else
    {
      if (hdl->flags & F_GH_IFHIT)
        {
          hdl->flags ^= F_GH_IFHIT;
          *r_p = 1;
        }
      md->hitcnt++;
    }
}

__g_match
g_global_register_match(void)
{
  md_init(_match_clvl, 32);

  if (_match_clvl->offset >= GM_MAX)
    {
      return NULL;
    }

  return (__g_match ) md_alloc(_match_clvl, sizeof(_g_match));

}

int
g_filter(__g_handle hdl, pmda md)
{
  g_setjmp(0, "g_filter", NULL, NULL);
  if (!((hdl->exec_args.exc || (hdl->flags & F_GH_ISMP))) || !md->count)
    {
      return 0;
    }

  if (g_proc_mr(hdl))
    {
      return -1;
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: %s: passing %llu records through filters..\n",
          hdl->file, (uint64_t) hdl->buffer.offset);
    }

  off_t s_offset = md->offset;

  p_md_obj ptr = NULL, o_ptr;

  if (hdl->j_offset == 2)
    {
      ptr = md_last(md);
    }
  else
    {
      ptr = md_first(md);
      hdl->j_offset = 1;
    }

  int r = 0;

  while (ptr)
    {
      if (ofl & F_BM_TERM)
        {
          if (gfl & F_OPT_KILL_GLOBAL)
            {
              gfl ^= F_OPT_KILL_GLOBAL;
            }
          break;
        }
      if (g_bmatch(ptr->ptr, hdl, md))
        {
          o_ptr = ptr;
          ptr = (p_md_obj) *((void**) ptr + hdl->j_offset);
          if (!(md_unlink(md, o_ptr)))
            {
              r = 2;
              break;
            }
          continue;
        }
      ptr = (p_md_obj) *((void**) ptr + hdl->j_offset);
    }

  if (gfl & F_OPT_VERBOSE3)
    {
      print_str("NOTICE: %s: filtered %llu records..\n", hdl->file,
          (uint64_t) (s_offset - md->offset));
    }

  if (s_offset != md->offset)
    {
      hdl->flags |= F_GH_APFILT;
    }

  if (!md->offset)
    {
      return 1;
    }

  return r;
}

static p_md_obj
bm_skip_irrelevant(p_md_obj ptr, uint32_t flags)
{
  p_md_obj l_ptr = ptr;
  __g_match _gm;

  while (ptr)
    {
      _gm = (__g_match) ptr->ptr;
      if (!(_gm->flags & flags) || (_gm->flags & F_GM_TFD))
        {
          return l_ptr;
        }

      l_ptr = ptr;
      ptr = ptr->next;
    }

  return l_ptr;
}

static int
g_bm_proc(void *d_ptr, __g_handle hdl, pmda match_rr)
{

  p_md_obj ptr = md_first(match_rr);
  int r, r_p = 1;
  __g_match _gm, _p_gm = NULL;

  while (ptr)
    {
      //r = 0;
      _gm = (__g_match) ptr->ptr;

      if ((_gm->flags & F_GM_IS_MOBJ))
        {
          r = !(g_bm_proc(d_ptr, hdl, _gm->next) == _gm->match_i_m);
        }
      else if ((_gm->flags & F_GM_ISLOM))
        {
          r = (g_lom_match(hdl, d_ptr, _gm) == _gm->match_i_m);
        }
      else if (_gm->flags & F_GM_TYPES_STR)
        {
          r = do_string_match(hdl, d_ptr, _gm);
        }
      else
        {
          goto l_end;
        }

      if (_p_gm && _p_gm->g_oper_ptr)
        {
          r_p = _p_gm->g_oper_ptr(r_p, r);
        }
      else
        {
          r_p = r;
        }

      if ((_gm->flags & F_GM_TFD) && 0 == r)
        {
          hdl->flags |= F_GH_TFD_PROCED;
        }

      if (r_p == 1 && (_gm->flags & F_GM_NOR))
        {
          if ( NULL == ptr->next)
            {
              break;
            }
          ptr = bm_skip_irrelevant(ptr, F_GM_NOR);
          _gm = (__g_match) ptr->ptr;
        }
      else if (r_p == 0 && (_gm->flags & F_GM_NAND))
        {
          /*ptr = ptr->next;
           if (ptr)
           {
           _p_gm = (__g_match) ptr->ptr;
           if (_p_gm->flags & F_GM_TFD)
           {
           _p_gm = _gm;
           continue;
           }
           ptr = ptr->next;
           }
           continue;*/
          if ( NULL == ptr->next)
            {
              break;
            }
          ptr = bm_skip_irrelevant(ptr, F_GM_NAND);
          _gm = (__g_match) ptr->ptr;
        }

      l_end:;

      _p_gm = _gm;
      ptr = ptr->next;
    }

  return r_p;
}

int
g_bmatch_dummy(void *d_ptr, __g_handle hdl, pmda md)
{
  return 0;
}

int
g_bmatch(void *d_ptr, __g_handle hdl, pmda md)
{
  if (NULL != md)
    {
      if (hdl->max_results && md->rescnt >= hdl->max_results)
        {
#ifdef _MAKE_SBIN
#ifdef _G_SSYS_THREAD
          mutex_lock(&mutex_glob00);
#endif
          ofl |= F_BM_TERM;
          gfl |= F_OPT_KILL_GLOBAL;
#ifdef _G_SSYS_THREAD
          pthread_mutex_unlock(&mutex_glob00);
#endif
#endif
          if (!(hdl->flags & F_GH_SPEC_SQ01))
            {
              hdl->flags |= F_GH_SPEC_SQ01;
            }
          return 1;
        }
      else if (hdl->max_hits && md->hitcnt >= hdl->max_hits)
        {
          return 0;
        }
    }

  int r_p = g_bm_proc(d_ptr, hdl, &hdl->_match_rr);

  if (NULL != hdl->ifrh_l0)
    {
      hdl->ifrh_l0((void*) hdl, md, &r_p, d_ptr);
    }

  if (r_p)
    {
      if (hdl->exec_args.exc)
        {
          int r_e = hdl->exec_args.exc(d_ptr, (void*) NULL, NULL, (void*) hdl);
          int r_stat = WEXITSTATUS(r_e);
          if (0 != r_stat)
            {
              r_p = 0;
#ifdef _G_SSYS_THREAD
              mutex_lock(&mutex_glob00);
#endif
              EXITVAL = r_stat;
#ifdef _G_SSYS_THREAD
              pthread_mutex_unlock(&mutex_glob00);
#endif
            }
        }
    }

  if ( NULL != hdl->ifrh_l1)
    {
      hdl->ifrh_l1((void*) hdl, md, &r_p, d_ptr);
    }

  if (((gfl & F_OPT_MATCHQ) && 0 == r_p) || ((gfl & F_OPT_IMATCHQ) && r_p))
    {
#ifdef _G_SSYS_THREAD
      mutex_lock(&mutex_glob00);
#endif
      ofl |= F_BM_TERM;
      gfl |= F_OPT_KILL_GLOBAL;
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock(&mutex_glob00);
#endif
    }

  return !(r_p);
}

int
do_string_match(__g_handle hdl, void *d_ptr, __g_match _gm)
{
  char *mstr;

  mstr = (char*) _gm->pmstr_cb(d_ptr, _gm->field, hdl->mv1_b, MAX_VAR_LEN,
      &_gm->dtr);

  if ( NULL == mstr)
    {
      return 0;
    }

  int r = 0;

  int rr;

  if ((_gm->flags & F_GM_ISREGEX)
      && (rr = regexec(&_gm->preg, mstr, 0, NULL, 0)) == _gm->match_i_m)
    {
      r = 1;
    }
  else if ((_gm->flags & F_GM_ISFNAME)
      && (rr = fnmatch(_gm->match, mstr, _gm->fname_flags)) == _gm->match_i_m)
    {
      r = 1;
    }
  else if ((_gm->flags & F_GM_ISMATCH))
    {
      size_t mstr_l = strlen(mstr);

      int irl = strlen(_gm->match) != mstr_l, ir = strncmp(mstr, _gm->match,
          mstr_l);

      if ((_gm->match_i_m && (ir || irl)) || (!_gm->match_i_m && (!ir && !irl)))
        {
          r = 1;
        }
      goto end;
    }

  end:

  return r;
}

int
opt_g_operator_or(void *arg, int m, void *opt)
{
  __g_match pgm = (__g_match) _match_rr_l.ptr;
  if (!pgm)
    {
      return 7100;
    }

  if ( pgm->flags & F_GM_NAND)
    {
      pgm->flags ^= F_GM_NAND;
    }

  pgm->g_oper_ptr = g_oper_or;
  pgm->flags |= F_GM_NOR;
  return 0;
}

int
opt_g_operator_and(void *arg, int m, void *opt)
{
  __g_match pgm = (__g_match) _match_rr_l.ptr;
  if (!pgm)
    {
      return 6100;
    }

  if ( pgm->flags & F_GM_NOR)
    {
      pgm->flags ^= F_GM_NOR;
    }

  pgm->g_oper_ptr = g_oper_and;
  pgm->flags |= F_GM_NAND;
  return 0;
}

int
opt_g_m_raise_level(void *arg, int m, void *opt)
{
  __g_match pgm = g_global_register_match();

  pgm->flags |= F_GM_IS_MOBJ;
  pgm->match_i_m = default_determine_negated();

  pgm->g_oper_ptr = g_oper_and;
  pgm->flags |= F_GM_NAND;

  _match_rr_l.ptr = (void *) pgm;

  if ( NULL != ar_find(&ar_vref, AR_VRP_OPT_TARGET_FD))
    {
      pgm->flags |= F_GM_TFD;
    }

  ar_remove(&ar_vref, AR_VRP_OPT_TARGET_FD);
  ar_remove(&ar_vref, AR_VRP_OPT_NEGATE_MATCH);

  pgm->next = calloc(1, sizeof(mda));
  md_init((pmda) pgm->next, 16);

  ((pmda) pgm->next)->lref_ptr = _match_clvl;
  _match_clvl = pgm->next;

  return 0;
}

int
opt_g_m_lower_level(void *arg, int m, void *opt)
{

  if (NULL == _match_clvl->lref_ptr)
    {
      return 99150;
    }

  _match_clvl = (void*) _match_clvl->lref_ptr;

  if (NULL == _match_clvl->pos)
    {
      return 99151;
    }

  __g_match pgm = _match_clvl->pos->ptr;

  if (NULL == pgm)
    {
      return 99152;
    }

  _match_rr_l.ptr = (void *) pgm;

  return 0;
}
