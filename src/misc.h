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
g_bin_compare(const void *p1, const void *p2, off_t size);
int
find_absolute_path(char *exec, char *output);

uint32_t LOGLVL;

#endif /* MISC_H_ */
