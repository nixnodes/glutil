/*
 * glutil_net.h
 *
 *  Created on: Nov 24, 2014
 *      Author: reboot
 */
#ifdef _G_SSYS_NET

#ifndef SRC_GLUTIL_NET_H_
#define SRC_GLUTIL_NET_H_

#include <glutil.h>

#include <memory_t.h>

#include <net_io.h>

int
net_deploy(void);

int
net_baseline_gl_data_in(__sock_o pso, pmda base, pmda threadr, void *data);
int
net_gl_socket_init0(__sock_o pso);
int
net_gl_socket_init1(__sock_o pso);



#endif /* SRC_GLUTIL_NET_H_ */

#endif
