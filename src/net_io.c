/*
 * net_io.c
 *
 *  Created on: Dec 29, 2013
 *      Author: reboot
 */

#include <net_io.h>
#include <thread.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

static pthread_mutex_t *mutex_buf = NULL;

static void
ssl_locking_function(int mode, int n, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    {
      pthread_mutex_lock(&mutex_buf[n]);
    }
  else
    {
      pthread_mutex_unlock(&mutex_buf[n]);
    }
}

static unsigned long
ssl_id_function(void)
{
  return ((unsigned long) pthread_self());
}

void
ssl_init(void)
{
  int i;

  /* static locks area */
  mutex_buf = malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
  if (mutex_buf == NULL)
    {
      print_str("ERROR: ssl_init could not allocate mutex memory\n");
      abort();
    }
  for (i = 0; i < CRYPTO_num_locks(); i++)
    {
      pthread_mutex_init(&mutex_buf[i], NULL);
    }
  /* static locks callbacks */
  CRYPTO_set_locking_callback(ssl_locking_function);
  CRYPTO_set_id_callback(ssl_id_function);

  SSL_library_init();
  OpenSSL_add_all_algorithms(); /* load & register all cryptos, etc. */
  SSL_load_error_strings(); /* load all error messages */
  RAND_load_file("/dev/urandom", 4096);

}

static int
net_chk_timeout(__sock_o pso)
{
  int r = 0;

  mutex_lock(&pso->mutex);

  if (pso->limits.sock_timeout /* idle timeout (data recieve)*/
  && (time(NULL) - pso->timers.last_act) >= pso->limits.sock_timeout)
    {
      if (pso->flags & F_OPSOCK_ST_SSL_ACCEPT)
        {
          print_str("WARNING: SSL_accept timed out [%d]\n", pso->sock);
        }
      else
        {
          print_str("WARNING: idle timeout occured on socket %d\n", pso->sock);
        }
      r = 1;
      goto end;
    }

  end: ;

  pthread_mutex_unlock(&pso->mutex);

  return r;
}

void
ssl_cleanup(void)
{
  int i;

  if (mutex_buf == NULL)
    {
      return;
    }

  /*CRYPTO_set_dynlock_create_callback(NULL);
   CRYPTO_set_dynlock_lock_callback(NULL);
   CRYPTO_set_dynlock_destroy_callback(NULL);*/

  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_id_callback(NULL);

  for (i = 0; i < CRYPTO_num_locks(); i++)
    {
      pthread_mutex_destroy(&mutex_buf[i]);
    }

  free(mutex_buf);
  mutex_buf = NULL;
}

