/*
 * arg_opts.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef ARG_OPTS_H_
#define ARG_OPTS_H_

#include <glutil.h>

#define AR_VRP_OPT_NEGATE_MATCH         0x1
#define AR_VRP_OPT_TARGET_FD            0x2

int
print_version_long(void *arg, int m);
int
print_help(void *arg, int m);

#endif /* ARG_OPTS_H_ */
