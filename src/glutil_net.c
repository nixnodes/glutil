#include <glutil.h>

#include <glutil_net.h>
#include <net_io.h>
#include <net_proto.h>
#include <log_op.h>
#include <log_io.h>
#include <omfp.h>
#include <m_general.h>

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <malloc.h>
#include <arpa/inet.h>

_net_opt net_opts =
  { .max_sock = 512, .thread_l = 1, .thread_r = 32, .st_p0 = NULL,
      .max_worker_threads = 64, .ssl_cert_def = "server.crt", .ssl_key_def =
          "server.key", .flags = 0 };

mda _sock_r =
  { 0 };
mda _boot_pca =
  { 0 };

pid_t _m_tid;

static int
process_ca_requests(pmda md)
{
  p_md_obj ptr = md->first;
  int ret, fail = 0;

  while (ptr)
    {
      __sock_ca pca = (__sock_ca) ptr->ptr;

      switch (pca->flags & F_OPSOCK_CREAT_MODE)
        {
          case F_OPSOCK_CONNECT:
          if ((ret = net_open_connection (pca->host, pca->port, pca)))
            {
              print_str ("ERROR: net_open_connection: host: %s port: %s, status:[%d] %s\n",
                  pca->host, pca->port, ret, ret < 0 ? strerror(errno) : "");
              fail++;
            }
          break;
          case F_OPSOCK_LISTEN:
          if ((ret = net_open_listening_socket (pca->host, pca->port, pca)))
            {
              print_str ("ERROR: net_open_listening_socket: host: %s port: %s, status:[%d] %s\n",
                  pca->host, pca->port, ret, ret < 0 ? strerror(errno) : "");
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
net_ping_threads(void)
{
  mutex_lock(&_net_thrd_r.mutex);
  p_md_obj ptr = _net_thrd_r.first;
  while (ptr)
    {
      po_thrd thrd = (po_thrd) ptr->ptr;
      mutex_lock(&thrd->mutex);

      if (thrd->status & F_THRD_STATUS_SUSPENDED)
        {
          if (thread_register_count(&thrd->in_objects) > (off_t) 0)
            {
              pthread_kill(thrd->pt, SIGUSR1);
              if (gfl & F_OPT_VERBOSE)
                {
                  print_str("NOTICE: [%X]: waking up worker\n", thrd->pt);

                }
            }
          else if (thread_register_count(&thrd->proc_objects) > (off_t) 0)
            {
              pthread_kill(thrd->pt, SIGUSR1);
              if (gfl & F_OPT_VERBOSE)
                {
                  print_str("NOTICE: [%X]: waking up worker\n", thrd->pt);

                }
            }
        }
      else
        {
          if (thrd->timers.t0
              > 0&& (time(NULL) - thrd->timers.t0) > T_THRD_PROC_TIMEOUT)
            {
              print_str("WARNING: [%X]: thread not responding\n", thrd->pt);
            }
        }

      pthread_mutex_unlock(&thrd->mutex);

      ptr = ptr->next;
    }
  pthread_mutex_unlock(&_net_thrd_r.mutex);
}

#define NET_WTHRD_CLEANUP_TIMEOUT               (time_t) 25

#define NET_TMON_SLEEP_DEFAULT                  (unsigned int) 15

int
net_deploy(void)
{
  if (((int) net_opts.thread_l + (int) net_opts.thread_r)
      > net_opts.max_worker_threads)
    {
      print_str(
          "ERROR: net_deploy: requested thread count exceeds 'max_worker_threads' [%d/%d]\n",
          ((int) net_opts.thread_l + (int) net_opts.thread_r),
          net_opts.max_worker_threads);
      return -1;
    }

  //_m_tid = getpid();

#ifdef M_ARENA_TEST
  mallopt(M_ARENA_TEST, 1);
#endif
#ifdef M_ARENA_MAX
  mallopt(M_ARENA_MAX, 1);
#endif
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);

  int sr = pthread_sigmask(SIG_BLOCK, &set, NULL);

  if (sr != 0)
    {
      print_str("ERROR: net_worker: pthread_sigmask failed: %d\n", sr);
      abort();
      return 1;
    }

  md_init_le(&_sock_r, (int) net_opts.max_sock);
  md_init_le(&_net_thrd_r, (int) net_opts.max_worker_threads);

  ssl_init();

  int r;

  if ((r = spawn_threads(net_opts.thread_l, net_worker, 0, &_net_thrd_r,
  THREAD_ROLE_NET_WORKER,
  SOCKET_OPMODE_LISTENER)))
    {
      print_str("ERROR: spawn_threads failed [SOCKET_OPMODE_LISTENER]: %d\n",
          r);
      return 2;
    }
  else
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str(
              "NOTICE: deployed %hu socket worker threads [SOCKET_OPMODE_LISTENER]\n",
              net_opts.thread_l);
        }
    }

  if ((r = spawn_threads(net_opts.thread_r, net_worker, 0, &_net_thrd_r,
  THREAD_ROLE_NET_WORKER,
  SOCKET_OPMODE_RECIEVER)))
    {
      print_str("ERROR: spawn_threads failed [SOCKET_OPMODE_RECIEVER]: %d\n",
          r);
      return 2;
    }
  else
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str(
              "NOTICE: deployed %hu socket worker threads [SOCKET_OPMODE_RECIEVER]\n",
              net_opts.thread_r);
        }
    }

  if (process_ca_requests(&_boot_pca))
    {
      print_str(
          "WARNING: process_ca_requests: not all connection requests were succesfull\n");
    }

  unsigned int tmon_ld = NET_TMON_SLEEP_DEFAULT;

  while (g_get_gkill())
    {
      if (thread_register_count(&_sock_r) == 0)
        {
          print_str("WARNING: nothing left to process\n");
          break;
        }
      else
        {
          net_ping_threads();
        }
      sleep(tmon_ld);
      mutex_lock(&mutex_glob00);
      if (status & F_STATUS_MSIG00)
        {
          status ^= F_STATUS_MSIG00;
          tmon_ld = NET_TMON_SLEEP_DEFAULT;
        }
      pthread_mutex_unlock(&mutex_glob00);
      tmon_ld *= 2;
    }

  if (gfl & F_OPT_VERBOSE3)
    {
      print_str("NOTICE: sending F_OPSOCK_TERM to all sockets\n");
    }

  net_nw_ssig_term_r(&_sock_r);

  if (gfl & F_OPT_VERBOSE3)
    {
      print_str("NOTICE: sending F_THRD_TERM to all worker threads\n");
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: waiting for threads to exit..\n");
    }

  time_t s = time(NULL), e;
  off_t l_count;

  while ((l_count = thread_register_count(&_net_thrd_r)) > 0)
    {
      thread_broadcast_kill(&_net_thrd_r);
      sleep(1);
      e = time(NULL);
      if ((e - s) > NET_WTHRD_CLEANUP_TIMEOUT)
        {
          print_str("WARNING: %llu worker threads remaining\n", l_count);
          break;
        }
    }

  md_g_free(&_net_thrd_r);
  md_g_free(&_sock_r);

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: server shutting down..\n");
    }

  return 0;
}

