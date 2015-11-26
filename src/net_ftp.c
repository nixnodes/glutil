/*
 * net_ftp.c
 *
 *  Created on: Oct 23, 2015
 *      Author: reboot
 */

#include "net_io.h"
#include "net_cmdproc.h"
#include "misc.h"
#include "str.h"
#include "net_dis.h"
#include "thread.h"
#include "taskd.h"

#include "net_ftp.h"

#include <errno.h>
#include <signal.h>

hashtable_t *ftp_cmd = NULL;

_ftp_opt ftp_opt =
  { .pasv_ports =
    { .low = 21500, .high = 31500 } };

_ftp_st ftp_state =
  {
    {
      { 0 } } };

static int
net_ftp_cmd_user (__sock_o pso, char *cmd, char *args)
{
  return net_ftp_msg_send (pso, 331, "Password required for %s", args);
}

static int
net_ftp_cmd_pass (__sock_o pso, char *cmd, char *args)
{
  return net_ftp_msg_send (pso, 230, "User logged in");
}

static int
net_ftp_cmd_syst (__sock_o pso, char *cmd, char *args)
{
  return net_ftp_msg_send (pso, 215, "UNIX Type: L8");
}

static int
net_ftp_cmd_quit (__sock_o pso, char *cmd, char *args)
{

  net_ftp_msg_send (pso, 200, "Bye");

  pso->flags |= F_OPSOCK_TERM;

  return 0;
}

static int
net_ftp_cmd_cwd (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  char pb[PATH_MAX];

  if ( args[0] != 0x2F )
    {
      snprintf(pb, sizeof(pb), "%s/%s", fsdi->cwd, args);
    }
  else
    {
      snprintf(pb, sizeof(pb), "%s", args);
    }

  remove_repeating_chars(pb, 0x2F);

  if (!(pb[0] == 0x2F && pb[1] == 0x0))
    {
      size_t plen = strlen(pb);
      plen--;

      if ( pb[plen] == 0x2F)
	{
	  pb[plen] = 0x0;
	}

    }

  if ( 0 == strlen(pb))
    {
      return net_ftp_msg_send (pso, 500, "Command not understood.");
    }

  //mutex_lock (&di_base.nd_pool.mutex);

  __do pool = d_lookup_path(pb, &di_base.nd_pool.pool, 0);

  if ( NULL == pool || (pool->flags & F_DO_FILE))
    {
      //pthread_mutex_unlock (&di_base.nd_pool.mutex);
      net_ftp_msg_send (pso, 550, "No such file or directory.");
      return 1;
    }
  else
    {
      snprintf (fsdi->cwd, sizeof(fsdi->cwd), "%s", pb);

      net_ftp_msg_send (pso, 257, "\x22%s\x22 is current directory.", args);
      //pthread_mutex_unlock (&di_base.nd_pool.mutex);
    }

  return 0;
}

static int
net_ftp_cmd_cdup (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  g_dirname_p(fsdi->cwd);

  net_ftp_msg_send (pso, 257, "\x22%s\x22 is current directory.", fsdi->cwd);

  return 0;
}

static int
net_ftp_cmd_pwd (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  return net_ftp_msg_send (pso, 257, "\x22%s\x22 is current directory.", fsdi->cwd);
}

static int
net_ftp_cmd_pbsz (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( NULL == args || 0 == strlen(args))
    {
      return net_ftp_msg_send (pso, 500, "Command not understood.");
    }

  if ( 0x30 == args[0] && 0 == args[1] )
    {
      fsdi->flags |= F_FTP_PBSZ;
      return net_ftp_msg_send (pso, 200, "%s %s successful", cmd, args);
    }
  else
    {
      return net_ftp_msg_send (pso, 500, "'%s %s': Command not understood.", cmd, args);
    }
}

static void
net_ftp_cleanup_pasv_host (__sock_o pso)
{
  //int ret = 0;

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if (fsdi->status & F_FTP_ST_MODE_PASSIVE)
    {
      //net_nw_ssig_term_r (fsdi->pasv_socks);
      fsdi->status ^= F_FTP_ST_MODE_PASSIVE;
    }
}

