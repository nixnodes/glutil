/*
 * omfp.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef OMFP_H_
#define OMFP_H_

#include <glutil.h>

#include <fp_types.h>

#include <stdio.h>

#define omfp_timeout { \
  if (g_omfp_sto) { \
    sleep(g_omfp_sto); \
  } else if (g_omfp_suto) { \
      usleep(g_omfp_suto); \
  } \
}

_d_omfp_fp g_omfp_norm, g_omfp_raw, g_omfp_ocomp, g_omfp_eassemble,
    g_omfp_eassemblef, g_omfp_eassemble_post, g_omfp_eassemblef_post,
    g_xproc_print_d, g_xproc_print;

int
(*int_printf)(const char *__restrict __format, ...);

int
g_print_stats(char *file, uint32_t flags, size_t block_sz);
void
g_do_ppprint(__g_handle hdl, uint64_t t_flags, pmda p_mech, _d_omfp g_proc);

int
g_omfp_write(int fd, char *buffer, size_t max_size, void*);
int
g_omfp_write_nl(int fd, char *buffer, size_t max_size, void*);
int
g_omfp_q_nssys(int fd, char *buffer, size_t max_size, void *arg);
int
g_omfp_q_nssys_nl(int fd, char *buffer, size_t size, void *arg);

int     fd_out;

#endif /* OMFP_H_ */