SSL_CTX*
ssl_init_ctx_server(__sock_o pso)
{
  if ((pso->ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
    { /* create new context from method */
      return NULL;
    }
  return pso->ctx;
}

SSL_CTX*
ssl_init_ctx_client(__sock_o pso)
{
  if ((pso->ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
    { /* create new context from method */
      return NULL;
    }
  return pso->ctx;
}

int
ssl_load_certs(SSL_CTX* ctx, char* cert_file, char* key_file)
{
  /* set the local certificate from CertFile */
  if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0)
    {
      return 1;
    }
  /* set the private key from KeyFile (may be the same as CertFile) */
  if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0)
    {
      return 2;
    }
  /* verify private key */
  if (!SSL_CTX_check_private_key(ctx))
    {
      print_str("ERROR: private key does not match the public certificate\n");
      return 3;
    }

  return 0;
}

void
ssl_show_certs(SSL* ssl)
{
  X509 *cert;
  char *line;

  cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
  if (cert != NULL)
    {
      print_str("NOTICE: Peer certificates:\n");
      line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
      print_str("NOTICE: Subject: %s\n", line);
      free(line);
      line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
      print_str("NOTICE: Issuer: %s\n", line);
      free(line);
      X509_free(cert);
    }
  else
    {
      print_str("WARNING: No certificates\n");
    }
}

int
bind_socket(int fd, struct addrinfo *aip)
{
  int y = 1;

  if (setsockopt(fd, SOL_SOCKET,
  SO_REUSEADDR, &y, sizeof(int)))
    {
      return 100;
    }

  fcntl(fd, F_SETFL, O_NONBLOCK);

  if (bind(fd, aip->ai_addr, aip->ai_addrlen) == -1)
    {
      return 101;
    }

  if (listen(fd, SOMAXCONN - (SOMAXCONN / 2)))
    {
      return 102;
    }

  return 0;
}

int
net_destroy_connection(__sock_o so)
{
  int ret;

  mutex_lock(&so->mutex);

  if (so->flags & F_OPSOCK_DISCARDED)
    {
      pthread_mutex_unlock(&so->mutex);
      return -1;
    }

  if (so->flags & F_OPSOCK_CONNECT)
    {
      if ((so->flags & F_OPSOCK_SSL) && so->ssl)
        {
          if (!(so->flags & F_OPSOCK_TS_DISCONNECTED)
              && (ret = SSL_shutdown(so->ssl)) < 1)
            {
              if (0 == ret)
                {
                  pthread_mutex_unlock(&so->mutex);
                  return 2;
                }

              int ssl_err = SSL_get_error(so->ssl, ret);
              if ((ssl_err == SSL_ERROR_WANT_READ
                  || ssl_err == SSL_ERROR_WANT_WRITE))
                {
                  pthread_mutex_unlock(&so->mutex);
                  return 2;
                }
              so->s_errno = ssl_err;
              so->status = ret;
              print_str(
                  "ERROR: socket: [%d] SSL_shutdown - code:[%d] sslerr:[%d]\n",
                  so->sock, ret, ssl_err);
              ERR_print_errors_fp(stderr);
            }

          SSL_free(so->ssl);
        }

      if (!(so->flags & F_OPSOCK_TS_DISCONNECTED))
        {
          if ((ret = shutdown(so->sock, SHUT_RDWR)) == -1)
            {
              char err_buffer[1024];
              print_str(

              "ERROR: socket: [%d] shutdown - code:[%d] errno:[%d] %s\n",
                  so->sock, ret, errno,
                  strerror_r(errno, err_buffer, sizeof(err_buffer)));
            }
          else
            {
              while (ret > 0)
                {
                  ret = recv(so->sock, so->buffer0, so->unit_size, 0);
                }
            }
        }
    }

  if ((so->flags & F_OPSOCK_SSL) && so->ctx)
    {
      SSL_CTX_free(so->ctx);
    }

  if (so->res)
    {
      freeaddrinfo(so->res);
    }

  if ((ret = close(so->sock)))
    {
      char err_buffer[1024];
      print_str(
          "ERROR: [%d] unable to close socket - code:[%d] errno:[%d] %s\n",
          so->sock, ret, errno, strerror_r(errno, err_buffer, 1024));
      pthread_mutex_unlock(&so->mutex);
      ret = 1;
    }

  if (NULL != so->buffer0)
    {
      free(so->buffer0);
    }

  md_g_free(&so->sendq);

  so->flags |= F_OPSOCK_DISCARDED;

  pthread_mutex_unlock(&so->mutex);

  return ret;
}

int
unregister_connection(pmda glob_reg, __sock_o pso)
{
  mutex_lock(&glob_reg->mutex);

  p_md_obj ptr = glob_reg->first;

  while (ptr)
    {
      if (((__sock_o ) ptr->ptr)->sock == pso->sock)
        {
          break;
        }
      ptr = ptr->next;
    }

  int r;

  if (ptr)
    {
      md_unlink_le(glob_reg, ptr);
      r = 0;
    }
  else
    {
      r = 1;
    }

  pthread_mutex_unlock(&glob_reg->mutex);

  return r;
}

static void
net_open_connection_cleanup(pmda sockr, struct addrinfo *aip, int fd)
{
  if (aip)
    {
      freeaddrinfo(aip);
    }
  close(fd);
  md_unlink_le(sockr, sockr->pos);
  pthread_mutex_unlock(&sockr->mutex);
}

int
net_open_connection(char *addr, char *port, __sock_ca args)
{
  struct addrinfo *aip;
  struct addrinfo hints =
    { 0 };

  int fd;

  hints.ai_flags = AI_ALL | AI_ADDRCONFIG;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(addr, port, &hints, &aip))
    {
      return -1;
    }

  if ((fd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol)) == -1)
    {
      freeaddrinfo(aip);
      return -2;
    }

  if (connect(fd, aip->ai_addr, aip->ai_addrlen) == -1)
    {
      freeaddrinfo(aip);
      close(fd);
      return -3;
    }

  fcntl(fd, F_SETFL, O_NONBLOCK);

  mutex_lock(&args->socket_register->mutex);

  __sock_o pso;

  if (!(pso = md_alloc_le(args->socket_register, sizeof(_sock_o), 0, NULL)))
    {
      freeaddrinfo(aip);
      close(fd);
      pthread_mutex_unlock(&args->socket_register->mutex);
      return 9;
    }

  pso->res = aip;
  pso->sock = fd;
  pso->oper_mode = SOCKET_OPMODE_RECIEVER;
  pso->flags = args->flags;
  pso->limits.sock_timeout = SOCK_DEFAULT_IDLE_TIMEOUT;
  pso->pcheck_r = (_t_stocb) net_chk_timeout;
  pso->timers.last_act = time(NULL);
  pso->st_p0 = args->st_p0;

  if (!args->unit_size)
    {
      pso->buffer0 = malloc(SOCK_RECVB_SZ);
      pso->unit_size = SOCK_RECVB_SZ;
    }
  else
    {
      pso->buffer0 = malloc(args->unit_size);
      pso->unit_size = args->unit_size;
    }

  pso->host_ctx = args->socket_register;

  if (mutex_init(&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      net_open_connection_cleanup(args->socket_register, aip, fd);
      return 10;
    }

  if (args->flags & F_OPSOCK_INIT_SENDQ)
    {
      md_init_le(&pso->sendq, 8192);
    }

  int r;

  if (args->flags & F_OPSOCK_SSL)
    {
      if (!ssl_init_ctx_client(pso))
        {
          net_open_connection_cleanup(args->socket_register, aip, fd);
          ERR_print_errors_fp(stderr);
          return 11;
        }

      if ((pso->ssl = SSL_new(pso->ctx)) == NULL)
        { /* get new SSL state with context */
          net_open_connection_cleanup(args->socket_register, aip, fd);
          ERR_print_errors_fp(stderr);
          return 12;
        }

      SSL_set_fd(pso->ssl, pso->sock);

      if (args->ssl_cert && args->ssl_key)
        {
          if ((r = ssl_load_certs(pso->ctx, args->ssl_cert, args->ssl_key)))
            {
              net_open_connection_cleanup(args->socket_register, aip, fd);
              print_str(
                  "ERROR: [%d] could not load SSL certificate/key pair [%d]: %s\n",
                  fd, r, args->ssl_cert);
              ERR_print_errors_fp(stderr);
              return 15;
            }
        }

      pso->flags |= F_OPSOCK_ST_SSL_CONNECT;
      pso->limits.sock_timeout = SOCK_SSL_CONNECT_TIMEOUT;
      pso->rcv_cb = (_p_s_cb) net_connect_ssl;
      pso->rcv_cb_t = (_p_s_cb) net_recv_ssl;
      pso->send0 = (_p_ssend) NULL;
    }
  else
    {
      pso->limits.sock_timeout = SOCK_DEFAULT_IDLE_TIMEOUT;
      pso->rcv_cb = (_p_s_cb) net_recv;
      if (args->flags & F_OPSOCK_INIT_SENDQ)
        {
          pso->send0 = (_p_ssend) net_ssend;
        }
      else
        {
          pso->send0 = (_p_ssend) net_ssend_b;
        }
    }

  pso->rcv1 = (_p_s_cb) args->proc;

  if (args->rc0)
    {
      args->rc0(pso);
    }

  if ((r = push_object_to_thread(pso, args->thread_register,
      (dt_score_ptp) net_get_score)))
    {
      print_str("ERROR: push_object_to_thread failed, code %d, sock %d\n", r,
          pso->sock);
      net_open_connection_cleanup(args->socket_register, aip, fd);
      return 13;
    }

  mutex_lock(&pso->mutex);

  if (args->rc1)
    {
      args->rc1(pso);
    }

  pso->flags |= F_OPSOCK_ACT;

  pthread_mutex_unlock(&pso->mutex);

  pthread_mutex_unlock(&args->socket_register->mutex);

  return 0;
}

static void
net_open_listening_socket_cleanup(pmda sockr, struct addrinfo *aip, int fd)
{
  if (aip)
    {
      freeaddrinfo(aip);
    }
  close(fd);
  md_unlink_le(sockr, sockr->pos);
  pthread_mutex_unlock(&sockr->mutex);
}

int
net_open_listening_socket(char *addr, char *port, __sock_ca args)
{
  struct addrinfo *aip;
  struct addrinfo hints =
    { 0 };

  int fd;

  errno = 0;

  hints.ai_flags = AI_ALL | AI_ADDRCONFIG;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(addr, port, &hints, &aip))
    {
      return -1;
    }

  if ((fd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol)) == -1)
    {
      freeaddrinfo(aip);
      return -2;
    }

  if (bind_socket(fd, aip))
    {
      freeaddrinfo(aip);
      close(fd);
      return -3;
    }

  mutex_lock(&args->socket_register->mutex);

  __sock_o pso;

  if (!(pso = md_alloc_le(args->socket_register, sizeof(_sock_o), 0, NULL)))
    {
      freeaddrinfo(aip);
      close(fd);
      pthread_mutex_unlock(&args->socket_register->mutex);
      return 9;
    }

  pso->res = aip;
  pso->sock = fd;
  pso->oper_mode = SOCKET_OPMODE_LISTENER;
  pso->flags = args->flags;
  pso->pcheck_r = (_t_stocb) net_chk_timeout;
  pso->rcv_cb = (_p_s_cb) net_accept;
  pso->host_ctx = args->socket_register;
  pso->unit_size = args->unit_size;
  pso->st_p0 = args->st_p0;

  if (mutex_init(&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      net_open_listening_socket_cleanup(args->socket_register, aip, fd);
      return 10;
    }

  int r;

  if (args->flags & F_OPSOCK_SSL)
    {
      if (!ssl_init_ctx_server(pso))
        {
          net_open_listening_socket_cleanup(args->socket_register, aip, fd);
          ERR_print_errors_fp(stderr);
          return 11;
        }

      if ((r = ssl_load_certs(pso->ctx, args->ssl_cert, args->ssl_key)))
        {
          net_open_listening_socket_cleanup(args->socket_register, aip, fd);
          print_str(
              "ERROR: [%d] could not load SSL certificate/key pair [%d]: %s / %s\n",
              fd, r, args->ssl_cert, args->ssl_key);
          ERR_print_errors_fp(stderr);
          return 12;
        }

      pso->rcv0 = (_p_s_cb) net_recv_ssl;
    }
  else
    {
      pso->rcv0 = (_p_s_cb) net_recv;
    }

  pso->rcv1_t = (_p_s_cb) args->proc;

  mutex_lock(&pso->mutex);

  if (args->rc0)
    {
      args->rc0(pso);
      pso->rc0 = (_t_stocb) args->rc0;
    }

  pthread_mutex_unlock(&pso->mutex);

  if ((r = push_object_to_thread(pso, args->thread_register,
      (dt_score_ptp) net_get_score)))
    {
      print_str("ERROR: push_object_to_thread failed, code %d, sock %d\n", r,
          pso->sock);
      net_open_listening_socket_cleanup(args->socket_register, aip, fd);
      return 13;
    }

  mutex_lock(&pso->mutex);

  if (args->rc1)
    {
      args->rc1(pso);
      pso->rc1 = (_t_stocb) args->rc1;
    }

  pso->flags |= F_OPSOCK_ACT;

  pthread_mutex_unlock(&pso->mutex);

  pthread_mutex_unlock(&args->socket_register->mutex);

  return 0;
}

