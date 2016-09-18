/*
 * memory.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef MEMORY_H_
#define MEMORY_H_

//#include <fp_types.h>
//#include <t_glob.h>

#include <stdio.h>
#include <stdint.h>

#define F_MDA_REFPTR                    ((uint32_t)1 << 1)
#define F_MDA_FREE                      ((uint32_t)1 << 2)
#define F_MDA_REUSE                     ((uint32_t)1 << 3)
#define F_MDA_WAS_REUSED                ((uint32_t)1 << 4)
#define F_MDA_EOF                       ((uint32_t)1 << 5)
#define F_MDA_FIRST_REUSED              ((uint32_t)1 << 6)
#define F_MDA_ARR_DIST                  ((uint32_t)1 << 7)
#define F_MDA_NO_REALLOC                ((uint32_t)1 << 8)
#define F_MDA_ORPHANED                  ((uint32_t)1 << 10)

#define F_MDA_ST_MISC00                 ((uint32_t)1 << 15)
#define F_MDA_ST_MISC01                 ((uint32_t)1 << 16)
#define F_MDA_ST_MISC02                 ((uint32_t)1 << 11)

#define MDA_MDALLOC_RE                  ((uint32_t)1 << 1)

typedef struct mda_object
{
  void *ptr;
  struct mda_object *next;
  struct mda_object *prev;
}*p_md_obj, md_obj;

#ifdef _G_SSYS_THREAD
#include        <pthread.h>
#endif

typedef struct mda_header
{
  p_md_obj objects; /* holds references */
  p_md_obj pos, r_pos, c_pos, first, last;
  off_t offset, r_offset, count, hitcnt, rescnt;
  uint32_t flags;
  void *lref_ptr;
#ifdef _G_SSYS_THREAD
  pthread_mutex_t mutex;
#endif
} mda, *pmda;

#pragma pack(push, 4)

typedef struct ___nn_2x64
{
  uint64_t u00, u01;
  uint16_t u16_00;
} _nn_2x64, *__nn_2x64;

#pragma pack(pop)

int
md_init (pmda md, int nm);
p_md_obj
md_first (pmda md);
int
md_free (pmda md);
int
md_g_free_l (pmda md);
int
md_g_free_cb (pmda md, int
(*cb) (void *));
ssize_t
md_relink (pmda md);
ssize_t
md_relink_n (pmda md, off_t base);
p_md_obj
md_first (pmda md);
p_md_obj
md_last (pmda md);
void *
md_swap_s (pmda md, p_md_obj md_o1, p_md_obj md_o2);
void *
md_swap (pmda md, p_md_obj md_o1, p_md_obj md_o2);
void *
md_unlink (pmda md, p_md_obj md_o);
void *
md_alloc (pmda md, int b);
int
md_copy (pmda source, pmda dest, size_t block_sz, int
(*cb) (void *source, void *dest, void *ptr));
int
md_copy_le (pmda source, pmda dest, size_t block_sz, int
(*cb) (void *source, void *dest, void *ptr));
int
is_memregion_null (void *addr, size_t size);
int
md_md_to_array (pmda source, void **dest);
int
md_array_to_md (void ** source, pmda dest);

#define F_MDALLOC_NOLINK                ((uint32_t)1 << 1)

void *
md_alloc_le (pmda md, size_t b, uint32_t flags, void *refptr);
void *
md_unlink_le (pmda md, p_md_obj md_o);
int
md_init_le (pmda md, int nm);

off_t
register_count (pmda thread_r);

#ifdef _G_SSYS_THREAD
off_t
md_get_off_ts (pmda md);
#endif

#if HAVE_MALLOC == 0

#undef malloc

#include <sys/types.h>

void *
malloc ();

/* Allocate an N-byte block of memory from the heap.
 If N is zero, allocate a 1-byte block.  */

void*
rpl_malloc (size_t n);
#endif

#endif /* MEMORY_H_ */
