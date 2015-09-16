/*
 * net_fsjproc.c
 *
 *  Created on: Apr 10, 2015
 *      Author: reboot
 */

#include "net_fsjproc.h"

#include "net_io.h"
#include "net_fs.h"
#include "misc.h"

int
net_fsj_socket_rc1_init1 (__sock_o pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;

      if (pso->flags & F_OPSOCK_TERM)
	{
	  print_str (
	      "DEBUG: net_fs_socket_init1_req_xfer: [%d]: skipping initialization (socket shutting down)\n",
	      pso->sock);
	  break;
	}

      __sock_ca ca = (__sock_ca) pso->sock_ca;

      if ( ca->ca_flags & F_CA_MISC00)
	{

	  print_str("DEBUG: net_fs_socket_init1_req_xfer: [%d]: sending xfer request\n", pso->sock);

	  net_fs_initialize_sts(pso);
	  __fs_sts psts = (__fs_sts ) pso->va_p1;

	  snprintf(psts->data0, sizeof(psts->data0), "%s", ca->b3);

	  psts->state |= F_FS_STSOCK_FASSOC|F_FS_STSOCK_XFER_R;
	  psts->stage = FS_STSS_XFER_R_WSTAT;
	  psts->notify_cb = net_baseline_fsproto_xfer_stat_ok;

	  psts->hstat.file_offset = ca->opt0.u00;
	  psts->hstat.file_size = ca->opt0.u01;

	  __fs_rh_enc packet = net_fs_compile_filereq(CODE_FS_RQH_STAT, psts->data0, NULL);

	  if ( NULL == packet)
	    {
	      print_str("ERROR: net_fs_socket_init1_req_xfer: [%d]: could not create xfer request\n", pso->sock);
	      pso->flags |= F_OPSOCK_TERM;
	      break;
	    }

	  int r;
	  if ( (r=net_push_to_sendq(pso, (void*) packet, packet->head.content_length, 0)) )
	    {
	      print_str("ERROR: net_fs_socket_init1_req_xfer: [%d]: net_push_to_sendq failed: [%d]\n", pso->sock, r);
	      pso->flags |= F_OPSOCK_TERM;
	    }

	  free (packet);
	}

      break;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}