void
net_send_sock_term_sig(__sock_o pso)
{
  mutex_lock(&pso->mutex);

  pso->flags |= F_OPSOCK_TERM;

  pthread_mutex_unlock(&pso->mutex);
}

float
net_get_score(pmda in, pmda out, __sock_o pso, po_thrd thread)
{
  mutex_lock(&pso->mutex);

  if (thread->oper_mode != pso->oper_mode)
    {
      pthread_mutex_unlock(&pso->mutex);
      return (float) -1.0;
    }

  pthread_mutex_unlock(&pso->mutex);

  float score = (float) (in->offset + out->offset);

  return score;
}

int
net_push_to_sendq(__sock_o pso, void *data, size_t size, uint16_t flags)
{
  mutex_lock(&pso->sendq.mutex);

  if ((pso->flags & F_OPSOCK_TERM))
    {
      pthread_mutex_unlock(&pso->sendq.mutex);
      return 2;
    }

  __sock_sqp ptr;
  if (NULL == (ptr = md_alloc_le(&pso->sendq, SSENDQ_PAYLOAD_SIZE, 0, NULL)))
    {
      pthread_mutex_unlock(&pso->sendq.mutex);
      return -1;
    }

  if (flags & NET_PUSH_SENDQ_ASSUME_PTR)
    {
      ptr->data = data;
    }
  else
    {
      ptr->data = malloc(size);
      memcpy(ptr->data, data, size);
    }

  ptr->size = size;

  pthread_mutex_unlock(&pso->sendq.mutex);

  return 0;
}