static int
net_ftp_cmd_prot (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( NULL == args || 0 == strlen(args))
    {
      return net_ftp_msg_send (pso, 500, "Command not understood.");
    }

  if ( 0x50 == args[0] && 0 == args[1] )
    {
      if ( fsdi->flags & F_FTP_PBSZ )
	{
	  if ( !(fsdi->flags & F_FTP_SSLDATA))
	    {
	      fsdi->flags |= F_FTP_SSLDATA;
	      //net_nw_ssig_term_r(fsdi->pasv_socks);
	      fsdi->status ^= fsdi->status & F_FTP_ST_MODE_PASSIVE;
	    }
	  return net_ftp_msg_send (pso, 200, "Protection set to Private", cmd, args);
	}
      else
	{
	  return net_ftp_msg_send (pso, 500, "Issue PBSZ 0 first.");
	}
    }
  /*else if ( 0x43 == args[0] && 0 == args[1] )
   {
   if ( (fsdi->flags & F_FTP_SSLDATA))
   {
   fsdi->flags ^= F_FTP_SSLDATA;
   net_nw_ssig_term_r(fsdi->pasv_socks);
   fsdi->status ^= fsdi->status & F_FTP_ST_MODE_PASSIVE;
   }

   return net_ftp_msg_send (pso, 200, "Protection set to Clear", cmd, args);
   }*/
  else
    {
      return net_ftp_msg_send (pso, 500, "'%s %s': Command not understood.", cmd, args);
    }
}

static int
net_ftp_cmd_type (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( NULL == args || 0 == strlen(args) )
    {
      return net_ftp_msg_send (pso, 500, "'%s': Command not understood.", fsdi->cwd);
    }

  if ( 0x49 == args[0] && 0 == args[1])
    {
      fsdi->type = 1;
      return net_ftp_msg_send (pso, 200, "Type set to %c.", args[0]);
    }
  else if ( 0x41 == args[0] && 0 == args[1])
    {
      fsdi->type = 2;
      return net_ftp_msg_send (pso, 200, "Type set to %c.", args[0]);
    }
  else
    {
      return net_ftp_msg_send (pso, 500, "'%s %s': Command not understood.", cmd, args);
    }
}

static int
net_ftp_pasv_host_do_endgame (__sock_o pso)
{
  if (!(pso->flags & F_OPSOCK_CONNECT))
    {
      return 0;
    }

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( NULL == fsdi->task_call)
    {
      print_str ("WARNING: net_ftp_pasv_host_do_endgame: nothing to do\n");
      return 1;
    }

  return fsdi->task_call (pso);

}

#define FTP_LIST_TIMEOUT	30

static int
net_ftp_list_task (__sock_o pso)
{

  if (pso->flags & F_OPSOCK_ERROR)
    {
      return 0;
    }

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  mutex_lock (&di_base.nd_pool.mutex);

  __do pool = d_lookup_path (fsdi->cwd, &di_base.nd_pool.pool, 0);

  if ( NULL == pool)
    {
      pthread_mutex_unlock (&di_base.nd_pool.mutex);
      net_ftp_msg_send (fsdi->ctl, 550,
			"Requested action not taken. (path unavailable)");
      goto exit;
    }

  size_t i = 0;

  hashtable_t *pool_ht = (hashtable_t *) pool->d;

  char buffer[8192];

  while (i < pool_ht->size)
    {
      entry_t *ptr = pool_ht->table[i];
      if ( NULL == ptr || ptr->key == NULL)
	{
	  goto loop_end;
	}

      while (ptr != NULL && ptr->key != NULL)
	{
	  __do pool_l = (__do) ptr->value;

	  switch ( pool_l->flags & F_DO_TYPE)
	    {
	      case F_DO_FILE:;

	      __do_fp pool_l_fp = (__do_fp) pool_l->d;

	      snprintf(buffer, sizeof(buffer), "-rw-r--r-- 1 nobody nobody %lu Dec 1 1970 %s\r\n", pool_l_fp->fd.size, pool_l->path_c);

	      net_send_direct (pso, buffer, strlen (buffer) );

	      break;
	      case F_DO_DIR:;

	      snprintf(buffer, sizeof(buffer), "drwxrwxrwx 1 nobody nobody %d Dec 1 1970 %s\r\n", (int)sizeof(_do), pool_l->path_c);

	      net_send_direct (pso, buffer, strlen (buffer) );

	      break;

	    }

	  ptr = ptr->next;
	}

      loop_end: ;

      i++;
    }

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  exit: ;

  pso->flags |= F_OPSOCK_TERM;

  return 0;
}

#define		NET_FTP_MAX_PASV_HOST_WAIT_TIME		30

static int
net_ftp_pasv_check_dead_ctl (__sock_o pso, __net_task task)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( time(NULL) - fsdi->t00 > NET_FTP_MAX_PASV_HOST_WAIT_TIME )
    {
      pso->flags |= F_OPSOCK_DETACH_THREAD;
    }

  return 1;
}

