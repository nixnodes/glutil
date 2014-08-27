/*
 * n_lom.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "m_lom.h"

#include <l_error.h>
#include <str.h>
#include <lc_oper.h>
#include <gv_off.h>
#include <i_math.h>
#include <lref_glob.h>

#include <stdio.h>
#include <errno.h>

#include <arg_proc.h>
#include <mc_glob.h>

static void
set_lom_vp(__g_lom lom, void *target)
{
  lom->g_lom_vp = target;
}

int
g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm)
{
  p_md_obj ptr = _gm->lom.objects;
  __g_lom lom, p_lom = NULL;

  int r_p = 1, i = 0;

  while (ptr)
    {
      lom = (__g_lom) ptr->ptr;
      //lom->result = 0;
      i++;
      lom->g_lom_vp(d_ptr, (void*)lom);

      if (!ptr->next && !p_lom && lom->result)
        {
          return !lom->result;
        }

      if ( p_lom && p_lom->g_oper_ptr )
        {
          if (!(r_p = p_lom->g_oper_ptr(r_p, lom->result)))
            {
              if ( !ptr->next)
                {
                  return 1;
                }
            }
          else
            {
              if (!ptr->next)
                {
                  return !r_p;
                }
            }
        }
      else
        {
          r_p = lom->result;
        }

      p_lom = lom;
      ptr = ptr->next;
    }

  return 1;
}

int
g_lom_accu(__g_handle hdl, void *d_ptr, pmda _accumulator)
{
  p_md_obj ptr = _accumulator->objects;
  __g_lom lom;

  while (ptr)
    {
      lom = (__g_lom) ptr->ptr;

      if ( lom)
        {
          lom->g_lom_vp(d_ptr, (void*)lom);
        }

      ptr = ptr->next;
    }

  return 0;
}

int
g_lom_match_bare(__g_handle hdl, void *d_ptr, __g_lom lom)
{
  lom->g_lom_vp(d_ptr, (void*) lom);
  return lom->result;
}

int
g_load_lom(__g_handle hdl)
{
  g_setjmp(0, "g_load_lom", NULL, NULL);

  if ((hdl->flags & F_GH_HASLOM))
    {
      return 0;
    }

  int rt = 0;

  p_md_obj ptr = md_first(&hdl->_match_rr);
  __g_match _m_ptr;
  int r, ret, c = 0;
  while (ptr)
    {
      _m_ptr = (__g_match) ptr->ptr;
      if ( _m_ptr->flags & F_GM_ISLOM )
        {
          if ((r = g_process_lom_string(hdl, _m_ptr->data, _m_ptr, &ret,
                      _m_ptr->flags)))
            {
              print_str("ERROR: %s: [%d] [%d]: could not load LOM string\n",
                  hdl->file, r, ret);
              rt = 1;
              break;
            }
          c++;
        }
      ptr = ptr->next;
    }

  if (!rt)
    {
      hdl->flags |= F_GH_HASLOM;
      if (gfl & F_OPT_VERBOSE3)
        {
          print_str("NOTICE: %s: loaded %d LOM matches\n", hdl->file, c);
        }
    }
  else
    {
      print_str("ERROR: %s: [%d] LOM specified, but none could be loaded\n",
          hdl->file, rt);
      gfl |= F_OPT_KILL_GLOBAL;
      EXITVAL = 1;

    }

  return rt;
}

int
g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
    uint32_t flags)
{
  g_setjmp(0, "g_process_lom_string", NULL, NULL);
  char *ptr = string;
  char c_b[3], o_b[3];
  char *left, *right, *comp = c_b, *oper = o_b;
  int comp_l, oper_l;

  while (ptr[0])
    {
      if (ptr[0] == 0x0)
        {
          break;
        }

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      left = ptr;
      right = NULL;
      oper = NULL;
      comp = NULL;
      comp_l = 0;
      oper_l = 0;

      if (ptr[0] == 0x28)
        {
          while (ptr[0] != 0x29 && ptr[0])
            {
              ptr++;
            }
          if (!ptr[0])
            {
              return 11;
            }
        }

      while ((is_opr(ptr[0]) && ptr[0]))
        {
          ptr++;
        }

      if (!ptr[0])
        {
          goto build;
        }

      if (!strncmp(ptr, "&&", 2) || !strncmp(ptr, "||", 2))
        {
          oper_l = 2;
          oper = ptr;
          goto build;
        }

      if (!strncmp(ptr, "+=", 2))
        {
          flags |= F_GM_ISACCU;
        }

      comp = ptr;

      while (!is_comp(ptr[0]))
        {
          ptr++;
          comp_l++;
        }

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      if (ptr[0] == 0x0)
        {
          break;
        }

      right = ptr;

      if (ptr[0] == 0x28)
        {
          while (ptr[0] != 0x29 && ptr[0])
            {
              ptr++;
            }
          if (!ptr[0])
            {
              return 11;
            }
        }

      while (is_opr(ptr[0]) && ptr[0] && ptr[0] != 0x20)
        {
          ptr++;
        }

      if (!ptr[0])
        {
          goto build;
        }

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      if (!strncmp(ptr, "&&", 2) || !strncmp(ptr, "||", 2))
        {
          oper_l = 2;
          oper = ptr;
        }

      while (!is_opr(ptr[0]))
        {
          ptr++;
        }

      build: ;

      __g_lom r_lom;

      *ret = g_build_lom_packet(hdl, left, right, comp, comp_l, oper, oper_l,
          _gm, &r_lom, flags);

      if (*ret)
        {
          return 6;
        }

      if (flags & F_GM_ISACCU)
        {
          flags ^= F_GM_ISACCU;
        }

    }

  return 0;
}

void *_lcs_isequal[] =
  { g_is_equal, g_is_equal_s, g_is_f };
void *_lcs_ishigher[] =
  { g_is_higher_2, g_is_higher_2_s, g_is_higher_f_2 };
void *_lcs_islower[] =
  { g_is_lower_2, g_is_lower_2_s, g_is_lower_f_2 };
void *_lcs_islowerorequal[] =
  { g_is_lowerorequal, g_is_lowerorequal_s, g_is_lowerorequal_f };
void *_lcs_ishigherorequal[] =
  { g_is_higherorequal, g_is_higherorequal_s, g_is_higherorequal_f };
void *_lcs_isnotequal[] =
  { g_is_not, g_is_not_s, g_is_not_f };

int
g_build_lom_packet_bare(__g_handle hdl, __g_lom lom, char *field, void *right,
    void *comp_set[], g_op lop)
{
  int rt = 0;

  if (!lom)
    {
      rt = 1;
      goto end;
    }

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb)
    {
      return 600;
    }

  if (off > hdl->block_sz)
    {
      rt = 601;
      goto end;
    }

  int r;

  if ((r = g_get_lom_alignment(lom, F_GLT_LEFT, &vb, off)))
    {
      rt = r;
      goto end;
    }

  uint64_t *tp_0;
  int64_t *tp_1;
  float *tp_2;

  switch (lom->flags & F_LOM_TYPES)
    {
  case F_LOM_INT:
    lom->g_comp_ptr = comp_set[0];
    tp_0 = (uint64_t*) lom->r_stor;
    *tp_0 = *(uint64_t*) right;
    break;
  case F_LOM_INT_S:
    lom->g_comp_ptr = comp_set[1];
    tp_1 = (int64_t*) lom->r_stor;
    *tp_1 = *(int64_t*) right;
    break;
  case F_LOM_FLOAT:
    lom->g_comp_ptr = comp_set[2];
    tp_2 = (float*) lom->r_stor;
    *tp_2 = *(float*) right;
    break;
    }

  lom->flags |= F_LOM_RVAR_KNOWN;
  lom->g_oper_ptr = lop;

  end:

  return rt;
}

static void
sel_g_comp(__g_lom lom, void *comp_set[])
{
  switch (lom->flags & F_LOM_TYPES)
    {
  case F_LOM_INT:
    lom->g_comp_ptr = comp_set[0];
    break;
  case F_LOM_INT_S:
    lom->g_comp_ptr = comp_set[1];
    break;
  case F_LOM_FLOAT:
    lom->g_comp_ptr = comp_set[2];
    break;
    }

}

int
g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
    size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
    uint32_t flags)
{
  g_setjmp(0, "g_build_lom_packet", NULL, NULL);
  int rt = 0;
  __g_lom lom;

  if (!(flags & F_GM_ISACCU))
    {
      md_init(&match->lom, 16);
      lom = (__g_lom ) md_alloc(&match->lom, sizeof(_g_lom));
    }
  else
    {
      md_init(&hdl->_accumulator, 8);
      lom = (__g_lom ) md_alloc(&hdl->_accumulator, sizeof(_g_lom));
    }

  if (!lom)
    {
      rt = 1;
      goto end;
    }

  int r = 0;

  /*if (left[0] == 0x29)
   {
   int m_ret2, m_ret;
   if ((m_ret2 = g_process_math_string(hdl, left, &lom->math, &lom->chains,
   &m_ret, NULL, 0, 0)))
   {
   printf("ERROR: [%d] [%d]: could not process math string\n", m_ret2,
   m_ret);
   rt = 2;
   goto end;
   }
   }
   else
   {*/
  if (!(flags & F_GM_ISACCU))
    {
      if ((r = g_get_lom_g_t_ptr(hdl, left, lom, F_GLT_LEFT)))
        {
          rt = r;
          goto end;
        }
    }
  else
    {
      int t_o;
      if (NULL == (lom->p_glob_stor = g_get_glob_ptr(hdl, left, &t_o)))
        {
          rt = 21;
          goto end;
        }
    }
  //}

  char *r_ptr = right;

  if (!r_ptr)
    {
      if (flags & F_GM_ISACCU)
        {
          rt = 21;
          goto end;
        }

      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        r_ptr = "0";
        break;
      case F_LOM_INT:
        r_ptr = "0";
        break;
      case F_LOM_INT_S:
        r_ptr = "0";
        break;
        }
    }

  if ((r = g_get_lom_g_t_ptr(hdl, r_ptr, lom, F_GLT_RIGHT)))
    {
      rt = r + 1;
      goto end;
    }

  if ((lom->flags & F_LOM_FLOAT)
      && ((lom->flags & F_LOM_INT) | (lom->flags & F_LOM_INT_S)))
    {
      lom->flags ^= (F_LOM_INT | F_LOM_INT_S);
    }
  else if ((lom->flags & F_LOM_INT) && (lom->flags & F_LOM_INT_S))
    {
      lom->flags ^= F_LOM_INT;
    }

  if (!(lom->flags & F_LOM_TYPES))
    {
      rt = 6;
      goto end;
    }

  if (!comp)
    {
      sel_g_comp(lom, _lcs_isnotequal);

    }
  else if ((comp_l == 1 || comp_l == 2) && !strncmp(comp, "=", 1))
    {
      sel_g_comp(lom, _lcs_isequal);
    }
  else if (comp_l == 1 && !strncmp(comp, "<", 1))
    {
      sel_g_comp(lom, _lcs_islower);
    }
  else if (comp_l == 1 && !strncmp(comp, ">", 1))
    {
      sel_g_comp(lom, _lcs_ishigher);
    }
  else if (comp_l == 2 && !strncmp(comp, "!=", 2))
    {
      sel_g_comp(lom, _lcs_isnotequal);
    }
  else if (comp_l == 2 && !strncmp(comp, ">=", 2))
    {
      sel_g_comp(lom, _lcs_ishigherorequal);
    }
  else if (comp_l == 2 && !strncmp(comp, "<=", 2))
    {
      sel_g_comp(lom, _lcs_islowerorequal);

    }
  else if (comp_l == 2 && !strncmp(comp, "+=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_lom_vp = g_lom_var_accu_float;
        break;
      case F_LOM_INT:
        lom->g_lom_vp = g_lom_var_accu_uint;
        break;
      case F_LOM_INT_S:
        lom->g_lom_vp = g_lom_var_accu_int;
        break;
        }
    }
  else
    {
      rt = 11;
      goto end;
    }

  if (oper)
    {
      if (oper_l == 2 && !strncmp(oper, "&&", 2))
        {
          lom->g_oper_ptr = g_oper_and;
        }
      else if (oper_l == 2 && !strncmp(oper, "||", 2))
        {
          lom->g_oper_ptr = g_oper_or;
        }
      else
        {
          lom->g_oper_ptr = g_oper_and;
        }
    }
  else
    {
      lom->g_oper_ptr = g_oper_and;
    }

  if (ret)
    {
      *ret = lom;
    }

  end:

  if (rt)
    {
      md_unlink(&match->lom, match->lom.pos);
    }
  else if (!(flags & F_GM_ISACCU))
    {
      match->flags |= F_GM_ISLOM;
      match->flags |= flags;
      if (match->flags & F_GM_IMATCH)
        {
          match->match_i_m = G_NOMATCH;
        }
      else
        {
          match->match_i_m = G_MATCH;
        }
    }

  return rt;
}