int
net_sendq_broadcast(pmda base, __sock_o source, void *data, size_t size)
{
  mutex_lock(&base->mutex);

  int ret = 0;

  p_md_obj ptr = base->first;

  while (ptr)
    {
      __sock_o pso = (__sock_o) ptr->ptr;
      mutex_lock (&pso->mutex);

      if (NULL != source && source->sock == pso->sock)
        {
          goto l_end;
        }

      if (pso->oper_mode != SOCKET_OPMODE_RECIEVER)
        {
          goto l_end;
        }

      if (!(pso->flags & F_OPSOCK_TERM))
        {
          if ((ret += net_push_to_sendq(pso, data, size, 0)) == -1)
            {
              printf ("ERROR: net_sendq_broadcast: net_push_to_sendq failed, socket:[%d]\n", pso->sock);
              net_send_sock_term_sig(pso);
            }
        }

      l_end:;

      pthread_mutex_unlock (&pso->mutex);
      ptr = ptr->next;
    }

  pthread_mutex_unlock(&base->mutex);

  return ret;
}

static void *
net_proc_sendq_destroy_item(__sock_sqp psqp, __sock_o pso, p_md_obj ptr)
{
  free(psqp->data);
  return md_unlink_le(&pso->sendq, ptr);
}

static ssize_t
net_proc_sendq(__sock_o pso)
{
  p_md_obj ptr = pso->sendq.first;

  while (ptr)
    {
      __sock_sqp psqp = (__sock_sqp) ptr->ptr;
      if (psqp->size)
        {
          int ret;
          switch ((ret = pso->send0(pso, psqp->data, psqp->size)))
            {
              case 0:
              ptr = net_proc_sendq_destroy_item(psqp, pso, ptr);
              continue;
              case 1:
              printf ("ERROR: [%d]: net_proc_sendq: send data failed, payload size: %zd\n", pso->sock, psqp->size);
              ptr = net_proc_sendq_destroy_item(psqp, pso, ptr);
              net_send_sock_term_sig(pso);
              goto end;
              case 2:
              break;
            }

        }
      ptr = ptr->next;
    }

  end: ;
  ssize_t off_ret = pso->sendq.offset;

  return off_ret;
}

void
net_nw_ssig_term_r(pmda objects)
{
  mutex_lock(&objects->mutex);

  p_md_obj ptr = objects->first;

  while (ptr)
    {
      __sock_o pso = (__sock_o) ptr->ptr;

      mutex_lock(&pso->mutex);

      if (!(pso->flags & F_OPSOCK_TERM))
        {
          pso->flags |= F_OPSOCK_TERM;
        }

      pthread_mutex_unlock(&pso->mutex);

      ptr = ptr->next;
    }

  pthread_mutex_unlock(&objects->mutex);
}

#define NET_SOCKWAIT_TO         ((time_t)30)

