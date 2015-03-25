/*
 * net_fs.h
 *
 *  Created on: Mar 25, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_FS_H_
#define SRC_NET_FS_H_

#include <net_proto.h>

#pragma pack(push, 4)

typedef struct ___fs_request_header {
    _net_auth_key key;

} _fs_req_h, *__fs_req_h;

typedef struct __fs_req_h_encaps {
  _bp_header head;
  _fs_req_h request;
} _fs_rh_enc, *__fs_rh_enc;

#pragma pack(pop)

#endif /* SRC_NET_FS_H_ */
