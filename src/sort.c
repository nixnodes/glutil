/*
 * sort.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "sort_hdr.h"

#include <arg_proc.h>
#include <gv_off.h>
#include <lc_oper.h>
#include <l_error.h>
#include <str.h>

mda _md_gsort =
  { 0 };
char *g_sort_field = NULL;

uint32_t g_sort_flags = 0;

static int
gs_t_is_lower(void *s, void *d, size_t offset, void * t_ptr)
{
  g_t64_ptr_p t64_ptr = (g_t64_ptr_p) t_ptr;
  return (t64_ptr(s, offset) < t64_ptr(d, offset));
}

static int
gs_t_is_higher(void *s, void *d, size_t offset, void * t_ptr)
{
  g_t64_ptr_p t64_ptr = (g_t64_ptr_p) t_ptr;
  return (t64_ptr(s, offset) > t64_ptr(d, offset));
}

static int
gs_ts_is_lower(void *s, void *d, size_t offset, void * t_ptr)
{
  g_ts64_ptr_p ts64_ptr = (g_ts64_ptr_p) t_ptr;
  return (ts64_ptr(s, offset) < ts64_ptr(d, offset));
}

static int
gs_ts_is_higher(void *s, void *d, size_t offset, void * t_ptr)
{
  g_ts64_ptr_p ts64_ptr = (g_ts64_ptr_p) t_ptr;
  return (ts64_ptr(s, offset) > ts64_ptr(d, offset));
}

static int
gs_tf_is_lower(void *s, void *d, size_t offset, void * t_ptr)
{
  g_tf_ptr_p tf_ptr = (g_tf_ptr_p) t_ptr;
  return (tf_ptr(s, offset) < tf_ptr(d, offset));
}

static int
gs_tf_is_higher(void *s, void *d, size_t offset, void * t_ptr)
{
  g_tf_ptr_p tf_ptr = (g_tf_ptr_p) t_ptr;
  return (tf_ptr(s, offset) > tf_ptr(d, offset));
}

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

  pmda m_ptr;

  if (!(hdl->flags & F_GH_FFBUFFER))
    {
      m_ptr = &hdl->buffer;
    }
  else
    {
      m_ptr = &hdl->w_buffer;
    }

  if ((gfl & F_OPT_VERBOSE))
    {
      print_str("NOTICE: %s: sorting %llu records..\n", hdl->file,
          (uint64_t) m_ptr->offset);
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

static void
g_swapi(void **x, void **y)
{
  void *z = *x;
  *x = *y;
  *y = z;
}

static void
g_siftdown(void **arr, int start, int end, size_t off, gs_cmp_p m_op, void *cb2)
{
  int root, child;

  root = start;
  while (2 * root + 1 < end)
    {
      child = 2 * root + 1;
      if ((child + 1 < end) && (!m_op(arr[child], arr[child + 1], off, cb2)))
        ++child;
      if (!m_op(arr[root], arr[child], off, cb2))
        {
          g_swapi(&arr[child], &arr[root]);
          root = child;
        }
      else
        return;
    }
}

static int
g_heapsort_exec(pmda m_ptr, size_t off, uint32_t flags, gs_cmp_p m_op,
    void *cb2)
{
  void **ref_arr = malloc(m_ptr->offset * sizeof(void*));

  if (md_md_to_array(m_ptr, ref_arr))
    {
      return 1;
    }

  int start, end;

  for (start = (m_ptr->offset - 2) / 2; start >= 0; --start)
    g_siftdown(ref_arr, start, m_ptr->offset, off, m_op, cb2);

  for (end = m_ptr->offset - 1; end; --end)
    {
      g_swapi(&ref_arr[end], &ref_arr[0]);
      g_siftdown(ref_arr, 0, end, off, m_op, cb2);
    }

  if (md_array_to_md(ref_arr, m_ptr))
    {
      return 1;
    }

  return 0;
}

static int
g_swapsort_exec(pmda m_ptr, size_t off, uint32_t flags, gs_cmp_p m_op,
    void *cb2)
{
  p_md_obj ptr, ptr_n;

  uint32_t ml_f = 0;

  for (;;)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          if (m_op(ptr->ptr, ptr_n->ptr, off, cb2))
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

      /*if (!(ml_f & F_INT_GSORT_DID_SORT))
       {
       ml_f |= F_INT_GSORT_DID_SORT;
       }*/
    }

  /*if (!(ml_f & F_INT_GSORT_DID_SORT))
   {
   return -1;
   }*/

  if ((flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return 0;
}

int
g_sort(__g_handle hdl, char *field, uint32_t flags)
{
  g_setjmp(0, "g_sort", NULL, NULL);
  void *m_op = NULL, *g_t_ptr_c = NULL;

  g_xsort_exec_p g_s_ex;

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

  switch (gfl0 & F_OPT_SMETHOD)
    {
  case F_OPT_SMETHOD_SWAP:
    g_s_ex = g_swapsort_exec;
    break;
  case F_OPT_SMETHOD_HEAP:
    g_s_ex = g_heapsort_exec;
    break;
  default:
    g_s_ex = g_heapsort_exec;
    }

  void *g_fh_f = NULL, *g_fh_s = NULL;

  switch (flags & F_GSORT_ORDER)
    {
  case F_GSORT_DESC:
    m_op = gs_t_is_lower;
    g_fh_f = gs_tf_is_lower;
    g_fh_s = gs_ts_is_lower;
    break;
  case F_GSORT_ASC:
    m_op = gs_t_is_higher;
    g_fh_f = gs_tf_is_higher;
    g_fh_s = gs_ts_is_higher;
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
    break;
  case 1:
    g_t_ptr_c = g_t8_ptr;
    break;
  case 2:
    g_t_ptr_c = g_t16_ptr;
    break;
  case 4:
    g_t_ptr_c = g_t32_ptr;
    break;
  case 8:
    g_t_ptr_c = g_t64_ptr;
    break;
  case -2:
    g_t_ptr_c = g_ts8_ptr;
    m_op = g_fh_s;
    break;
  case -3:
    g_t_ptr_c = g_ts16_ptr;
    m_op = g_fh_s;
    break;
  case -5:
    g_t_ptr_c = g_ts32_ptr;
    m_op = g_fh_s;
    break;
  case -9:
    g_t_ptr_c = g_ts64_ptr;
    m_op = g_fh_s;
    break;
  default:
    return 14;
    }

  return g_s_ex(m_ptr, off, flags, (gs_cmp_p) m_op, g_t_ptr_c);

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

  l_sfo = L_STFO_SORT;

  return 0;
}