static int
net_proc_sock_term(po_thrd thrd)
{

  off_t num_active = 0;
  num_active += md_get_off_ts(&thrd->proc_objects);
  num_active += md_get_off_ts(&thrd->in_objects);

  if (num_active == 0)
    {
      return 0;
    }
  else
    {
      return 1;
    }

}

int
net_worker(void *args)
{
  p_md_obj ptr;
  int r;
  uint8_t int_state = 0;
  uint32_t pooling_timeout = SOCKET_POOLING_FREQUENCY_MIN;

  po_thrd thrd = (po_thrd) args;

  char buffer0[1024];

  mutex_lock(&thrd->mutex);

  thrd->timers.t1 = time(NULL);

  //thrd->buffer0 = malloc(THREAD_DEFAULT_BUFFER0_SIZE);
  //thrd->buffer0_size = THREAD_DEFAULT_BUFFER0_SIZE;

  pthread_t _pt = thrd->pt;
  int _tid = syscall(SYS_gettid);

  pthread_mutex_unlock(&thrd->mutex);

  for (;;)
    {
      mutex_lock(&thrd->mutex);

      if (thrd->flags & F_THRD_TERM)
        {
          if (net_proc_sock_term(thrd) == 0)
            {
              /*print_str("NOTICE: net_worker: [%d]: thread shutting down..\n",
               _tid);*/
              pthread_mutex_unlock(&thrd->mutex);
              break;
            }
        }

      pthread_mutex_unlock(&thrd->mutex);

      mutex_lock(&thrd->in_objects.mutex);

      if (0 == thrd->in_objects.offset)
        {
          pthread_mutex_unlock(&thrd->in_objects.mutex);
          goto begin_proc;
        }
      else
        {
          /*
           print_str(
           "NOTICE: [%d]: push %llu items onto worker thread stack, %llu exist in chain\n",
           _tid, (unsigned long long int) thrd->in_objects.offset,
           (unsigned long long int) thrd->proc_objects.offset);
           */
        }

      mutex_lock(&thrd->proc_objects.mutex);

      p_md_obj iobj_ptr = thrd->in_objects.first;
      while (iobj_ptr)
        {
          if (thrd->proc_objects.offset == thrd->proc_objects.count)
            {
              break;
            }
          __sock_o t_pso = (__sock_o ) iobj_ptr->ptr;

          mutex_lock(&t_pso->mutex);
          if (!(t_pso->flags & F_OPSOCK_ACT))
            {
              print_str("NOTICE: [%d]: not importing %d, still active\n",
                  (int) _tid, t_pso->sock);
              pthread_mutex_unlock(&t_pso->mutex);
              goto io_loend;
            }
          pthread_mutex_unlock(&t_pso->mutex);

          if (!md_alloc_le(&thrd->proc_objects, 0, 0, iobj_ptr->ptr))
            {
              break;
            }

          int_state |= F_WORKER_INT_STATE_ACT;
          md_unlink_le(&thrd->in_objects, iobj_ptr);

          io_loend: ;

          iobj_ptr = iobj_ptr->next;
        }

      pthread_mutex_unlock(&thrd->in_objects.mutex);

      if (!thrd->proc_objects.offset)
        {
          pthread_mutex_unlock(&thrd->proc_objects.mutex);
          goto loop_end;
        }

      pthread_mutex_unlock(&thrd->proc_objects.mutex);

      begin_proc: ;

      ptr = thrd->proc_objects.first;

      while (ptr)
        {
          __sock_o pso = (__sock_o ) ptr->ptr;

          if (NULL == pso)
            {
              print_str("ERROR: proc_objects empty member\n");
              abort();
            }

          mutex_lock(&pso->mutex);

          if (pso->flags & F_OPSOCK_TERM)
            {
              errno = 0;

              if (2 == (r = net_destroy_connection(pso)))
                {
                  goto l_end;
                }

              if (r == 1)
                {
                  print_str(
                      "ERROR: [%d] net_destroy_connection failed, socket [%d], critical\n",
                      (int) _tid, pso->sock);
                  abort();
                }

              pthread_mutex_unlock(&pso->mutex);

              pmda host_ctx = pso->host_ctx;

              if (pso->shutdown_cleanup)
                {
                  pso->shutdown_cleanup(pso);
                }

              ptr = md_unlink_le(&thrd->proc_objects, ptr);

              if (unregister_connection(host_ctx, pso))
                {
                  print_str(
                      "ERROR: could not find socket entry in register: %d (report this)\n",
                      pso->sock);
                  continue;
                }

              int_state |= F_WORKER_INT_STATE_ACT;

              continue;
            }

          if ((r = pso->pcheck_r(pso)))
            {
              pso->flags |= F_OPSOCK_TERM;
            }

          pso->flags |= F_OPSOCK_ST_HOOKED;

          errno = 0;

          //pthread_mutex_unlock(&pso->mutex);

          switch ((r = pso->rcv_cb(pso, pso->host_ctx, &_net_thrd_r,
              pso->buffer0)))
            {
          case 2:
            //mutex_lock(&pso->mutex);
            /*if (!pso->counters.b_read)
             {
             mutex_lock(&pso->sendq.mutex);
             if (!pso->sendq.offset)
             {
             pthread_mutex_unlock(&pso->sendq.mutex);
             //pthread_mutex_unlock(&pso->mutex);
             goto e_end;
             }
             pthread_mutex_unlock(&pso->sendq.mutex);
             }*/

            if (pso->counters.b_read)
              {
                break;
              }
            else
              {
                goto send_q;
              }

            //pthread_mutex_unlock(&pso->mutex);
            break;
          case 0:
            int_state |= F_WORKER_INT_STATE_ACT;
            break;
          default:
            print_str("ERROR: %s: socket:[%d] status:[%d] errno:[%d] %s\n",
                pso->oper_mode == SOCKET_OPMODE_LISTENER ? "rx/tx accept" :
                pso->oper_mode == SOCKET_OPMODE_RECIEVER ?
                    "rx/tx data" : "socket operation", pso->sock, pso->status,
                errno,
                errno ? strerror_r(errno, (char*) buffer0, 1024) : "");

            //mutex_lock(&pso->mutex);

            pso->flags |= F_OPSOCK_TERM;

            if (!pso->counters.b_read)
              {
                goto e_end;
              }

            //pthread_mutex_unlock(&pso->mutex);

            break;
            }

          if (NULL != pso->rcv1)
            {
              switch ((r = pso->rcv1(pso, pso->host_ctx, &_net_thrd_r,
                  pso->buffer0)))
                {
              case 0:
                break;
              case -2:
                break;
              default:
                // mutex_lock(&pso->mutex);

                print_str(
                    "ERROR: data processor failed with status %d, socket: [%d]\n",
                    r, pso->sock);

                pso->flags |= F_OPSOCK_TERM;

                //pthread_mutex_unlock(&pso->mutex);

                break;
                }
            }

          if (pso->unit_size == pso->counters.b_read)
            {
              pso->counters.b_read = 0;
            }

          send_q: ;

          mutex_lock(&pso->sendq.mutex);

          if (pso->sendq.offset > 0)
            {
              ssize_t sendq_rem;
              if ((sendq_rem = net_proc_sendq(pso)) != 0)
                {
                  print_str("WARNING: sendq: %zd items remain, socket:[%d]\n",
                      pso->sendq.offset, pso->sock);
                }
            }

          pthread_mutex_unlock(&pso->sendq.mutex);

          e_end: ;

          //mutex_lock(&pso->mutex);

          if (pso->flags & F_OPSOCK_ST_HOOKED)
            {
              pso->flags ^= F_OPSOCK_ST_HOOKED;
            }

          l_end: ;

          pthread_mutex_unlock(&pso->mutex);

          ptr = ptr->next;
        }

      loop_end: ;

      thrd->timers.t0 = time(NULL);

      if (int_state & F_WORKER_INT_STATE_ACT)
        {
          int_state ^= F_WORKER_INT_STATE_ACT;
          pooling_timeout = SOCKET_POOLING_FREQUENCY_MIN;
          thrd->timers.t1 = time(NULL);
        }
      else
        {
          if (pooling_timeout < SOCKET_POOLING_FREQUENCY_MAX)
            {
              int32_t thread_inactive = (int32_t) (time(NULL) - thrd->timers.t1);
              /*print_str("%d - throttling pooling interval.. %u - %d - %u\n",
                  (int) _tid, pooling_timeout, thread_inactive);*/
              pooling_timeout = (pooling_timeout * (thread_inactive/4))
                  + SOCKET_POOLING_FREQUENCY_MIN;
            }
        }

      usleep(pooling_timeout);

      //print_str("%d - pooling socket..\n", (int) _tid);
    }

  pmda thread_host_ctx = thrd->host_ctx;

  mutex_lock(&thread_host_ctx->mutex);

  p_md_obj ptr_thread = search_thrd_id(thread_host_ctx, &_pt);

  if (NULL == ptr_thread)
    {
      print_str(
          "ERROR: net_worker: [%d]: thread already unregistered on exit (search_thrd_id failed)\n",
          (int) _tid);
    }
  else
    {
      md_unlink_le(&_net_thrd_r, ptr_thread);
    }

  pthread_mutex_unlock(&thread_host_ctx->mutex);

  return 0;
}

