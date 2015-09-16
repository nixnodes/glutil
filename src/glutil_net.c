#include <glutil.h>

#include <glutil_net.h>
#include <net_io.h>
#include <net_proto.h>
#include <log_op.h>
#include <log_io.h>
#include <omfp.h>
#include <m_general.h>
#include <signal_t.h>

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <malloc.h>

_net_opt net_opts =
  { .max_sock = 512, .thread_l = 1, .thread_r = 2, .st_p0 = NULL,
      .max_worker_threads = 128, .ssl_cert_def = "server.cert", .ssl_key_def =
	  "server.key", .flags = 0 };

mda _sock_r =
  { 0 };
mda _boot_pca =
  { 0 };

pid_t _m_tid;

static int
process_ca_requests (pmda md)
{
  p_md_obj ptr = md->first;
  int ret, fail = 0;
  char buf_err[1024];

  while (ptr)
    {
      if (!g_get_gkill ())
	{
	  break;
	}

      __sock_ca pca = (__sock_ca) ptr->ptr;

      switch (pca->flags & F_OPSOCK_CREAT_MODE)
	{
	case F_OPSOCK_CONNECT:
	  if ((ret = net_open_connection (pca->host, pca->port, pca)))
	    {
	      print_str (
		  "ERROR: net_open_connection: host: %s port: %s, status:[%d] %s\n",
		  pca->host, pca->port, ret,
		  ret < 0 ? strerror_r (errno, buf_err, 1024) : "");
	      fail++;
	    }
	  break;
	case F_OPSOCK_LISTEN:
	  if ((ret = net_open_listening_socket (pca->host, pca->port, pca)))
	    {
	      print_str (
		  "ERROR: net_open_listening_socket: host: %s port: %s, status:[%d] %s\n",
		  pca->host, pca->port, ret,
		  ret < 0 ? strerror_r (errno, buf_err, 1024) : "");
	      fail++;
	    }
	  break;
	}

      ptr = ptr->next;
    }

  return fail;
}

#define T_THRD_PROC_TIMEOUT             (time_t) 60

static void
net_ping_threads (void)
{
  mutex_lock (&_net_thrd_r.mutex);
  p_md_obj ptr = _net_thrd_r.first;
  while (ptr)
    {
      po_thrd thrd = (po_thrd) ptr->ptr;
      mutex_lock (&thrd->mutex);

      if (thrd->status & F_THRD_STATUS_SUSPENDED)
	{
	  if ((register_count (&thrd->in_objects) > (off_t) 0
	      || register_count (&thrd->proc_objects) > (off_t) 0))
	    {
	      pthread_kill (thrd->pt, SIGUSR1);
	      if (gfl & F_OPT_VERBOSE)
		{
		  print_str ("D6: [%X]: waking up worker\n", thrd->pt);

		}
	    }
	}
      else
	{
	  if ((thrd->timers.act_f & F_TTIME_ACT_T0)
	      && (time (NULL) - thrd->timers.t0) > T_THRD_PROC_TIMEOUT)
	    {
	      pthread_kill (thrd->pt, SIGUSR1);
	      print_str ("D2: [%X]: thread not responding\n", thrd->pt);
	    }
	}

      pthread_mutex_unlock (&thrd->mutex);

      ptr = ptr->next;
    }
  pthread_mutex_unlock (&_net_thrd_r.mutex);
}

static void
net_def_sig_handler (int signal)
{
  if (!(gfl & F_OPT_KILL_GLOBAL) && register_count (&_sock_r) == 0)
    {
      print_str (
	  "NOTICE: net_def_sig_handler: [%ld]: nothing left to process [%d]\n",
	  syscall (SYS_gettid), signal);
      mutex_lock (&mutex_glob00);
      gfl |= F_OPT_KILL_GLOBAL;
      pthread_mutex_unlock (&mutex_glob00);
    }
  else
    {
      //print_str("D6: net_def_sig_handler: pinging threads..\n");
      net_ping_threads ();
      //sleep(1000);
    }

  return;
}

