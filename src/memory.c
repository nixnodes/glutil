/*
 * memory.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include "memory_t.h"

#include <stdlib.h>
#include <string.h>

#include "misc.h"

#include "thread.h"

#include "t_glob.h"

int
md_init (pmda md, int nm)
{
  if (!md || md->objects)
    {
      return 1;
    }

  //bzero(md, sizeof(mda));
  if (!(md->objects = calloc (nm + 1, sizeof(md_obj))))
    {
      fprintf (stderr, "ERROR: md_init: could not allocate memory\n");
      abort ();
    }

  md->count = nm;
  md->pos = md->objects;
  md->r_pos = md->objects;
  md->first = md->objects;

#ifdef _G_SSYS_THREAD
  mutex_init (&md->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);
#endif

  return 0;
}

int
md_free (pmda md)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif
  if (!md || !md->objects)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return 1;
    }

  if (!(md->flags & F_MDA_REFPTR))
    {
      p_md_obj ptr = md_first (md), ptr_s;
      while (ptr)
	{
	  ptr_s = ptr->next;
	  if (ptr->ptr)
	    {
	      free (ptr->ptr);
	      ptr->ptr = NULL;
	    }
	  ptr = ptr_s;
	}
    }
  free (md->objects);
  md->objects = NULL;

#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif

  bzero (md, sizeof(mda));

  return 0;
}

int
md_g_free_l (pmda md)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif
  if (!md || !md->objects)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return 1;
    }

  if (!(md->flags & F_MDA_REFPTR))
    {
      p_md_obj ptr = md->first, ptr_s;
      while (ptr)
	{
	  ptr_s = ptr->next;
	  if (ptr->ptr)
	    {
	      free (ptr->ptr);
	      ptr->ptr = NULL;
	    }
	  ptr = ptr_s;
	}
    }

  free (md->objects);
  md->objects = NULL;

#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif

  bzero (md, sizeof(mda));

  return 0;
}

int
md_g_free_cb (pmda md, int
(*cb) (void *))
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif

  if (!md || !md->objects)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return 1;
    }

  if (!(md->flags & F_MDA_REFPTR))
    {
      p_md_obj ptr = md_first (md), ptr_s;
      while (ptr)
	{
	  cb (ptr->ptr);
	  ptr_s = ptr->next;
	  if (ptr->ptr)
	    {
	      free (ptr->ptr);
	      ptr->ptr = NULL;
	    }
	  ptr = ptr_s;
	}
    }

#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif

  free (md->objects);
  bzero (md, sizeof(mda));

  return 0;
}

ssize_t
md_relink (pmda md)
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

ssize_t
md_relink_n (pmda md, off_t base)
{
  size_t off, l = 1;

  p_md_obj last = NULL, cur = md->objects;

  size_t tot = (size_t) md->objects + ((size_t) md->count * sizeof(md_obj));

  size_t totl = (size_t) md->count / base;

  size_t cp = 0, cnt = 0;

  for (cp = 0; cp < base; cp++)
    {
      for (off = 0; off <= totl; off++)
	{
	  cur = md->objects + ((base * off) + cp);
	  //cnt++;
	  if ((size_t) cur < tot)
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
md_first (pmda md)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif

  off_t off = 0;
  p_md_obj ptr = md->objects;

  for (off = 0; off < md->count; off++, ptr++)
    {
      if (ptr->ptr)
	{
#ifdef _G_SSYS_THREAD
	  pthread_mutex_unlock (&md->mutex);
#endif
	  return ptr;
	}
    }
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif
  return NULL;
}

p_md_obj
md_last (pmda md)
{
  p_md_obj ptr = md_first (md);

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
md_alloc (pmda md, int b)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif

  uint32_t flags = 0;

  if (!md->count)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return NULL;
    }

  if (md->offset >= md->count)
    {
      if (md->flags & F_MDA_NO_REALLOC)
	{
	  print_str ("ERROR: md structure ran out of memory\n");
#ifdef _G_SSYS_THREAD
	  pthread_mutex_unlock (&md->mutex);
#endif
	  return NULL;
	}

      print_str (
	  "DEBUG: re-allocating memory segment to increase size; current address: 0x%.16llX, current size: %llu\n",
	  (size_t) md->objects, (ulint64_t) md->count);

      md->objects = realloc (md->objects, (md->count * sizeof(md_obj)) * 2);
      md->pos = md->objects;
      md->pos += md->count;
      bzero (md->pos, md->count * sizeof(md_obj));

      md->count *= 2;
      //ssize_t rlc;

      if (md->flags & F_MDA_ARR_DIST)
	{
	  md_relink_n (md, 100);
	}
      else
	{
	  md_relink (md);
	}
      flags |= MDA_MDALLOC_RE;

      print_str (
	  "DEBUG: re-allocation done; new address: 0x%.16llX, new size: %llu\n",
	  (size_t) md->objects, (ulint64_t) md->count);

    }

  p_md_obj prev = md->pos;
  off_t pcntr = 0;
  while (md->pos->ptr
      && (pcntr = (off_t) ((md->pos - md->objects) / sizeof(md_obj)))
	  < md->count)
    {
      md->pos++;
    }

  if (pcntr >= md->count)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return NULL;
    }

  if (!(md->flags & F_MDA_NO_REALLOC))
    {
      if (md->pos > md->objects && !(md->pos - 1)->ptr)
	{
	  flags |= MDA_MDALLOC_RE;
	}
    }

  if (md->pos->ptr)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return NULL;
    }

  if (md->flags & F_MDA_REFPTR)
    {
      md->pos->ptr = md->lref_ptr;
    }
  else
    {
      md->pos->ptr = calloc (1, b);
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
	  md_relink_n (md, 100);
	}
      else
	{
	  md_relink (md);
	}
    }
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif

  return md->pos->ptr;
}

void *
md_unlink (pmda md, p_md_obj md_o)
{
  if (!md_o)
    {
      return NULL;
    }

#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif

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
  if (md->pos == md_o)
    {
      if (NULL != c_ptr)
	{
	  md->pos = c_ptr;
	}
      else
	{
	  md->pos = md->objects;
	}
    }
  if (!(md->flags & F_MDA_REFPTR) && md_o->ptr)
    {
      free (md_o->ptr);
    }

  md_o->ptr = NULL;
  md_o->next = NULL;
  md_o->prev = NULL;

#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif

  return (void*) c_ptr;
}

void *
md_swap (pmda md, p_md_obj md_o1, p_md_obj md_o2)
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
md_swap_s (pmda md, p_md_obj md_o1, p_md_obj md_o2)
{
  void *ptr = md_o1->ptr;
  md_o1->ptr = md_o2->ptr;
  md_o2->ptr = ptr;

  return md_o1->next;
}

int
md_copy (pmda source, pmda dest, size_t block_sz, int
(*cb) (void *source, void *dest, void *ptr))
{
  if (!source || !dest)
    {
      return 1;
    }

  if (dest->count)
    {
      return 2;
    }
#ifdef _G_SSYS_THREAD
  mutex_lock (&source->mutex);
#endif

  int ret = 0;
  p_md_obj ptr = md_first (source);
  void *d_ptr;

  md_init (dest, source->count);

  while (ptr)
    {
      d_ptr = md_alloc (dest, block_sz);
      if (!d_ptr)
	{
	  ret = 10;
	  break;
	}
      memcpy (d_ptr, ptr->ptr, block_sz);
      if (NULL != cb)
	{
	  cb ((void*) ptr->ptr, (void*) dest, (void*) d_ptr);
	}
      ptr = ptr->next;
    }

  if (ret)
    {
      md_free (dest);
    }

  if (source->offset != dest->offset)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&source->mutex);
#endif
      return 3;
    }
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&source->mutex);
#endif
  return 0;
}

int
md_copy_le (pmda source, pmda dest, size_t block_sz, int
(*cb) (void *source, void *dest, void *ptr))
{
  if (!source || 0 == source->count || !dest)
    {
      return 1;
    }

  if (dest->count)
    {
      return 2;
    }
#ifdef _G_SSYS_THREAD
  mutex_lock (&source->mutex);
#endif

  int ret = 0;
  p_md_obj ptr = source->first;
  void *d_ptr;

  md_init_le (dest, (int) source->count);

  while (ptr)
    {
      d_ptr = md_alloc_le (dest, block_sz, 0, NULL);
      if (!d_ptr)

	{
	  ret = 10;
	  break;
	}
      memcpy (d_ptr, ptr->ptr, block_sz);
      if (NULL != cb)
	{
	  cb ((void*) ptr->ptr, (void*) dest, (void*) d_ptr);
	}
      ptr = ptr->next;
    }

  if (ret)
    {
      md_g_free_l (dest);
    }

  if (source->offset != dest->offset)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&source->mutex);
#endif
      return 3;
    }
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&source->mutex);
#endif
  return 0;
}

int
md_md_to_array (pmda source, void **dest)
{
  if (!source || !dest)
    {
      return 1;
    }

  p_md_obj ptr = md_first (source);
  size_t index = 0;

  while (ptr)
    {
      dest[index] = ptr->ptr;
      index++;
      if (index == UINTPTR_MAX)
	{
	  return 2;
	}
      ptr = ptr->next;
    }

  return 0;
}

int
md_array_to_md (void **source, pmda dest)
{
  if (!source || !dest)
    {
      return 1;
    }

  p_md_obj ptr = md_first (dest);

  size_t index = 0;

  while (ptr)
    {
      ptr->ptr = source[index];
      index++;
      if (index == UINTPTR_MAX)
	{
	  return 2;
	}
      ptr = ptr->next;
    }

  return 0;
}

int
is_memregion_null (void *addr, size_t size)
{
  size_t i = size - 1;
  unsigned char *ptr = (unsigned char*) addr;
  while (!ptr[i] && i)
    {
      i--;
    }
  return i;
}

int
md_init_le (pmda md, int nm)
{
  if (!md || md->objects)
    {
      return 1;
    }

  bzero (md, sizeof(mda));
  if (!(md->objects = calloc (nm + 1, sizeof(md_obj))))
    {
      fprintf (stderr, "ERROR: md_init: could not allocate memory\n");
      abort ();
    }

  md->count = (off_t) nm;
  md->pos = md->objects;
  md->first = NULL;
#ifdef _G_SSYS_THREAD
  mutex_init (&md->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);
#endif

  return 0;
}

void *
md_alloc_le (pmda md, size_t b, uint32_t flags, void *refptr)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif

  if (md->offset >= md->count)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return NULL;
    }

  p_md_obj pos = md->objects;
  ssize_t pcntr = 0;

  while (pos->ptr && pcntr < md->count)
    {
      pcntr++;
      pos++;
    }

  if (pcntr >= md->count)
    {
#ifdef _G_SSYS_THREAD
      pthread_mutex_unlock (&md->mutex);
#endif
      return NULL;
    }

  if (md->flags & F_MDA_REFPTR)
    {
      pos->ptr = refptr;
    }
  else
    {
      pos->ptr = calloc (1, b);
    }

  if (NULL == md->first)
    {
      md->first = pos;
    }
  else
    {
      if (!(flags & F_MDALLOC_NOLINK))
	{
	  md->pos->next = pos;
	  pos->prev = md->pos;
	}
    }

  md->pos = pos;
  md->offset++;

#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif
  return md->pos->ptr;
}

void *
md_unlink_le (pmda md, p_md_obj md_o)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&md->mutex);
#endif

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

  if (md->first == md_o)
    {
      md->first = c_ptr;
    }

  md->offset--;

  if (NULL == md->first && md->offset > 0)
    {
      abort ();
    }

  if (md->pos == md_o)
    {
      if (NULL != c_ptr)
	{
	  md->pos = c_ptr;
	}
      else
	{
	  md->pos = md->objects;
	}
    }

  if (!(md->flags & F_MDA_REFPTR) && NULL != md_o->ptr)
    {
      free (md_o->ptr);
    }

  md_o->ptr = NULL;
  md_o->next = NULL;
  md_o->prev = NULL;

#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&md->mutex);
#endif

  return (void*) c_ptr;
}

#ifdef _G_SSYS_THREAD
off_t
md_get_off_ts (pmda md)
{
  mutex_lock (&md->mutex);
  off_t ret = md->offset;
  pthread_mutex_unlock (&md->mutex);
  return ret;
}
#endif

off_t
register_count (pmda thread_r)
{
#ifdef _G_SSYS_THREAD
  mutex_lock (&thread_r->mutex);
#endif
  off_t ret = thread_r->offset;
#ifdef _G_SSYS_THREAD
  pthread_mutex_unlock (&thread_r->mutex);
#endif
  return ret;
}

#if HAVE_MALLOC == 0
void*
rpl_malloc (size_t n)
{
  if (n == 0)
    n = 1;
  return malloc (n);
}
#endif