static void
net_proc_piped_q(__sock_o pso, __g_handle hdl)
{
  uint8_t buffer[8192];

  ssize_t r_sz;

  while ((r_sz = read(hdl->pfd_out[0], buffer, sizeof(buffer))) > 0)
    {
      if (net_send_direct(pso, (const void*) buffer, (size_t) r_sz) == -1)
        {
          printf(
              "ERROR: net_proc_piped_q: net_send_direct failed, socket: [%d]\n",
              pso->sock);
        }
    }

  if (r_sz == -1)
    {
      print_str("ERROR: net_proc_piped_q: [%d]: pipe read failed [%s]\n",
          pso->sock, g_strerr_r(errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
    }

  if (close(hdl->pfd_out[0]) == -1)
    {
      print_str("ERROR: net_proc_piped_q: [%d]: pipe close failed [%s]\n",
          pso->sock, g_strerr_r(errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
    }

  hdl->flags ^= F_GH_EXECRD_HAS_PIPE;

}

int
net_baseline_gl_data_in(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  if ((pso->counters.b_read < pso->unit_size) || (pso->flags & F_OPSOCK_TERM))
    {
      pthread_mutex_unlock(&pso->mutex);
      return 0;
    }

  //print_str("NOTICE: got packet [%s]\n", pso->st_p0);

  __g_handle hdl = (__g_handle ) pso->va_p0;

  int r;

  if ((r = g_bmatch(data, hdl, &hdl->buffer)))
    {
      if (r == -1)
        {
          print_str(
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
      hdl->v_b0 = pso->st_p1;
      hdl->g_proc4((void*) hdl, data, (void*) pso);
    }

  omfp_timeout;

  l_end_at: ;

  if ((hdl->flags & F_GH_EXECRD_PIPE_OUT)
      && (hdl->flags & F_GH_EXECRD_HAS_PIPE))
    {
      net_proc_piped_q(pso, hdl);
    }

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

static int
net_gl_socket_destroy(__sock_o pso)
{
  int _tid = syscall(SYS_gettid);

  mutex_lock(&pso->mutex);

  __g_handle hdl = (__g_handle ) pso->va_p0;

  /*if ( NULL != hdl->v_b0)
   {
   free(hdl->v_b0);
   }/*/

  int r = g_cleanup(hdl);

  if (NULL != pso->va_p0)
    {
      free(pso->va_p0);
      pso->va_p0 = NULL;
    }

  if (gfl & F_OPT_VERBOSE3)
    {
      print_str("NOTICE: [%d] socket closed: [%d]\n", (int) _tid, pso->sock);
    }

  pthread_mutex_unlock(&pso->mutex);

  return r;
}

uint16_t
net_get_addrinfo_port(__sock_o pso)
{
  void *port_data;
  switch (pso->res->ai_family)
    {
  case AF_INET:
    ;
    port_data = &((struct sockaddr_in*) pso->res->ai_addr)->sin_port;
    break;
  case AF_INET6:
    ;
    port_data = &((struct sockaddr_in6*) pso->res->ai_addr)->sin6_port;
    break;
  default:
    ;
    return 0;
    break;
    }

  return ntohs(*((uint16_t*) port_data));
}

const char *
net_get_addrinfo_ip(__sock_o pso, char *out, socklen_t len)
{
  void *ip_data;
  switch (pso->res->ai_family)
    {
  case AF_INET:
    ;
    ip_data = (void*) &((struct sockaddr_in*) pso->res->ai_addr)->sin_addr;
    break;
  case AF_INET6:
    ;
    ip_data = (void*) &((struct sockaddr_in6*) pso->res->ai_addr)->sin6_addr;
    break;
  default:
    ;
    out[0] = 1;
    return out;
    }

  return inet_ntop(pso->res->ai_family, ip_data, out, len);

}

int
net_gl_socket_init0(__sock_o pso)
{
  switch (pso->oper_mode)
    {
  case SOCKET_OPMODE_RECIEVER:
    ;

    if (pso->parent == NULL)
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
        print_str("ERROR: net_gl_socket_init0: [%d] missing log pointer\n",
            pso->sock);
        pso->flags |= F_OPSOCK_TERM;
        return 2;
      }

    __g_handle hdl;

    hdl = pso->va_p0 = calloc(1, sizeof(_g_handle));

    char *p_logf = g_dgetf((char*) pso->st_p0);

    if (NULL == p_logf)
      {
        print_str(
            "ERROR: net_gl_socket_init0: [%d] non existant log type: '%s'\n",
            pso->sock, (char *) pso->st_p0);
        pso->flags |= F_OPSOCK_TERM;
        return 2;
      }

    if (determine_datatype(hdl, p_logf))
      {
        print_str(
            "ERROR: net_gl_socket_init0: [%d] non existant log reference: '%s'\n",
            pso->sock, p_logf);
        pso->flags |= F_OPSOCK_TERM;
        return 2;
      }

    int r;

    hdl->flags |= (F_GH_W_NSSYS | F_GH_EXECRD_PIPE_OUT);
    snprintf(hdl->file, sizeof(hdl->file), "%s", (char*) pso->st_p0);

    mutex_lock(&mutex_glob00);
    if ((r = g_proc_mr(hdl)))
      {
        pthread_mutex_unlock(&mutex_glob00);
        print_str("ERROR: [%d] net_gl_socket_init0: g_proc_mr failed [%d]\n",
            pso->sock, r);
        pso->flags |= F_OPSOCK_TERM;
        return 2;
      }
    pthread_mutex_unlock(&mutex_glob00);

    hdl->v_b0_sz = MAX_PRINT_OUT - 4;

    int _tid = syscall(SYS_gettid);

    if (gfl & F_OPT_VERBOSE3)
      {
        char ip[128];
        uint16_t port = net_get_addrinfo_port(pso);

        net_get_addrinfo_ip(pso, (char*) ip, sizeof(ip));

        print_str("NOTICE: [%d]: [%d] socket interface active [%s] [%s:%hu]\n",
            _tid, pso->sock, (char*) pso->st_p0, ip, port);
      }

    pso->unit_size = (ssize_t) hdl->block_sz;
    pso->shutdown_cleanup = (_t_stocb) net_gl_socket_destroy;

    kill(getpid(), SIGUSR2);

    break;
    }
  return 0;
}

int
net_gl_socket_init1(__sock_o pso)
{
  switch (pso->oper_mode)
    {
  case SOCKET_OPMODE_RECIEVER:
    ;
    if (pso->parent == NULL)
      {
        break;
      }

    break;
    }

  return 0;
}

int
net_gl_socket_connect_init1(__sock_o pso)
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
net_gl_socket_init1_dc_on_ac(__sock_o pso)
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
    print_str("NOTICE: [%d]: sending F_OPSOCK_TERM to parent: %d\n", pso->sock,
        spso->sock);
    break;
    }

  return 0;
}