#include <unistd.h>
#include <sys/syscall.h>

static int
net_ftp_pasv_process_ctl (__sock_o pso, __net_task task)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  net_proc_worker_detached_socket (fsdi->ctl, F_NW_NO_SOCK_KILL|F_NW_HALT_PROC|F_NW_HALT_SEND);

  if (fsdi->ctl->flags & F_OPSOCK_TERM)
    {
      pso->flags |= F_OPSOCK_TERM;
    }

  return 1;
}

static int
net_ftp_handle_pasv_host (__sock_o pso)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( fsdi->pasv_socks->offset < 1 || NULL == fsdi->pasv_socks->first )
    {
      net_ftp_msg_send (
	  pso, 425,
	  "Can't open data connection. (%s : %d)", "no listeners", (int)fsdi->pasv_socks->offset );
      return 1;
    }

  if ( fsdi->pasv_socks->offset >= 2 )
    {
      net_ftp_msg_send (
	  pso, 425,
	  "Can't open data connection. (%s : %d)", "already open", (int)fsdi->pasv_socks->offset );
      return 1;
    }

  __sock_o listener = fsdi->pasv_socks->first->ptr;

  o_thrd dummy_thread =
    { 0};

  dummy_thread.in_objects.lref_ptr = (void*) listener;
  dummy_thread.flags |= F_THRD_NOINIT;

  if (mutex_init (&dummy_thread.mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      return 2;
    }

  fsdi->t00 = time(NULL);

  listener->flags ^= listener->flags & F_OPSOCK_DETACH_THREAD;

  net_worker_mono((void*) &dummy_thread);

  if ( fsdi->pasv_socks->offset < 2 )
    {
      net_ftp_msg_send (
	  pso, 425,
	  "Can't open data connection. (%s)", "Accept timed out");
      return 2;
    }

  __sock_o pasv_data = fsdi->pasv_socks->first->next->ptr;
  dummy_thread.in_objects.lref_ptr = (void*) pasv_data;

  pasv_data->flags |= F_OPSOCK_TERM;

  net_worker_mono((void*) &dummy_thread);

  if ( listener->flags & F_OPSOCK_TERM)
    {
      dummy_thread.in_objects.lref_ptr = (void*) listener;

      listener->flags ^= listener->flags & F_OPSOCK_DETACH_THREAD;

      net_worker_mono((void*) &dummy_thread);
    }

  /* int ret;

   if ( (ret=net_register_task(fsdi->pasv_socks, t_pso, &t_pso->tasks, net_ftp_list_task, NULL, 0) ))
   {
   print_str (
   "ERROR: net_ftp_handle_passive_list: [%d] net_register_task failed: %d\n",pso->sock, ret);
   abort();
   }
   */

  return 0;

}
/*
 static int
 net_ftp_cmd_wait_xfer (__sock_o pso)
 {
 __fsd_info fsdi = (__fsd_info) pso->va_p3;

 time_t start = time(NULL);

 int ret = 0;

 while ( 1 )
 {
 pso->timers.last_act = time (NULL);

 mutex_lock (&fsdi->pasv_socks->mutex);

 if ( pso->flags & F_OPSOCK_TERM)
 {
 pthread_mutex_unlock (&fsdi->pasv_socks->mutex);
 break;
 }

 if (fsdi->pasv_socks->offset < 2)
 {
 pthread_mutex_unlock (&fsdi->pasv_socks->mutex);
 break;
 }

 pthread_mutex_unlock (&fsdi->pasv_socks->mutex);

 usleep(400000);
 }

 return 0;
 }*/

static int
net_ftp_cmd_list (__sock_o pso, char *cmd, char *args)
{
  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  if ( fsdi->status & F_FTP_ST_MODE_PASSIVE)
    {
      char *message;

      fsdi->task_call = (_t_rcall) net_ftp_list_task;

      int ret;

      if ( (ret=net_ftp_handle_pasv_host(pso) ))
	{

	}
      else
	{

	}

      //net_nw_ssig_term_r_ex(fsdi->pasv_socks);
      fsdi->status ^= F_FTP_ST_MODE_PASSIVE;

    }
  else
    {
      net_ftp_msg_send (
	  pso, 425,
	  "Can't open data connection. (not in passive mode)");
    }

  return 0;
}

