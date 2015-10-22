/*
 * net_dis.h
 *
 *  Created on: Oct 20, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_DIS_H_
#define SRC_NET_DIS_H_

#define 	PROT_CODE_DIS           0xC0

#define		CODE_DIS_UPDATE		1
#define		CODE_DIS_REQUPD		2

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

struct entry_s
{
  char *key;
  void *value;
  struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s
{
  int size;
  struct entry_s **table;
};

typedef struct hashtable_s hashtable_t;

#define		F_DO_FILE 	((uint8_t)1 << 1)
#define		F_DO_DIR 	((uint8_t)1 << 2)

#include <pthread.h>

typedef struct ___do
{
  pthread_mutex_t mutex;
  char *path_c;
  uint8_t flags;
  void *d;
  struct ___do *p_pool;
} _do, *__do;

#include "g_crypto.h"
#include "net_proto.h"

#define 	F_DH_REQUPD_ALL		((uint8_t)1 << 1)  // m00_8

#pragma pack(push, 4)

typedef struct ___do_fdata
{
  _pid_sha1 digest;
  uint64_t size;
} _do_fd, *__do_fd;

typedef struct ___ipr  /* !!ipv4 only */
{
  uint8_t ip[4];
  uint16_t port;
} _ipr, *__ipr;

typedef struct ___do_xfdata
{
  _do_fd fd;
} _do_xfd, *__do_xfd;

typedef struct ___do_base_h
{
  uint8_t code, status_flags;
  uint8_t m00_8;
  int32_t err_code;
  uint32_t ex_len, ex_len1, ex_len2;
} _do_base_h, *__do_base_h;

typedef struct __do_base_h_encaps
{
  _bp_header head;
  _do_base_h body;
} _do_base_h_enc, *__do_base_h_enc;

/* update data */

typedef struct ___do_updex
{
  _do_xfd xfd;
  _pid_sha1 digest;
} _do_updex, *__do_updex;

#pragma pack(pop)

typedef struct ___ipr_a
{
  uint64_t links;
  _ipr ipr;
} _ipr_a, *__ipr_a;

typedef struct ___do_fp
{
  _do_fd fd;
  uint16_t *hosts;
} _do_fp, *__do_fp;

#define  	DIS_MAX_HOSTS_GLOBAL		UINT16_MAX
#define 	DIS_MAX_HLINKS_PER_FILE		16

#include "memory.h"

#define		F_DIS_ACTIVE		(uint32_t) 1 << 1

typedef struct ___dis {
  pthread_mutex_t mutex;
  _ipr_a hosts[DIS_MAX_HOSTS_GLOBAL];
  mda hosts_linked;
  mda index_linked;
  _do nd_pool;
  _ipr host;
  uint32_t status;
  char root[PATH_MAX];
} _dis, *__dis;

_dis di_base;


typedef int
(*d_enum_i_cb) (__do base_pool, void *data);

int
htest ();

int
net_baseline_dis (__sock_o pso, pmda base, pmda threadr, void *data);
int
net_addr_to_ipr (__sock_o pso, __ipr out);
__do_base_h_enc
net_dis_compile_update (int code, char *data, __ipr ipr, size_t ipr_count,
			__do_xfd xfd);
__do_base_h_enc
net_dis_compile_genreq (int code, uint8_t f, void *data, size_t size);
int
net_dis_socket_init1_accept (__sock_o pso);
int
d_build_path_index (__do pool, char *out);
int
net_dis_socket_init1_connect (__sock_o pso);
__do_base_h_enc
d_assemble_update (__do pool, char *path);
int
dis_rescan (void *);

hashtable_t *
ht_create (int size);

#endif /* SRC_NET_DIS_H_ */
