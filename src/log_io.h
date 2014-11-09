/*
 * log_io.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LOG_IO_H_
#define LOG_IO_H_

#define F_GBM_SHM_NO_DATAFILE           (a32 << 1)

#define MAX_SDENTRY_LEN                 20000
#define MAX_SENTRY_LEN                  1024

#include <glutil.h>

#include <stdint.h>

#include <im_hdr.h>

#define MSG_REDF_ABORT                  "WARNING: %s: aborting rebuild\n"
#define MSG_UNRECOGNIZED_DATA_TYPE      "ERROR: [%s] unrecognized data type\n"

#include <errno_int.h>

#define _E_MSG_MLI_DEF                  "import failed"
#define _E_MSG_MLI_NOREF                "g_proc0 is null"
#define _E_MSG_MLI_NOMEMB               "d_memb is zero"
#define _E_MSG_MLI_NORES                "no input data to import"
#define _E_MSG_MLI_UTERM                "could not process all mandatory items or an unterminated record was encountered"
#define _E_MSG_MLI_MALF                 "malformed input data"

#define _E_MSG_FDM_DEF                  "flushing data failed"
#define _E_MSG_FDM_NOW                  _E_MSG_DEF_BNW
#define _E_MSG_FDM_NOWC                 _E_MSG_DEF_BNW _E_MSG_DEF_CC
#define _E_MSG_FDM_WF                   _E_MSG_DEF_WF
#define _E_MSG_FDM_WFC                  _E_MSG_DEF_WF _E_MSG_DEF_CC
#define _E_MSG_FDM_NORECW               "no data was written"

#define F_LOAD_RECORD_FLUSH             ((uint16_t)1 << 1)
#define F_LOAD_RECORD_DATA              ((uint16_t)1 << 2)

#define F_LOAD_RECORD_ALL               (F_LOAD_RECORD_FLUSH|F_LOAD_RECORD_DATA)

#define MAX_BWHOLD_BYTES                8388608

typedef int
(*__g_mdref)(__g_handle hdl, pmda md, off_t count);

int
d_write(char *arg);
int
g_fopen(char *file, char *mode, uint32_t flags, __g_handle hdl);
void
g_set_compression_opts(uint8_t level, __g_handle hdl);
void *
g_read(void *buffer, __g_handle hdl, size_t size);
int
g_close(__g_handle hdl);
size_t
g_load_data_md(void *output, size_t max, char *file, __g_handle hdl);
int
load_data_md(pmda md, char *file, __g_handle hdl);
int
gen_md_data_ref_cnull(__g_handle hdl, pmda md, off_t count);
int
gen_md_data_ref(__g_handle hdl, pmda md, off_t count);
int
gh_rewind(__g_handle hdl);
int
g_claf_mech(void *ptr);
int
g_buffer_into_memory(char *file, __g_handle hdl);
int
g_cleanup(__g_handle hdl);
int
rebuild_data_file(char *file, __g_handle hdl);
int
flush_data_md(__g_handle hdl, char *outfile);
int
m_load_input_n(__g_handle hdl, FILE *input);
int
g_d_post_proc_gcb(void *buffer, void *p_hdl);
int
g_enum_log(_d_enuml callback, __g_handle hdl, off_t *nres, void *arg);
int
determine_temp_path(char *file, char *output, size_t max_out);
int
g_load_record(__g_handle hdl, pmda w_buffer, const void *data, off_t max, uint16_t flags);

#define OPLOG_OUTPUT_NSTATS(dfile, sdst) { \
  if (sdst.bw > 0) \
    { \
      fprintf(stderr, MSG_GEN_WROTE, dfile, (double) sdst.bw / 1024.0, \
          (unsigned long long int)sdst.rw); \
    } \
  else \
    { \
      fprintf(stderr, MSG_GEN_NO_WRITE, dfile); \
    } \
}

#endif /* LOG_IO_H_ */