static int
gl_var_known(__g_handle hdl, char *field, uint32_t flags, __g_lom lom)
{
  errno = 0;
  uint32_t t_f = 0;
  int base = 10;

  if (field[0] == 0x30 && (field[1] == 0x78 || field[1] == 0x58))
    {
      if ((lom->flags & F_LOM_FLOAT))
        {
          return 201;
        }
      field = &field[2];
      base = 16;
    }

  if (!(lom->flags & F_LOM_TYPES))
    {
      if (field[0] == 0x2D || field[0] == 0x2B)
        {
          lom->flags |= F_LOM_INT_S;
        }
      else if (base == 10 && s_char(field, 0x2E, g_floatstrlen(field)))
        {
          lom->flags |= F_LOM_FLOAT;
        }
      else
        {
          lom->flags |= F_LOM_INT;
        }
    }

  uint64_t *tp_0;
  int64_t *tp_1;
  float *tp_2;

  switch (flags & F_GLT_DIRECT)
    {
  case F_GLT_LEFT:
    switch (lom->flags & F_LOM_TYPES)
      {
    case F_LOM_INT:
      ;
      tp_0 = (uint64_t*) lom->l_stor;
      *tp_0 = (uint64_t) strtoull(field, NULL, base);
      break;
    case F_LOM_INT_S:
      ;
      tp_1 = (int64_t*) lom->l_stor;
      *tp_1 = (int64_t) strtoll(field, NULL, base);
      break;
    case F_LOM_FLOAT:
      ;
      tp_2 = (float*) lom->l_stor;
      *tp_2 = (float) strtof(field, NULL);
      break;
      }
    t_f |= F_LOM_LVAR_KNOWN;
    lom->left = (void*) lom->l_stor;
    set_lom_vp(lom, g_lom_var);
    break;
  case F_GLT_RIGHT:
    switch (lom->flags & F_LOM_TYPES)
      {
    case F_LOM_INT:
      ;
      tp_0 = (uint64_t*) lom->r_stor;
      *tp_0 = (uint64_t) strtoull(field, NULL, base);
      break;
    case F_LOM_INT_S:
      ;
      tp_1 = (int64_t*) lom->r_stor;
      *tp_1 = (int64_t) strtoll(field, NULL, base);
      break;
    case F_LOM_FLOAT:
      ;
      tp_2 = (float*) lom->r_stor;
      *tp_2 = (float) strtof(field, NULL);
      break;
      }
    t_f |= F_LOM_RVAR_KNOWN;
    lom->right = (void*) lom->r_stor;
    set_lom_vp(lom, g_lom_var);
    break;
    }

  if (errno == ERANGE)
    {
      return 4;
    }

  lom->flags |= t_f;

  return 0;
}

