/*
 * n_lom.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <m_lom.h>
#include <glutil.h>
#include <l_error.h>
#include <str.h>
#include <lc_oper.h>
#include <gv_off.h>

#include <stdio.h>
#include <errno.h>

#include <arg_proc.h>

int
g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm)
{
  p_md_obj ptr = _gm->lom.objects;
  __g_lom lom, p_lom = NULL;

  int r_p = 1, i = 0;

  while (ptr)
    {
      lom = (__g_lom) ptr->ptr;
      lom->result = 0;
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

      while (is_opr(ptr[0]) && ptr[0])
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

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      if (!is_comp(ptr[0]))
        {
          comp = ptr;
        }
      else
        {
          comp = NULL;
        }

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

      build:

      *ret = g_build_lom_packet(hdl, left, right, comp, comp_l, oper, oper_l,
          _gm,
          NULL, flags);

      if (*ret)
        {
          return 6;
        }

    }

  return 0;
}

void *_lcs_isequal[] =
  { g_is_equal, g_is_equal_s, g_is_equal_f };

int
g_build_lom_packet_bare(__g_handle hdl, __g_match match, char *field,
    void *right, void *comp_set[], g_op lop)
{
  md_init(&match->lom, 2);
  int rt = 0;

  __g_lom lom = (__g_lom ) md_alloc(&match->lom, sizeof(_g_lom));

  if (!lom)
    {
      rt = 1;
      goto end;
    }

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb) {
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

  switch (lom->flags & F_LOM_TYPES)
    {
  case F_LOM_INT:
    lom->g_icomp_ptr = comp_set[0];
    lom->t_right = *(uint64_t*) right;
    break;
  case F_LOM_INT_S:
    lom->g_iscomp_ptr = comp_set[1];
    lom->ts_right = *(int64_t*) right;
    break;
  case F_LOM_FLOAT:
    lom->g_fcomp_ptr = comp_set[2];
    lom->tf_right = *(float*) right;
    break;
    }

  lom->flags |= F_LOM_RVAR_KNOWN;
  lom->g_oper_ptr = lop;

  match->flags |= F_GM_ISLOM;

  end:

  if (rt)
    {
      md_unlink(&match->lom, match->lom.pos);
    }

  return rt;
}

int
g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
    size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
    uint32_t flags)
{
  g_setjmp(0, "g_build_lom_packet", NULL, NULL);
  int rt = 0;
  md_init(&match->lom, 16);

  __g_lom lom = (__g_lom ) md_alloc(&match->lom, sizeof(_g_lom));

  if (!lom)
    {
      rt = 1;
      goto end;
    }

  int r = 0;

  if ((r = g_get_lom_g_t_ptr(hdl, left, lom, F_GLT_LEFT)))
    {
      rt = r;
      goto end;
    }

  char *r_ptr = right;

  if (!r_ptr)
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        r_ptr = "1.0";
        break;
      case F_LOM_INT:
        r_ptr = "1";
        break;
      case F_LOM_INT_S:
        r_ptr = "1";
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
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_s;
        break;
        }

    }
  else if ((comp_l == 1 || comp_l == 2) && !strncmp(comp, "=", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_equal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_equal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_equal_s;
        break;
        }
    }
  else if (comp_l == 1 && !strncmp(comp, "<", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_lower_f_2;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_lower_2;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_lower_2_s;
        break;
        }
    }
  else if (comp_l == 1 && !strncmp(comp, ">", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_higher_f_2;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_higher_2;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_higher_2_s;
        break;
        }
    }
  else if (comp_l == 2 && !strncmp(comp, "!=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_notequal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_not_equal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_not_equal_s;
        break;
        }
    }
  else if (comp_l == 2 && !strncmp(comp, ">=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_higherorequal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_higherorequal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_higherorequal_s;
        break;
        }
    }
  else if (comp_l == 2 && !strncmp(comp, "<=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_lowerorequal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_lowerorequal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_lowerorequal_s;
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
  else
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

int
g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags)
{
  g_setjmp(0, "g_get_lom_g_t_ptr", NULL, NULL);

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb)
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

      switch (flags & F_GLT_DIRECT)
        {
      case F_GLT_LEFT:
        switch (lom->flags & F_LOM_TYPES)
          {
        case F_LOM_INT:
          lom->t_left = (uint64_t) strtoull(field, NULL, base);
          lom->g_lom_vp = g_lom_var_uint;
          break;
        case F_LOM_INT_S:
          lom->ts_left = (int64_t) strtoll(field, NULL, base);
          lom->g_lom_vp = g_lom_var_int;
          break;
        case F_LOM_FLOAT:
          lom->tf_left = (float) strtof(field, NULL);
          lom->g_lom_vp = g_lom_var_float;
          break;
          }
        t_f |= F_LOM_LVAR_KNOWN;
        break;
      case F_GLT_RIGHT:
        switch (lom->flags & F_LOM_TYPES)
          {
        case F_LOM_INT:
          lom->t_right = (uint64_t) strtoull(field, NULL, base);
          lom->g_lom_vp = g_lom_var_uint;
          break;
        case F_LOM_INT_S:
          lom->ts_right = (int64_t) strtoll(field, NULL, base);
          lom->g_lom_vp = g_lom_var_int;
          break;
        case F_LOM_FLOAT:
          lom->tf_right = (float) strtof(field, NULL);
          lom->g_lom_vp = g_lom_var_float;
          break;
          }
        t_f |= F_LOM_RVAR_KNOWN;
        break;
        }

      if (errno == ERANGE)
        {
          return 4;
        }

      lom->flags |= t_f;

      return 0;
    }

  if (off > hdl->block_sz)
    {
      return 601;
    }

  return g_get_lom_alignment(lom, flags, &vb, off);

}

int
g_get_lom_alignment(__g_lom lom, uint32_t flags, int *vb, size_t off)
{
  switch (*vb)
    {
  case -32:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_tf_ptr_left = g_tf_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_tf_ptr_right = g_tf_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_float;
    lom->flags |= F_LOM_FLOAT;
    break;
  case -2:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts8_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts8_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case -3:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts16_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts16_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case -5:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts32_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts32_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case -9:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts64_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts64_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case 1:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t8_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t8_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  case 2:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t16_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t16_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  case 4:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t32_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t32_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  case 8:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t64_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t64_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
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
g_lom_var_float(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;
  if (lom->g_tf_ptr_left)
    {
      lom->tf_left = lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_t_ptr_left)
    {
      lom->tf_left = (float) lom->g_t_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_ts_ptr_left)
    {
      lom->tf_left = (float) lom->g_ts_ptr_left(d_ptr, lom->t_l_off);
    }

  if (lom->g_tf_ptr_right)
    {
      lom->tf_right = lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_t_ptr_right)
    {
      lom->tf_right = (float) lom->g_t_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_ts_ptr_right)
    {
      lom->tf_right = (float) lom->g_ts_ptr_right(d_ptr, lom->t_r_off);
    }

  lom->result = lom->g_fcomp_ptr(lom->tf_left, lom->tf_right);

  return 0;
}

int
g_lom_var_int(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;

  if (lom->g_ts_ptr_left)
    {
      lom->ts_left = lom->g_ts_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_t_ptr_left)
    {
      lom->ts_left = (int64_t) lom->g_t_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_tf_ptr_left)
    {
      lom->ts_left = (int64_t) lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
    }

  if (lom->g_ts_ptr_right)
    {
      lom->ts_right = lom->g_ts_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_tf_ptr_right)
    {
      lom->ts_right = (int64_t) lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_t_ptr_right)
    {
      lom->ts_right = (int64_t) lom->g_t_ptr_right(d_ptr, lom->t_r_off);
    }

  lom->result = lom->g_iscomp_ptr(lom->ts_left, lom->ts_right);

  return 0;
}

int
g_lom_var_uint(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;
  if (lom->g_t_ptr_left)
    {
      lom->t_left = lom->g_t_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_tf_ptr_left)
    {
      lom->t_left = (uint64_t) lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_ts_ptr_left)
    {
      lom->t_left = (uint64_t) lom->g_ts_ptr_left(d_ptr, lom->t_l_off);
    }

  if (lom->g_t_ptr_right)
    {
      lom->t_right = lom->g_t_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_tf_ptr_right)
    {
      lom->t_right = (uint64_t) lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_ts_ptr_right)
    {
      lom->t_right = (uint64_t) lom->g_ts_ptr_right(d_ptr, lom->t_r_off);
    }

  lom->result = lom->g_icomp_ptr(lom->t_left, lom->t_right);

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

  //g_cpg(arg, pgm->data, m, a_i);
  pgm->data = buffer;

  return 0;
}