int
net_accept(__sock_o spso, pmda base, pmda threadr, void *data)
{
  int fd;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  struct sockaddr_in a;

  mutex_lock(&spso->mutex);

  spso->s_errno = 0;

  if ((fd = accept(spso->sock, (struct sockaddr *) &a, &sin_size)) == -1)
    {
      spso->status = -1;
      pthread_mutex_unlock(&spso->mutex);
      pthread_mutex_unlock(&base->mutex);
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          return 2;
        }
      return 1;
    }

  int ret;

  if ((ret = fcntl(fd, F_SETFL, O_NONBLOCK)) == -1)
    {
      close(fd);
      spso->status = -2;
      pthread_mutex_unlock(&spso->mutex);
      pthread_mutex_unlock(&base->mutex);
      return 1;
    }

  pthread_mutex_unlock(&spso->mutex);

  mutex_lock(&base->mutex);

  __sock_o pso;

  if (NULL == (pso = md_alloc_le(base, sizeof(_sock_o), 0, NULL)))
    {
      print_str("ERROR: net_accept: out of resources [%llu/%llu]\n",
          (unsigned long long int) base->offset,
          (unsigned long long int) base->count);
      spso->status = 3;
      close(fd);
      pthread_mutex_unlock(&base->mutex);
      return 0;
    }

  pso->sock = fd;
  pso->rcv0 = spso->rcv1;
  pso->rcv1 = spso->rcv1_t;
  pso->parent = (void *) spso;
  pso->st_p0 = spso->st_p0;
  pso->oper_mode = SOCKET_OPMODE_RECIEVER;
  pso->flags |= F_OPSOCK_CONNECT
      | (spso->flags & (F_OPSOCK_SSL | F_OPSOCK_INIT_SENDQ));
  pso->pcheck_r = (_t_stocb) net_chk_timeout;
  pso->timers.last_act = time(NULL);

  if (!spso->unit_size)
    {
      pso->buffer0 = malloc(SOCK_RECVB_SZ);
      pso->unit_size = SOCK_RECVB_SZ;
    }
  else
    {
      pso->unit_size = spso->unit_size;
      pso->buffer0 = malloc(pso->unit_size);
    }

  pso->host_ctx = base;

  if (mutex_init(&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      spso->status = 4;
      shutdown(fd, SHUT_RDWR);
      close(fd);
      md_unlink_le(base, base->pos);
      pthread_mutex_unlock(&base->mutex);
      return 1;
    }

  if (pso->flags & F_OPSOCK_INIT_SENDQ)
    {
      md_init_le(&pso->sendq, 8192);
    }

  p_md_obj pso_ptr = base->pos;

  if (pso->flags & F_OPSOCK_SSL)
    {
      if ((pso->ssl = SSL_new(spso->ctx)) == NULL)
        {
          spso->s_errno = 0;
          spso->status = 5;
          shutdown(fd, SHUT_RDWR);
          close(fd);
          md_unlink_le(base, pso_ptr);
          ERR_print_errors_fp(stderr);
          return 1;
        }

      SSL_set_fd(pso->ssl, pso->sock);
      SSL_set_accept_state(pso->ssl);
      SSL_set_read_ahead(pso->ssl, 1);

      pso->limits.sock_timeout = SOCK_SSL_ACCEPT_TIMEOUT;
      pso->flags |= F_OPSOCK_ST_SSL_ACCEPT;
      pso->rcv_cb = (_p_s_cb) net_accept_ssl;
      pso->rcv_cb_t = spso->rcv0;
      pso->send0 = (_p_ssend) NULL;
    }
  else
    {
      pso->limits.sock_timeout = SOCK_DEFAULT_IDLE_TIMEOUT;
      pso->rcv_cb = spso->rcv0;

      if (pso->flags & F_OPSOCK_INIT_SENDQ)
        {
          pso->send0 = (_p_ssend) net_ssend;
        }
      else
        {
          pso->send0 = (_p_ssend) net_ssend_b;
        }
    }

  if (spso->rc0)
    {
      spso->rc0((void*) pso);
    }

  pthread_mutex_unlock(&base->mutex);

  int r;

  if ((r = push_object_to_thread(pso, threadr, (dt_score_ptp) net_get_score)))
    {
      print_str(
          "ERROR: push_object_to_thread failed, code %d, sock %d (accept)\n", r,
          pso->sock);
      mutex_lock(&spso->mutex);
      spso->status = 6;
      spso->s_errno = 0;
      pthread_mutex_unlock(&spso->mutex);
      net_destroy_connection(pso);
      md_unlink_le(base, pso_ptr);
      return 1;
    }

  mutex_lock(&pso->mutex);

  if (spso->rc1)
    {
      spso->rc1((void*) pso);
    }

  if (!(pso->flags & F_OPSOCK_SSL))
    {
      pso->flags |= F_OPSOCK_ACT;

    }

  pthread_mutex_unlock(&pso->mutex);

  /*if (!(pso->flags & F_OPSOCK_SSL))
   {
   printf ("NOTICE: accept: %d, count: %llu ..\n", fd,
   (unsigned long long int) base->offset);
   pso->flags |= F_OPSOCK_ACT;
   }

   pthread_mutex_unlock (&base->mutex);
   pthread_mutex_unlock (&pso->mutex);*/

  return 0;
}

