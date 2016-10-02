/*
 * net-io.h
 *
 *  Created on: Dec 29, 2013
 *      Author: reboot
 */
#ifdef _G_SSYS_NET

#ifndef NET_IO_H_
#define NET_IO_H_

#define F_OPSOCK_CONNECT                ((uint32_t)1 << 1)
#define F_OPSOCK_LISTEN                 ((uint32_t)1 << 2)
#define F_OPSOCK_TERM                   ((uint32_t)1 << 3)
#define F_OPSOCK_DISCARDED              ((uint32_t)1 << 4)
#define F_OPSOCK_SSL                    ((uint32_t)1 << 5)
#define F_OPSOCK_ACT                    ((uint32_t)1 << 6)
#define F_OPSOCK_ST_SSL_ACCEPT          ((uint32_t)1 << 7)
#define F_OPSOCK_ST_SSL_CONNECT         ((uint32_t)1 << 8)
#define F_OPSOCK_ST_CLEANUP_READY       ((uint32_t)1 << 9)
#define F_OPSOCK_TS_DISCONNECTED        ((uint32_t)1 << 10)
#define F_OPSOCK_ST_HOOKED              ((uint32_t)1 << 11)
#define F_OPSOCK_INIT_SENDQ             ((uint32_t)1 << 12)
#define F_OPSOCK_PROC_READY             ((uint32_t)1 << 13)
#define F_OPSOCK_SD_FIRST_DC            ((uint32_t)1 << 14)
#define F_OPSOCK_HALT_RECV              ((uint32_t)1 << 15)
#define F_OPSOCK_SKIP_SSL_SD            ((uint32_t)1 << 16)
#define F_OPSOCK_SSL_KEYCERT_L          ((uint32_t)1 << 17)
#define F_OPSOCK_IN                     ((uint32_t)1 << 18)
#define F_OPSOCK_PERSIST                ((uint32_t)1 << 19)
#define F_OPSOCK_ORPHANED               ((uint32_t)1 << 20)
#define F_OPSOCK_CS_MONOTHREAD          ((uint32_t)1 << 21)
#define F_OPSOCK_CS_NOASSIGNTHREAD      ((uint32_t)1 << 22)
#define F_OPSOCK_DETACH_THREAD          ((uint32_t)1 << 23)
#define F_OPSOCK_SIGNALING_INIT         ((uint32_t)1 << 24)
#define F_OPSOCK_NOKILL            	((uint32_t)1 << 25)
#define F_OPSOCK_ERROR            	((uint32_t)1 << 26)

#define F_OPSOCK_OPER_MODE              (F_OPSOCK_CONNECT|F_OPSOCK_LISTEN)
#define F_OPSOCK_STATES                 (F_OPSOCK_ST_SSL_ACCEPT|F_OPSOCK_ST_SSL_CONNECT)

#define F_MODE_GHS                      ((uint32_t)1 << 1)

#define SOCK_RECVB_SZ                   32768 + 256
#define SOCK_DEFAULT_IDLE_TIMEOUT       60
#define SOCK_SSL_ACCEPT_TIMEOUT         60
#define SOCK_SSL_CONNECT_TIMEOUT        60

#define SOCKET_POOLING_FREQUENCY_MAX    50000
#define SOCKET_POOLING_FREQUENCY_MIN    1
#define SOCKET_POOLING_FREQUENCY_HIRES  1

#define SOCKET_SSL_MAX_READ_RETRY       1000
#define SOCKET_SSL_READ_RETRY_TIMEOUT   1000

#define THREAD_ROLE_NET_WORKER          0x1

#define SOCKET_OPMODE_LISTENER          0x1
#define SOCKET_OPMODE_RECIEVER          0x2

#define F_WORKER_INT_STATE_ACT          ((uint8_t)1 << 1)
#define F_WORKER_INT_STATE_ACT_HIRES    ((uint8_t)1 << 2)

#define NET_PUSH_SENDQ_ASSUME_PTR	((uint16_t)1 << 1)

#include <memory_t.h>
#include <thread.h>

#include <pthread.h>

#include <limits.h>
#include <sys/types.h>
#include <stdint.h>
#include <openssl/ssl.h>

#pragma pack(push, 4)

typedef struct ___ipr
{
  uint8_t ip[16];
  uint16_t port;
} _ipr, *__ipr;

#pragma pack(pop)

typedef int
(*_p_s_cb) (void *, void *, void *, void *);
typedef int
(*_t_rcall) (void *);
typedef int
(*_p_ssend) (void*, void *, size_t length);

#define F_ST_MISC00_ACT         ((uint32_t)1 << 1)
#define F_ST_MISC02_ACT         ((uint32_t)1 << 3)

