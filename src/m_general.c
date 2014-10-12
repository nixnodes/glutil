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

void
g_ipcbm(void *phdl, pmda md, int *r_p, void *ptr)
{
  __g_handle hdl = (__g_handle) phdl;

  if (*r_p)
    {
      if (hdl->flags & F_GH_IFRES)
        {
          hdl->flags ^= F_GH_IFRES;
          *r_p = 1;
        }
      md->rescnt++;
      if (!(hdl->flags & F_GH_NO_ACCU))
        {
          g_lom_accu(hdl, ptr, &hdl->_accumulator);
        }
    }
  else
    {
      if (hdl->flags & F_GH_IFHIT)
        {
          hdl->flags ^= F_GH_IFHIT;
          *r_p = 0;
        }
      md->hitcnt++;
    }
}

__g_match
g_global_register_match(void)
{
  md_init(&_match_rr, 32);

  if (_match_rr.offset >= GM_MAX)
    {
      return NULL;
    }

  return (__g_match ) md_alloc(&_match_rr, sizeof(_g_match));

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

int
g_bmatch(void *d_ptr, __g_handle hdl, pmda md)
{
  if (md)
    {
      if (hdl->max_results && md->rescnt >= hdl->max_results)
        {
#ifdef _MAKE_SBIN
          ofl |= F_BM_TERM;
          gfl |= F_OPT_KILL_GLOBAL;
#endif
          return 1;
        }
      if (hdl->max_hits && md->hitcnt >= hdl->max_hits)
        {
          return 0;
        }
    }

  p_md_obj ptr = md_first(&hdl->_match_rr);
  int r, r_p = 1;
  __g_match _gm, _p_gm = NULL;

  while (ptr)
    {
      r = 0;
      _gm = (__g_match) ptr->ptr;

      if ((_gm->flags & F_GM_ISLOM))
        {
          if ((g_lom_match(hdl, d_ptr, _gm)) == _gm->match_i_m)
            {
              r = 1;
              goto l_end;
            }
        }

      r = do_match(hdl, d_ptr, _gm);

      l_end:

//printf("-:: %d\n", r);

      if (_p_gm && _p_gm->g_oper_ptr)
        {
          //           printf("::> %d/%d, %d\n", r, r_p, _p_gm->g_oper_ptr == g_oper_and);
          r_p = _p_gm->g_oper_ptr(r_p, r);
//          printf("::< %d\n",  r_p);
        }
      else
        {
          r_p = r;
        }

      if ((_gm->flags & F_GM_TFD) && 0 == r)
        {
          hdl->flags |= F_GH_TFD_PROCED;
        }

      _p_gm = _gm;
      ptr = ptr->next;
    }

  //printf("!!:: %d\n\n", r_p);
  if (hdl->ifrh_l0)
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
            }
        }
    }

  if (hdl->ifrh_l1)
    {
      hdl->ifrh_l1((void*) hdl, md, &r_p, d_ptr);
    }

  if (((gfl & F_OPT_MATCHQ) && 0 == r_p) || ((gfl & F_OPT_IMATCHQ) && r_p))
    {
      ofl |= F_BM_TERM;
      gfl |= F_OPT_KILL_GLOBAL;
    }

  return !(r_p);
}

int
do_match(__g_handle hdl, void *d_ptr, __g_match _gm)
{
  char *mstr;

  if (_gm->pmstr_cb)
    {
      mstr = _gm->pmstr_cb(d_ptr, _gm->field, hdl->mv1_b, MAX_VAR_LEN,
          &_gm->dtr);
    }
  else
    {
      mstr = (char*) (d_ptr + hdl->jm_offset);
    }

  /*if (!mstr)
   {
   print_str("ERROR: could not get match string\n");
   gfl |= F_OPT_KILL_GLOBAL;
   ofl |= F_BM_TERM;
   return 0;
   }*/

  int r = 0;
  if ((_gm->flags & F_GM_ISMATCH))
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
  int rr;

  if ((_gm->flags & F_GM_ISREGEX)
      && (rr = regexec(&_gm->preg, mstr, 0, NULL, 0)) == _gm->reg_i_m)
    {
      r = 1;
    }

  end:

  return r;
}

int
opt_g_operator_or(void *arg, int m)
{
  __g_match pgm = (__g_match) _match_rr_l.ptr;
  if (!pgm)
    {
      return 7100;
    }
  switch (_match_rr_l.flags & F_LM_TYPES)
    {
      case F_LM_CPRG:
      ;
      pgm->g_oper_ptr = g_oper_or;
      break;
      case F_LM_LOM:;
      pgm->g_oper_ptr = g_oper_or;
      break;
      default:
      return 7110;
      break;
    }
  pgm->flags |= F_GM_NOR;
  return 0;
}

int
opt_g_operator_and(void *arg, int m)
{
  __g_match pgm = (__g_match) _match_rr_l.ptr;
  if (!pgm)
    {
      return 6100;
    }
  switch (_match_rr_l.flags & F_LM_TYPES)
    {
      case F_LM_CPRG:
      ;
      pgm->g_oper_ptr = g_oper_and;
      break;
      case F_LM_LOM:;
      pgm->g_oper_ptr = g_oper_and;
      break;
      default:
      return 6110;
      break;
    }
  pgm->flags |= F_GM_NAND;
  return 0;
}