static int
copy_and_term_math_field(char *in, char *out, size_t max_size)
{
  size_t c = 0, d = 0;
  while (c < max_size)
    {
      if (in[0] == 0x28 && in[0] != 0x0)
        {
          d++;
        }
      if (in[0] == 0x29)
        {
          d--;
        }
      if (d == 0)
        {
          break;
        }
      out[0] = in[0];
      in++;
      out++;
      c++;
    }

  if (0 != d)
    {
      return 1;
    }

  out[0] = in[0];
  out++;
  in++;
  out[0] = 0x0;

  return 0;
}

int
g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags)
{
  g_setjmp(0, "g_get_lom_g_t_ptr", NULL, NULL);

  if (field[0] == 0x28)
    {
      md_init(&lom->chains, 8);
      md_init(&lom->math, 8);

      size_t field_l = strlen(field);
      char *m_field = calloc(field_l, 1);
      if (copy_and_term_math_field(field, m_field, field_l))
        {
          return 31;
        }

      int m_ret2, m_ret;
      if ((m_ret2 = g_process_math_string(hdl, m_field, &lom->math,
          &lom->chains, &m_ret, NULL, 0, 0)))
        {
          print_str(
              "ERROR: [%d] [%d]: could not process math string (LOM): %s\n",
              m_ret2, m_ret, field);
          return 32;
        }

      __g_math math = m_get_def_val(&lom->math);

      lom->flags |= (math->flags & F_MATH_TYPES);

      switch (flags & F_GLT_DIRECT)
        {
      case F_GLT_LEFT:
        lom->flags |= F_LOM_IS_LVAR_MATH;
        break;
      case F_GLT_RIGHT:
        lom->flags |= F_LOM_IS_RVAR_MATH;
        break;
        }

      lom->g_lom_vp = g_lom_var;

      return 0;
    }

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb)
    {
      return gl_var_known(hdl, field, flags, lom);
    }
  else
    {

      if (off > hdl->block_sz)
        {
          return 601;
        }

      set_lom_vp(lom, g_lom_var);

      return g_get_lom_alignment(lom, flags, &vb, off);
    }

  return -2;
}

