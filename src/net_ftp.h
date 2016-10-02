/*
 * net_ftp.h
 *
 *  Created on: Oct 23, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_FTP_H_
#define SRC_NET_FTP_H_

#define		NET_FTP_MAX_MSG_SIZE	4096

#include "hasht.h"

#include "net_io.h"
#include "memory_t.h"

hashtable_t *ftp_cmd;

#define 	F_FTP_ST_MODE_PASSIVE	(uint32_t)1 << 1
#define 	F_FTP_ST_MODE_HALTED	(uint32_t)1 << 2
#define 	F_FTP_ST_DATA_COMPLETED	(uint32_t) 1 << 3

#define 	F_FTP_DESTROYED		(uint32_t)1 << 1
#define 	F_FTP_SSLDATA		(uint32_t)1 << 2
#define 	F_FTP_PBSZ		(uint32_t)1 << 3

typedef struct ___fsd_info
{
  //__do pool;  // current pool
  //pthread_mutex_t mutex;
  char cwd[PATH_MAX];
  _ipr l_ip;
  uint32_t status, flags;
  pmda pasv_socks;
  __sock ctl;
  uint8_t type;
  time_t t00;
  _t_rcall task_call;
} _fsd_info, *__fsd_info;

typedef struct _pasv_range
{
  uint16_t low;
  uint16_t high;
} _prange, *__prange;

typedef struct ___ftp_opt
{
  _prange pasv_ports;
} _ftp_opt, *__ftp_opt;

_ftp_opt ftp_opt;

#include <pthread.h>

#define 	F_FTP_STATE_PASV_IP	(uint32_t)1 << 1

typedef struct ___ftp_state
{
  pthread_mutex_t mutex;
  _ipr pasv;
  uint16_t pasv_ports;
  uint32_t flags;
  hashtable_t *used_pasv_ports;
} _ftp_st, *__ftp_st;

_ftp_st ftp_state;

int
net_baseline_ftp (__sock pso, pmda base, pmda threadr, void *data);
int
net_ftp_ctl_socket_init1_accept (__sock pso);
int
net_ftp_ctl_socket_rc0_destroy (__sock pso);
int
net_ftp_msg_send (__sock pso, int code, char *message, ...);
int
net_ftp_msg_send_q (__sock pso, int code, char *message, ...);
void
net_ftp_init (hashtable_t *ht);

#endif /* SRC_NET_FTP_H_ */
