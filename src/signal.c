/*
 * signal.c
 *
 *  Created on: Dec 6, 2013
 *      Author: reboot
 */
#include "config.h"
#include <glutil.h>
#include "signal_t.h"

#include <t_glob.h>
#include <l_error.h>

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

  sa.sa_handler = &sig_handler;
  sa.sa_flags = SA_RESTART;

  sa_c.sa_sigaction = &child_sig_handler;
  sa_c.sa_flags = SA_RESTART | SA_SIGINFO;

  sa_e.sa_sigaction = sighdl_error;
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
