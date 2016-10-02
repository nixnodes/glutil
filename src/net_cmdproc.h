/*
 * net_cmdproc.h
 *
 *  Created on: Oct 23, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_CMDPROC_H_
#define SRC_NET_CMDPROC_H_

#include "hasht.h"
#include "net_io.h"

typedef int
(*_ncp) (__sock pso, char *cmd, char *args);

typedef struct __net_c_proc
{
  _ncp call;
} _nc_proc, *__nc_proc;

void
nc_register (hashtable_t *ht, char *cmd, _ncp ncp);
__nc_proc
nc_get (hashtable_t *ht, char *cmd);
int
nc_proc (hashtable_t *ht, __sock pso, char *cmd, char *args);


#endif /* SRC_NET_CMDPROC_H_ */
