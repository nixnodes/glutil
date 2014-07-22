/*
 * sort_hdr.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef SORT_HDR_H_
#define SORT_HDR_H_

#include <glutil.h>

#include <memory.h>
#include <im_hdr.h>

#include <stdio.h>

#define F_GSORT_DESC                    (a32 << 1)
#define F_GSORT_ASC                     (a32 << 2)
#define F_GSORT_RESETPOS                (a32 << 3)
#define F_GSORT_NUMERIC                 (a32 << 4)

#define F_GSORT_ORDER                   (F_GSORT_DESC|F_GSORT_ASC)

#define MAX_SORT_LOOPS                          MAX_uint64_t

#define F_INT_GSORT_LOOP_DID_SORT       (a32 << 1)
#define F_INT_GSORT_DID_SORT            (a32 << 2)

mda _md_gsort;
char *g_sort_field;

uint32_t g_sort_flags;

#include <lc_oper.h>

typedef struct g_sref_data
{
  void *g_t_ptr_c;
  gs_cmp_p m_op, m_op_opp;
  uint32_t flags;
  size_t off;
} _srd, *__p_srd;

typedef int
(*g_xsort_exec_p)(pmda m_ptr, __p_srd psrd);

int
do_sort(__g_handle hdl, char *field, uint32_t flags);
int
opt_g_sort(void *arg, int m);
int
g_sort(__g_handle hdl, char *field, uint32_t flags);


#endif /* SORT_HDR_H_ */
