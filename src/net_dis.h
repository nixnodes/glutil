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
#define		CODE_DIS_NOTIFY		3

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#define		F_DO_FILE 	((uint8_t)1 << 1)
#define		F_DO_DIR 	((uint8_t)1 << 2)

#define		F_DO_TYPE	(F_DO_FILE|F_DO_DIR)

#define		F_DO_INUSE 	((uint8_t)1 << 3)
#define		F_DO_KEEPMEM 	((uint8_t)1 << 4)

#include <pthread.h>

#include "memory_t.h"
#include "hasht.h"

typedef struct ___do
{
  //pthread_mutex_t mutex;
  char *path_c;
  uint8_t flags;
  void *d;
  struct ___do *p_pool;
  p_md_obj link;
} _do, *__do;

#include "g_crypto.h"
#include "net_proto.h"

#define 	F_DH_REQUPD_ALL		((uint8_t)1 << 1)  // m00_8  CODE_DIS_REQUPD

#define 	F_DH_UPDATE_EOS		((uint8_t)1 << 1)  // m00_8  CODE_DIS_NOTIFY

#define 	F_DH_UPDATE_DESTROY	((uint8_t)1 << 2)  // m00_8  CODE_DIS_UPDATE

#pragma pack(push, 4)

typedef struct ___do_fdata
{
  _pid_sha1 digest;
  uint64_t size;
} _do_fd, *__do_fd;

typedef struct ___do_xfdata
{
  _do_fd fd;
} _do_xfd, *__do_xfd;

typedef struct ___do_timespec
{
  uint32_t tv_sec; /* seconds */
  uint32_t tv_nsec; /* nanoseconds */
} _do_ts, *__do_ts;

typedef struct ___do_base_h
{
  uint8_t code, status_flags;
  uint8_t m00_8;
  _do_ts ts;
  uint32_t rand;
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

#define		F_DO_SSTATE_CONNECTED 	(uint32_t) 1 << 1
#define		F_DO_SSTATE_INITSYNC 	(uint32_t) 1 << 2

typedef struct ___do_sockstate
{
  uint32_t status;
  _ipr ipr;
} _do_sst, *__do_sst;

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

#define		DIS_RMSGL_MAX			10000

struct __rmsgl
{
  pthread_mutex_t mutex;
  hashtable_t *ht;
  mda links;
};

#define		F_DIS_ACTIVE		(uint32_t) 1 << 1

struct ___nd_pool_w
{
  pthread_mutex_t mutex;
  _do pool;
};

typedef struct ___dis
{
  pthread_mutex_t mutex;
  _ipr_a hosts[DIS_MAX_HOSTS_GLOBAL];
  mda hosts_linked;
  mda index_linked;
  struct ___nd_pool_w nd_pool;
  _ipr host;
  uint32_t status;
  char root[PATH_MAX];
  uint32_t seed;
  FILE *fh_urandom;
  struct __rmsgl msg_log;
} _dis, *__dis;

_dis di_base;

#define 	F_DUUF_UPDATE_ORIGIN_NETWORK 	(uint32_t) 1 << 1
#define 	F_DUUF_UPDATE_DESTROY	 	(uint32_t) 1 << 2

typedef int
(*d_enum_i_cb) (__do base_pool, void *data);

int
htest ();

int
net_baseline_dis (__sock pso, pmda base, pmda threadr, void *data);

__do_base_h_enc
net_dis_compile_update (int code, char *data, __ipr ipr, size_t ipr_count,
			__do_xfd xfd, uint8_t f);
__do_base_h_enc
net_dis_compile_genreq (int code, uint8_t f, void *data, size_t size);
int
net_dis_socket_init1_accept (__sock pso);
int
d_build_path_index (__do pool, char *out);
int
net_dis_socket_init1_connect (__sock pso);
int
net_dis_socket_dc_cleanup (__sock pso);
__do_base_h_enc
d_assemble_update (__do pool, __do basepool, char *path, uint8_t flags);
int
dis_rescan (void *);
int
d_enum_index (__do base_pool, pmda lindex, void *data, d_enum_i_cb call);
__do
d_lookup_path (char *path, __do pool, uint8_t flags);



#endif /* SRC_NET_DIS_H_ */
