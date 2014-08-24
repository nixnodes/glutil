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

#endif /* OMFP_H_ */
