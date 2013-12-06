/*
 * memory.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef MEMORY_H_
#define MEMORY_H_


#include <t_glob.h>


#define F_MDA_REFPTR                    (a32 << 1)
#define F_MDA_FREE                      (a32 << 2)
#define F_MDA_REUSE                     (a32 << 3)
#define F_MDA_WAS_REUSED                (a32 << 4)
#define F_MDA_EOF                       (a32 << 5)
#define F_MDA_FIRST_REUSED              (a32 << 6)

#define MDA_MDALLOC_RE                  (a32 << 1)


typedef struct mda_object
{
  void *ptr;
  void *next;
  void *prev;
}*p_md_obj, md_obj;

typedef struct mda_header
{
  p_md_obj objects; /* holds references */
  p_md_obj pos, r_pos, c_pos, first, last;
  off_t offset, r_offset, count, hitcnt, rescnt;
  uint32_t flags;
  void *lref_ptr;
} mda, *pmda;


int
md_init(pmda md, int nm);
p_md_obj
md_first(pmda md);
int
md_g_free(pmda md);
uintaa_t
md_relink(pmda md);
p_md_obj
md_first(pmda md);
p_md_obj
md_last(pmda md);
void *
md_swap_s(pmda md, p_md_obj md_o1, p_md_obj md_o2);
void *
md_swap(pmda md, p_md_obj md_o1, p_md_obj md_o2);
void *
md_unlink(pmda md, p_md_obj md_o);
void *
md_alloc(pmda md, int b);
int
md_copy(pmda source, pmda dest, size_t block_sz);
int
is_memregion_null(void *addr, size_t size);


#endif /* MEMORY_H_ */
