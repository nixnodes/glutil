/*
 * dirlog.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef DIRLOG_H_
#define DIRLOG_H_

#define MSG_PS_UWSFV    "WARNING: %s: unable to wipe temp. SFV file (remove it manually)\n"

#define MAX_WBUFFER_HOLD        100000

#include <glutil.h>
#include <xref.h>

#include <stdio.h>

typedef struct e_arg
{
  int depth;
  uint32_t flags;
  char buffer[PATH_MAX];
  char buffer2[PATH_MAX + 10];
  struct dirlog *dirlog;
  time_t t_stor;
  __g_eds eds;
} ear;

#define F_EARG_SFV                      (a32 << 1)
#define F_EAR_NOVERB                    (a32 << 2)

#define F_PS_STOP_TRAVERSE              ((uint8_t)1 << 1)

#define MAX_BWHOLD_BYTES_DL             524288 * 2

int
update_records (char *dirname, int depth);
int
proc_directory (char *name, unsigned char type, void *arg, __g_eds eds);
int
rebuild_dirlog (void);
int
proc_section (char *name, unsigned char type, void *arg, __g_eds eds);

typedef uint64_t
__d_dlfind (char *, int, uint32_t, void *);

__d_dlfind dirlog_find, dirlog_find_old, dirlog_find_simple;

int
release_generate_block (char *name, ear *iarg);

int
dirlog_write_record (struct dirlog *buffer, off_t offset, int whence);
int
get_relative_path (char *subject, char *root, char *output);
char*
get_relative_path_n (char *subject, char *root);
uint64_t
nukelog_find (char *dirn, int mode, struct nukelog *output);
int
dirlog_check_records (void);
int
dirlog_update_record (char *argv);
int
dirlog_check_dupe (void);

typedef  p_md_obj
(*df_func) (void *, uint32_t, void *);

p_md_obj
dirlog_find_a (void *, uint32_t, void *);
p_md_obj
dirlog_find_ht (void *, uint32_t, void *);

#endif /* DIRLOG_H_ */