static int
net_ftp_pasv_host_task_init (__sock_o pso)
{
  __sock_ca ca = (__sock_ca) pso->sock_ca;

  pso->va_p3 = ca->va_p3;

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
      case F_OPSOCK_CONNECT:
      ;

      //pso->parent->flags |= F_OPSOCK_TERM;
      break;
      case F_OPSOCK_LISTEN:
      ;
      int ret;

      if ((ret = net_register_task (fsdi->pasv_socks, pso, &pso->tasks,
		  net_ftp_pasv_check_dead_ctl, NULL, 0)))
	{
	  print_str (
	      "ERROR: net_baseline_pasv_host_init: [%d] net_register_task failed: %d\n",
	      pso->sock, ret);
	  abort ();
	}
      if ((ret = net_register_task (fsdi->pasv_socks, pso, &pso->tasks,
		  net_ftp_pasv_process_ctl, NULL, 0)))
	{
	  print_str (
	      "ERROR: net_baseline_pasv_host_init: [%d] net_register_task failed: %d\n",
	      pso->sock, ret);
	  abort ();
	}
      break;
    }

  return 0;
}

static int
net_ftp_pasv_host_init0_ssl (__sock_o pso)
{
  mutex_lock (&pso->mutex);

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;

      pso->parent->flags |= F_OPSOCK_DETACH_THREAD;

      //pso->parent->flags |= F_OPSOCK_TERM;
      break;
    case F_OPSOCK_LISTEN:
      ;

      break;
    }

  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

static int
net_ftp_pasv_host_init0 (__sock_o pso)
{
  mutex_lock (&pso->mutex);

  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;
      pso->parent->flags |= F_OPSOCK_DETACH_THREAD;

      break;
    case F_OPSOCK_LISTEN:
      ;

      break;
    }

  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

static int
net_baseline_pasv_host_init1 (__sock_o pso)
{
  mutex_lock (&pso->mutex);

  __sock_ca ca = (__sock_ca) pso->sock_ca;

  pso->va_p3 = ca->va_p3;

  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;

      __fsd_info fsdi = (__fsd_info) pso->va_p3;

      char *data_type;

      switch (fsdi->type)
	{
	  case 1:
	  ;
	  data_type = "BINARY";
	  break;
	  case 2:
	  ;
	  data_type = "ASCII";
	  break;
	  default:
	  ;
	  data_type = "FOO";
	}

      if (fsdi->flags & F_FTP_SSLDATA)
	{
	  net_ftp_msg_send (
	      fsdi->ctl,
	      150,
	      "Opening %s mode data connection for directory listing using SSL/TLS.",
	      data_type);
	}
      else
	{
	  net_ftp_msg_send (
	      fsdi->ctl, 150,
	      "Opening %s mode data connection for directory listing.",
	      data_type);
	}

      //pso->parent->flags |= F_OPSOCK_TERM;
      break;
      case F_OPSOCK_LISTEN:
      ;

      break;
    }

  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

/*
 static int
 net_ftp_pasv_host_disconnect (__sock_o pso, void *arg)
 {
 //net_ftp_msg_send_q (pso, 226, "Closing data connection.");

 return 0;
 }

 int
 net_ftp_task_listener_clean (__task task)
 {
 __sock_o pso = (__sock_o) task->data;

 mutex_lock(&pso->mutex);

 uint32_t children = pso->children;
 uint32_t orphaned = pso->flags & F_OPSOCK_ORPHANED;

 pthread_mutex_unlock(&pso->mutex);

 if (0 == children && 0 != orphaned)
 {
 free(pso);

 return 0;
 }
 else
 {
 return 1;
 }

 }*/

static int
net_baseline_pasv_host_rc0_destroy (__sock_o pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;
      __fsd_info fsdi = (__fsd_info) pso->va_p3;

      if ( !(pso->flags & F_OPSOCK_ERROR))
	{
	  net_ftp_msg_send (fsdi->ctl, 226, "Closing data connection.");
	}
      else
	{
	  pso->parent->flags |= F_OPSOCK_TERM;

	  net_ftp_msg_send (fsdi->ctl, 425,
	      "Can't open data connection. (internal error: %d)", pso->sslerr);
	}

      //net_ftp_msg_send (fsdi->ctl, 226, "Closing data connection.");
      break;
      case F_OPSOCK_LISTEN:
      ;

      __sock_ca ca = (__sock_ca) pso->sock_ca;

      mutex_lock (&ftp_state.mutex);

      if ( pso->ipr.port > 0)
	{
	  ht_remove (ftp_state.used_pasv_ports, (unsigned char*) &pso->ipr.port,
	      sizeof(uint16_t));
	}

      pthread_mutex_unlock (&ftp_state.mutex);

      free(ca->host);
      free(ca->port);
      md_g_free_l(&ca->init_rc0);
      md_g_free_l(&ca->init_rc0_ssl);
      md_g_free_l(&ca->init_rc1);
      md_g_free_l(&ca->shutdown_rc0);
      md_g_free_l(&ca->shutdown_rc1);

      free (ca);

      /*int r;

       if ( (r=register_task(net_ftp_task_listener_clean, (void*)pso, 0)) )
       {
       print_str (
       "ERROR: net_baseline_pasv_host_rc0_destroy: [%d]: register_task failed: [%d]\n",
       pso->sock, r);
       abort();
       }*/
      break;

    }
  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