typedef struct __sock_timers
{
  time_t last_act, last_proc, misc00, misc01, misc02, misc03;
  uint32_t flags;
} _sock_tm;

typedef struct __sock_timeouts
{
  time_t sock_timeout;
} _sock_to;

typedef struct __sock_counters
{
  ssize_t b_read, t_read, session_read, session_write, total_write;
} _sock_c;

typedef struct __sock_sendq_payload
{
  size_t size;
  void *data;
} _sock_sqp, *__sock_sqp;

#define SSENDQ_PAYLOAD_SIZE		sizeof(struct __sock_sendq_payload)

#include <netdb.h>

typedef struct ___proc_ic_o
{
  _t_rcall call;
  uint32_t flags;
} _proc_ic_o, *__proc_ic_o;

typedef struct ___sock_policy
{
  uint32_t max_sim_ip, max_connects;
  time_t idle_timeout, listener_idle_timeout, connect_timeout, accept_timeout,
      close_timeout, ssl_accept_timeout, ssl_connect_timeout, send_timeout;
  uint8_t mode;
  int ssl_verify;
} _net_sp, *__net_sp;

struct __sock_common_args
{
  uint32_t thread_inherit_flags;
  uint32_t thread_flags;
};

typedef struct ___sock_o
{
  int sock;
  uint32_t flags, ac_flags, opmode;
  _p_s_cb rcv_cb, rcv_cb_t, rcv0, rcv1, rcv1_t;
  uint32_t children;
  mda init_rc0, init_rc1;
  mda init_rc0_ssl;
  mda shutdown_rc0, shutdown_rc1;
  _p_ssend send0;
  _t_rcall pcheck_r;
  struct addrinfo res;
  void *cc;
  struct ___sock_o *parent;
  pmda host_ctx;
  ssize_t unit_size;
  _sock_c counters;
  uint16_t oper_mode;
  uint8_t mode;
  uint32_t status;
  SSL_CTX *ctx;
  SSL *ssl;
  int s_errno;
  int sslerr;
  _sock_tm timers;
  _sock_to limits;
  void *buffer0;
  size_t buffer0_len;
  mda sendq;
  void *ptr0;
  pthread_mutex_t mutex;
  void *va_p0, *va_p1, *va_p2, *va_p3;
  void *st_p0;
  void *st_p1; // thread-specific buffer
  _net_sp policy;
  void *sock_ca;
  _ipr ipr;
  pthread_t thread;
  po_thrd pthread;
  struct __sock_common_args common;
  mda tasks;
} _sock, *__sock;

typedef struct ___sock_cret
{
  __sock pso;
  uint32_t ret;
} _sock_cret, *__sock_cret;

typedef int
(*_p_sc_cb) (__sock sock_o);

typedef int
p_sc_cb (__sock sock_o);

typedef int
(p_s_cb) (__sock spso, pmda base, pmda threadr, void *data);

typedef int
(*__p_s_cb) (__sock spso, pmda base, pmda threadr, void *data);

typedef int
(*_p_enumsr_cb) (__sock sock_o, void *arg);

typedef int
p_enumsr_cb (__sock sock_o, void *arg);

/*
 * int
 net_open_listening_socket (char *addr, char *port, uint32_t flags, _p_sc_cb rc,
 _p_sc_cb proc, pmda sockr, char *ssl_cert,
 char *ssl_key)
 * */

#define F_CA_HAS_LOG               ((uint32_t)1 << 1)
#define F_CA_HAS_SSL_CERT          ((uint32_t)1 << 2)
#define F_CA_HAS_SSL_KEY           ((uint32_t)1 << 3)
#define F_CA_MISC00                ((uint32_t)1 << 10)
#define F_CA_MISC01                ((uint32_t)1 << 11)
#define F_CA_MISC02                ((uint32_t)1 << 12)
#define F_CA_MISC03                ((uint32_t)1 << 13)
#define F_CA_MISC04                ((uint32_t)1 << 14)

typedef struct ___sock_create_args
{
  char *host, *port;
  uint32_t flags, ca_flags, ac_flags;
  _p_sc_cb proc;
  pmda socket_register, socket_register_ac, thread_register;
  char *ssl_cert;
  char *ssl_key;
  ssize_t unit_size;
  void *st_p0;
  char b0[4096];
  char b1[PATH_MAX];
  char b2[PATH_MAX];
  char b3[PATH_MAX];
  char b4[64];
  char b5[64];
  uint8_t mode;
  _net_sp policy;
  mda init_rc0, init_rc1;
  mda init_rc0_ssl;
  mda shutdown_rc0, shutdown_rc1;
  _nn_2x64 opt0;
  _ipr ipr00;
  struct __sock_common_args common;
  int ref_id;
  int
  (*scall) (char *addr, char *port, struct ___sock_create_args *args);
  void *va_p3;
} _sock_ca, *__sock_ca;

