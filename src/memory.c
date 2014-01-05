/*
 * memory.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <memory_t.h>

int
md_init(pmda md, int nm)
{
  if (!md || md->objects)
    {
      return 1;
    }
  bzero(md, sizeof(mda));
  md->objects = calloc(nm, sizeof(md_obj));
  md->count = nm;
  md->pos = md->objects;
  md->r_pos = md->objects;
  md->first = md->objects;
  return 0;
}

int
md_g_free(pmda md)
{
  if (!md || !md->objects)
    return 1;

  if (!(md->flags & F_MDA_REFPTR))
    {
      p_md_obj ptr = md_first(md), ptr_s;
      while (ptr)
        {
          ptr_s = ptr->next;
          if (ptr->ptr)
            {
              free(ptr->ptr);
              ptr->ptr = NULL;
            }
          ptr = ptr_s;
        }
    }

  free(md->objects);
  bzero(md, sizeof(mda));

  return 0;
}

int
md_g_free_cb(pmda md, int
(*cb)(void *))
{
  if (!md || !md->objects)
    return 1;

  if (!(md->flags & F_MDA_REFPTR))
    {
      p_md_obj ptr = md_first(md), ptr_s;
      while (ptr)
        {
          cb(ptr->ptr);
          ptr_s = ptr->next;
          if (ptr->ptr)
            {
              free(ptr->ptr);
              ptr->ptr = NULL;
            }
          ptr = ptr_s;
        }
    }

  free(md->objects);
  bzero(md, sizeof(mda));

  return 0;
}

uintaa_t
md_relink(pmda md)
{
  off_t off, l = 1;

  p_md_obj last = NULL, cur = md->objects;

  for (off = 0; off < md->count; off++)
    {
      if (cur->ptr)
        {
          if (last)
            {
              last->next = cur;
              cur->prev = last;
              l++;
            }
          else
            {
              md->first = cur;
            }
          last = cur;
        }
      cur++;
    }
  return l;
}

#include <unistd.h>

uintaa_t
md_relink_n(pmda md, off_t base)
{
  size_t off, l = 1;

  p_md_obj last = NULL, cur = md->objects;

  off_t tot = (off_t) md->objects + (md->count * sizeof(md_obj));

  off_t totl = (size_t) md->count / base;

  off_t cp = 0, cnt = 0;

  for (cp = 0; cp < base; cp++)
    {
      for (off = 0; off <= totl; off++)
        {
          cur = md->objects + ((base * off) + cp);
          //cnt++;
          if ((off_t) cur < tot)
            {
              cnt++;
              if (cur->ptr)
                {
                  if (last)
                    {
                      last->next = cur;
                      cur->prev = last;
                      cur->next = NULL;
                      l++;
                    }
                  else
                    {
                      md->first = cur;
                    }
                  last = cur;
                }
            }
          else
            {
              break;
            }
        }
    }

  return l;
}

p_md_obj
md_first(pmda md)
{

  /*if (md->first && md->first != md->objects) {
   if (md->first->ptr) {
   return md->first;
   }
   }*/

  off_t off = 0;
  p_md_obj ptr = md->objects;

  for (off = 0; off < md->count; off++, ptr++)
    {
      if (ptr->ptr)
        {
          return ptr;
        }
    }

  return NULL;
}

p_md_obj
md_last(pmda md)
{
  p_md_obj ptr = md_first(md);

  if (!ptr)
    {
      return ptr;
    }

  while (ptr->next)
    {
      ptr = ptr->next;
    }

  return ptr;
}