static int
net_baseline_pasv_host (__sock_o pso, pmda base, pmda threadr, void *data)
{
  return 0;
}

static int
net_deploy_pasv_host (__sock_o pso, uint16_t pasv_port)
{

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  __sock_ca ca = calloc (1, sizeof(_sock_ca)), p_ca = (__sock_ca)pso->sock_ca;

  ca->flags |= F_OPSOCK_LISTEN | F_OPSOCK_CS_NOASSIGNTHREAD;
  ca->ac_flags |= F_OPSOCK_CS_NOASSIGNTHREAD;

  if ( fsdi->flags & F_FTP_SSLDATA )
    {
      ca->ca_flags |= p_ca->flags & (F_CA_HAS_SSL_KEY|F_CA_HAS_SSL_CERT);
      ca->flags |= pso->flags & F_OPSOCK_SSL;
      ca->ssl_key = p_ca->ssl_key;
      ca->ssl_cert = p_ca->ssl_cert;
    }

  md_init_le (&ca->init_rc0, 16);
  md_init_le (&ca->init_rc1, 16);
  md_init_le (&ca->shutdown_rc0, 16);
  md_init_le (&ca->shutdown_rc1, 16);

  ca->scall = net_open_listening_socket;

  ca->socket_register = fsdi->pasv_socks;
  ca->proc = (_p_sc_cb) net_baseline_pasv_host;

  net_push_rc (&ca->init_rc1, (_t_rcall) net_ftp_pasv_host_do_endgame, 0);
  net_push_rc (&ca->init_rc1, (_t_rcall) net_baseline_pasv_host_init1, 0);

  if ( ca->flags & F_OPSOCK_SSL)
    {
      md_init_le (&ca->init_rc0_ssl, 16);
      net_push_rc (&ca->init_rc0_ssl, (_t_rcall) net_ftp_pasv_host_init0_ssl, 0);
    }
  else
    {
      net_push_rc (&ca->init_rc0, (_t_rcall) net_ftp_pasv_host_init0, 0);
    }

  net_push_rc (&ca->init_rc0, (_t_rcall) net_ftp_pasv_host_task_init, 0);
  net_push_rc (&ca->init_rc0, (_t_rcall) net_baseline_socket_init1, 0);
  net_push_rc (&ca->init_rc0, (_t_rcall) net_socket_init_enforce_policy, 0);

  net_push_rc (&ca->shutdown_rc0, (_t_rcall) net_generic_socket_destroy0, 0);
  net_push_rc (&ca->shutdown_rc0, (_t_rcall) net_parent_proc_rc0_destroy, 0);
  net_push_rc (&ca->shutdown_rc0, (_t_rcall) net_baseline_pasv_host_rc0_destroy, 0);

  ca->policy.ssl_accept_timeout = 30;
  ca->policy.accept_timeout = 30;
  ca->policy.ssl_connect_timeout = 30;
  ca->policy.connect_timeout = 30;
  ca->policy.idle_timeout = 25;
  ca->policy.close_timeout = 30;
  ca->policy.send_timeout = 30;
  ca->policy.max_connects = 1;
  ca->policy.max_sim_ip = 4;
  //ca->policy.listener_idle_timeout = 29;
  ca->ipr00 = pso->ipr;
  //ca->common.thread_inherit_flags |= F_THRD_DETACH;
  //ca->common.thread_flags |= F_THRD_NOREG | F_THRD_DETACH;

  ca->va_p3 = (void*) fsdi;

  ca->opt0.u16_00 = pasv_port;

  ca->policy.mode = ca->mode;
  //ca->thread_register = &fl_threadr;

  ca->host = malloc (32);
  ca->port = malloc (8);

  snprintf (ca->host, 32, "0.0.0.0");
  snprintf (ca->port, 8, "%hu", pasv_port);

  int r;

  //pthread_t pt;

  if ((r = net_open_listening_socket (ca->host, ca->port, ca)))
    {
      if (r > 0)
	{
	  net_ftp_msg_send (pso, 425,
	      "Can't open data connection. (internal error [%d])",
	      r);
	}
      else
	{
	  char eb[1024];
	  net_ftp_msg_send (pso, 425,
	      "Can't open data connection. (%s) [%s:%s] [%d]",
	      strerror_r (errno, eb, sizeof(eb)), ca->host,
	      ca->port, r);
	}
      free (ca->host);
      free (ca->port);
      md_g_free_l (&ca->init_rc0);
      md_g_free_l (&ca->init_rc0_ssl);
      md_g_free_l (&ca->init_rc1);
      md_g_free_l (&ca->shutdown_rc0);
      md_g_free_l (&ca->shutdown_rc1);
      free (ca);
      return r;
    }

  return 0;

}

