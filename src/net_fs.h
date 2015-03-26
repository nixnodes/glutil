/*
 * net_fs.h
 *
 *  Created on: Mar 25, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_FS_H_
#define SRC_NET_FS_H_

#define PROT_CODE_FS            0xAC

#include <net_proto.h>

#include <sys/types.h>
#include <limits.h>

#define CODE_FS_RQH_SEND        1
#define CODE_FS_RQH_STAT        2
#define CODE_FS_RESP_NOTIFY     3
#define CODE_FS_RQH_S_RESP      4

#define F_RQH_OP_FAILED         ((uint8_t)1 << 1)
#define F_RQH_OP_OK             ((uint8_t)1 << 2)

#pragma pack(push, 4)

typedef struct ___fs_request_header
{
  _net_auth_key key;
  uint8_t code, status_flags;
  uint8_t m00_8, m01_8;
  int32_t err_code;
  uint64_t offset, size;
  uint32_t ex_len;
} _fs_req_h, *__fs_req_h;

typedef struct __fs_req_h_encaps
{
  _bp_header head;
  _fs_req_h body;
} _fs_rh_enc, *__fs_rh_enc;

typedef struct ___fs_stat_header
{
  int32_t m_tim;
  uint64_t size;
} _fs_hstat, *__fs_hstat;

#pragma pack(pop)

#define F_FS_STSOCK_XFER_R              ((uint64_t)1 << 1)
#define F_FS_STSOCK_FASSOC              ((uint64_t)1 << 2)

#define FS_STSS_XFER_R_WSTAT            1
#define FS_STSS_XFER_R_WDATA            2

typedef struct ___fs_state_sock
{
  uint64_t state;
  uint16_t stage;
  _fs_hstat hstat;
  uint64_t file_offset, file_size;
  char data0[PATH_MAX], data1[PATH_MAX];
} _fs_sts, *__fs_sts;

int
net_baseline_fsproto(__sock_o pso, pmda base, pmda threadr, void *data);
__fs_rh_enc
net_fs_compile_filereq(int code, char *path, void *arg);
__fs_rh_enc
net_fs_compile_hstat(__fs_hstat data, void *arg);

int
net_fs_socket_init1_rqstat(__sock_o pso);

#endif /* SRC_NET_FS_H_ */