#define LOM_ALIGN(flags, bt_ptr_l, bt_ptr_r, t_ptr, g_vp, flg) { \
    lom->flags |= flg; \
}

int
g_get_lom_alignment(__g_lom lom, uint32_t flags, int *vb, size_t off)
{
  switch (*vb)
    {
  case -32:
    LOM_ALIGN(flags, g_tf_ptr_left, g_tf_ptr_right, g_tf_ptr, g_lom_var,
        F_LOM_FLOAT)
    break;
  case -2:
    LOM_ALIGN(flags, g_ts_ptr_left, g_ts_ptr_right, g_ts8_ptr, g_lom_var,
        F_LOM_INT_S)
    break;
  case -3:
    LOM_ALIGN(flags, g_ts_ptr_left, g_ts_ptr_right, g_ts16_ptr, g_lom_var,
        F_LOM_INT_S)
    break;
  case -5:
    LOM_ALIGN(flags, g_ts_ptr_left, g_ts_ptr_right, g_ts32_ptr, g_lom_var,
        F_LOM_INT_S)
    break;
  case -9:
    LOM_ALIGN(flags, g_ts_ptr_left, g_ts_ptr_right, g_ts64_ptr, g_lom_var,
        F_LOM_INT_S)
    break;
  case 1:
    LOM_ALIGN(flags, g_t_ptr_left, g_t_ptr_right, g_t8_ptr, g_lom_var,
        F_LOM_INT)
    break;
  case 2:
    LOM_ALIGN(flags, g_t_ptr_left, g_t_ptr_right, g_t16_ptr, g_lom_var,
        F_LOM_INT)
    break;
  case 4:
    LOM_ALIGN(flags, g_t_ptr_left, g_t_ptr_right, g_t32_ptr, g_lom_var,
        F_LOM_INT)
    break;
  case 8:
    LOM_ALIGN(flags, g_t_ptr_left, g_t_ptr_right, g_t64_ptr, g_lom_var,
        F_LOM_INT)
    break;
  default:
    return 608;
    break;
    }

  switch (flags & F_GLT_DIRECT)
    {
  case F_GLT_LEFT:
    lom->t_l_off = off;
    break;
  case F_GLT_RIGHT:
    lom->t_r_off = off;
    break;
    }

  return 0;
}

