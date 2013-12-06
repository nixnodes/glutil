/*
 * sort.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <sort_hdr.h>
#include <arg_proc.h>
#include <gv_off.h>
#include <lc_oper.h>
#include <l_error.h>
#include <str.h>

mda _md_gsort =
  { 0 };
char *g_sort_field = NULL;

uint32_t g_sort_flags = 0;


int
do_sort(__g_handle hdl, char *field, uint32_t flags)
{
  if (!(gfl & F_OPT_SORT))
    {
      return 0;
    }

  if (!field)
    {
      print_str("ERROR: %s: sorting requested but no field was set\n",
          hdl->file);
      return 1;
    }

  if ((gfl & F_OPT_VERBOSE))
    {
      print_str("NOTICE: %s: sorting %llu records..\n", hdl->file,
          (uint64_t) hdl->buffer.offset);
    }

  int r = g_sort(hdl, field, flags);

  if (r)
    {
      print_str("ERROR: %s: [%d]: could not sort data\n", hdl->file, r);
    }
  else
    {
      if (gfl & F_OPT_VERBOSE4)
        {
          print_str("NOTICE: %s: sorting done\n", hdl->file);
        }
    }

  return r;
}


int
g_sorti_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(uint64_t s, uint64_t d) = cb1;
  uint64_t
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;

  uint64_t t_b, t_b_n;
  uint32_t ml_f = 0;
  uint64_t ml_i;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if ((flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return 0;
}

int
g_sortis_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(int64_t s, int64_t d) = cb1;
  int64_t
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;

  int64_t t_b, t_b_n;
  uint32_t ml_f = 0;
  uint64_t ml_i;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if ((flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return 0;
}

int
g_sortd_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(double s, double d) = cb1;
  double
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;
  int r = 0;
  uint64_t ml_i;
  uint32_t ml_f = 0;
  double t_b, t_b_n;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if (!r && (flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return r;
}

int
g_sortf_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(float s, float d) = cb1;
  float
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;
  int r = 0;
  uint64_t ml_i;
  uint32_t ml_f = 0;
  float t_b, t_b_n;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if (!r && (flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return r;
}

int
g_sort(__g_handle hdl, char *field, uint32_t flags)
{
  g_setjmp(0, "g_sort", NULL, NULL);
  void *m_op = NULL, *g_t_ptr_c = NULL;

  int
  (*g_s_ex)(pmda, size_t, uint32_t, void *, void *) = NULL;
  pmda m_ptr;

  if (!hdl)
    {
      return 1;
    }

  if (!(hdl->flags & F_GH_FFBUFFER))
    {
      m_ptr = &hdl->buffer;
    }
  else
    {
      m_ptr = &hdl->w_buffer;
    }

  if (!hdl->g_proc2)
    {
      return 2;
    }

  void *g_fh_f = NULL, *g_fh_s = NULL;

  switch (flags & F_GSORT_ORDER)
    {
  case F_GSORT_DESC:
    m_op = g_is_lower;
    g_fh_f = g_is_lower_f;
    g_fh_s = g_is_lower_s;
    break;
  case F_GSORT_ASC:
    m_op = g_is_higher;
    g_fh_f = g_is_higher_f;
    g_fh_s = g_is_higher_s;
    break;
    }

  if (!m_op)
    {
      return 3;
    }

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb)
    {
      return 13;
    }

  switch (vb)
    {
  case -32:
    m_op = g_fh_f;
    g_t_ptr_c = g_tf_ptr;
    g_s_ex = g_sortf_exec;
    break;
  case 1:
    g_t_ptr_c = g_t8_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case 2:
    g_t_ptr_c = g_t16_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case 4:
    g_t_ptr_c = g_t32_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case 8:
    g_t_ptr_c = g_t64_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case -2:
    g_t_ptr_c = g_ts8_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  case -3:
    g_t_ptr_c = g_ts16_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  case -5:
    g_t_ptr_c = g_ts32_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  case -9:
    g_t_ptr_c = g_ts64_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  default:
    return 14;
    break;
    }

  return g_s_ex(m_ptr, off, flags, m_op, g_t_ptr_c);

}


int
opt_g_sort(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (gfl & F_OPT_SORT)
    {
      return 0;
    }

  if (_md_gsort.offset >= 64)
    {
      return 4600;
    }

  md_init(&_md_gsort, 3);

  int r = split_string(buffer, 0x2C, &_md_gsort);

  if (r != 3)
    {
      return 4601;
    }

  p_md_obj ptr = md_first(&_md_gsort);

  if (!ptr)
    {
      return 4602;
    }

  char *s_ptr = (char*) ptr->ptr;

  if (!strncmp(s_ptr, "num", 3))
    {
      g_sort_flags |= F_GSORT_NUMERIC;
    }
  else
    {
      return 4603;
    }

  ptr = ptr->next;
  s_ptr = (char*) ptr->ptr;

  if (!strncmp(s_ptr, "desc", 4))
    {
      g_sort_flags |= F_GSORT_DESC;
    }
  else if (!strncmp(s_ptr, "asc", 3))
    {
      g_sort_flags |= F_GSORT_ASC;
    }
  else
    {
      return 4604;
    }

  ptr = ptr->next;
  g_sort_field = (char*) ptr->ptr;

  if (!strlen(g_sort_field))
    {
      return 4605;
    }

  g_sort_flags |= F_GSORT_RESETPOS;

  gfl |= F_OPT_SORT;

  return 0;
}
