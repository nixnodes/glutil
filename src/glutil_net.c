#ifdef _G_SSYS_NET

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
              printf ("error: net_open_connection: host:%s port:%s, status:[%d] %s\n",
                  pca->host, pca->port, ret, ret < 0 ? strerror(errno) : "");
              fail++;
            }
          break;
          case F_OPSOCK_LISTEN:
          if ((ret = net_open_listening_socket (pca->host, pca->port, pca)))
            {
              printf ("error: net_open_listening_socket: host:%s port:%s, status:[%d] %s\n",
                  pca->host, pca->port, ret, ret < 0 ? strerror(errno) : "");
              fail++;
            }
          break;
        }
      ptr = md_unlink_le(md, ptr);
    }

  return fail;
}

int
net_deploy(void)
{
  md_init_le(&_sock_r, 512);
  md_init_le(&_thrd_r, 64);

  ssl_init();

  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  int s = pthread_sigmask(SIG_BLOCK, &set, NULL);

  if (s != 0)
    {
      print_str("ERROR: pthread_sigmask failed: %d\n", s);
      return 1;
    }

  int r;

  if ((r = spawn_threads(4, net_worker, 0, &_thrd_r, THREAD_ROLE_NET_WORKER,
  SOCKET_OPMODE_LISTENER)))
    {
      print_str("ERROR: spawn_threads failed (listeners): %d\n", r);
      return 2;
    }

  if ((r = spawn_threads(8, net_worker, 0, &_thrd_r, THREAD_ROLE_NET_WORKER,
  SOCKET_OPMODE_RECIEVER)))
    {
      print_str("ERROR: spawn_threads failed (recievers): %d\n", r);
      return 2;
    }

  if (process_ca_requests(&_boot_pca))
    {
      print_str(
          "WARNING: process_ca_requests: not all connection requests were succesfull\n");
    }

  while (!(gfl & F_OPT_KILL_GLOBAL))
    {
      sleep(5);
    }

  return 0;
}

int
net_baseline_gl_data_in(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  if (pso->counters.b_read < pso->unit_size)
    {
      pthread_mutex_unlock(&pso->mutex);
      return 0;
    }

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
      goto l_end;
    }

  omfp_timeout;

  hdl->g_proc4((void*) hdl, data, (void*) pso);

  l_end: ;

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

static int
net_gl_socket_destroy(__sock_o pso)
{
  mutex_lock(&pso->mutex);

  int r = g_cleanup((__g_handle ) pso->va_p0);
  free(pso->va_p0);
  pso->va_p0 = NULL;

  pthread_mutex_unlock(&pso->mutex);

  return r;
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

    print_str("NOTICE: accepted %d\n", pso->sock);

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

    if ((r = g_proc_mr(hdl)))
      {
        print_str("ERROR: [%d] net_gl_socket_init0: g_proc_mr failed [%d]\n",
            pso->sock, r);
        pso->flags |= F_OPSOCK_TERM;
        return 2;
      }

    if (gfl & F_OPT_VERBOSE)
      {
        print_str("NOTICE: [%d] socket interface active; '%s'\n", pso->sock,
            (char*) pso->st_p0);
      }

    pso->unit_size = (ssize_t) hdl->block_sz;
    pso->shutdown_cleanup = (_t_stocb) net_gl_socket_destroy;

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

#endif
