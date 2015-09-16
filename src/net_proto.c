/*
 * net_proto.c
 *
 *  Created on: Mar 9, 2014
 *      Author: reboot
 */
#ifdef _G_SSYS_NET

#include "net_proto.h"

#include <stdio.h>

#include "memory_t.h"
#include "net_io.h"
#include "misc.h"

mda pc_a =
  { 0 };

int
net_baseline_socket_init0 (__sock_o pso)
{
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      pso->unit_size = BP_HEADER_SIZE;
      break;
    }

  return 0;
}

int
net_baseline_socket_init1 (__sock_o pso)
{
  char ip[128];
  uint16_t port = net_get_addrinfo_port (pso);
  net_get_addrinfo_ip (pso, (char*) ip, sizeof(ip));

  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      if (pso->flags & F_OPSOCK_IN)
	{
	  print_str ("INFO: [%d] client connected from %s:%hu\n", pso->sock, ip,
		     port);

	}
      else
	{
	  print_str ("INFO: [%d] connected to host %s:%hu\n", pso->sock, ip,
		     port);
	}
      break;
    case SOCKET_OPMODE_LISTENER:
      ;
      print_str ("INFO: [%d]: listening on %s:%hu\n", pso->sock, ip, port);
      break;
    }

  return 0;
}

static void
net_baseline_respond_protocol_version (__sock_o pso)
{
  int ret;

  char buffer[32];

  snprintf (buffer, 32, "%d.%d\n", BASELINE_PROTOCOL_VERSION_MAJOR,
  BASELINE_PROTOCOL_VERSION_MINOR);

  if ((ret = net_send_direct (pso, &buffer, 4)) == -1)
    {
      print_str (
	  "ERROR: net_baseline_respond_protocol_version: net_send_direct failed: %d\n",
	  ret);
    }
  else
    {
      print_str (
	  "DEBUG: net_baseline_respond_protocol_version: net_send_direct ok\n");
    }

  //pso->flags |= F_OPSOCK_TERM;
}

static int
net_baseline_proc_tier1_req (__sock_o pso, __bp_header bph)
{
  switch (bph->prot_code)
    {
    case PROT_CODE_BASELINE_PROTO_VERSION:
      net_baseline_respond_protocol_version (pso);
      break;
    case PROT_CODE_BASELINE_KEEPALIVE:
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

  if (pso->counters.b_read < BP_HEADER_SIZE)
    {
      pthread_mutex_unlock (&pso->mutex);
      return -2;
    }

  if (!net_baseline_proc_tier1_req (pso, (__bp_header ) data))
    {
      pso->counters.b_read = 0;
      goto end;
    }

  __bp_header bph = (__bp_header) data;

  _p_s_cb protf = (_p_s_cb) pc_a.objects[bph->prot_code].ptr;

  if (NULL == protf)
    {
      print_str (
	  "ERROR: net_baseline_prochdr: [%d]: invalid protocol code: %hhu\n",
	  pso->sock, bph->prot_code);
      pthread_mutex_unlock (&pso->mutex);
      return -11;
    }

  if (0 == bph->content_length)
    {
      print_str (
	  "ERROR: net_baseline_prochdr: [%d]: protocol %hhu empty packet\n",
	  pso->sock, bph->prot_code);
      return -12;
    }

  //printf("%d - %d\n", (int) bph->content_length, (int) pso->unit_size);

  if (bph->content_length > pso->buffer0_len)
    {
      print_str (
	  "ERROR: net_baseline_prochdr: [%d]: protocol %d: packet too large\n",
	  pso->sock, bph->prot_code);
      pthread_mutex_unlock (&pso->mutex);
      return -13;
    }

  pso->unit_size = bph->content_length;

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

#endif

//int net_baseline_response_version