static int
net_deploy_wait_for_all_threads (pmda thread_reg)
{
  int l_res, ret;

  while ((ret = g_get_gkill ()))
    {
      mutex_lock (&thread_reg->mutex);

      p_md_obj ptr = thread_reg->first;

      l_res = 1;

      while (ptr)
	{
	  po_thrd thrd = (po_thrd) ptr->ptr;

	  mutex_lock (&thrd->mutex);

	  if (!(thrd->status & F_THRD_STATUS_INITIALIZED))
	    {
	      l_res = 0;
	      pthread_mutex_unlock (&thrd->mutex);
	      break;
	    }

	  pthread_mutex_unlock (&thrd->mutex);

	  ptr = ptr->next;
	}

      pthread_mutex_unlock (&thread_reg->mutex);

      if (l_res)
	{
	  break;
	}

      usleep (25000);

    }

  return ret;
}

#define NET_WTHRD_CLEANUP_TIMEOUT               (time_t) 30

#define NET_TMON_SLEEP_DEFAULT                  (unsigned int) 30

#define F_ND_SSL_INIT                           ((uint32_t)1 << 1)

int
net_deploy (void)
{
  if (((int) net_opts.thread_l + (int) net_opts.thread_r)
      > net_opts.max_worker_threads)
    {
      print_str (
	  "ERROR: net_deploy: requested thread count exceeds 'max_worker_threads' [%d/%d]\n",
	  ((int) net_opts.thread_l + (int) net_opts.thread_r),
	  net_opts.max_worker_threads);
      return -1;
    }

  //_m_tid = getpid();

#ifdef M_ARENA_TEST
  mallopt (M_ARENA_TEST, 1);
#endif
#ifdef M_ARENA_MAX
  mallopt (M_ARENA_MAX, 1);
#endif

  struct sigaction sa_wthrd, sa_mthrd;

  sa_wthrd.sa_handler = sig_handler_null;
  sa_wthrd.sa_flags = SA_RESTART;

  sigfillset (&sa_wthrd.sa_mask);
  //sigaddset(&sa_wthrd.sa_mask, SIGIO);
  //sigaddset(&sa_wthrd.sa_mask, SIGUSR1);

  sigaction (SIGIO, &sa_wthrd, NULL);
  sigaction (SIGURG, &sa_wthrd, NULL);
  sigaction (SIGUSR1, &sa_wthrd, NULL);

  sa_mthrd.sa_handler = net_def_sig_handler;
  sa_mthrd.sa_flags = SA_RESTART;

  sigfillset (&sa_mthrd.sa_mask);
  //sigaddset(&sa_mthrd.sa_mask, SIGUSR2);

  sigaction (SIGUSR2, &sa_mthrd, NULL);

  sigset_t set;

  sigemptyset (&set);
  sigaddset (&set, SIGPIPE);
  sigaddset (&set, SIGURG);
  sigaddset (&set, SIGIO);
  sigaddset (&set, SIGUSR1);

  int sr = pthread_sigmask (SIG_BLOCK, &set, NULL);

  uint32_t in_f = 0;

  if (sr != 0)
    {
      print_str ("ERROR: net_deploy: pthread_sigmask failed: %d\n", sr);
      abort ();
    }

  md_init_le (&_sock_r, (int) net_opts.max_sock);
  md_init_le (&_net_thrd_r, (int) net_opts.max_worker_threads);

  if (net_opts.flags & F_NETOPT_SSLINIT)
    {
      print_str ("DEBUG: initializing TLS/SSL subsystem..\n");
      ssl_init ();
      in_f |= F_ND_SSL_INIT;
    }

  int r;

  if ((r = spawn_threads (net_opts.thread_l, net_worker, 0, &_net_thrd_r,
  THREAD_ROLE_NET_WORKER,
			  SOCKET_OPMODE_LISTENER)))
    {
      print_str ("ERROR: spawn_threads failed [SOCKET_OPMODE_LISTENER]: %d\n",
		 r);
      return 2;
    }
  else
    {
      print_str (
	  "DEBUG: deployed %hu socket worker threads [SOCKET_OPMODE_LISTENER]\n",
	  net_opts.thread_l);
    }

  if ((r = spawn_threads (net_opts.thread_r, net_worker, 0, &_net_thrd_r,
  THREAD_ROLE_NET_WORKER,
			  SOCKET_OPMODE_RECIEVER)))
    {
      print_str ("ERROR: spawn_threads failed [SOCKET_OPMODE_RECIEVER]: %d\n",
		 r);
      return 2;
    }
  else
    {
      print_str (
	  "DEBUG: deployed %hu socket worker threads [SOCKET_OPMODE_RECIEVER]\n",
	  net_opts.thread_r);
    }

  print_str ("DEBUG: waiting for workers to initialize..\n");

  if (net_deploy_wait_for_all_threads (&_net_thrd_r))
    {
      print_str ("D5: all workers online\n");
    }
  else
    {
      goto _t_kill;
    }

  int fail;

  if ((fail = process_ca_requests (&_boot_pca)))
    {
      if ((off_t) fail == _boot_pca.offset)
	{
	  print_str (
	      "WARNING: process_ca_requests: no connection requests succeeded\n");
	  goto _t_kill;
	}
      else
	{
	  print_str (
	      "WARNING: process_ca_requests: not all connection requests were succesfull\n");
	}
    }
  else
    {
      print_str ("DEBUG: deployed %llu socket(s)\n",
		 (uint64_t) _boot_pca.offset);

    }

  if (net_opts.flags & (F_NETOPT_HUSER | F_NETOPT_HGROUP))
    {
      if (net_opts.flags & F_NETOPT_HUSER)
	{
	  snprintf (G_USER, sizeof(G_USER), "%s", net_opts.user);
	  gfl0 |= F_OPT_SETUID;
	}
      if (net_opts.flags & F_NETOPT_HGROUP)
	{
	  snprintf (G_GROUP, sizeof(G_GROUP), "%s", net_opts.group);
	  gfl0 |= F_OPT_SETGID;
	}

      g_setxid ();
    }

  //unsigned int tmon_ld = 15;

  while (g_get_gkill ())
    {
      //net_ping_threads();
      sleep (-1);
    }

  if (register_count (&_sock_r))
    {
      print_str ("DEBUG: sending F_OPSOCK_TERM to all sockets\n");
      net_nw_ssig_term_r (&_sock_r);
    }

  _t_kill: ;

  print_str ("DEBUG: sending F_THRD_TERM to all worker threads\n");

  print_str ("DEBUG: waiting for threads to exit..\n");

  time_t s = time (NULL), e;
  off_t l_count;

  while ((l_count = register_count (&_net_thrd_r)) > 0)
    {
      thread_broadcast_kill (&_net_thrd_r);
      e = time (NULL);
      if ((e - s) > NET_WTHRD_CLEANUP_TIMEOUT)
	{
	  print_str ("WARNING: %llu worker threads remaining\n",
		     (uint64_t) l_count);
	  break;
	}
      sleep (1);
    }

  md_g_free_l (&_net_thrd_r);
  md_g_free_l (&_sock_r);
  free (pc_a.objects);
  md_g_free_l (&_boot_pca);

  if (in_f & F_ND_SSL_INIT)
    {
      print_str ("DEBUG: releasing TLS/SSL resources..\n");
      ssl_cleanup ();
    }

  print_str ("INFO: server shutting down..\n");

  return 0;
}