int
g_lom_var(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;

  void *left, *right;

  if (!(lom->flags & F_LOM_LVAR_KNOWN))
    {
      if (!(lom->flags & F_LOM_IS_LVAR_MATH))
        {
          left = (d_ptr + lom->t_l_off);
        }
      else
        {
          g_math_res(d_ptr, &lom->math, lom->l_stor);
          left = (void*) lom->l_stor;
        }
    }
  else
    {
      left = (void*) lom->l_stor;
    }

  if (!(lom->flags & F_LOM_RVAR_KNOWN))
    {
      if (!(lom->flags & F_LOM_IS_RVAR_MATH))
        {
          right = (d_ptr + lom->t_r_off);
        }
      else
        {
          g_math_res(d_ptr, &lom->math, lom->r_stor);
          right = (void*) lom->r_stor;

        }
    }
  else
    {
      right = (void*) lom->r_stor;
    }

  lom->result = lom->g_comp_ptr(left, right);

  return 0;
}

#define LOM_VAR_ACCU(d_ptr, _lom) { \
  if (!(lom->flags & F_LOM_RVAR_KNOWN)) \
    { \
      right = (d_ptr + lom->t_r_off); \
    } \
  else \
    { \
      right = (void*) lom->r_stor; \
    } \
  lom->result = 1; \
}

