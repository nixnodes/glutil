/*
 * misc.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef MISC_H_
#define MISC_H_

#include <glutil.h>

#include <fp_types.h>

#include <unistd.h>

#define F_MSG_TYPE_ANY                  MAX_uint32_t
#define F_MSG_TYPE_EXCEPTION            (a32 << 1)
#define F_MSG_TYPE_ERROR                (a32 << 2)
#define F_MSG_TYPE_WARNING              (a32 << 3)
#define F_MSG_TYPE_NOTICE               (a32 << 4)
#define F_MSG_TYPE_STATS                (a32 << 5)
#define F_MSG_TYPE_NORMAL               (a32 << 6)

#define F_MSG_TYPE_EEW                  (F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_ERROR|F_MSG_TYPE_WARNING)

void
enable_logging(void);
char *
build_data_path(char *file, char *path, char *sd);
uint32_t
get_msg_type(char *msg);
int
w_log(char *w, char *ow);
int
g_print_info(void);
int
g_memcomp(const void *p1, const void *p2, off_t size);
char *
g_bitstr(uint64_t value, uint8_t bits, char *buffer);

uint32_t
opt_get_msg_type(char *msg);
int
build_msg_reg(char *arg, uint32_t *opt_r);

uint32_t LOGLVL;
uint32_t STDOUT_LVL;

int
print_version_long(void *arg, int m, void *opt);
#endif /* MISC_H_ */