static int
net_ftp_sock_shutdown (__sock_o pso, void *arg)
{
  pso->flags |= F_OPSOCK_TERM;

  return 0;
}

static int
net_ftp_cmd_pasv (__sock_o pso, char *cmd, char *args)
{
  uint16_t pasv_port;

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  mutex_lock (&fsdi->pasv_socks->mutex);

  if (fsdi->pasv_socks->offset > 0)
    {
      //print_str ("D6: [%d] already in passive mode\n", pso->sock);

      uint8_t *p_port = (uint8_t*) &fsdi->l_ip.port;

      fsdi->status |= F_FTP_ST_MODE_PASSIVE;

      pthread_mutex_unlock (&fsdi->pasv_socks->mutex);

      net_ftp_msg_send (pso, 227,
			"Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hhu,%hhu)",
			di_base.host.ip[0], di_base.host.ip[1],
			di_base.host.ip[2], di_base.host.ip[3], p_port[1],
			p_port[0]);

      return 0;
    }

  pthread_mutex_unlock (&fsdi->pasv_socks->mutex);

  mutex_lock (&ftp_state.mutex);

  if (fsdi->l_ip.port)
    {
      pasv_port = fsdi->l_ip.port;

    }
  else
    {
      pasv_port = ftp_state.pasv.port;
    }

  uint16_t c = 0;

  while ( NULL
      != ht_get (ftp_state.used_pasv_ports, (unsigned char*) &pasv_port,
		 sizeof(uint16_t)))
    {
      if (c == ftp_state.pasv_ports)
	{
	  pthread_mutex_unlock (&ftp_state.mutex);
	  net_ftp_msg_send (pso, 425,
			    "Can't open data connection. (Out of resources)");

	  return 1;

	}
      pasv_port++;

      if (ftp_state.pasv.port > ftp_opt.pasv_ports.high)
	{
	  pasv_port = ftp_opt.pasv_ports.low;
	}

      c++;
    }

  ftp_state.pasv.port = pasv_port;
  fsdi->l_ip.port = pasv_port;

  ht_set (ftp_state.used_pasv_ports, (unsigned char*) &pasv_port,
	  sizeof(uint16_t), (unsigned int *) 1234, 0);

  pthread_mutex_unlock (&ftp_state.mutex);

  if (!(net_deploy_pasv_host (pso, pasv_port)))
    {

      fsdi->status |= F_FTP_ST_MODE_PASSIVE;

      uint8_t *p_port = (uint8_t*) &pasv_port;

      net_ftp_msg_send (pso, 227,
			"Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hhu,%hhu)",
			di_base.host.ip[0], di_base.host.ip[1],
			di_base.host.ip[2], di_base.host.ip[3], p_port[1],
			p_port[0]);

    }
  else
    {
      mutex_lock (&ftp_state.mutex);
      ht_remove (ftp_state.used_pasv_ports, (unsigned char*) &pasv_port,
		 sizeof(uint16_t));
      pthread_mutex_unlock (&ftp_state.mutex);
      return 1;
    }

  return 0;
}