#include <fcntl.h>

static int
net_proc_piped_q (__sock_o pso, __g_handle hdl)
{
  unsigned char buffer[32768];

  ssize_t r_sz;

  //mutex_lock(&pso->mutex);

  while ((r_sz = read (hdl->pipe.pfd_out[0], (void*) buffer, 32768)) > 0)
    {
      if (net_send_direct (pso, (void*) buffer, (size_t) r_sz))
	{
	  pso->flags |= F_OPSOCK_TERM;
	  return 1;
	}
      else
	{
	  /*if (r_sz != pso->counters.session_write)
	   {
	   print_str(
	   "ERROR: net_proc_piped_q: [%d]: failed sending complete data block\n",
	   pso->sock);
	   pso->flags |= F_OPSOCK_TERM;
	   return 1;
	   }*/

	  hdl->pipe.data_in += r_sz;
	}

    }

  if (r_sz == -1)
    {
      if ( errno != EAGAIN && errno != EWOULDBLOCK)
	{
	  print_str (
	      "ERROR: net_proc_piped_q: [%d]: pipe read failed [%d] [%s]\n",
	      pso->sock, errno,
	      g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  //pso->flags |= F_OPSOCK_TERM;
	  return 2;
	}
    }
  else if (r_sz == 0)
    {
      print_str ("D1: net_proc_piped_q: [%d]: pipe read EOF [%d]\n", pso->sock,
		 hdl->pipe.child);
    }

  return 0;
  //pthread_mutex_unlock(&pso->mutex);

}

int
net_baseline_gl_data_in (__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  if ((pso->counters.b_read < pso->unit_size) || (pso->flags & F_OPSOCK_TERM))
    {
      pthread_mutex_unlock (&pso->mutex);
      return 0;
    }

  //print_str("NOTICE: got packet [%s]\n", pso->st_p0);

  __g_handle hdl = (__g_handle ) pso->va_p0;

  pthread_mutex_unlock (&pso->mutex);

  int r;

  if ((r = g_bmatch (data, hdl, &hdl->buffer)))
    {
      if (r == -1)
	{
	  print_str (
	      "ERROR: net_baseline_gl_data_in: %s: [%d] matching record failed\n",
	      pso->st_p0, r);
	}
      else if (hdl->flags & F_GH_SPEC_SQ01)
	{
	  pso->flags |= F_OPSOCK_TERM;
	  hdl->flags ^= F_GH_SPEC_SQ01;
	}

      goto l_end_at;
    }

  if (hdl->flags & F_GH_PRINT)
    {
      hdl->g_proc4 ((void*) hdl, data, (void*) pso);
    }

  omfp_timeout;

  l_end_at: ;

  /*if ((hdl->flags & F_GH_EXECRD_PIPE_OUT)
   && (hdl->flags & F_GH_EXECRD_HAS_STDOUT_PIPE))
   {
   if (close(hdl->pipe.pfd_out[0]) == -1)
   {
   print_str(
   "ERROR: net_baseline_gl_data_in: [%d]: pipe close failed [%s]\n",
   pso->sock,
   g_strerr_r(errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
   }

   hdl->flags ^= F_GH_EXECRD_HAS_STDOUT_PIPE;
   }*/

  //pthread_mutex_unlock(&pso->mutex);
  return 0;
}

static void
net_baseline_pipedata_cleanup (__sock_o pso, __g_handle hdl)
{
  if (pso->flags & F_OPSOCK_HALT_RECV)
    {
      pso->flags ^= F_OPSOCK_HALT_RECV;
    }
  pso->rcv1 = (_p_s_cb) net_baseline_gl_data_in;
  g_handle_pipe_cleanup (hdl);
}

int
net_baseline_pipe_data (__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  if ((pso->flags & F_OPSOCK_TERM))
    {
      pthread_mutex_unlock (&pso->mutex);
      return -3;
    }

  //print_str("NOTICE: got packet [%s]\n", pso->st_p0);

  __g_handle hdl = (__g_handle ) pso->va_p0;

  int exit_status;

  int wp_ret;

  if ((wp_ret = waitpid (hdl->pipe.child, &exit_status, WNOHANG)) == -1)
    {
      print_str (
	  "ERROR: [%d]: failed waiting for child process to finish [%s]\n",
	  (uint32_t) hdl->pipe.child,
	  g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
    }

  int nppq = net_proc_piped_q ((__sock_o ) hdl->pso_ref, hdl);

  if ((wp_ret == 0 || wp_ret == -1) && (nppq))
    {
      print_str (
	  "ERROR: [%d]: net_proc_piped_q failed, sending SIGKILL to child\n",
	  (uint32_t) hdl->pipe.child);
      kill (hdl->pipe.child, SIGKILL);
    }

  if (wp_ret > 0 || wp_ret == -1)
    {
      //net_baseline_pipedata_cleanup(pso, hdl);
      /*int i = 10;
       while (i--) {
       net_proc_piped_q((__sock_o ) hdl->pso_ref, hdl);
       sleep (1);
       }*/

      net_baseline_pipedata_cleanup (pso, hdl);

    }

  pthread_mutex_unlock (&pso->mutex);

  return -3;
}

int
net_gl_socket_destroy (__sock_o pso)
{

  mutex_lock (&pso->mutex);

  int r;

  if (NULL != pso->va_p0)
    {
      __g_handle hdl = (__g_handle) pso->va_p0;

      if ( (hdl->flags & F_GH_EXECRD_WAS_PIPED))
	{
	  if (gfl & F_OPT_VERBOSE4)
	    {
	      print_str(
		  "D2: net_baseline_pipe_data: PIPE: %llu b -> SOCK: %llu b\n",
		  ((unsigned long long int) hdl->pipe.data_in),
		  pso->counters.total_write);
	    }
	}

      r = g_cleanup((__g_handle ) pso->va_p0);
      free(pso->va_p0);
      pso->va_p0 = NULL;
    }
  else
    {
      r = 0;
    }

  if (gfl & F_OPT_VERBOSE3)
    {
      int _tid = syscall (SYS_gettid);
      print_str ("NOTICE: [%d] socket closed: [%d]\n", (int) _tid, pso->sock);
    }

  pthread_mutex_unlock (&pso->mutex);

  return r;
}

static int
net_l_wp_setup_pipe (pid_t c_pid, void *arg)
{
  __g_handle hdl = (__g_handle) arg;

  __sock_o pso = (__sock_o)hdl->pso_ref;

  fcntl(hdl->pipe.pfd_out[0], F_SETFL, O_NONBLOCK);

  mutex_lock(&pso->mutex);

  pso->rcv1 = (_p_s_cb) net_baseline_pipe_data;
  hdl->pipe.child = c_pid;
  pso->flags |= F_OPSOCK_HALT_RECV;

  hdl->flags |= F_GH_EXECRD_WAS_PIPED;

  /*int cur_fl = fcntl(pso->sock, F_GETFL);

   if ( cur_fl & O_NONBLOCK) {
   cur_fl ^= O_NONBLOCK;
   cur_fl |= O_SYNC;
   }
   ;
   fcntl(pso->sock, F_SETFL, cur_fl);*/

  pthread_mutex_unlock(&pso->mutex);

  return status;
}

int
net_gl_socket_init0 (__sock_o pso)
{

  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      if (!(pso->flags & F_OPSOCK_CONNECT))
	{
	  break;
	}

      if ((pso->flags & F_OPSOCK_TERM))
	{
	  break;
	}

//print_str("NOTICE: accepted %d\n", pso->sock);

      if ( NULL == pso->st_p0)
	{
	  print_str ("ERROR: net_gl_socket_init0: [%d] missing log pointer\n",
		     pso->sock);
	  pso->flags |= F_OPSOCK_TERM;
	  return 2;
	}

      __g_handle hdl;

      hdl = pso->va_p0 = calloc (1, sizeof(_g_handle));

      char *p_logf = g_dgetf ((char*) pso->st_p0);

      if (NULL == p_logf)
	{
	  print_str (
	      "ERROR: net_gl_socket_init0: [%d] non existant log type: '%s'\n",
	      pso->sock, (char *) pso->st_p0);
	  pso->flags |= F_OPSOCK_TERM;
	  return 2;
	}

      if (determine_datatype (hdl, p_logf))
	{
	  print_str (
	      "ERROR: net_gl_socket_init0: [%d] non existant log reference: '%s'\n",
	      pso->sock, p_logf);
	  pso->flags |= F_OPSOCK_TERM;
	  return 2;
	}

      int r;

      hdl->execv_wpid_fp = net_l_wp_setup_pipe;
      hdl->flags |= (F_GH_W_NSSYS | F_GH_EXECRD_PIPE_OUT);
      snprintf (hdl->file, sizeof(hdl->file), "%s", (char*) pso->st_p0);

      mutex_lock (&mutex_glob00);
      if ((r = g_proc_mr (hdl)))
	{
	  pthread_mutex_unlock (&mutex_glob00);
	  print_str ("ERROR: [%d] net_gl_socket_init0: g_proc_mr failed [%d]\n",
		     pso->sock, r);
	  pso->flags |= F_OPSOCK_TERM;
	  return 2;
	}
      pthread_mutex_unlock (&mutex_glob00);

      hdl->v_b0 = pso->st_p1;
      hdl->v_b0_sz = MAX_PRINT_OUT - 4;
      hdl->pso_ref = (void*) pso;

      pso->unit_size = (ssize_t) hdl->block_sz;

      kill (getpid (), SIGUSR2);

      break;
    }
  return 0;
}

int
net_gl_socket_init1 (__sock_o pso)
{
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      if (gfl & F_OPT_VERBOSE3)
	{
	  int _tid = syscall (SYS_gettid);

	  char ip[128];
	  uint16_t port = net_get_addrinfo_port (pso);

	  net_get_addrinfo_ip (pso, (char*) ip, sizeof(ip));

	  print_str ("NOTICE: [%d]: [%d] data socket active [%s] [%s:%hu]\n",
		     _tid, pso->sock, (char*) pso->st_p0, ip, port);
	}

      break;
    case SOCKET_OPMODE_LISTENER:
      ;
      if (gfl & F_OPT_VERBOSE3)
	{
	  int _tid = syscall (SYS_gettid);

	  char ip[128];
	  uint16_t port = net_get_addrinfo_port (pso);

	  net_get_addrinfo_ip (pso, (char*) ip, sizeof(ip));

	  print_str (
	      "NOTICE: [%d]: [%d] listener socket active [%s] [%s:%hu]\n", _tid,
	      pso->sock, (char*) pso->st_p0, ip, port);
	}
      break;
    }

  return 0;
}

