/*
 * signal_t.h
 *
 *  Created on: Dec 6, 2013
 *      Author: reboot
 */

#ifndef SIGNAL_T_H_
#define SIGNAL_T_H_

#define SIG_BREAK_TIMEOUT_NS (useconds_t)1000000.0

#include <glutil.h>

#include <signal.h>

void
sig_handler_null(int signal);
void
sig_handler_test(int signal);
int
setup_sighandlers(void);
void
child_sig_handler(int signal, siginfo_t * si, void *p);
void
sig_handler(int signal);

#endif /* SIGNAL_T_H_ */
