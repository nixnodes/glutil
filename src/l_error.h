/*
 * l_error.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef L_ERROR_H_
#define L_ERROR_H_

#include <stdint.h>
#include <setjmp.h>
#include <signal.h>

#define F_SIGERR_CONTINUE               0x1  /* continue after exception */

#define ID_SIGERR_UNSPEC                0x0
#define ID_SIGERR_MEMCPY                0x1
#define ID_SIGERR_STRCPY                0x2
#define ID_SIGERR_FREE                  0x3
#define ID_SIGERR_FREAD                 0x4
#define ID_SIGERR_FWRITE                0x5
#define ID_SIGERR_FOPEN                 0x6
#define ID_SIGERR_FCLOSE                0x7
#define ID_SIGERR_MEMMOVE               0x8

typedef struct sig_jmp_buf
{
  sigjmp_buf env, p_env;
  uint32_t flags, pflags;
  int id, pid;
  unsigned char ci, pci;
  char type[32];
  void *callback, *arg;
} sigjmp, *p_sigjmp;

void
g_setjmp (uint32_t flags, char *type, void *callback, void *arg);
void
sighdl_error (int sig, siginfo_t* siginfo, void* context);

sigjmp g_sigjmp;

#if __x86_64__ || __ppc64__
#define __AE_INTTL	long long unsigned int
#define __AE_SPFH       "%.16llX"
#else
#define __AE_INTTL	unsigned int
#define __AE_SPFH       "%.8X"
#endif


#endif /* L_ERROR_H_ */