int
g_lom_var_accu_uint(void *d_ptr, void *_lom)
{
  __g_lom lom = (__g_lom ) _lom;
  void *right;

  LOM_VAR_ACCU(d_ptr, lom)

  uint64_t *res = (uint64_t*) lom->p_glob_stor;
  *res = *res + *(uint64_t*) right;

  return 0;
}

int
g_lom_var_accu_int(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;
  void *right;

  LOM_VAR_ACCU(d_ptr, lom)

  int64_t *res = (int64_t*) lom->p_glob_stor;
  *res = *res + *(int64_t*) right;

  return 0;
}

int
g_lom_var_accu_float(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;
  void *right;

  LOM_VAR_ACCU(d_ptr, lom)

  float *res = (float*) lom->p_glob_stor;
  *res = *res + *(float*) right;

  return 0;
}

int
opt_g_lom(void *arg, int m, uint32_t flags)
{
  char *buffer = g_pg(arg, m);

  size_t a_i = strlen(buffer);

  if (!a_i)
    {
      return 0;
    }

  if (a_i > MAX_LOM_STRING)
    {
      return 8900;
    }

  __g_match pgm = g_global_register_match();

  if (!pgm)
    {
      return 8901;
    }

  pgm->flags = flags | F_GM_ISLOM;
  if (pgm->flags & F_GM_IMATCH)
    {
      pgm->g_oper_ptr = g_oper_or;
      pgm->flags |= F_GM_NOR;
    }
  else
    {
      pgm->g_oper_ptr = g_oper_and;
      pgm->flags |= F_GM_NAND;
    }

  gfl |= F_OPT_HAS_G_LOM;

  bzero(&_match_rr_l, sizeof(_match_rr_l));
  _match_rr_l.ptr = (void *) pgm;
  _match_rr_l.flags = F_LM_LOM;

  pgm->data = buffer;

  return 0;
}