void
net_ftp_init (hashtable_t *ht)
{
  nc_register (ht, "USER", net_ftp_cmd_user);
  nc_register (ht, "PASS", net_ftp_cmd_pass);
  nc_register (ht, "SYST", net_ftp_cmd_syst);
  nc_register (ht, "CWD", net_ftp_cmd_cwd);
  nc_register (ht, "CDUP", net_ftp_cmd_cdup);
  nc_register (ht, "PWD", net_ftp_cmd_pwd);
  nc_register (ht, "TYPE", net_ftp_cmd_type);
  nc_register (ht, "QUIT", net_ftp_cmd_quit);
  nc_register (ht, "PASV", net_ftp_cmd_pasv);
  nc_register (ht, "LIST", net_ftp_cmd_list);
  nc_register (ht, "PBSZ", net_ftp_cmd_pbsz);
  nc_register (ht, "PROT", net_ftp_cmd_prot);
  //nc_register (ht, "PORT", net_ftp_cmd_port);

  int r;

  /*md_init_le (&fl_threadr, 10000);
   if ((r = spawn_threads (1, net_worker, 0, &fl_threadr,
   THREAD_ROLE_NET_WORKER,
   SOCKET_OPMODE_LISTENER, F_THRD_NOWPID)))
   {
   print_str (
   "ERROR: net_ftp_init: spawn_threads failed [SOCKET_OPMODE_LISTENER]: %d\n",
   r);
   abort ();
   }
   else
   {
   print_str (
   "DEBUG: net_ftp_init: deployed %hu socket worker threads [SOCKET_OPMODE_LISTENER]\n",
   1);
   }*/

  //thread_broadcast_sig (&fl_threadr, SIGINT);
}

static int
net_ftp_initialize_sts (__sock_o pso)
{
  if ( NULL == pso->va_p3)
    {
      pso->va_p3 = calloc (1, sizeof(_fsd_info));
    }

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  snprintf (fsdi->cwd, sizeof(fsdi->cwd), "/");

  fsdi->pasv_socks = calloc (1, sizeof(mda));
  md_init_le (fsdi->pasv_socks, 8);

  fsdi->ctl = pso;

  return 0;
}

int
net_baseline_ftp (__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  char *cmd_in = (char*) data, *ptr = cmd_in, *cmd_args = NULL, *arg_clr =
  NULL;
  ssize_t c = 0;

  while (ptr[c] && !(ptr[c] == 0xD && ptr[c + 1] == 0xA)
      && c < pso->counters.b_read)
    {
      if (ptr[c] == 0x20)
	{
	  arg_clr = &ptr[c];
	  //ptr[c] = 0x0;
	  while (ptr[c] == 0x20)
	    {
	      c++;
	    }
	  cmd_args = &ptr[c];
	}
      c++;
    }

  if (!(ptr[c] == 0xD && ptr[c + 1] == 0xA))
    {
      pthread_mutex_unlock (&pso->mutex);
      return -2;
    }

  __fsd_info fsdi = (__fsd_info) pso->va_p3;

  ptr[c] = 0x0;
  ptr[c + 1] = 0x0;

  if (0 == c)
    {
      print_str ("ERROR: net_baseline_ftp: [%d] empty command\n", pso->sock);
      pthread_mutex_unlock (&pso->mutex);
      return 4;
    }

  if (c > NET_FTP_MAX_MSG_SIZE)
    {
      print_str ("ERROR: net_baseline_ftp: [%d] command too long\n", pso->sock);
      pthread_mutex_unlock (&pso->mutex);
      return 5;
    }

  if ( NULL != arg_clr)
    {
      arg_clr[0] = 0x0;
    }

  int msg_len = c + 2;

  __nc_proc ncp;

  if ( NULL
      == (ncp = ht_get (ftp_cmd, (unsigned char*) cmd_in, strlen (cmd_in) + 1)))
    {
      print_str ("ERROR: net_baseline_ftp: unknown command: %s %s\n", cmd_in,
		 cmd_args ? cmd_args : "");
      net_ftp_msg_send (pso, 502, "%s: Command not implemented.", cmd_in);
      //pso->flags = F_OPSOCK_TERM;
      net_ftp_cleanup_pasv_host (pso);
    }
  else
    {
      int ret = ncp->call (pso, cmd_in, cmd_args);

      if (ret)
	{
	  net_ftp_cleanup_pasv_host (pso);
	}

    }

  //nc_proc(ftp_cmd, pso, )

  pso->counters.b_read -= msg_len;

  memmove (data, &((unsigned char*) data)[msg_len],
	   (size_t) pso->counters.b_read);

  pthread_mutex_unlock (&pso->mutex);

  if (pso->counters.b_read)
    {
      return net_baseline_ftp (pso, base, threadr, data);
    }

  return 0;

}

int
net_ftp_ctl_socket_init1_accept (__sock_o pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;

      net_ftp_initialize_sts (pso);

      mutex_lock (&di_base.mutex);

      uint32_t status = (di_base.status & F_DIS_ACTIVE);

      pthread_mutex_unlock (&di_base.mutex);

      if (!(status))
	{
	  net_ftp_msg_send (
	      pso, 425,
	      "Can't open data connection. (Index server uninitialized)");
	  pso->flags |= F_OPSOCK_TERM;
	}
      else
	{
	  net_ftp_msg_send (pso, 220, "Welcome");
	}
      break;
    case F_OPSOCK_LISTEN:
      ;

      break;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}