typedef struct ___net_task
{
  int
  (*net_task_proc) (__sock pso, struct ___net_task *task);
  void *data;
  uint16_t flags;
} _net_task, *__net_task;

typedef int
(*_net_task_proc) (__sock pso, struct ___net_task *task);

int
net_register_task (pmda host_ctx, __sock pso, pmda rt, _net_task_proc proc,
		   void *data, uint16_t flags);
int
net_proc_worker_tasks (__sock pso);

p_sc_cb rc_tst, rc_ghs, net_socket_init_enforce_policy;

#define F_NET_BROADCAST_SENDQ	(uint32_t) 1 << 1

typedef int
(*_net_bc) (__sock pso, void *arg, void *data);

int
net_broadcast (pmda base, void *data, size_t size, _net_bc net_bc, void *arg,
	       uint32_t flags);

int
net_connect_socket (int fd, struct addrinfo *aip);
int
bind_socket (int fd, struct addrinfo *aip);
int
check_socket_event (__sock pso);
void*
net_worker (void *args);
void *
net_worker_mono (void *args);
void
net_worker_dispatcher (int signal);

int
net_enum_sockr (pmda base, _p_enumsr_cb p_ensr_cb, void *arg);

void
net_nw_ssig_term_r (pmda objects);
void
net_nw_ssig_term_r_ex (pmda objects, uint32_t flags);

void
ssl_init (void);
void
ssl_cleanup (void);

p_s_cb net_recv, net_recv_ssl, net_accept_ssl, net_accept, net_connect_ssl;

int
net_open_listening_socket (char *addr, char *port, __sock_ca args);
int
net_open_listening_socket_e (char *addr, char *port, __sock_ca args,
			     pthread_t *pt_ret);
__sock
net_open_listening_socket_bare (char *addr, char *port, __sock_ca args);
int
net_open_connection (char *addr, char *port, __sock_ca args);
int
net_destroy_connection (__sock so);
float
net_get_score (pmda in, pmda out, __sock pso, po_thrd thread);
void
net_send_sock_sigterm (__sock pso);
int
net_ssend_b (__sock pso, void *data, size_t length);
int
net_ssend_ssl_b (__sock pso, void *data, size_t length);
int
net_ssend_ssl (__sock pso, void *data, size_t length);
int
net_ssend (__sock pso, void *data, size_t length);
int
net_link_sockets (__sock pso1, __sock pso2);
int
net_link_with_all_registered_sockets (pmda sockr, __sock pso,
				      uint16_t match_oper_mode);
int
net_push_to_sendq (__sock pso, void *data, size_t size, uint16_t flags);
int
net_sendq_broadcast (pmda base, __sock source, void *data, size_t size);
int
net_send_direct (__sock pso, const void *data, size_t size);
int
net_pop_rc (__sock pso, pmda rc);
int
net_push_rc (pmda rc, _t_rcall call, uint32_t flags);
const char *
net_get_addrinfo_ip_str (__sock pso, char *out, socklen_t len);
uint16_t
net_get_addrinfo_port (__sock pso);
int
net_addr_to_ipr (__sock pso, __ipr out);

typedef int
(*_ne_sock) (__sock pso, void *arg);

int
net_exec_sock (pmda base, __ipr ipr, uint32_t flags, _ne_sock call, void *arg);

int
net_generic_socket_init1 (__sock pso);
int
net_generic_socket_destroy0 (__sock pso);
int
net_parent_proc_rc0_destroy (__sock pso);
int
net_join_threads (pmda base);

#define		F_NW_STATUS_WAITSD	(uint32_t)1 << 1
#define		F_NW_STATUS_SOCKSD	(uint32_t)1 << 2

#define		F_NW_NO_SOCK_KILL	(uint32_t)1 << 1
#define		F_NW_HALT_PROC		(uint32_t)1 << 2
#define		F_NW_HALT_SEND		(uint32_t)1 << 3

p_md_obj
net_worker_process_socket (__sock pso, p_md_obj ptr, uint8_t *int_state,
			   uint32_t flags, po_thrd thrd, pid_t *_tid,
			   char *buffer0, uint32_t *status_flags);
uint32_t
net_proc_worker_detached_socket (__sock pso, uint32_t flags);
void
net_worker_cleanup_socket (__sock pso);

#endif

#endif
