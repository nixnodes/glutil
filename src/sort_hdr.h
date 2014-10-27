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
#define F_GSORT_STRING                  (a32 << 5)
#define F_GSORT_F_DATE                  (a32 << 6)

#define F_GSORT_ORDER                   (F_GSORT_DESC|F_GSORT_ASC)
#define F_GSORT_TYPE                    (F_GSORT_NUMERIC|F_GSORT_STRING)
#define F_GSORT_TYPE_F                  (F_GSORT_STRING|F_GSORT_F_DATE)

#define MAX_SORT_LOOPS                          MAX_uint64_t

#define F_INT_GSORT_LOOP_DID_SORT       (a32 << 1)
#define F_INT_GSORT_DID_SORT            (a32 << 2)

mda _md_gsort;
char *g_sort_field;

uint32_t g_sort_flags;

#include <lc_oper.h>
#include <m_comp.h>

typedef struct g_sref_data
{
  void *g_t_ptr_c;
  gs_cmp_p m_op, m_op_opp;
  uint32_t flags;
  size_t off;
  char m_buf1[4096], m_buf2[4096];
  _d_drt_h mppd;
  int64_t off_right;
  void *sp_0;
} _srd, *__p_srd;

typedef int
(*g_xsort_exec_p)(pmda m_ptr, __p_srd psrd);

int
do_sort(__g_handle hdl, char *field, uint32_t flags);
int
opt_g_sort(void *arg, int m);
int
g_sort(__g_handle hdl, char *field, uint32_t flags);
void
g_heapsort(void **ref_arr, int64_t offset, int64_t dummy, __p_srd psrd);
void
g_qsort(void **arr, int64_t left, int64_t right, __p_srd psrd);

typedef void
(*__d_sort_p)(void **arr, int64_t left, int64_t right, __p_srd psrd);

#endif /* SORT_HDR_H_ */
