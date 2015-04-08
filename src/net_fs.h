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

typedef struct ___fs_tmstat_header
{
  int32_t sec, nsec;
} _fs_tms, *__fs_tms;

typedef struct ___fs_stat_header
{
  uint64_t size;
  uint32_t uid, gid;
  _fs_tms mtime, ctime, atime;
  uint64_t file_offset, file_size;
} _fs_hstat, *__fs_hstat;

#pragma pack(pop)

#define F_FS_STSOCK_XFER_R              ((uint64_t)1 << 1)
#define F_FS_STSOCK_XFER_W              ((uint64_t)1 << 2)
#define F_FS_STSOCK_FASSOC              ((uint64_t)1 << 3)
#define F_FS_STSOCK_XFER_FIN            ((uint64_t)1 << 4)

#define FS_STSS_XFER_R_WSTAT            1
#define FS_STSS_XFER_R_WDATA            2
#define FS_STSS_XFER_RECV               3
#define FS_STSS_XFER_SEND               4
#define FS_STSS_XFER_R_WSHA             5

typedef int
(*_nfs_ncb)(__sock_o pso, __fs_rh_enc packet, void *arg);
typedef int
nfs_ncb(__sock_o pso, __fs_rh_enc packet, void *arg);

#include <g_crypto.h>

typedef struct ___sha_v
{
  SHA_CTX context;
  _pid_sha1 value;
} _sha_v, *__sha_v;

typedef struct ___fs_state_sock
{
  uint64_t state;
  uint16_t stage;
  _fs_hstat hstat;
  int handle;
  _nfs_ncb notify_cb;
  uint64_t data_in, data_out;
  char data0[PATH_MAX], data1[PATH_MAX];
  _sha_v sha_00;
} _fs_sts, *__fs_sts;

#define BASELINE_FS_TCODE_XFER       "XFER"

nfs_ncb net_baseline_fsproto_xfer_stat_ok, net_baseline_fsproto_xfer_in_ok,
    net_baseline_fsproto_default;

int
net_baseline_fsproto(__sock_o pso, pmda base, pmda threadr, void *data);
int
net_baseline_fsproto_send(__sock_o pso, pmda base, pmda threadr, void *data);
int
net_baseline_fsproto_recv(__sock_o pso, pmda base, pmda threadr, void *data);
__fs_rh_enc
net_fs_compile_filereq(int code, char *data, void *arg);
__fs_rh_enc
net_fs_compile_breq(int code, unsigned char *data, size_t p_len, void *arg);
__fs_rh_enc
net_fs_compile_hstat(__fs_hstat data, void *arg);

int
net_fs_socket_init1_req_xfer(__sock_o pso);

#endif /* SRC_NET_FS_H_ */
