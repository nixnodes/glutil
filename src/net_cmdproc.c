/*
 * net_cmdproc.c
 *
 *  Created on: Oct 23, 2015
 *      Author: reboot
 */

#include "hasht.h"
#include "net_io.h"
#include "misc.h"

#include "net_cmdproc.h"

#include <stdlib.h>

void
nc_register (hashtable_t *ht, char *cmd, _ncp call)
{
  size_t cmdlen = strlen (cmd) + 1;

  __nc_proc data = calloc (1, sizeof(_nc_proc));

  if ( NULL == data)
    {
      print_str ("ERROR: nc_register: out of memory\n");
      abort ();
    }

  data->call = call;

  ht_set (ht, (unsigned char*) cmd, cmdlen, (void*) data, sizeof(_nc_proc));
}

__nc_proc
nc_get (hashtable_t *ht, char *cmd)
{
  size_t cmdlen = strlen (cmd) + 1;

  return ht_get (ht, (unsigned char*) cmd, cmdlen);
}

int
nc_proc (hashtable_t *ht, __sock_o pso, char *cmd, char *args)
{
  size_t cmdlen = strlen (cmd) + 1;
  __nc_proc ncp;

  if ( NULL == (ncp = ht_get (ht, (unsigned char*) cmd, cmdlen)))
    {
      return -2;
    }

  return ncp->call (pso, cmd, args);
}