/*
 static int
 net_ftp_task_pasv_clean (__task task)
 {
 __fsd_info fsdi = (__fsd_info) task->data;

 mutex_lock (&fsdi->pasv_socks->mutex);

 off_t clean = fsdi->pasv_socks->offset;

 pthread_mutex_unlock (&fsdi->pasv_socks->mutex);

 if (0 == clean)
 {
 md_g_free_l (fsdi->pasv_socks);
 free (fsdi->pasv_socks);
 free(fsdi);
 return 0;
 }
 else
 {
 return 1;
 }

 }
 */
int
net_ftp_ctl_socket_rc0_destroy (__sock_o pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;

      net_ftp_cleanup_pasv_host (pso);

      __fsd_info fsdi = (__fsd_info) pso->va_p3;

      uint64_t c = 0;

      if ( fsdi->pasv_socks->offset > 2)
	{
	  printf("fsdfds\n");
	  abort();
	}

      while ( fsdi->pasv_socks->offset > 0 )
	{
	  c++;

	  __sock_o ptr = fsdi->pasv_socks->pos->ptr;

	  ptr->flags ^= ptr->flags & F_OPSOCK_DETACH_THREAD;
	  ptr->flags |= F_OPSOCK_TERM;

	  net_proc_worker_detached_socket(fsdi->pasv_socks->pos->ptr, 0);
	  usleep(100);
	}

      print_str("D4: net_ftp_ctl_socket_rc0_destroy; ran net_proc_worker_detached_socket for %llu cycles\n", c);

      md_g_free_l (fsdi->pasv_socks);
      free (fsdi->pasv_socks);
      free(fsdi);

      /*int c = net_join_threads(fsdi->pasv_socks);

       mutex_lock(&fsdi->pasv_socks->mutex);
       off_t psvs_count = fsdi->pasv_socks->offset;
       pthread_mutex_unlock(&fsdi->pasv_socks->mutex);

       if ( psvs_count > 0 )
       {
       print_str (
       "WARNING: net_ftp_ctl_socket_rc0_destroy: [%d] not all sockets/threads got cleaned up [%llu / %d], tasking net_ftp_task_pasv_clean\n",
       pso->sock, (uint64_t ) psvs_count, c );

       }
       else
       {
       md_g_free_l (fsdi->pasv_socks);
       free (fsdi->pasv_socks);
       }*/

      //pso->flags |= F_OPSOCK_PERSIST;
      /*int r;

       if ( (r=register_task(net_ftp_task_pasv_clean, fsdi, 0)) )
       {
       print_str (
       "ERROR: net_ftp_ctl_socket_rc0_destroy: [%d]: register_task failed: [%d]\n",
       pso->sock, r);
       abort();
       }*/

      //net_ftp_msg_send_q (pso, 500, "Bye");
      break;
      case F_OPSOCK_LISTEN:
      ;

      break;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_ftp_msg_send (__sock_o pso, int code, char *message, ...)
{

  char packet[NET_FTP_MAX_MSG_SIZE], msg_b[2048];

  va_list al;
  va_start(al, message);

  vsnprintf (msg_b, sizeof(msg_b), message, al);

  va_end(al);

  int b_len = snprintf (packet, sizeof(packet), "%d %s\r\n", code, msg_b);

  int r;
  if ((r = net_send_direct (pso, (void*) packet, b_len)))
    {
      print_str (
	  "ERROR: net_ftp_msg_send: [%d]: net_send_direct failed: [%d]\n",
	  pso->sock, r);
      pso->flags |= F_OPSOCK_TERM;
    }
  return r;
}

int
net_ftp_msg_send_q (__sock_o pso, int code, char *message, ...)
{
  char packet[NET_FTP_MAX_MSG_SIZE], msg_b[2048];

  va_list al;
  va_start(al, message);

  vsnprintf (msg_b, sizeof(msg_b), message, al);

  va_end(al);

  int b_len = snprintf (packet, sizeof(packet), "%d %s\r\n", code, msg_b);

  int r;
  if ((r = net_push_to_sendq (pso, (void*) packet, b_len, 0)))
    {
      print_str (
	  "ERROR: net_ftp_msg_send: [%d]: net_push_to_sendq failed: [%d]\n",
	  pso->sock, r);
      pso->flags |= F_OPSOCK_TERM;
    }
  return r;
}