void *
md_alloc(pmda md, int b)
{
  uint32_t flags = 0;

  if (md->offset >= md->count)
    {
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str(
              "NOTICE: re-allocating memory segment to increase size; current address: 0x%.16llX, current size: %llu\n",
              (ulint64_t) (uintaa_t) md->objects, (ulint64_t) md->count);
        }
      md->objects = realloc(md->objects, (md->count * sizeof(md_obj)) * 2);
      md->pos = md->objects;
      md->pos += md->count;
      bzero(md->pos, md->count * sizeof(md_obj));

      md->count *= 2;
      uintaa_t rlc;

      if (md->flags & F_MDA_ARR_DIST)
        {
          rlc = md_relink_n(md, 100);
        }
      else
        {
          rlc = md_relink(md);
        }
      flags |= MDA_MDALLOC_RE;
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str(
              "NOTICE: re-allocation done; new address: 0x%.16llX, new size: %llu, re-linked %llu records\n",
              (ulint64_t) (uintaa_t) md->objects, (ulint64_t) md->count,
              (ulint64_t) rlc);
        }
    }

  p_md_obj prev = md->pos;
  uintaa_t pcntr = 0;
  while (md->pos->ptr
      && (pcntr = ((md->pos - md->objects) / sizeof(md_obj))) < md->count)
    {
      md->pos++;
    }

  if (pcntr >= md->count)
    {
      return NULL;
    }

  if (md->pos > md->objects && !(md->pos - 1)->ptr)
    {
      flags |= MDA_MDALLOC_RE;
    }

  if (md->pos->ptr)
    return NULL;

  if (md->flags & F_MDA_REFPTR)
    {
      md->pos->ptr = md->lref_ptr;
    }
  else
    {
      md->pos->ptr = calloc(1, b);
    }

  if (prev != md->pos)
    {
      prev->next = md->pos;
      md->pos->prev = prev;
    }

  md->offset++;

  if (flags & MDA_MDALLOC_RE)
    {
      if (md->flags & F_MDA_ARR_DIST)
        {
          md_relink_n(md, 100);
        }
      else
        {
          md_relink(md);
        }
    }

  return md->pos->ptr;
}

void *
md_unlink(pmda md, p_md_obj md_o)
{
  if (!md_o)
    {
      return NULL;
    }

  p_md_obj c_ptr = NULL;

  if (md_o->prev)
    {
      ((p_md_obj) md_o->prev)->next = (p_md_obj) md_o->next;
      c_ptr = md_o->prev;
    }

  if (md_o->next)
    {
      ((p_md_obj) md_o->next)->prev = (p_md_obj) md_o->prev;
      c_ptr = md_o->next;
    }

  md->offset--;
  if (md->pos == md_o && c_ptr)
    {
      md->pos = c_ptr;
    }
  if (!(md->flags & F_MDA_REFPTR) && md_o->ptr)
    {
      free(md_o->ptr);
    }
  md_o->ptr = NULL;
  md_o->next = NULL;
  md_o->prev = NULL;

  return (void*) c_ptr;
}

void *
md_swap(pmda md, p_md_obj md_o1, p_md_obj md_o2)
{
  if (!md_o1 || !md_o2)
    {
      return NULL;
    }

  void *ptr2_s;

  ptr2_s = md_o1->prev;
  md_o1->next = md_o2->next;
  md_o1->prev = md_o2;
  md_o2->next = md_o1;
  md_o2->prev = ptr2_s;

  if (md_o2->prev)
    {
      ((p_md_obj) md_o2->prev)->next = md_o2;
    }

  if (md_o1->next)
    {
      ((p_md_obj) md_o1->next)->prev = md_o1;
    }

  if (md->first == md_o1)
    {
      md->first = md_o2;
    }

  return md_o2->next;
}

void *
md_swap_s(pmda md, p_md_obj md_o1, p_md_obj md_o2)
{
  void *ptr = md_o1->ptr;
  md_o1->ptr = md_o2->ptr;
  md_o2->ptr = ptr;

  return md_o1->next;
}

int
md_copy(pmda source, pmda dest, size_t block_sz)
{
  if (!source || !dest)
    {
      return 1;
    }

  if (dest->count)
    {
      return 2;
    }
  int ret = 0;
  p_md_obj ptr = md_first(source);
  void *d_ptr;

  md_init(dest, source->count);

  while (ptr)
    {
      d_ptr = md_alloc(dest, block_sz);
      if (!d_ptr)
        {
          ret = 10;
          break;
        }
      memcpy(d_ptr, ptr->ptr, block_sz);
      ptr = ptr->next;
    }

  if (ret)
    {
      md_g_free(dest);
    }

  if (source->offset != dest->offset)
    {
      return 3;
    }

  return 0;
}

int
is_memregion_null(void *addr, size_t size)
{
  size_t i = size - 1;
  unsigned char *ptr = (unsigned char*) addr;
  while (!ptr[i] && i)
    {
      i--;
    }
  return i;
}
