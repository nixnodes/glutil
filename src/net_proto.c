/*
 * net_proto.c
 *
 *  Created on: Mar 9, 2014
 *      Author: reboot
 */
#ifdef _G_SSYS_NET

#include <glutil.h>
#include "net_proto.h"

#include <memory_t.h>
#include <net_io.h>

#include <stdio.h>

static void
net_baseline_respond_protocol_version (__sock_o pso)
{
  int ret;

  char buffer[4];

  snprintf (buffer, 4, "%d.%d", BASELINE_PROTOCOL_VERSION_MAJOR,
  BASELINE_PROTOCOL_VERSION_MINOR);

  if ((ret = net_push_to_sendq (pso, &buffer, 4, 0)) == -1)
    {
      print_str (
	  "ERROR: net_baseline_respond_protocol_version: net_push_to_sendq failed: %d\n",
	  ret);
    }
  else
    {
      print_str ("NOTICE: net_baseline_respond_protocol_version: ok\n");
    }

  pso->flags |= F_OPSOCK_TERM;
}

static int
net_baseline_proc_tier1_req (__sock_o pso, __bp_header bph)
{
  switch (bph->prot_code)
    {
    case PROT_CODE_BASELINE_PROTO_VERSION:
      net_baseline_respond_protocol_version (pso);
      break;
    default:
      return 1;
    }

  return 0;
}

int
net_baseline_prochdr (__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  if (pso->counters.b_read >= 1)
    {
      if (!net_baseline_proc_tier1_req (pso, (__bp_header ) data))
	{
	  pso->counters.b_read = 0;
	  goto end;
	}
    }

  if (pso->counters.b_read < BP_HEADER_SIZE)
    {
      pthread_mutex_unlock (&pso->mutex);
      return -2;
    }

  __bp_header bph = (__bp_header) data;

  _p_s_cb protf = (_p_s_cb) pc_a.objects[bph->prot_code].ptr;

  if (!protf)
    {
      print_str ("ERROR: invalid protocol code %d, socket:[%d]\n", bph->prot_code,
	      pso->sock);
      pthread_mutex_unlock (&pso->mutex);
      return -11;
    }

  if (!bph->content_length) {
      return -12;
  }

  pso->unit_size += bph->content_length;

  if (pso->unit_size > SOCK_RECVB_SZ)
    {
      pthread_mutex_unlock (&pso->mutex);
      return -13;
    }

  pso->rcv1 = protf;

  end: ;

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

void
net_proto_reset_to_baseline (__sock_o pso)
{
  pso->unit_size = BP_HEADER_SIZE;
  pso->rcv1 = (_p_s_cb) net_baseline_prochdr;
  pso->counters.b_read = 0;
}

void
net_proto_na24_copy (_np_netaddr24 *src, _np_netaddr24 *dst)
{
  dst->b_1 = src->b_1;
  dst->b_2 = src->b_2;
  dst->b_3 = src->b_3;
}

#endif

//int net_baseline_response_version
