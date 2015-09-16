/*
 * pce_signal.h
 *
 *  Created on: Dec 11, 2013
 *      Author: reboot
 */

#ifndef PCE_SIGNAL_H_
#define PCE_SIGNAL_H_

void
pce_sighdl_error (int sig, siginfo_t* siginfo, void* context);

#endif /* PCE_SIGNAL_H_ */
