/*
 * net-io.h
 *
 *  Created on: Dec 29, 2013
 *      Author: reboot
 */
#ifdef _G_SSYS_NET

#ifndef NET_IO_H_
#define NET_IO_H_

#include <glutil.h>

#define F_OPSOCK_CONNECT                (a32 << 1)
#define F_OPSOCK_LISTEN                 (a32 << 2)
#define F_OPSOCK_TERM                   (a32 << 3)
#define F_OPSOCK_DISCARDED              (a32 << 4)
#define F_OPSOCK_SSL                    (a32 << 5)
#define F_OPSOCK_ACT                    (a32 << 6)
#define F_OPSOCK_ST_SSL_ACCEPT          (a32 << 7)
#define F_OPSOCK_ST_SSL_CONNECT         (a32 << 8)
#define F_OPSOCK_ST_CLEANUP_READY       (a32 << 9)
#define F_OPSOCK_TS_DISCONNECTED        (a32 << 10)
#define F_OPSOCK_ST_HOOKED              (a32 << 11)
#define F_OPSOCK_INIT_SENDQ             (a32 << 12)
#define F_OPSOCK_PROC_READY             (a32 << 13)
#define F_OPSOCK_SD_FIRST_DC            (a32 << 14)

#define F_OPSOCK_CREAT_MODE             (F_OPSOCK_CONNECT|F_OPSOCK_LISTEN)
#define F_OPSOCK_STATES                 (F_OPSOCK_ST_SSL_ACCEPT|F_OPSOCK_ST_SSL_CONNECT)

#define F_MODE_GHS                      (a32 << 1)

#define SOCK_RECVB_SZ                   8192
#define SOCK_DEFAULT_IDLE_TIMEOUT       60
#define SOCK_SSL_ACCEPT_TIMEOUT         60
#define SOCK_SSL_CONNECT_TIMEOUT        60

#define SOCKET_POOLING_FREQUENCY_MAX    250000
#define SOCKET_POOLING_FREQUENCY_MIN    10
#define SOCKET_POOLING_FREQUENCY_HIRES  1

#define SOCKET_SSL_MAX_READ_RETRY       1000
#define SOCKET_SSL_READ_RETRY_TIMEOUT   1000

#define THREAD_ROLE_NET_WORKER          0x1

#define SOCKET_OPMODE_LISTENER          0x1
#define SOCKET_OPMODE_RECIEVER          0x2

#define F_WORKER_INT_STATE_ACT          (a8 << 1)
#define F_WORKER_INT_STATE_ACT_HIRES    (a8 << 2)

#define NET_PUSH_SENDQ_ASSUME_PTR	(a16 << 1)

#include <memory_t.h>
#include <thread.h>

#include <pthread.h>

#include <sys/types.h>
#include <stdint.h>
#include <openssl/ssl.h>

typedef int
(*_p_s_cb)(void *, void *, void *, void *);
typedef int
(*_t_stocb)(void *);
typedef int
(*_p_ssend)(void*, void *, size_t length);

typedef struct __sock_timers
{
  time_t last_act, last_proc, misc00, misc01;
} _sock_tm;

typedef struct __sock_timeouts
{
  time_t sock_timeout;
} _sock_to;

typedef struct __sock_counters
{
  ssize_t b_read, t_read;
} _sock_c;

typedef struct __sock_sendq_payload
{
  size_t size;
  void *data;
} _sock_sqp, *__sock_sqp;

#define SSENDQ_PAYLOAD_SIZE		sizeof(struct __sock_sendq_payload)

typedef struct ___sock_o
{
  int sock;
  uint32_t flags, opmode;
  _p_s_cb rcv_cb, rcv_cb_t, rcv0, rcv1, rcv1_t;
  _t_stocb rc0, rc1, shutdown_cleanup;
  _p_ssend send0;
  _t_stocb pcheck_r;
  struct addrinfo *res;
  void *parent, *cc;
  pmda host_ctx;
  ssize_t unit_size;
  _sock_c counters;
  uint16_t oper_mode;
  uint32_t status;
  SSL_CTX *ctx;
  SSL *ssl;
  int s_errno;
  _sock_tm timers;
  _sock_to limits;
  void *buffer0;
  mda sendq;
  void *ptr0;
  pthread_mutex_t mutex;
  void *va_p0;
  void *st_p0, *st_p1;
} _sock_o, *__sock_o;

typedef int
(*_p_sc_cb)(__sock_o sock_o);

typedef int
p_sc_cb(__sock_o sock_o);

typedef int
(p_s_cb)(__sock_o spso, pmda base, pmda threadr, void *data);

typedef int
(*__p_s_cb)(__sock_o spso, pmda base, pmda threadr, void *data);

/*
 * int
 net_open_listening_socket (char *addr, char *port, uint32_t flags, _p_sc_cb rc,
 _p_sc_cb proc, pmda sockr, char *ssl_cert,
 char *ssl_key)
 * */

#define F_CA_HAS_LOG               (a32 << 1)
#define F_CA_HAS_SSL_CERT          (a32 << 2)
#define F_CA_HAS_SSL_KEY           (a32 << 3)

typedef struct ___sock_create_args
{
  char *host, *port;
  uint32_t flags, ca_flags;
  _p_sc_cb rc0, rc1, proc;
  pmda socket_register, thread_register;
  char *ssl_cert;
  char *ssl_key;
  ssize_t unit_size;
  void *st_p0;
  char b0[4096];
  char b1[PATH_MAX];
  char b2[PATH_MAX];
  uint8_t mode;
} _sock_ca, *__sock_ca;

p_sc_cb rc_tst, rc_ghs;

int
net_connect_socket(int fd, struct addrinfo *aip);
int
bind_socket(int fd, struct addrinfo *aip);
int
check_socket_event(__sock_o pso);
int
net_worker(void *args);

void
net_nw_ssig_term_r(pmda objects);

void
ssl_init(void);

p_s_cb net_recv, net_recv_ssl, net_accept_ssl, net_accept, net_connect_ssl;

int
net_open_listening_socket(char *addr, char *port, __sock_ca args);
int
net_open_connection(char *addr, char *port, __sock_ca args);
float
net_get_score(pmda in, pmda out, __sock_o pso, po_thrd thread);
void
net_send_sock_term_sig(__sock_o pso);
int
net_ssend_b(__sock_o pso, void *data, size_t length);
int
net_ssend_ssl_b(__sock_o pso, void *data, size_t length);
int
net_ssend_ssl(__sock_o pso, void *data, size_t length);
int
net_ssend(__sock_o pso, void *data, size_t length);
int
net_link_sockets(__sock_o pso1, __sock_o pso2);
int
net_link_with_all_registered_sockets(pmda sockr, __sock_o pso,
    uint16_t match_oper_mode);
int
net_push_to_sendq(__sock_o pso, void *data, size_t size, uint16_t flags);
int
net_sendq_broadcast(pmda base, __sock_o source, void *data, size_t size);
int
net_send_direct(__sock_o pso, const void *data, size_t size);

#endif

#endif
