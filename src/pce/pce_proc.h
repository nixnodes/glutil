/*
 * pce_proc.h
 *
 *  Created on: Dec 7, 2013
 *      Author: reboot
 */

#ifndef PCE_PROC_H_
#define PCE_PROC_H_

#include <glutil.h>
#include <lref_sconf.h>

#define F_PCE_DONE_STR_PREPROC          (a32 << 1)
#define F_PCE_LOCK_IMDB                 (a32 << 2)

typedef struct __g_dgetr {
  uint64_t pf;
  char *d_field;
  char *d_yf;
}_g_dgetr, *__d_dgetr;

int
pce_proc(char *subject);

char *
pce_decomp(char *subject, char *sct_b, size_t max_sct);

int
pce_match_build(void *hdl, void *ptr, void *arg);
char *
pce_dgetlf(__d_dgetr dgetr, int logc);

__g_handle
pce_lhref_fst(pmda lh_ref, uint64_t *pf);

char *
pce_get_year_result(char *subject, char *output, size_t max_size);
char*
pce_do_str_preproc(char *subject);

char cl_presub[PATH_MAX];
char cl_sub[PATH_MAX*2];
char cl_yr[32], *s_year;
char *cl_dir;
char *cl_g_sub;

uint32_t pce_f;
int EXITVAL;

int
pce_run_log_match(void *_hdl, void *_ptr, void *arg);
int
pce_match_log(__g_handle hdl, __d_sconf sconf, int m_i_m);
int
pce_do_regex_match(char *pattern, char *match, int cflags, int m_i_m);
int
pce_lh_ref_clean(pmda lh_ref);
int
pce_do_lookup(__g_handle p_log, __d_dgetr dgetr);
void
pce_enable_logging(void);
void
pce_pcl_stat(int r, __d_sconf ptr);

typedef int _d_pce_plm(__g_handle hdl, __d_sconf ptr);

_d_pce_plm pce_process_string_match, pce_process_lom_match, pce_process_execv;

mda g_opt;

#endif /* PCE_PROC_H_ */
