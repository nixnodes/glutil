/*
 * glutil_net.h
 *
 *  Created on: Nov 24, 2014
 *      Author: reboot
 */
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
int
net_gl_socket_connect_init1(__sock_o pso);
int
net_gl_socket_post_clean(__sock_o pso);
int
net_gl_socket_pre_clean(__sock_o pso);
int
net_gl_socket_init1_dc_on_ac(__sock_o pso);

int
net_gl_socket_destroy(__sock_o pso);

const char *
net_get_addrinfo_ip(__sock_o pso, char *out, socklen_t len);
uint16_t
net_get_addrinfo_port(__sock_o pso);

#define F_NETOPT_HUSER                  (a64 << 1)
#define F_NETOPT_HGROUP                 (a64 << 2)
#define F_NETOPT_SSLINIT                (a64 << 3)
#define F_NETOPT_CHROOT                 (a64 << 4)

typedef struct ___net_opt
{
  uint16_t thread_l, thread_r;
  int max_worker_threads;
  uint16_t max_sock;
  uint64_t flags;
  char *st_p0;
  char *ssl_cert_def, *ssl_key_def;
  char user[64];
  char group[64];
  char chroot[PATH_MAX];
} _net_opt, *__net_opt;

_net_opt net_opts;

mda _sock_r;
mda _boot_pca;

#endif /* SRC_GLUTIL_NET_H_ */

