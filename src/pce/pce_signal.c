/*
 * pce_signal.c
 *
 *  Created on: Dec 11, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "pce_config.h"
#include "pce_signal.h"

#include <l_error.h>
#include <signal_t.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int
setup_sighandlers(void)
{
  struct sigaction sa =
    {
      { 0 } }, sa_c =
    {
      { 0 } }, sa_e =
    {
      { 0 } };
  int r = 0;

  sa.sa_handler = sig_handler;
  sa.sa_flags = SA_RESTART;

  sa_c.sa_sigaction = child_sig_handler;
  sa_c.sa_flags = SA_RESTART | SA_SIGINFO;

  sa_e.sa_sigaction = pce_sighdl_error;
  sa_e.sa_flags = SA_RESTART | SA_SIGINFO;

  sigfillset(&sa.sa_mask);
  sigfillset(&sa_c.sa_mask);
  sigemptyset(&sa_e.sa_mask);

  r += sigaction(SIGINT, &sa, NULL);
  r += sigaction(SIGQUIT, &sa, NULL);
  r += sigaction(SIGABRT, &sa, NULL);
  r += sigaction(SIGTERM, &sa, NULL);
  r += sigaction(SIGCHLD, &sa_c, NULL);
  r += sigaction(SIGSEGV, &sa_e, NULL);
  r += sigaction(SIGILL, &sa_e, NULL);
  r += sigaction(SIGFPE, &sa_e, NULL);
  r += sigaction(SIGBUS, &sa_e, NULL);
  r += sigaction(SIGTRAP, &sa_e, NULL);

  signal(SIGKILL, sig_handler);

  return r;
}

#define MSG_DEF_UNKN1   "(unknown)"

void
pce_sighdl_error(int sig, siginfo_t* siginfo, void* context)
{

  char *s_ptr1 = MSG_DEF_UNKN1, *s_ptr2 = MSG_DEF_UNKN1, *s_ptr3 = "";
  char buffer1[4096] =
    { 0 };

  switch (sig)
    {
  case SIGSEGV:
    s_ptr1 = "SEGMENTATION FAULT";
    break;
  case SIGFPE:
    s_ptr1 = "FLOATING POINT EXCEPTION";
    break;
  case SIGILL:
    s_ptr1 = "ILLEGAL INSTRUCTION";
    break;
  case SIGBUS:
    s_ptr1 = "BUS ERROR";
    break;
  case SIGTRAP:
    s_ptr1 = "TRACE TRAP";
    break;
  default:
    s_ptr1 = "UNKNOWN EXCEPTION";
    }

  snprintf(buffer1, 4096, ", fault address: 0x%.16llX",
      (ulint64_t) siginfo->si_addr);

  switch (g_sigjmp.id)
    {
  case ID_SIGERR_MEMCPY:
    s_ptr2 = "memcpy";
    break;
  case ID_SIGERR_STRCPY:
    s_ptr2 = "strncpy";
    break;
  case ID_SIGERR_FREE:
    s_ptr2 = "free";
    break;
  case ID_SIGERR_FREAD:
    s_ptr2 = "fread";
    break;
  case ID_SIGERR_FWRITE:
    s_ptr2 = "fwrite";
    break;
  case ID_SIGERR_FCLOSE:
    s_ptr2 = "fclose";
    break;
  case ID_SIGERR_MEMMOVE:
    s_ptr2 = "memove";
    break;
    }

  if (g_sigjmp.flags & F_SIGERR_CONTINUE)
    {
      s_ptr3 = ", resuming execution..";
    }

  print_str("EXCEPTION: %s: [%s] [%s] [%s]%s%s\n", s_ptr1, g_sigjmp.type,
      s_ptr2, strerror(siginfo->si_errno), buffer1, s_ptr3);

  usleep(450000);

  g_sigjmp.ci++;

  if (g_sigjmp.flags & F_SIGERR_CONTINUE)
    {
      siglongjmp(g_sigjmp.env, 0);
    }

  g_sigjmp.ci = 0;
  g_sigjmp.flags = 0;

  exit(siginfo->si_errno);
}

void
child_sig_handler(int signal, siginfo_t * si, void *p)
{
  switch (si->si_code)
    {
  case CLD_KILLED:
    print_str(
        "NOTICE: Child process caught SIGINT (hit CTRL^C again to quit)\n");
    usleep(1000000);
    break;
  case CLD_EXITED:
    break;
  default:
    if (gfl & F_OPT_VERBOSE3)
      {
        print_str("NOTICE: Child caught signal: %d \n", si->si_code);
      }
    break;
    }
}

void
sig_handler(int signal)
{
  switch (signal)
    {
  case SIGTERM:
    print_str("NOTICE: Caught SIGTERM, terminating gracefully.\n");
    gfl |= F_OPT_KILL_GLOBAL;
    break;
  case SIGINT:
    if (gfl & F_OPT_KILL_GLOBAL)
      {
        print_str("NOTICE: Caught SIGINT twice, terminating..\n");
        exit(0);
      }
    else
      {
        print_str(
            "NOTICE: Caught SIGINT, quitting (hit CTRL^C again to terminate by force)\n");
        gfl |= F_OPT_KILL_GLOBAL;
      }
    usleep(SIG_BREAK_TIMEOUT_NS);
    break;
  default:
    usleep(SIG_BREAK_TIMEOUT_NS);
    print_str("NOTICE: Caught signal %d\n", signal);
    break;
    }
}
