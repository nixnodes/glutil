/*
 * misc.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef MISC_H_
#define MISC_H_

#include <fp_types.h>

#include <unistd.h>

#define F_MSG_TYPE_ANY                  MAX_uint32_t
#define F_MSG_TYPE_EXCEPTION            ((uint32_t)1 << 1)
#define F_MSG_TYPE_ERROR                ((uint32_t)1 << 2)
#define F_MSG_TYPE_WARNING              ((uint32_t)1 << 3)
#define F_MSG_TYPE_MACRO                ((uint32_t)1 << 4)
#define F_MSG_TYPE_NOTICE               ((uint32_t)1 << 5)
#define F_MSG_TYPE_INFO                 ((uint32_t)1 << 6)
#define F_MSG_TYPE_STATS                ((uint32_t)1 << 7)
#define F_MSG_TYPE_OTHER                ((uint32_t)1 << 8)
#define F_MSG_TYPE_DEBUG0               ((uint32_t)1 << 9)
#define F_MSG_TYPE_DEBUG1               ((uint32_t)1 << 10)
#define F_MSG_TYPE_DEBUG2               ((uint32_t)1 << 11)
#define F_MSG_TYPE_DEBUG3               ((uint32_t)1 << 12)
#define F_MSG_TYPE_DEBUG4               ((uint32_t)1 << 13)
#define F_MSG_TYPE_DEBUG5               ((uint32_t)1 << 14)
#define F_MSG_TYPE_DEBUG6               ((uint32_t)1 << 15)

#define F_MSG_TYPE_EEW                  (F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_ERROR|F_MSG_TYPE_WARNING|F_MSG_TYPE_MACRO)
#define F_MSG_TYPE_NORMAL               (F_MSG_TYPE_EEW|F_MSG_TYPE_NOTICE|F_MSG_TYPE_OTHER|F_MSG_TYPE_STATS|F_MSG_TYPE_INFO|F_MSG_TYPE_MACRO)
#define F_MSG_TYPE_DEBUG                (F_MSG_TYPE_DEBUG0|F_MSG_TYPE_DEBUG1|F_MSG_TYPE_DEBUG2|F_MSG_TYPE_DEBUG3|F_MSG_TYPE_DEBUG4|F_MSG_TYPE_DEBUG5|F_MSG_TYPE_DEBUG6)

int
enable_logging (void);
char *
build_data_path (char *file, char *path, char *sd);
uint32_t
get_msg_type (char *msg);
void
w_log (char *w);
int
g_print_info (void);
int
g_memcomp (const void *p1, const void *p2, off_t size);
char *
g_bitstr (uint64_t value, uint8_t bits, char *buffer);

uint32_t
opt_get_msg_type (char *msg);
int
build_msg_reg (char *arg, uint32_t *opt_r);

uint32_t STDLOG_LVL;

int
print_version_long (void *arg, int m, void *opt);

int
(*print_str) (const char * volatile buf, ...);

#endif /* MISC_H_ */