int
net_gl_socket_connect_init1 (__sock_o pso)
{
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;

      break;
    }

  return 0;
}

int
net_gl_socket_pre_clean (__sock_o pso)
{
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      __g_handle hdl = (__g_handle) pso->va_p0;

      if ( NULL != hdl && (hdl->flags & F_GH_EXECRD_WAS_PIPED))
	{
	  if (gfl & F_OPT_VERBOSE4)
	    {
	      print_str("DEBUG: net_baseline_pipe_data: PIPE: %llu b -> SOCK: %llu b\n",
		  ((unsigned long long int) hdl->pipe.data_in),
		  pso->counters.total_write);
	    }
	}
      break;
    }

  return 0;
}

int
net_gl_socket_post_clean (__sock_o pso)
{
  kill (getpid (), SIGUSR2);

  return 0;
}

int
net_gl_socket_init1_dc_on_ac (__sock_o pso)
{
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      if (pso->parent == NULL)
	{
	  break;
	}
      __sock_o spso = pso->parent;
      spso->flags |= F_OPSOCK_TERM;
      print_str ("NOTICE: [%d]: sending F_OPSOCK_TERM to parent: %d\n",
		 pso->sock, spso->sock);
      break;
    }

  return 0;
}

/*
 static void
 sig_handler_default(int signal)
 {
 #ifdef _G_SSYS_THREAD
 mutex_lock(&mutex_glob00);
 #endif
 status |= F_STATUS_MSIG00;
 #ifdef _G_SSYS_THREAD
 pthread_mutex_unlock(&mutex_glob00);
 #endif
 return;
 }*/
