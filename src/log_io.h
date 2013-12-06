/*
 * log_io.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LOG_IO_H_
#define LOG_IO_H_

#define F_GBM_SHM_NO_DATAFILE           (a32 << 1)

#define MAX_SDENTRY_LEN         20000
#define MAX_SENTRY_LEN          1024

#include <stdint.h>

#include <im_hdr.h>

#define MSG_REDF_ABORT "WARNING: %s: aborting rebuild (will not be writing what was done up to here)\n"
#define MSG_UNRECOGNIZED_DATA_TYPE      "ERROR: [%s] unrecognized data type\n"

typedef int
(*__g_mdref)(__g_handle hdl, pmda md, off_t count);

int
d_write(char *arg);
int
g_fopen(char *file, char *mode, uint32_t flags, __g_handle hdl);
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
g_buffer_into_memory(char *file, __g_handle hdl);
int
g_cleanup(__g_handle hdl);
int
rebuild_data_file(char *file, __g_handle hdl);
int
flush_data_md(__g_handle hdl, char *outfile);
int
m_load_input_n(__g_handle hdl, FILE *input);

#endif /* LOG_IO_H_ */
