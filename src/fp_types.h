/*
 * fp_types.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef FP_TYPES_H_
#define FP_TYPES_H_

#include <sys/ipc.h>
#include <sys/shm.h>

#include <memory_t.h>


#define MAX_VAR_LEN                             4096 * 8

/* generic types */

typedef int
_d_achar_i(char *);
typedef int
(*__d_avoidp_i)(void *);
typedef int
*_d_avoidp_i(void *);
typedef char *
_d_achar_p(char *);
typedef int
_d_avoid_i(void);
typedef int
(*__d_avoid_i)(void);
typedef int
_d_is_am(char in_c);
typedef int
(*__d_is_am)(uint8_t in_c);
typedef void
(*__d_is_wb)(int, char*, size_t, void*);

/* specific types */

typedef int
__d_ref_to_val(void *, char *, char *, size_t, void *mppd);
typedef int
__d_format_block(void *, char *);
typedef pmda
__d_cfg(pmda md, char * file);
typedef int
__d_mlref(void *buffer, char *key, char *val);
typedef uint64_t
__g_t_ptr(void *base, size_t offset);
typedef void *
__d_ref_to_pval(void *arg, char *match, int *output);
typedef char *
__g_proc_rv(void *arg, char *match, char *output, size_t max_size, void *mppd);
typedef void *
_d_rtv_lk(void *arg, char *match, char *output, size_t max_size, void *mppd);
typedef void
_d_omfp_fp(void *hdl, void *ptr, char *sbuffer);
typedef int64_t
g_sint_p(void *base, size_t offset);
typedef void *
(*__g_proc_v)(void *, char *, char *, size_t, void *);
typedef void
(*__g_ipcbm)(void *hdl, pmda md, int *r_p, void *ptr);
typedef int
(*__g_proc_t)(void *, char *, char *, size_t);
typedef void *
(*__d_ref_to_pv)(void *arg, char *match, int *output);
typedef void
(*_d_omfp)(void *hdl, void *ptr, char *sbuffer);
typedef int
(*_d_proc3)(void *, char *);
typedef int
(*_d_gcb_pp_hook)(void *, void *hdl);
typedef int
(*_d_enuml)(void *hdl, void *ptr, void *buffer);

#endif /* FP_TYPES_H_ */