int
net_recv(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  ssize_t rcv_limit = pso->unit_size - pso->counters.b_read;

  if (rcv_limit <= 0)
    {
      pthread_mutex_unlock(&pso->mutex);
      return 0;
    }

  ssize_t rcvd = recv(pso->sock, ((uint8_t*) data + pso->counters.b_read),
      rcv_limit, 0);

  if (0 == rcvd)
    {
      pso->timers.last_act = time(NULL);
      pso->flags |= F_OPSOCK_TERM | F_OPSOCK_TS_DISCONNECTED;
      goto fin;
    }

  if (rcvd == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          pthread_mutex_unlock(&pso->mutex);
          return 2;
        }

      pso->s_errno = errno;
      pso->status = -1;
      pso->flags |= F_OPSOCK_TERM;
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  pso->timers.last_act = time(NULL);
  pso->counters.b_read += rcvd;
  pso->counters.t_read += rcvd;

  fin: ;

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

int
net_recv_ssl(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  ssize_t rcv_limit = pso->unit_size - pso->counters.b_read;

  if (rcv_limit <= 0)
    {
      pthread_mutex_unlock(&pso->mutex);
      return 0;
    }

  int rcvd;
  ssize_t session_rcvd = 0;

  while (rcv_limit > 0
      && (rcvd = SSL_read(pso->ssl, (data + pso->counters.b_read),
          (int) rcv_limit)) > 0)
    {
      pso->counters.b_read += (ssize_t) rcvd;
      pso->counters.t_read += (ssize_t) rcvd;
      rcv_limit = pso->unit_size - pso->counters.b_read;
      session_rcvd += (ssize_t) rcvd;
    }

  if (rcvd < 1)
    {
      if (session_rcvd)
        {
          pso->timers.last_act = time(NULL);
        }

      pso->s_errno = SSL_get_error(pso->ssl, rcvd);
      if (pso->s_errno == SSL_ERROR_WANT_READ
          || pso->s_errno == SSL_ERROR_WANT_WRITE)
        {
          pthread_mutex_unlock(&pso->mutex);
          return 2;
        }

      pso->status = rcvd;

      ERR_print_errors_fp(stderr);

      if ((pso->s_errno == SSL_ERROR_WANT_CONNECT
          || pso->s_errno == SSL_ERROR_WANT_ACCEPT
          || pso->s_errno == SSL_ERROR_WANT_X509_LOOKUP))
        {
          pthread_mutex_unlock(&pso->mutex);
          return 2;
        }

      pso->flags |= F_OPSOCK_TERM;

      if (rcvd == 0)
        {
          pso->flags |= F_OPSOCK_TS_DISCONNECTED;
          if (!pso->counters.b_read)
            {
              pthread_mutex_unlock(&pso->mutex);
              return 2;
            }
        }

      pthread_mutex_unlock(&pso->mutex);

      return 1;

    }

  pso->timers.last_act = time(NULL);

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

int
net_accept_ssl(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  int ret;

  if ((ret = SSL_accept(pso->ssl)) != 1)
    {
      int ssl_err = SSL_get_error(pso->ssl, ret);

      if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
        {
          pthread_mutex_unlock(&pso->mutex);
          return 2;
        }

      pso->s_errno = ssl_err;
      pso->flags |= F_OPSOCK_TERM;
      pso->status = ret;
      ERR_print_errors_fp(stderr);
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  pso->timers.last_act = time(NULL);
  pso->rcv_cb = pso->rcv_cb_t;
  pso->limits.sock_timeout = SOCK_DEFAULT_IDLE_TIMEOUT;
  pso->flags ^= F_OPSOCK_ST_SSL_ACCEPT;

  print_str("NOTICE: SSL_accept: %d, count: %llu - %s\n", pso->sock,
      (unsigned long long int) base->offset, SSL_get_cipher(pso->ssl));

  BIO_set_buffer_size(pso->ssl->rbio, 65536);

  ssl_show_certs(pso->ssl);

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

int
net_connect_ssl(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);
  int ret;

  if ((ret = SSL_connect(pso->ssl)) != 1)
    {
      int ssl_err = SSL_get_error(pso->ssl, ret);

      if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
        {
          pthread_mutex_unlock(&pso->mutex);
          return 2;
        }

      pso->flags |= F_OPSOCK_TERM;
      pso->status = ret;
      pso->s_errno = ssl_err;
      ERR_print_errors_fp(stderr);

      print_str("ERROR: SSL_connect failed socket:[%d] code:[%d] sslerr:[%d]\n",
          pso->sock, ret, ssl_err);

      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  pso->timers.last_act = time(NULL);
  pso->rcv_cb = pso->rcv_cb_t;
  pso->limits.sock_timeout = SOCK_DEFAULT_IDLE_TIMEOUT;
  pso->flags ^= F_OPSOCK_ST_SSL_CONNECT;

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

int
net_ssend_b(__sock_o pso, void *data, size_t length)
{
  mutex_lock(&pso->mutex);

  int ret;

  while ((ret = send(pso->sock, data, length, MSG_NOSIGNAL)) == -1)
    {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK))
        {
          break;
        }
      usleep(1000);
    }

  pthread_mutex_unlock(&pso->mutex);

  return ret;
}

int
net_ssend_ssl_b(__sock_o pso, void *data, size_t length)
{
  if (!length)
    {
      return -2;
    }

  mutex_lock(&pso->mutex);

  int ret;

  while ((ret = SSL_write(pso->ssl, data, length)) < 1)
    {
      int ssl_err = SSL_get_error(pso->ssl, ret);

      if (!(ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE))
        {
          break;
        }

      usleep(1000);
    }

  pthread_mutex_unlock(&pso->mutex);

  return ret;
}

int
net_ssend(__sock_o pso, void *data, size_t length)
{
  int ret;

  mutex_lock(&pso->mutex);

  if ((ret = send(pso->sock, data, length, MSG_NOSIGNAL)) == -1)
    {
      if ((errno == EAGAIN || errno == EWOULDBLOCK))
        {
          pthread_mutex_unlock(&pso->mutex);
          return 2;
        }
      pso->status = -1;
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  if (ret != length)
    {
      pso->status = 11;
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}
