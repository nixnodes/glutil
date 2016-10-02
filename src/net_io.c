/*
 * net_io.c
 *
 *  Created on: Dec 29, 2013
 *      Author: reboot
 */

#include "net_io.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <limits.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#include "thread.h"
#include "misc.h"

static pthread_mutex_t *mutex_buf = NULL;

static void
ssl_locking_function (int mode, int n, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    {
      pthread_mutex_lock (&mutex_buf[n]);
    }
  else
    {
      pthread_mutex_unlock (&mutex_buf[n]);
    }
}

static unsigned long
ssl_id_function (void)
{
  return ((unsigned long) pthread_self ());
}

void
ssl_init (void)
{
  int i;

  CRYPTO_malloc_debug_init()
  ;

  CRYPTO_mem_ctrl (CRYPTO_MEM_CHECK_ON);

  mutex_buf = malloc (CRYPTO_num_locks () * sizeof(pthread_mutex_t));
  if (mutex_buf == NULL)
    {
      print_str ("ERROR: ssl_init: could not allocate mutex memory\n");
      abort ();
    }
  for (i = 0; i < CRYPTO_num_locks (); i++)
    {
      pthread_mutex_init (&mutex_buf[i], NULL);
    }

  setenv ("OPENSSL_DEFAULT_ZLIB", "1", 1);

  CRYPTO_set_locking_callback (ssl_locking_function);
  CRYPTO_set_id_callback (ssl_id_function);

  SSL_library_init ();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings ();

  if (!RAND_load_file ("/dev/urandom", 4096))
    {
      print_str (
	  "ERROR: ssl_init: no bytes were added to PRNG from seed source\n");
      abort ();
    }

  COMP_METHOD *comp_method = COMP_zlib ();

  if (comp_method != NULL)
    {
      SSL_COMP_add_compression_method (1, comp_method);
    }

}

void
ssl_cleanup (void)
{
  int i;

  if (mutex_buf == NULL)
    {
      return;
    }

  /*CRYPTO_set_dynlock_create_callback(NULL);
   CRYPTO_set_dynlock_lock_callback(NULL);
   CRYPTO_set_dynlock_destroy_callback(NULL);*/

  CRYPTO_set_locking_callback (NULL);
  CRYPTO_set_id_callback (NULL);

  for (i = 0; i < CRYPTO_num_locks (); i++)
    {
      pthread_mutex_destroy (&mutex_buf[i]);
    }

  free (mutex_buf);
  mutex_buf = NULL;

  FIPS_mode_set (0);

  EVP_cleanup ();
  CRYPTO_cleanup_all_ex_data ();
  ERR_remove_state (0);
  ERR_free_strings ();

}

static void
ssl_init_setctx (__sock pso)
{
  SSL_CTX_set_options(pso->ctx, SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);

  SSL_CTX_set_cipher_list (pso->ctx, "-ALL:ALL:-ADH:-aNULL");

  SSL_CTX_set_verify (pso->ctx, pso->policy.ssl_verify, NULL);

  //SSL_CTX_set_mode(pso->ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
}

SSL_CTX*
ssl_init_ctx_server (__sock pso)
{
  if ((pso->ctx = SSL_CTX_new (SSLv23_server_method ())) == NULL)
    { /* create new context from method */
      return NULL;
    }

  ssl_init_setctx (pso);

  //SSL_CTX_sess_set_cache_size(pso->ctx, 1024);
  //SSL_CTX_set_session_cache_mode(pso->ctx, SSL_SESS_CACHE_BOTH);

  return pso->ctx;
}

SSL_CTX*
ssl_init_ctx_client (__sock pso)
{
  if ((pso->ctx = SSL_CTX_new (SSLv23_client_method ())) == NULL)
    { /* create new context from method */
      return NULL;
    }

  ssl_init_setctx (pso);

  //SSL_CTX_sess_set_cache_size(pso->ctx, 1024);
  //SSL_CTX_set_session_cache_mode(pso->ctx, SSL_SESS_CACHE_SERVER);

  return pso->ctx;
}

int
ssl_load_client_certs (SSL* ssl, char* cert_file, char* key_file)
{
  /* set the local certificate from CertFile */
  if (SSL_use_certificate_file (ssl, cert_file, SSL_FILETYPE_PEM) <= 0)
    {
      return 1;
    }
  /* set the private key from KeyFile (may be the same as CertFile) */
  if (SSL_use_PrivateKey_file (ssl, key_file, SSL_FILETYPE_PEM) <= 0)
    {
      return 2;
    }
  /* verify private key */
  if (!SSL_check_private_key (ssl))
    {
      print_str ("ERROR: private key does not match the public certificate\n");
      return 3;
    }

  return 0;
}

int
ssl_load_server_certs (SSL_CTX* ctx, char* cert_file, char* key_file)
{
  if (SSL_CTX_load_verify_locations (ctx, cert_file, key_file) != 1)
    {
      return 1;
    }

  if (SSL_CTX_set_default_verify_paths (ctx) != 1)
    {
      return 1;
    }

  /* set the local certificate from CertFile */
  if (SSL_CTX_use_certificate_file (ctx, cert_file, SSL_FILETYPE_PEM) <= 0)
    {
      return 1;
    }
  /* set the private key from KeyFile (may be the same as CertFile) */
  if (SSL_CTX_use_PrivateKey_file (ctx, key_file, SSL_FILETYPE_PEM) <= 0)
    {
      return 2;
    }
  /* verify private key */
  if (!SSL_CTX_check_private_key (ctx))
    {
      print_str ("ERROR: private key does not match the public certificate\n");
      return 3;
    }

  return 0;
}

static void
ssl_show_client_certs (__sock pso, SSL* ssl)
{
  X509 *cert;

  cert = SSL_get_peer_certificate (ssl); /* Get certificates (if available) */
  if (cert != NULL)
    {
      char ssl_cb[1024];
      char *line;

      //print_str("NOTICE: Peer certificates:\n");
      line = X509_NAME_oneline (X509_get_subject_name (cert), ssl_cb,
				sizeof(ssl_cb));
      print_str ("NOTICE: [%d]: subject: %s\n", pso->sock, line);
      line = X509_NAME_oneline (X509_get_issuer_name (cert), ssl_cb,
				sizeof(ssl_cb));
      print_str ("NOTICE: [%d]: issuer: %s\n", pso->sock, line);
      X509_free (cert);
    }
  else
    {
      print_str ("D4: ssl_show_client_certs: [%d]: no client certs\n",
		 pso->sock);
    }
}

static int
net_chk_timeout (__sock pso)
{
  int r = 0;

  mutex_lock (&pso->mutex);

  if (pso->policy.idle_timeout /* idle timeout (data recieve)*/
  && (time (NULL) - pso->timers.last_act) >= pso->policy.idle_timeout)
    {
      print_str ("WARNING: idle timeout occured on socket %d [%u]\n", pso->sock,
		 time (NULL) - pso->timers.last_act);
      if (pso->flags & F_OPSOCK_SSL)
	{
	  //pso->flags |= F_OPSOCK_SKIP_SSL_SD;
	}
      r = 1;
      //goto end;

    }

  //end: ;

  pthread_mutex_unlock (&pso->mutex);

  return r;
}

static int
net_listener_chk_timeout (__sock pso)
{
  int r = 0;

  mutex_lock (&pso->mutex);

  time_t last_act = (time (NULL) - pso->timers.last_act);

  if (pso->policy.listener_idle_timeout)
    {
      if (last_act >= pso->policy.listener_idle_timeout)
	{
	  print_str (
	      "WARNING: idle timeout occured on listener socket %d [%u]\n",
	      pso->sock, last_act);
	  r = 1;
	}
      else
	{
	  r = -1;
	}
    }

  exit: ;

  pthread_mutex_unlock (&pso->mutex);

  return r;

  /*
   int r = 0;

   mutex_lock(&pso->mutex);

   if (pso->flags & (F_OPSOCK_ST_SSL_ACCEPT))
   {
   if (pso->policy.ssl_accept_timeout
   && (time(NULL) - pso->timers.last_act) >= pso->policy.ssl_accept_timeout)
   {
   print_str("WARNING: SSL_accept timed out [%d]\n", pso->sock);

   r = 1;
   goto end;
   }
   }
   else if (pso->flags & (F_OPSOCK_ST_SSL_ACCEPT))
   {
   if (pso->policy.ssl_accept_timeout
   && (time(NULL) - pso->timers.last_act) >= pso->policy.ssl_accept_timeout)
   {
   print_str("WARNING: SSL_connect timed out [%d]\n", pso->sock);

   r = 1;
   goto end;
   }
   }

   end: ;

   pthread_mutex_unlock(&pso->mutex);

   return r;*/
}

uint16_t
net_get_addrinfo_port (__sock pso)
{
  void *port_data;
  switch (pso->res.ai_family)
    {
    case AF_INET:
      ;
      port_data = &((struct sockaddr_in*) pso->res.ai_addr)->sin_port;
      break;
    case AF_INET6:
      ;
      port_data = &((struct sockaddr_in6*) pso->res.ai_addr)->sin6_port;
      break;
    default:
      ;
      return 0;
      break;
    }

  return ntohs (*((uint16_t*) port_data));
}

const char *
net_get_addrinfo_ip_str (__sock pso, char *out, socklen_t len)
{
  void *ip_data;
  switch (pso->res.ai_family)
    {
    case AF_INET:
      ;
      ip_data = (void*) &((struct sockaddr_in*) pso->res.ai_addr)->sin_addr;
      break;
    case AF_INET6:
      ;
      ip_data = (void*) &((struct sockaddr_in6*) pso->res.ai_addr)->sin6_addr;
      break;
    default:
      ;
      out[0] = 1;
      return out;
    }

  return inet_ntop (pso->res.ai_family, ip_data, out, len);

}

int
net_addr_to_ipr (__sock pso, __ipr out)
{
  uint16_t *port_data;
  uint8_t *ip_data;
  int len;
  switch (pso->res.ai_family)
    {
    case AF_INET:
      ;
      ip_data = (uint8_t*) &((struct sockaddr_in*) pso->res.ai_addr)->sin_addr;
      port_data =
	  (uint16_t*) &((struct sockaddr_in*) pso->res.ai_addr)->sin_port;
      len = sizeof(struct in_addr);
      break;
    case AF_INET6:
      ;
      ip_data =
	  (uint8_t*) &((struct sockaddr_in6*) pso->res.ai_addr)->sin6_addr;
      port_data =
	  (uint16_t*) &((struct sockaddr_in6*) pso->res.ai_addr)->sin6_port;
      len = sizeof(struct in6_addr);
      break;
    default:
      ;
      return 1;
    }

  out->port = ntohs (*port_data);

  int i;
  for (i = 0; i < len && i < sizeof(out->ip); i++)
    {
      out->ip[i] = ip_data[i];
    }

  return 0;

}

int
net_addr_to_ipr_sa (struct sockaddr* sa, __sock pso, __ipr out)
{
  uint16_t *port_data;
  uint8_t *ip_data;
  int len;
  switch (pso->res.ai_family)
    {
    case AF_INET:
      ;
      ip_data = (uint8_t*) &((struct sockaddr_in*) sa)->sin_addr;
      port_data = (uint16_t*) &((struct sockaddr_in*) sa)->sin_port;
      len = sizeof(struct in_addr);
      break;
    case AF_INET6:
      ;
      ip_data = (uint8_t*) &((struct sockaddr_in6*) sa)->sin6_addr;
      port_data = (uint16_t*) &((struct sockaddr_in6*) sa)->sin6_port;
      len = sizeof(struct in6_addr);
      break;
    default:
      ;
      return 1;
    }

  out->port = ntohs (*port_data);

  int i;
  for (i = 0; i < len && i < sizeof(out->ip); i++)
    {
      out->ip[i] = ip_data[i];
    }

  return 0;

}

int
bind_socket (int fd, struct addrinfo *aip)
{
  int y = 1;

  if (setsockopt (fd, SOL_SOCKET,
  SO_REUSEADDR,
		  &y, sizeof(int)))
    {
      return 100;
    }

  int ret;

  if ((ret = fcntl (fd, F_SETFL, O_NONBLOCK)) == -1)
    {
      close (fd);
      return 101;;
    }

  if (bind (fd, aip->ai_addr, aip->ai_addrlen) == -1)
    {
      return 111;
    }

  if (listen (fd, 5))
    {
      return 122;
    }

  return 0;
}

static void
net_destroy_tnsat (__sock pso)
{
  if (!(pso->timers.flags & F_ST_MISC02_ACT))
    {
      pso->timers.flags |= F_ST_MISC02_ACT;
      pso->timers.misc02 = time (NULL);
    }
  else
    {
      pso->timers.misc03 = time (NULL);
      time_t pt_diff = (pso->timers.misc03 - pso->timers.misc02);
      if (pt_diff > pso->policy.close_timeout)
	{
	  print_str (
	      "WARNING: net_destroy_tnsat: [%d] shutdown timed out after %u seconds\n",
	      pso->sock, pt_diff);
	  pso->flags |= F_OPSOCK_SKIP_SSL_SD;
	}
    }
}

int
net_destroy_connection (__sock so)
{
  int ret;

  mutex_lock (&so->mutex);

  if (so->flags & F_OPSOCK_DISCARDED)
    {
      pthread_mutex_unlock (&so->mutex);
      return -1;
    }

  if (so->flags & F_OPSOCK_CONNECT)
    {
      /*char b;
       while (recv(so->sock, &b, 1, 0))
       {
       }*/

      if ((so->flags & F_OPSOCK_SSL) && so->ssl)
	{
	  if (SSL_get_shutdown (so->ssl) & SSL_RECEIVED_SHUTDOWN)
	    {
	      print_str (
		  "DEBUG: net_destroy_connection: [%d]: SSL_RECEIVED_SHUTDOWN is set, skipping SSL_shutdown\n",
		  so->sock);
	      goto ssl_cleanup;
	    }
	  else if (so->flags & F_OPSOCK_SKIP_SSL_SD)
	    {
	      print_str (
		  "DEBUG: net_destroy_connection: [%d]: F_OPSOCK_SKIP_SSL_SD is set, skipping SSL_shutdown\n",
		  so->sock);
	      goto ssl_cleanup;
	    }
	  else if (so->flags & F_OPSOCK_TS_DISCONNECTED)
	    {
	      print_str (
		  "DEBUG: net_destroy_connection: [%d]: F_OPSOCK_TS_DISCONNECTED is set, skipping SSL_shutdown\n",
		  so->sock);
	      goto ssl_cleanup;
	    }

	  errno = 0;
	  if ((ret = SSL_shutdown (so->ssl)) < 1)
	    {
	      if (0 == ret)
		{
		  /*print_str(
		   "D5: net_destroy_connection: [%d]: SSL_shutdown not yet finished\n",
		   so->sock);*/
		  net_destroy_tnsat (so);
		  pthread_mutex_unlock (&so->mutex);
		  return 2;
		}

	      int ssl_err = SSL_get_error (so->ssl, ret);
	      ERR_print_errors_fp (stderr);
	      ERR_clear_error ();

	      if ((ssl_err == SSL_ERROR_WANT_READ
		  || ssl_err == SSL_ERROR_WANT_WRITE))
		{
		  /*print_str(
		   "D5: net_destroy_connection: [%d]: SSL_shutdown needs action %d to complete\n",
		   so->sock, ssl_err);*/
		  net_destroy_tnsat (so);
		  pthread_mutex_unlock (&so->mutex);
		  return 2;
		}
	      so->s_errno = ssl_err;
	      so->status = ret;

	      if (ssl_err == 5 && so->status == -1)
		{
		  print_str (
		      "D6: SSL_shutdown: socket: [%d] [SSL_ERROR_SYSCALL]: code:[%d] [%s]\n",
		      so->sock, so->status, "-");
		}
	      else
		{
		  print_str (
		      "ERROR: socket: [%d] SSL_shutdown - code:[%d] sslerr:[%d]\n",
		      so->sock, so->status, ssl_err);
		}

	    }

	  ssl_cleanup: ;

	  /*if (so->flags & F_OPSOCK_SSL_KEYCERT_L)
	   {
	   SSL_certs_clear(so->ssl);
	   }*/

	  SSL_free (so->ssl);

	  ERR_remove_state (0);
	  ERR_clear_error ();
	}

      if (!(so->flags & F_OPSOCK_TS_DISCONNECTED))
	{
	  if ((ret = shutdown (so->sock, SHUT_RDWR)) == -1)
	    {
	      char err_buffer[1024];
	      if ( errno == ENOTCONN)
		{
		  print_str (
		      "D5: socket: [%d] shutdown: code:[%d] errno:[%d] %s\n",
		      so->sock, ret, errno,
		      strerror_r (errno, err_buffer, sizeof(err_buffer)));
		}
	      else
		{
		  print_str (
		      "ERROR: socket: [%d] shutdown: code:[%d] errno:[%d] %s\n",
		      so->sock, ret, errno,
		      strerror_r (
		      errno,
				  err_buffer, sizeof(err_buffer)));
		}
	    }
	  else
	    {
	      while (ret > 0)
		{
		  ret = recv (so->sock, so->buffer0, so->unit_size, 0);
		}
	    }
	}
    }

  if ((so->flags & F_OPSOCK_SSL) && NULL != so->ctx)
    {
      SSL_CTX_free (so->ctx);
    }

  if (so->res.ai_addr)
    {
      free (so->res.ai_addr);
    }

  if ((ret = close (so->sock)))
    {
      char err_buffer[1024];
      print_str (
	  "ERROR: [%d] unable to close socket - code:[%d] errno:[%d] %s\n",
	  so->sock, ret, errno, strerror_r (errno, err_buffer, 1024));
      ret = 1;
    }

  if (NULL != so->buffer0)
    {
      free (so->buffer0);
    }

  /*if ( NULL != so->va_p1)
   {
   free(so->va_p1);
   }*/

  md_g_free_l (&so->sendq);

  md_g_free_l (&so->init_rc0);
  md_g_free_l (&so->init_rc1);
  md_g_free_l (&so->init_rc0_ssl);
  md_g_free_l (&so->tasks);

  so->flags |= F_OPSOCK_DISCARDED;

  pthread_mutex_unlock (&so->mutex);

  return ret;
}

int
unregister_connection (pmda glob_reg, __sock pso)
{
  mutex_lock (&glob_reg->mutex);

  p_md_obj ptr = glob_reg->first;

  while (ptr)
    {
      if ((__sock ) ptr->ptr == pso)
	{
	  break;
	}
      ptr = ptr->next;
    }

  int r;

  if (ptr)
    {
      md_unlink_le (glob_reg, ptr);
      r = 0;
    }
  else
    {
      r = 1;
    }

  pthread_mutex_unlock (&glob_reg->mutex);

  return r;
}

static void
net_failclean (__sock so)
{
  md_g_free_l (&so->sendq);

  md_g_free_l (&so->init_rc0);
  md_g_free_l (&so->init_rc1);
  md_g_free_l (&so->shutdown_rc0);
  md_g_free_l (&so->shutdown_rc1);
  md_g_free_l (&so->init_rc0_ssl);
  md_g_free_l (&so->tasks);

}

static void
net_open_connection_cleanup (pmda sockr, struct addrinfo *aip, int fd)
{
  close (fd);
  md_unlink_le (sockr, sockr->pos);
  pthread_mutex_unlock (&sockr->mutex);
}

int
net_open_connection (char *addr, char *port, __sock_ca args)
{
  struct addrinfo *aip;
  struct addrinfo hints =
    { 0 };

  int fd;

  hints.ai_flags = AI_ALL | AI_ADDRCONFIG;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo (addr, port, &hints, &aip))
    {
      return -1;
    }

  if ((fd = socket (aip->ai_family, aip->ai_socktype, aip->ai_protocol)) == -1)
    {
      freeaddrinfo (aip);
      return -2;
    }

  if (connect (fd, aip->ai_addr, aip->ai_addrlen) == -1)
    {
      freeaddrinfo (aip);
      close (fd);
      return -3;
    }

  int ret;

  if ((ret = fcntl (fd, F_SETFL, O_NONBLOCK)) == -1)
    {
      close (fd);
      return -4;
    }

  mutex_lock (&args->socket_register->mutex);

  __sock pso;

  if (!(pso = md_alloc_le (args->socket_register, sizeof(_sock), 0, NULL)))
    {
      freeaddrinfo (aip);
      close (fd);
      pthread_mutex_unlock (&args->socket_register->mutex);
      return 9;
    }

  pso->res = *aip;

  pso->res.ai_addr = malloc (sizeof(struct sockaddr));
  memcpy (pso->res.ai_addr, aip->ai_addr, sizeof(struct sockaddr));

  freeaddrinfo (aip);

  pso->sock = fd;
  pso->oper_mode = SOCKET_OPMODE_RECIEVER;
  pso->flags = args->flags;
  pso->pcheck_r = (_t_rcall) net_chk_timeout;
  pso->timers.last_act = time (NULL);
  pso->st_p0 = args->st_p0;
  pso->policy = args->policy;
  pso->sock_ca = (void*) args;
  pso->common = args->common;

  net_addr_to_ipr (pso, &pso->ipr);

  md_copy_le (&args->init_rc0, &pso->init_rc0, sizeof(_proc_ic_o), NULL);
  md_copy_le (&args->init_rc1, &pso->init_rc1, sizeof(_proc_ic_o), NULL);
  md_copy_le (&args->shutdown_rc0, &pso->shutdown_rc0, sizeof(_proc_ic_o),
  NULL);
  md_copy_le (&args->shutdown_rc1, &pso->shutdown_rc1, sizeof(_proc_ic_o),
  NULL);
  md_copy_le (&args->init_rc0_ssl, &pso->init_rc0_ssl, sizeof(_proc_ic_o),
  NULL);
  md_init_le (&pso->tasks, 8);

  if (!args->unit_size)
    {
      pso->unit_size = SOCK_RECVB_SZ;
    }
  else
    {
      pso->unit_size = args->unit_size;
    }

  pso->buffer0 = malloc (pso->unit_size);
  pso->buffer0_len = pso->unit_size;

  pso->host_ctx = args->socket_register;

  if (mutex_init (&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      net_failclean (pso);
      net_open_connection_cleanup (args->socket_register, aip, fd);
      return 10;
    }

  if (args->flags & F_OPSOCK_INIT_SENDQ)
    {
      md_init_le (&pso->sendq, 8192);
    }

  int r;

  if (args->flags & F_OPSOCK_SSL)
    {
      if (!ssl_init_ctx_client (pso))
	{
	  net_failclean (pso);
	  net_open_connection_cleanup (args->socket_register, aip, fd);
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  return 11;
	}

      if ((pso->ssl = SSL_new (pso->ctx)) == NULL)
	{ /* get new SSL state with context */
	  net_failclean (pso);
	  net_open_connection_cleanup (args->socket_register, aip, fd);
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  return 12;
	}

      if (args->ssl_cert && args->ssl_key)
	{
	  if ((r = ssl_load_client_certs (pso->ssl, args->ssl_cert,
					  args->ssl_key)))
	    {
	      net_failclean (pso);
	      net_open_connection_cleanup (args->socket_register, aip, fd);
	      ERR_print_errors_fp (stderr);
	      ERR_clear_error ();
	      print_str (
		  "ERROR: [%d] could not load SSL certificate/key pair [%d]: %s\n",
		  fd, r, args->ssl_cert);
	      return 15;
	    }
	  pso->flags |= F_OPSOCK_SSL_KEYCERT_L;
	}

      SSL_set_fd (pso->ssl, pso->sock);

      //pso->flags |= F_OPSOCK_ST_SSL_CONNECT;

      pso->rcv_cb = (_p_s_cb) net_connect_ssl;
      pso->rcv_cb_t = (_p_s_cb) net_recv_ssl;

      pso->send0 = (_p_ssend) net_ssend_ssl_b;

      print_str ("D4: net_open_connection: enabling SSL..\n");
      //pso->send0 = (_p_ssend) net_ssend_ssl;
    }
  else
    {
      pso->rcv_cb = (_p_s_cb) net_recv;

      pso->send0 = (_p_ssend) net_ssend_b;

      pso->flags |= F_OPSOCK_PROC_READY;
    }

  pso->rcv1 = (_p_s_cb) args->proc;

  net_pop_rc (pso, &pso->init_rc0);

  pthread_t pt;

  if (pso->flags & F_OPSOCK_CS_NOASSIGNTHREAD)
    {
      goto ready;
    }
  if (pso->flags & F_OPSOCK_CS_MONOTHREAD)
    {
      r = 1;
      print_str ("DEBUG: net_open_connection: spawning new thread for [%d]\n",
		 pso->sock);
      pso->flags |= F_OPSOCK_ACT;
    }
  else
    {
      r = 0;
    }

  pthread_mutex_unlock (&pso->mutex);

  if (r == 1)
    {
      o_thrd data =
	{ 0 };
      data.id = 0;
      data.role = THREAD_ROLE_NET_WORKER;
      data.oper_mode = SOCKET_OPMODE_RECIEVER;
      data.host_ctx = args->thread_register;
      data.in_objects.lref_ptr = pso;
      data.flags |= F_THRD_DETACH | F_THRD_NOWPID | pso->common.thread_flags;
      mutex_init (&data.mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);

      pthread_t pt;

      if ((r = thread_create (net_worker_mono, 0, args->thread_register, 0, 0,
      F_THRD_MISC00,
			      F_THC_USER_DATA, &data, NULL, &pt)))
	{
	  print_str (
	      "ERROR: net_open_connection: thread_create failed, code %d, sock %d (accept)\n",
	      r, pso->sock);
	  uint32_t pso_flags = pso->flags;
	  net_failclean (pso);
	  net_open_connection_cleanup (args->socket_register, aip, fd);

	  return 17;

	}
      else
	{
	  //pthread_kill (pt, SIGUSR1);
	  goto exit;
	}
    }
  else if ((r = push_object_to_thread (pso, args->thread_register,
				       (dt_score_ptp) net_get_score, &pt)))
    {
      print_str (
	  "ERROR: net_open_connection: push_object_to_thread failed, code %d, sock %d\n",
	  r, pso->sock);
      net_failclean (pso);
      net_open_connection_cleanup (args->socket_register, aip, fd);
      return 13;
    }

  mutex_lock (&pso->mutex);

  pso->thread = pt;
  net_pop_rc (pso, &pso->init_rc1);

  ready: ;

  pso->flags |= F_OPSOCK_ACT;

  pthread_mutex_unlock (&pso->mutex);

  exit: ;

  pthread_mutex_unlock (&args->socket_register->mutex);

  return 0;
}

static void
net_open_listening_socket_cleanup (pmda sockr, struct addrinfo *aip, int fd)
{
  close (fd);
  md_unlink_le (sockr, sockr->pos);
  pthread_mutex_unlock (&sockr->mutex);
}

int
net_open_listening_socket_e (char *addr, char *port, __sock_ca args,
			     pthread_t *pt_ret)
{
  struct addrinfo *aip;
  struct addrinfo hints =
    { 0 };

  int fd;

  errno = 0;

  hints.ai_flags = AI_ALL | AI_ADDRCONFIG | AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_family = AF_UNSPEC;

  mutex_lock (&args->socket_register->mutex);

  if (getaddrinfo (addr, port, &hints, &aip))
    {
      pthread_mutex_unlock (&args->socket_register->mutex);
      return -1;
    }

  if ((fd = socket (aip->ai_family, aip->ai_socktype, aip->ai_protocol)) == -1)
    {
      freeaddrinfo (aip);
      pthread_mutex_unlock (&args->socket_register->mutex);
      return -2;
    }

  if (bind_socket (fd, aip))
    {
      freeaddrinfo (aip);
      close (fd);
      pthread_mutex_unlock (&args->socket_register->mutex);
      return -3;
    }

  __sock pso;

  if (!(pso = md_alloc_le (args->socket_register, sizeof(_sock), 0, NULL)))
    {
      freeaddrinfo (aip);
      close (fd);
      pthread_mutex_unlock (&args->socket_register->mutex);
      return 9;
    }

  pso->res = *aip;

  pso->res.ai_addr = malloc (sizeof(struct sockaddr));
  memcpy (pso->res.ai_addr, aip->ai_addr, sizeof(struct sockaddr));

  freeaddrinfo (aip);

  pso->sock = fd;
  pso->oper_mode = SOCKET_OPMODE_LISTENER;
  pso->flags = args->flags;
  pso->ac_flags = args->ac_flags;
  pso->pcheck_r = (_t_rcall) net_listener_chk_timeout;
  pso->rcv_cb = (_p_s_cb) net_accept;
  pso->host_ctx = args->socket_register;
  pso->unit_size = args->unit_size;
  pso->st_p0 = args->st_p0;
  pso->policy = args->policy;
  pso->common = args->common;

  net_addr_to_ipr (pso, &pso->ipr);

  md_copy_le (&args->init_rc0, &pso->init_rc0, sizeof(_proc_ic_o), NULL);
  md_copy_le (&args->init_rc1, &pso->init_rc1, sizeof(_proc_ic_o), NULL);
  md_copy_le (&args->shutdown_rc0, &pso->shutdown_rc0, sizeof(_proc_ic_o),
  NULL);
  md_copy_le (&args->shutdown_rc1, &pso->shutdown_rc1, sizeof(_proc_ic_o),
  NULL);
  md_copy_le (&args->init_rc0_ssl, &pso->init_rc0_ssl, sizeof(_proc_ic_o),
  NULL);
  md_init_le (&pso->tasks, 8);

  pso->sock_ca = (void*) args;

  if (mutex_init (&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      net_failclean (pso);
      net_open_listening_socket_cleanup (args->socket_register, aip, fd);
      return 10;
    }

  int r;

  if (args->flags & F_OPSOCK_SSL)
    {
      if (!ssl_init_ctx_server (pso))
	{
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  net_failclean (pso);
	  net_open_listening_socket_cleanup (args->socket_register, aip, fd);
	  return 11;
	}

      if ((r = ssl_load_server_certs (pso->ctx, args->ssl_cert, args->ssl_key)))
	{
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  net_failclean (pso);
	  net_open_listening_socket_cleanup (args->socket_register, aip, fd);
	  print_str (
	      "ERROR: [%d] could not load SSL certificate/key pair [%d]: %s / %s\n",
	      fd, r, args->ssl_cert, args->ssl_key);
	  return 12;
	}

      pso->flags |= F_OPSOCK_SSL_KEYCERT_L;

      pso->rcv0 = (_p_s_cb) net_recv_ssl;
    }
  else
    {
      pso->rcv0 = (_p_s_cb) net_recv;
    }

  pso->rcv1_t = (_p_s_cb) args->proc;

  mutex_lock (&pso->mutex);

  net_pop_rc (pso, &pso->init_rc0);

  if (pso->flags & F_OPSOCK_CS_NOASSIGNTHREAD)
    {
      goto ready;
    }
  else if (pso->flags & F_OPSOCK_CS_MONOTHREAD)
    {
      r = 1;
      print_str (
	  "DEBUG: net_open_listening_socket: spawning new thread for [%d]\n",
	  pso->sock);
      pso->flags |= F_OPSOCK_ACT;
    }
  else
    {
      r = 0;
    }

  pthread_mutex_unlock (&pso->mutex);

  pthread_t pt;

  if (r == 1)
    {
      o_thrd data =
	{ 0 };
      data.id = 0;
      data.role = THREAD_ROLE_NET_WORKER;
      data.oper_mode = SOCKET_OPMODE_LISTENER;
      data.host_ctx = args->thread_register;
      data.in_objects.lref_ptr = pso;
      data.flags |= F_THRD_DETACH | F_THRD_NOWPID | pso->common.thread_flags;
      mutex_init (&data.mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);

      pthread_t pt;

      if ((r = thread_create (net_worker_mono, 0, args->thread_register, 0, 0,
      F_THRD_MISC00,
			      F_THC_USER_DATA, &data, NULL, &pt)))
	{
	  print_str (
	      "ERROR: net_open_listening_socket: thread_create failed, code %d, sock %d (accept)\n",
	      r, pso->sock);
	  uint32_t pso_flags = pso->flags;
	  net_failclean (pso);
	  net_open_connection_cleanup (args->socket_register, aip, fd);

	  return 17;

	}
      else
	{
	  /*sigset_t set;
	   sigemptyset (&set);
	   sigaddset (&set, SIGHUP);

	   int sig;

	   int re;
	   if ((re = sigwait (&set, &sig)))
	   {
	   fprintf (
	   stderr,
	   "WARNING: net_open_listening_socket: [%d]: sigwait (SIGINT) failed with %d\n",
	   syscall (SYS_gettid), re);
	   }*/

	  //pthread_kill (pt, SIGUSR1);
	  if ( NULL != pt_ret)
	    {
	      *pt_ret = pt;
	    }
	  goto exit;
	}
    }
  else if ((r = push_object_to_thread (pso, args->thread_register,
				       (dt_score_ptp) net_get_score, &pt)))
    {
      print_str (
	  "ERROR: net_open_listening_socket: push_object_to_thread failed, code %d, sock %d\n",
	  r, pso->sock);
      net_failclean (pso);
      net_open_listening_socket_cleanup (args->socket_register, aip, fd);
      return 13;
    }

  mutex_lock (&pso->mutex);

  pso->thread = pt;
  net_pop_rc (pso, &pso->init_rc1);

  ready: ;

  pso->flags |= F_OPSOCK_ACT;

  pthread_mutex_unlock (&pso->mutex);

  exit: ;

  pthread_mutex_unlock (&args->socket_register->mutex);

  return 0;
}

int
net_open_listening_socket (char *addr, char *port, __sock_ca args)
{
  return net_open_listening_socket_e (addr, port, args, NULL);
}

__sock
net_open_listening_socket_bare (char *addr, char *port, __sock_ca args)
{
  struct addrinfo *aip;
  struct addrinfo hints =
    { 0 };

  int fd;

  errno = 0;

  hints.ai_flags = AI_ALL | AI_ADDRCONFIG | AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_family = AF_UNSPEC;

  if (getaddrinfo (addr, port, &hints, &aip))
    {
      return NULL;
    }

  if ((fd = socket (aip->ai_family, aip->ai_socktype, aip->ai_protocol)) == -1)
    {
      freeaddrinfo (aip);
      return NULL;
    }

  if (bind_socket (fd, aip))
    {
      freeaddrinfo (aip);
      close (fd);
      return NULL;
    }

  __sock pso = calloc (1, sizeof(_sock));

  pso->res = *aip;

  pso->res.ai_addr = malloc (sizeof(struct sockaddr));
  memcpy (pso->res.ai_addr, aip->ai_addr, sizeof(struct sockaddr));

  freeaddrinfo (aip);

  pso->sock = fd;
  pso->oper_mode = SOCKET_OPMODE_LISTENER;
  pso->flags = args->flags;
  pso->ac_flags = args->ac_flags;
  pso->pcheck_r = (_t_rcall) net_listener_chk_timeout;
  pso->rcv_cb = (_p_s_cb) net_accept;
  pso->host_ctx = args->socket_register;
  pso->unit_size = args->unit_size;
  pso->st_p0 = args->st_p0;
  pso->policy = args->policy;

  net_addr_to_ipr (pso, &pso->ipr);

  pso->sock_ca = (void*) args;

  if (mutex_init (&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      free (pso);
      freeaddrinfo (aip);
      close (fd);
      return NULL;
    }

  int r;

  if (args->flags & F_OPSOCK_SSL)
    {
      if (!ssl_init_ctx_server (pso))
	{
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  free (pso);
	  freeaddrinfo (aip);
	  close (fd);
	  return NULL;
	}

      if ((r = ssl_load_server_certs (pso->ctx, args->ssl_cert, args->ssl_key)))
	{
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  free (pso);
	  freeaddrinfo (aip);
	  close (fd);
	  print_str (
	      "ERROR: [%d] could not load SSL certificate/key pair [%d]: %s / %s\n",
	      fd, r, args->ssl_cert, args->ssl_key);
	  return NULL;
	}

      pso->flags |= F_OPSOCK_SSL_KEYCERT_L;

      pso->rcv0 = (_p_s_cb) net_recv_ssl;
    }
  else
    {
      pso->rcv0 = (_p_s_cb) net_recv;
    }

  pso->rcv1_t = (_p_s_cb) args->proc;

  return pso;
}

void
net_send_sock_sigterm (__sock pso)
{
  mutex_lock (&pso->mutex);
  pso->flags |= F_OPSOCK_TERM;
  if (pso->thread != (pthread_t) 0)
    {
      pthread_kill (pso->thread, SIGUSR1);
    }
  pthread_mutex_unlock (&pso->mutex);
}

float
net_get_score (pmda in, pmda out, __sock pso, po_thrd thread)
{
  mutex_lock (&pso->mutex);

  if (thread->oper_mode != pso->oper_mode)
    {
      pthread_mutex_unlock (&pso->mutex);
      return (float) -1.0;
    }

  pthread_mutex_unlock (&pso->mutex);

  float score = (float) (in->offset + out->offset);

  return score;
}

int
net_push_to_sendq (__sock pso, void *data, size_t size, uint16_t flags)
{
  mutex_lock (&pso->sendq.mutex);

  if ((pso->flags & F_OPSOCK_TERM))
    {
      pthread_mutex_unlock (&pso->sendq.mutex);
      return 2;
    }

  __sock_sqp ptr;
  if (NULL == (ptr = md_alloc_le (&pso->sendq, SSENDQ_PAYLOAD_SIZE, 0, NULL)))
    {
      pthread_mutex_unlock (&pso->sendq.mutex);
      return -1;
    }

  if (flags & NET_PUSH_SENDQ_ASSUME_PTR)
    {
      ptr->data = data;
    }
  else
    {
      ptr->data = malloc (size);
      memcpy (ptr->data, data, size);
    }

  ptr->size = size;

  print_str ("D5: net_push_to_sendq: [%d]: suceeded\n", pso->sock);

  if ((pthread_t) 0 != pso->thread)
    {
      pthread_kill (pso->thread, SIGUSR1);
    }

  pthread_mutex_unlock (&pso->sendq.mutex);

  return 0;
}

int
net_sendq_broadcast (pmda base, __sock source, void *data, size_t size)
{
  mutex_lock (&base->mutex);

  int ret = 0;

  p_md_obj ptr = base->first;

  while (ptr)
    {
      __sock pso = (__sock) ptr->ptr;
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
	      print_str ("ERROR: net_sendq_broadcast: net_push_to_sendq failed, socket:[%d]\n", pso->sock);
	      net_send_sock_sigterm(pso);
	    }
	}

      l_end:;

      pthread_mutex_unlock (&pso->mutex);
      ptr = ptr->next;
    }

  pthread_mutex_unlock (&base->mutex);

  return ret;
}

int
net_broadcast (pmda base, void *data, size_t size, _net_bc net_bc, void *arg,
	       uint32_t flags)
{
  mutex_lock (&base->mutex);

  int ret = 0;

  p_md_obj ptr = base->first;

  while (ptr)
    {
      __sock pso = (__sock) ptr->ptr;

      mutex_lock (&pso->mutex);

      if (pso->oper_mode != SOCKET_OPMODE_RECIEVER || (pso->flags & F_OPSOCK_TERM))
	{
	  goto l_end;
	}

      if ( NULL != net_bc && net_bc(pso, arg, data) )
	{
	  goto l_end;
	}

      if (flags & F_NET_BROADCAST_SENDQ)
	{
	  if (net_push_to_sendq(pso, data, size, 0))
	    {
	      print_str ("ERROR: net_sendq_broadcast: net_push_to_sendq failed, socket:[%d]\n", pso->sock);
	      net_send_sock_sigterm(pso);
	      ret++;
	    }
	}
      else
	{
	  if (net_send_direct(pso, data, size))
	    {
	      net_send_sock_sigterm(pso);
	      ret++;
	    }
	}

      l_end:;

      pthread_mutex_unlock (&pso->mutex);

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&base->mutex);

  return ret;
}

int
net_send_direct (__sock pso, const void *data, size_t size)
{
  int ret;
  if (0 != (ret = pso->send0 (pso, (void*) data, size)))
    {
      print_str (
	  "ERROR: [%d] [%d %d]: net_send_direct: send data failed, payload size: %zd\n",
	  pso->sock, ret, pso->s_errno, size);
      net_send_sock_sigterm (pso);
      return -1;
    }

  return 0;

}

static void *
net_proc_sendq_destroy_item (__sock_sqp psqp, __sock pso, p_md_obj ptr)
{
  free (psqp->data);
  return md_unlink_le (&pso->sendq, ptr);
}

static ssize_t
net_proc_sendq (__sock pso)
{
  p_md_obj ptr = pso->sendq.first;

  off_t ok = 0;

  while (ptr)
    {
      __sock_sqp psqp = (__sock_sqp) ptr->ptr;
      if (psqp->size)
	{
	  int ret;
	  switch ((ret = pso->send0(pso, psqp->data, psqp->size)))
	    {
	      case 0:;
	      ptr = net_proc_sendq_destroy_item(psqp, pso, ptr);
	      ok++;
	      continue;
	      case 1:;
	      print_str ("ERROR: [%d] [%d]: net_proc_sendq: send data failed, payload size: %zd\n", pso->sock, pso->s_errno, psqp->size);
	      //ptr = net_proc_sendq_destroy_item(psqp, pso, ptr);
	      net_send_sock_sigterm(pso);
	      goto end;
	      case 2:;

	      break;
	    }

	}
      ptr = ptr->next;
    }

  print_str ("D5: net_proc_sendq: [%d]: OK: %llu\n", pso->sock, (uint64_t) ok);

  end: ;

  return (ssize_t) pso->sendq.offset;
}

int
net_enum_sockr (pmda base, _p_enumsr_cb p_ensr_cb, void *arg)
{
  mutex_lock (&base->mutex);

  int g_ret = 0;

  p_md_obj ptr = base->first;

  while (ptr)
    {
      __sock pso = (__sock) ptr->ptr;

      //mutex_lock(&pso->mutex);

      int ret = p_ensr_cb(pso, arg);

      if ( ret > 0 )
	{
	  g_ret = ret;
	  //pthread_mutex_unlock(&pso->mutex);
	  break;
	}

      //pthread_mutex_unlock(&pso->mutex);

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&base->mutex);

  return g_ret;
}

void
net_nw_ssig_term_r (pmda objects)
{
  mutex_lock (&objects->mutex);

  p_md_obj ptr = objects->first;

  while (ptr)
    {
      __sock pso = (__sock) ptr->ptr;

      net_send_sock_sigterm(pso);

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&objects->mutex);
}

void
net_nw_ssig_term_r_ex (pmda objects, uint32_t flags)
{
  mutex_lock (&objects->mutex);

  p_md_obj ptr = objects->first;

  while (ptr)
    {
      __sock pso = (__sock) ptr->ptr;

      mutex_lock (&pso->mutex);

      if ( pso->flags & flags)
	{
	  net_send_sock_sigterm(pso);
	}
      pthread_mutex_unlock (&pso->mutex);

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&objects->mutex);
}

#define NET_SOCKWAIT_TO         ((time_t)30)

static int
net_proc_sock_hmemb (po_thrd thrd)
{

  off_t num_active = 0;

  num_active += md_get_off_ts (&thrd->proc_objects);
  num_active += md_get_off_ts (&thrd->in_objects);

  return (int) num_active;
}

#include <sys/ioctl.h>

#define T_NET_WORKER_SD                 (time_t) 45
#define I_NET_WORKER_IDLE_ALERT         (time_t) 30

#define ST_NET_WORKER_ACT               ((uint8_t)1 << 1)
#define ST_NET_WORKER_IDLE_ALERT        ((uint8_t)1 << 2)

void
net_worker_cleanup_socket (__sock pso)
{
  mutex_lock (&pso->mutex);

  pmda host_ctx = pso->host_ctx;

  mda p_rc1 = pso->shutdown_rc1;

  uint32_t t_flags = pso->flags;

  net_pop_rc (pso, &pso->shutdown_rc0);
  md_g_free_l (&pso->shutdown_rc0);

  int sock = pso->sock;

  if ((t_flags & F_OPSOCK_PERSIST))
    {
      pso->flags |= F_OPSOCK_ORPHANED;
    }

  pthread_mutex_unlock (&pso->mutex);

  mutex_lock (&host_ctx->mutex);

  if ((t_flags & F_OPSOCK_PERSIST))
    {
      host_ctx->flags |= F_MDA_REFPTR;
    }

  if (unregister_connection (host_ctx, pso))
    {
      print_str (
	  "ERROR: [%d] could not find socket entry in register: %d (report this)\n",
	  syscall (SYS_gettid), sock);
      abort ();
    }

  host_ctx->flags ^= (host_ctx->flags & F_MDA_REFPTR);

  pthread_mutex_unlock (&host_ctx->mutex);

  net_pop_rc (NULL, &p_rc1);

  md_g_free_l (&p_rc1);

  /*if ((t_flags & F_OPSOCK_PERSIST))
   {
   mutex_lock (&pso->mutex);
   pso->flags |= F_OPSOCK_ORPHANED;
   pthread_mutex_unlock (&pso->mutex);
   }*/
}

uint32_t
net_proc_worker_detached_socket (__sock pso, uint32_t flags)
{
  uint8_t int_state = 0;
  pid_t _tid = syscall (SYS_gettid);
  char buffer0[1024];
  uint32_t status_flags = 0;

  o_thrd dummy_thread =
    { 0 };

  if (mutex_init (&dummy_thread.mutex, PTHREAD_MUTEX_RECURSIVE,
		  PTHREAD_MUTEX_ROBUST))
    {
      abort ();
    }

  net_worker_process_socket (pso, NULL, &int_state, flags, &dummy_thread, &_tid,
			     buffer0, &status_flags);

  return status_flags;
}

p_md_obj
net_worker_process_socket (__sock pso, p_md_obj ptr, uint8_t *int_state,
			   uint32_t flags, po_thrd thrd, pid_t *_tid,
			   char *buffer0, uint32_t *status_flags)
{
  int r;

  mutex_lock (&pso->mutex);

  if (pso->flags & F_OPSOCK_DETACH_THREAD)
    {
      pso->flags ^= F_OPSOCK_DETACH_THREAD;
      *int_state |= ST_NET_WORKER_ACT;
      *status_flags |= F_NW_STATUS_SOCKSD;
      goto l_end;
    }

  if (pso->flags & F_OPSOCK_TERM)
    {
      if (flags & F_NW_NO_SOCK_KILL)
	{
	  *int_state |= ST_NET_WORKER_ACT;
	  goto l_end;
	}

      if (net_proc_worker_tasks (pso) == 0)
	{
	  thrd->timers.t1 = time (NULL);
	  *int_state |= ST_NET_WORKER_ACT;
	}

      errno = 0;

      if (2 == (r = net_destroy_connection (pso)))
	{
	  thrd->timers.t1 = time (NULL);
	  *int_state |= ST_NET_WORKER_ACT;
	  *status_flags |= F_NW_STATUS_WAITSD;
	  goto l_end;
	}

      if (r == 1)
	{
	  print_str (
	      "ERROR: [%d] net_destroy_connection failed, socket [%d], critical\n",
	      (int) *_tid, pso->sock);
	  abort ();
	}

      if (0 != thrd->proc_objects.count)
	{
	  ptr = md_unlink_le (&thrd->proc_objects, ptr);
	}

      pthread_mutex_unlock (&pso->mutex);

      net_worker_cleanup_socket (pso);

      mutex_lock (&thrd->mutex);

      thrd->timers.t1 = time (NULL);
      *int_state |= ST_NET_WORKER_ACT;

      pthread_mutex_unlock (&thrd->mutex);

      *status_flags |= F_NW_STATUS_SOCKSD;

      return ptr;

    }

  if ((r = pso->pcheck_r (pso)))
    {
      if (-1 == r)
	{
	  *int_state |= ST_NET_WORKER_IDLE_ALERT;
	}
      else
	{
	  thrd->timers.t1 = time (NULL);
	  *int_state |= ST_NET_WORKER_ACT;
	  pso->flags |= F_OPSOCK_TERM;
	}
    }

  pso->flags |= F_OPSOCK_ST_HOOKED;

  errno = 0;

  if ((pso->flags & F_OPSOCK_HALT_RECV) )
    {
      pthread_mutex_unlock (&pso->mutex);
      goto process_data;
    }

  pthread_mutex_unlock (&pso->mutex);

  switch ((r = pso->rcv_cb (pso, pso->host_ctx, &_net_thrd_r, pso->buffer0)))
    {
    case 2:
      ;
      if (pso->counters.b_read > 0)
	{
	  break;
	}
      else
	{
	  goto send_q;
	}
      break;
    case 0:
      ;
      break;
    case -3:
      goto int_st;
      break;
    default:
      print_str (
	  "ERROR: %s: socket:[%d] code:[%d] status:[%d] errno:[%d] %s\n",
	  pso->oper_mode == SOCKET_OPMODE_LISTENER ? "rx/tx accept" :
	  pso->oper_mode == SOCKET_OPMODE_RECIEVER ?
	      "rx/tx data" : "socket operation",
	  pso->sock, r, pso->status,
	  errno,
	  errno ? strerror_r (errno, (char*) buffer0, 1024) : "");

      pso->flags |= F_OPSOCK_ERROR;

      mutex_lock (&pso->mutex);
      pso->flags |= F_OPSOCK_ERROR | F_OPSOCK_TERM;
      pthread_mutex_unlock (&pso->mutex);

      if (!pso->counters.b_read)
	{
	  goto e_end;
	}

      break;
    }

  process_data: ;

  if (NULL != pso->rcv1 && !(flags & F_NW_HALT_PROC))
    {
      switch ((r = pso->rcv1 (pso, pso->host_ctx, &thrd->host_ctx, pso->buffer0)))
	{
	case 0:
	  break;
	case -2:
	  break;
	case -3:
	  goto int_st;
	case -4:
	  goto e_end;
	default:
	  print_str (
	      "ERROR: data processor failed with status %d, socket: [%d]\n", r,
	      pso->sock);

	  mutex_lock (&pso->mutex);
	  pso->flags |= F_OPSOCK_ERROR | F_OPSOCK_TERM;
	  pthread_mutex_unlock (&pso->mutex);

	  break;
	}
    }

  if (pso->unit_size == pso->counters.b_read)
    {
      pso->counters.b_read = 0;
    }

  int_st: ;

  thrd->timers.t1 = time (NULL);
  *int_state |= ST_NET_WORKER_ACT;

  send_q: ;

  mutex_lock (&pso->mutex);

  mutex_lock (&pso->sendq.mutex);

  if (pso->sendq.offset > 0 && (pso->flags & F_OPSOCK_PROC_READY)
      && !(flags & F_NW_HALT_SEND))
    {
      ssize_t sendq_rem;
      if ((sendq_rem = net_proc_sendq (pso)) != 0)
	{
	  print_str ("WARNING: sendq: %zd items remain, socket:[%d]\n",
		     pso->sendq.offset, pso->sock);
	}
      thrd->timers.t1 = time (NULL);
      *int_state |= ST_NET_WORKER_ACT;
    }

  pthread_mutex_unlock (&pso->sendq.mutex);

  pthread_mutex_unlock (&pso->mutex);

  e_end: ;

  mutex_lock (&pso->mutex);

  if (pso->flags & F_OPSOCK_ST_HOOKED)
    {
      pso->flags ^= F_OPSOCK_ST_HOOKED;
    }

  l_end: ;

  if (net_proc_worker_tasks (pso) == 0)
    {
      usleep (1000);
      thrd->timers.t1 = time (NULL);
      *int_state |= ST_NET_WORKER_ACT;
    }

  pthread_mutex_unlock (&pso->mutex);

  if ( NULL != ptr)
    {
      return ptr->next;
    }

  return ptr;
}

void *
net_worker_mono (void *args)
{
  int r, s;
  uint8_t int_state = ST_NET_WORKER_ACT;
  pid_t _tid = (pid_t) syscall (SYS_gettid);

  po_thrd thrd = (po_thrd) args;

  char buffer0[1024];

  mutex_lock (&thrd->mutex);

  pthread_t _pt = thrd->pt;

  __sock pso = (__sock ) thrd->in_objects.lref_ptr;
  uint32_t thread_flags = thrd->flags;

  if (thread_flags & F_THRD_NOINIT)
    {
      pthread_mutex_unlock (&thrd->mutex);
      goto start_proc;
    }

  sigset_t set;

  sigfillset (&set);

  s = pthread_sigmask (SIG_SETMASK, &set, NULL);

  if (s != 0)
    {
      print_str (
	  "ERROR: net_worker_mono: pthread_sigmask (SIG_BLOCK) failed: %d\n",
	  s);
      abort ();
    }

  sigemptyset (&set);
  sigaddset (&set, SIGURG);
  sigaddset (&set, SIGIO);
  sigaddset (&set, SIGUSR1);

  s = pthread_sigmask (SIG_UNBLOCK, &set, NULL);

  if (s != 0)
    {
      print_str (
	  "ERROR: net_worker_mono: pthread_sigmask (SIG_UNBLOCK) failed: %d\n",
	  s);
      abort ();
    }

  thrd->timers.t1 = time (NULL);
  thrd->timers.t0 = time (NULL);
  thrd->timers.act_f |= (F_TTIME_ACT_T0 | F_TTIME_ACT_T1);

  thrd->status |= F_THRD_STATUS_INITIALIZED;

  //pthread_t parent = thrd->caller;

  if ( NULL == pso)
    {
      print_str ("ERROR: net_worker_mono: [%d]: no socket to process", _tid);
      pthread_mutex_unlock (&thrd->mutex);
      goto shutdown;
    }

  pthread_mutex_unlock (&thrd->mutex);

  mutex_lock (&pso->mutex);
  pso->thread = _pt;
  pthread_mutex_unlock (&pso->mutex);

  //pthread_kill (parent, SIGHUP);

  if (thread_flags & F_THRD_NOWPID)
    {
      goto init_done;
    }

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);

  int sig;

  int re;
  if ((re = sigwait (&set, &sig)))
    {
      print_str (
	  "WARNING: net_worker_mono: [%d]: sigwait (SIGUSR1) failed with %d\n",
	  _tid, re);
    }

  init_done: ;

  print_str ("DEBUG: net_worker_mono: [%d]: thread online [%d]\n", _tid,
	     thrd->oper_mode);

  start_proc: ;

  mutex_lock (&pso->mutex);

  net_pop_rc (pso, &pso->init_rc1);

  struct f_owner_ex fown_ex =
    { 0 };

  pid_t async = 1;

  if (ioctl (pso->sock, FIOASYNC, &async) == -1)
    {
      char err_buf[1024];
      print_str (
	  "ERROR: net_worker_mono: [%d]: ioctl (FIOASYNC) failed [%d] [%s]\n",
	  pso->sock, errno, strerror_r (errno, err_buf, sizeof(err_buf)));
      pso->flags |= F_OPSOCK_TERM;
      goto end_sockp;
    }

  fown_ex.pid = _tid;
  fown_ex.type = F_OWNER_TID;

  if (fcntl (pso->sock, F_SETOWN_EX, &fown_ex) == -1)
    {
      char err_buf[1024];
      print_str (
	  "ERROR: net_worker_mono: [%d]: fcntl (F_SETOWN_EX) failed [%d] [%s]\n",
	  pso->sock, errno, strerror_r (errno, err_buf, sizeof(err_buf)));
      pso->flags |= F_OPSOCK_TERM;
    }
  else
    {
      print_str ("D4: net_worker: [%d]: fcntl set F_SETOWN_EX on [%d]\n", _tid,
		 pso->sock);
    }

  pso->pthread = thrd;

  end_sockp: ;

  pthread_mutex_unlock (&pso->mutex);

  uint32_t status;

  for (;;)
    {
      status = 0;

      mutex_lock (&thrd->mutex);
      thrd->timers.t0 = time (NULL);

      if (thrd->flags & F_THRD_TERM)
	{
	  /*print_str("NOTICE: net_worker: [%d]: thread shutting down..\n",
	   _tid);*/
	  pthread_mutex_unlock (&thrd->mutex);
	  break;
	}

      pthread_mutex_unlock (&thrd->mutex);

      if (NULL == pso)
	{
	  print_str ("ERROR: net_worker: empty socket data reference\n");
	  abort ();
	}

      ///

      net_worker_process_socket (pso, NULL, &int_state, 0, thrd, &_tid, buffer0,
				 &status);

      if ((status & F_NW_STATUS_SOCKSD))
	{
	  break;
	}

      usleep (100);

      //print_str("%d - pooling socket..   \n", (int) _tid);

      if (int_state & ST_NET_WORKER_ACT)
	{
	  int_state ^= ST_NET_WORKER_ACT;
	  continue;
	}

      time_t thread_inactive = (time (NULL) - thrd->timers.t1);

      if (thread_inactive > 1)
	{
	  unsigned int t_interval = I_NET_WORKER_IDLE_ALERT;

	  //t_interval = 5;

	  mutex_lock (&thrd->mutex);
	  print_str ("D6: [%d]: putting worker to sleep [%hu] [%u]\n", _tid,
		     thrd->oper_mode, t_interval);

	  ts_flag_32 (&thrd->mutex, F_THRD_STATUS_SUSPENDED, &thrd->status);
	  pthread_mutex_unlock (&thrd->mutex);
	  sleep (t_interval);

	  mutex_lock (&thrd->mutex);
	  thrd->timers.t1 = time (NULL);
	  ts_unflag_32 (&thrd->mutex, F_THRD_STATUS_SUSPENDED, &thrd->status);
	  print_str ("D6: [%d]: thread waking up [%hu]\n", _tid,
		     thrd->oper_mode);
	  pthread_mutex_unlock (&thrd->mutex);

	}

    }

  shutdown: ;

  if (thread_flags & F_THRD_MISC00)
    {
      /*mutex_lock (&sock_host_ctx->mutex);

       int cleanup;
       if (0 == sock_host_ctx->offset)
       {
       cleanup = 1;
       }
       else
       {
       cleanup = 0;
       }
       sock_host_ctx->offset = -1;

       pthread_mutex_unlock (&sock_host_ctx->mutex);

       if (1 == cleanup)
       {
       md_g_free_l (sock_host_ctx);
       print_str (
       "DEBUG: net_worker_mono: [%d]: cleaned up guest socket's empty registry\n",
       _tid);
       }*/

    }

  /*if (NULL != thrd->proc_objects.lref_ptr)
   {
   uint32_t *status_ret = (uint32_t*) thrd->proc_objects.lref_ptr;
   *status_ret = status;
   }*/

  if (thrd->flags & F_THRD_REGINIT)
    {
      pmda thread_host_ctx = thrd->host_ctx;

      mutex_lock (&thread_host_ctx->mutex);

      mutex_lock (&thrd->mutex);

      if (NULL != thrd->buffer0)
	{
	  free (thrd->buffer0);
	}

      pthread_mutex_unlock (&thrd->mutex);

      p_md_obj ptr_thread = search_thrd_id (thread_host_ctx, &_pt);

      if (NULL == ptr_thread)
	{
	  print_str (
	      "ERROR: net_worker: [%d]: thread already unregistered on exit (search_thrd_id failed)\n",
	      (int) _tid);
	  abort ();
	}
      else
	{
	  //thread_host_ctx->flags |= F_MDA_REFPTR;
	  md_unlink_le (thread_host_ctx, ptr_thread);
	  //thread_host_ctx->offset--;
	}

      pthread_mutex_unlock (&thread_host_ctx->mutex);
      print_str ("DEBUG: net_worker_mono: [%d]: unregistered thread\n", _tid);
    }
  else if (thrd->flags & F_THRD_CINIT)
    {
      if (NULL != thrd->buffer0)
	{
	  free (thrd->buffer0);
	}
      free (thrd);
      print_str ("DEBUG: net_worker_mono: [%d]: released thread resources\n",
		 _tid);
    }

  print_str ("DEBUG: net_worker_mono: [%d]: thread shutting down..\n", _tid);

  ERR_remove_state (0);

  kill (getpid (), SIGUSR2);

  return NULL;
}

void *
net_worker (void *args)
{
  p_md_obj ptr;
  int r, s;
  uint8_t int_state = ST_NET_WORKER_ACT;

  time_t s_00 = 0, e_00;

  po_thrd thrd = (po_thrd) args;

  char buffer0[1024];

  mutex_lock (&thrd->mutex);

  sigset_t set;

  sigfillset (&set);
  /*sigaddset(&set, SIGPIPE);
   sigaddset(&set, SIGINT);
   sigaddset(&set, SIGUSR2);
   sigaddset(&set, SIGIO);
   sigaddset(&set, SIGURG);*/

  s = pthread_sigmask (SIG_SETMASK, &set, NULL);

  if (s != 0)
    {
      print_str ("ERROR: net_worker: pthread_sigmask (SIG_BLOCK) failed: %d\n",
		 s);
      abort ();
    }

  sigemptyset (&set);
  sigaddset (&set, SIGURG);
  sigaddset (&set, SIGIO);
  sigaddset (&set, SIGUSR1);

  s = pthread_sigmask (SIG_UNBLOCK, &set, NULL);

  if (s != 0)
    {
      print_str (
	  "ERROR: net_worker: pthread_sigmask (SIG_UNBLOCK) failed: %d\n", s);
      abort ();
    }

  thrd->timers.t1 = time (NULL);
  thrd->timers.t0 = time (NULL);
  thrd->timers.act_f |= (F_TTIME_ACT_T0 | F_TTIME_ACT_T1);

  thrd->buffer0 = malloc (262144);
  thrd->buffer0_size = 262144;

  pthread_t _pt = thrd->pt;
  pid_t _tid = (pid_t) syscall (SYS_gettid);

  thrd->status |= F_THRD_STATUS_INITIALIZED;

  //pthread_t parent = thrd->caller;

  pthread_mutex_unlock (&thrd->mutex);

  //pthread_kill (parent, SIGINT);

  mutex_lock (&thrd->mutex);
  if (thrd->flags & F_THRD_NOWPID)
    {
      goto init_done;
    }
  pthread_mutex_unlock (&thrd->mutex);

  sigemptyset (&set);
  sigaddset (&set, SIGINT);

  int sig = SIGINT;

  int re;
  if ((re = sigwait (&set, &sig)))
    {
      print_str ("WARNING: net_worker: [%d]: sigwait (SIGINT) failed with %d\n",
		 _tid, re);
    }

  mutex_lock (&thrd->mutex);

  init_done: ;

  print_str ("DEBUG: net_worker: [%d]: thread initializing [%d]\n", _tid,
	     thrd->oper_mode);

  pthread_mutex_unlock (&thrd->mutex);

  for (;;)
    {
      mutex_lock (&thrd->mutex);
      thrd->timers.t0 = time (NULL);

      if (thrd->flags & F_THRD_TERM)
	{
	  if (0 == s_00)
	    {
	      s_00 = time (NULL);
	    }
	  else
	    {
	      e_00 = time (NULL);
	      if ((e_00 - s_00) > T_NET_WORKER_SD)
		{
		  print_str (
		      "WARNING: net_worker: [%d]: sockets still active [%llu / %llu]\n",
		      _tid,
		      (unsigned long long int) md_get_off_ts (
			  &thrd->proc_objects),
		      (unsigned long long int) md_get_off_ts (
			  &thrd->in_objects));
		  pthread_mutex_unlock (&thrd->mutex);
		  break;
		}
	    }
	  if (net_proc_sock_hmemb (thrd) == 0)
	    {
	      /*print_str("NOTICE: net_worker: [%d]: thread shutting down..\n",
	       _tid);*/
	      pthread_mutex_unlock (&thrd->mutex);
	      break;
	    }
	}

      mutex_lock (&thrd->in_objects.mutex);

      if (0 == thrd->in_objects.offset)
	{
	  pthread_mutex_unlock (&thrd->in_objects.mutex);
	  pthread_mutex_unlock (&thrd->mutex);
	  goto begin_proc;
	}
      else
	{
	  print_str (
	      "D3: [%d]: push %llu items onto worker thread stack, %llu exist in chain\n",
	      _tid, (unsigned long long int) thrd->in_objects.offset,
	      (unsigned long long int) thrd->proc_objects.offset);

	}

      mutex_lock (&thrd->proc_objects.mutex);

      p_md_obj iobj_ptr = thrd->in_objects.first;
      while (iobj_ptr)
	{
	  if (thrd->proc_objects.offset == thrd->proc_objects.count)
	    {
	      break;
	    }
	  __sock t_pso = (__sock ) iobj_ptr->ptr;

	  mutex_lock (&t_pso->mutex);
	  if (!(t_pso->flags & F_OPSOCK_ACT))
	    {
	      print_str (
		  "D3: net_worker: [%d]: not importing %d, still processing\n",
		  (int) _tid, t_pso->sock);
	      pthread_mutex_unlock (&t_pso->mutex);
	      goto io_loend;
	    }

	  t_pso->st_p1 = thrd->buffer0;

	  struct f_owner_ex fown_ex;

	  pid_t async = 1;

	  if (ioctl (t_pso->sock, FIOASYNC, &async) == -1)
	    {
	      char err_buf[1024];
	      print_str (
		  "ERROR: net_worker: [%d]: ioctl (FIOASYNC) failed [%d] [%s]\n",
		  t_pso->sock, errno,
		  strerror_r (errno, err_buf, sizeof(err_buf)));
	      t_pso->flags |= F_OPSOCK_TERM;
	    }

	  fown_ex.pid = _tid;
	  fown_ex.type = F_OWNER_TID;

	  if (fcntl (t_pso->sock, F_SETOWN_EX, &fown_ex) == -1)
	    {
	      char err_buf[1024];
	      print_str (
		  "ERROR: net_worker: [%d]: fcntl (F_SETOWN_EX) failed [%d] [%s]\n",
		  t_pso->sock, errno,
		  strerror_r (errno, err_buf, sizeof(err_buf)));
	      t_pso->flags |= F_OPSOCK_TERM;
	    }
	  else
	    {
	      print_str (
		  "D4: net_worker: [%d]: fcntl set F_SETOWN_EX on [%d]\n", _tid,
		  t_pso->sock);
	    }

	  t_pso->pthread = thrd;

	  pthread_mutex_unlock (&t_pso->mutex);

	  if (NULL == md_alloc_le (&thrd->proc_objects, 0, 0, iobj_ptr->ptr))
	    {
	      print_str (
		  "ERROR: net_worker: [%d]: could not allocate socket to thread %d (out of memory)\n",
		  t_pso->sock, _tid);
	      abort ();
	    }

	  thrd->timers.t1 = time (NULL);
	  int_state |= ST_NET_WORKER_ACT;

	  md_unlink_le (&thrd->in_objects, iobj_ptr);

	  mutex_lock (&t_pso->sendq.mutex);

	  if (t_pso->sendq.offset > 0 && (t_pso->flags & F_OPSOCK_PROC_READY))
	    {
	      ssize_t sendq_rem;
	      if ((sendq_rem = net_proc_sendq (t_pso)) != 0)
		{
		  print_str (
		      "WARNING: net_proc_sendq: %zd items remain, socket:[%d]\n",
		      t_pso->sendq.offset, t_pso->sock);
		}
	      thrd->timers.t1 = time (NULL);
	      int_state |= ST_NET_WORKER_ACT;
	    }

	  pthread_mutex_unlock (&t_pso->sendq.mutex);

	  io_loend: ;

	  iobj_ptr = iobj_ptr->next;
	}

      pthread_mutex_unlock (&thrd->in_objects.mutex);

      if (!thrd->proc_objects.offset)
	{
	  pthread_mutex_unlock (&thrd->proc_objects.mutex);
	  pthread_mutex_unlock (&thrd->mutex);
	  goto loop_end;
	}

      pthread_mutex_unlock (&thrd->proc_objects.mutex);
      pthread_mutex_unlock (&thrd->mutex);

      begin_proc: ;

      ptr = thrd->proc_objects.first;

      while (ptr)
	{
	  __sock pso = (__sock ) ptr->ptr;

	  if (NULL == pso)
	    {
	      print_str ("ERROR: net_worker: empty socket data reference\n");
	      abort ();
	    }

	  uint32_t status = 0;

	  ptr = net_worker_process_socket (pso, ptr, &int_state, 0, thrd, &_tid,
					   buffer0, &status);
	}

      loop_end: ;

      usleep (100);

      if (int_state & ST_NET_WORKER_ACT)
	{
	  int_state ^= ST_NET_WORKER_ACT;
	  continue;
	}

      time_t thread_inactive = (time (NULL) - thrd->timers.t1);

      if (thread_inactive > 1)
	{
	  unsigned int t_interval;

	  mutex_lock (&thrd->mutex);

	  if (md_get_off_ts (&thrd->in_objects))
	    {
	      pthread_mutex_unlock (&thrd->mutex);
	      continue;
	    }
	  else if ((int_state & ST_NET_WORKER_IDLE_ALERT)
	      || (thrd->oper_mode == SOCKET_OPMODE_RECIEVER
		  && md_get_off_ts (&thrd->proc_objects) > 0))
	    {
	      t_interval = I_NET_WORKER_IDLE_ALERT;
	    }
	  else
	    {
	      t_interval = UINT_MAX;
	    }

	  pthread_mutex_unlock (&thrd->mutex);

	  print_str ("D6: [%d]: putting worker to sleep [%hu] [%u]\n", _tid,
		     thrd->oper_mode, t_interval);
	  ts_flag_32 (&thrd->mutex, F_THRD_STATUS_SUSPENDED, &thrd->status);
	  sleep (t_interval);
	  thrd->timers.t1 = time (NULL);
	  ts_unflag_32 (&thrd->mutex, F_THRD_STATUS_SUSPENDED, &thrd->status);
	  print_str ("D6: [%d]: thread waking up [%hu]\n", _tid,
		     thrd->oper_mode);
	}

      int_state ^= int_state & ST_NET_WORKER_IDLE_ALERT;

    }

  pmda thread_host_ctx = thrd->host_ctx;

  mutex_lock (&thread_host_ctx->mutex);

  mutex_lock (&thrd->mutex);

  free (thrd->buffer0);

  pthread_mutex_unlock (&thrd->mutex);

  p_md_obj ptr_thread = search_thrd_id (thread_host_ctx, &_pt);

  if (NULL == ptr_thread)
    {
      print_str (
	  "ERROR: net_worker: [%d]: thread already unregistered on exit (search_thrd_id failed)\n",
	  (int) _tid);
      abort ();
    }
  else
    {
      //thread_host_ctx->flags |= F_MDA_REFPTR;
      md_unlink_le (thread_host_ctx, ptr_thread);
      //thread_host_ctx->offset--;
    }

  pthread_mutex_unlock (&thread_host_ctx->mutex);

  print_str ("DEBUG: net_worker: [%d]: thread shutting down..\n", _tid);

  ERR_remove_state (0);

  kill (getpid (), SIGUSR2);

  return NULL;
}

#define NET_ASSIGN_SOCK_CLEANUP(c) { \
  mutex_lock (&spso->mutex); \
  spso->status = c; \
  spso->s_errno = 0; \
  pthread_mutex_unlock (&spso->mutex); \
  net_destroy_connection ( pso); \
  mutex_lock (&base->mutex); \
  mutex_lock (&pso->mutex); \
  net_failclean (pso); \
  pthread_mutex_unlock (&pso->mutex); \
  md_unlink_le (base, base->pos); \
  pthread_mutex_unlock (&base->mutex); \
};

static int
net_assign_sock (pmda base, pmda threadr, __sock pso, __sock spso)
{
  int r;

  mutex_lock (&pso->mutex);

  if (pso->flags & F_OPSOCK_CS_NOASSIGNTHREAD)
    {
      goto ready;
    }

  if (pso->flags & F_OPSOCK_CS_MONOTHREAD)
    {
      r = 1;
      print_str ("DEBUG: net_assign_sock: spawning new thread for [%d]\n",
		 pso->sock);
      pso->flags |= F_OPSOCK_ACT | F_OPSOCK_PROC_READY;
    }
  else
    {
      r = 0;
    }

  pthread_mutex_unlock (&pso->mutex);

  pthread_t pt;

  if (r == 1)
    {
      o_thrd data =
	{ 0 };
      data.id = 0;
      data.role = THREAD_ROLE_NET_WORKER;
      data.oper_mode = SOCKET_OPMODE_RECIEVER;
      data.host_ctx = threadr;
      data.in_objects.lref_ptr = pso;
      data.flags |= F_THRD_DETACH | F_THRD_NOWPID
	  | pso->common.thread_inherit_flags;
      mutex_init (&data.mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);
      pthread_t pt;

      if ((r = thread_create (net_worker_mono, 0, threadr, 0, 0, 0,
      F_THC_USER_DATA,
			      &data, NULL, &pt)))
	{
	  print_str (
	      "ERROR: net_assign_sock: thread_create failed, code %d, sock %d (accept)\n",
	      r, pso->sock);

	  NET_ASSIGN_SOCK_CLEANUP(9)
	  return 1;
	}
      else
	{
	  /*sigset_t set;
	   sigemptyset (&set);
	   sigaddset (&set, SIGHUP);

	   int sig;

	   int re;
	   if ((re = sigwait (&set, &sig)))
	   {
	   fprintf (
	   stderr,
	   "WARNING: net_open_listening_socket: [%d]: sigwait (SIGINT) failed with %d\n",
	   syscall (SYS_gettid), re);
	   }*/

	  return 0;
	}
    }
  else if ((r = push_object_to_thread (pso, threadr,
				       (dt_score_ptp) net_get_score, &pt)))
    {
      print_str (
	  "ERROR: push_object_to_thread failed, code %d, sock %d (accept)\n", r,
	  pso->sock);
      NET_ASSIGN_SOCK_CLEANUP(6)
      return 1;
    }

  mutex_lock (&pso->mutex);

  pso->thread = pt;

  net_pop_rc (pso, &pso->init_rc1);

  ready: ;

  //if (!(pso->flags & F_OPSOCK_SSL))
  // {
  pso->flags |= F_OPSOCK_ACT | F_OPSOCK_PROC_READY;

  // }

  pthread_mutex_unlock (&pso->mutex);

  return 0;

}

static __sock
net_prep_acsock (pmda base, pmda threadr, __sock spso, int fd,
		 struct sockaddr *sa)
{
  mutex_lock (&base->mutex);

  __sock pso;

  if (NULL == (pso = md_alloc_le (base, sizeof(_sock), 0, NULL)))
    {
      print_str ("ERROR: net_prep_acsock: out of resources [%llu/%llu]\n",
		 (unsigned long long int) base->offset,
		 (unsigned long long int) base->count);
      spso->status = 23;
      close (fd);
      pthread_mutex_unlock (&base->mutex);
      return NULL;
    }

  pso->sock = fd;
  pso->rcv0 = spso->rcv1;
  pso->rcv1 = spso->rcv1_t;
  pso->parent = (void *) spso;
  pso->st_p0 = spso->st_p0;
  pso->oper_mode = SOCKET_OPMODE_RECIEVER;
  pso->flags |= spso->ac_flags | F_OPSOCK_CONNECT | F_OPSOCK_IN
      | (spso->flags & (F_OPSOCK_SSL | F_OPSOCK_INIT_SENDQ));
  pso->pcheck_r = (_t_rcall) net_chk_timeout;
  pso->policy = spso->policy;
  //pso->limits.sock_timeout = spso->policy.idle_timeout;
  pso->timers.last_act = time (NULL);
  spso->timers.last_act = time (NULL);

  pso->sock_ca = spso->sock_ca;
  pso->common = spso->common;

  pso->res = spso->res;

  pso->res.ai_addr = malloc (sizeof(struct sockaddr));
  memcpy (pso->res.ai_addr, sa, sizeof(struct sockaddr));

  net_addr_to_ipr (pso, &pso->ipr);

  md_copy_le (&spso->init_rc0, &pso->init_rc0, sizeof(_proc_ic_o), NULL);
  md_copy_le (&spso->init_rc1, &pso->init_rc1, sizeof(_proc_ic_o), NULL);
  md_copy_le (&spso->shutdown_rc0, &pso->shutdown_rc0, sizeof(_proc_ic_o),
  NULL);
  md_copy_le (&spso->shutdown_rc1, &pso->shutdown_rc1, sizeof(_proc_ic_o),
  NULL);
  md_copy_le (&spso->init_rc0_ssl, &pso->init_rc0_ssl, sizeof(_proc_ic_o),
  NULL);
  md_init_le (&pso->tasks, 8);

  if (!spso->unit_size)
    {
      pso->unit_size = SOCK_RECVB_SZ;
    }
  else
    {
      pso->unit_size = spso->unit_size;
    }

  pso->buffer0 = malloc (pso->unit_size);
  pso->buffer0_len = pso->unit_size;

  pso->host_ctx = base;

  if (mutex_init (&pso->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      spso->status = 4;
      shutdown (fd, SHUT_RDWR);
      close (fd);
      net_failclean (pso);
      md_unlink_le (base, base->pos);
      pthread_mutex_unlock (&base->mutex);
      return NULL;
    }

  if (pso->flags & F_OPSOCK_INIT_SENDQ)
    {
      md_init_le (&pso->sendq, 8192);
    }

  p_md_obj pso_ptr = base->pos;

  if ((pso->flags & F_OPSOCK_SSL))
    {
      if ((pso->ssl = SSL_new (spso->ctx)) == NULL)
	{
	  ERR_print_errors_fp (stderr);
	  ERR_clear_error ();
	  spso->s_errno = 0;
	  spso->status = 5;
	  shutdown (fd, SHUT_RDWR);
	  close (fd);
	  net_failclean (pso);
	  md_unlink_le (base, pso_ptr);
	  pthread_mutex_unlock (&base->mutex);
	  return NULL;
	}

      SSL_set_fd (pso->ssl, pso->sock);
      SSL_set_accept_state (pso->ssl);
      SSL_set_read_ahead (pso->ssl, 1);

      spso->rcv_cb_t = spso->rcv_cb;
      spso->rcv_cb = (_p_s_cb) net_accept_ssl;
      spso->flags |= F_OPSOCK_ST_SSL_ACCEPT;

      pso->rcv_cb = spso->rcv0;

      //pso->rcv_cb_t = spso->rcv0;

      pso->send0 = (_p_ssend) net_ssend_ssl_b;

    }
  else
    {
      pso->rcv_cb = spso->rcv0;

      pso->send0 = (_p_ssend) net_ssend_b;

    }

  net_pop_rc (pso, &pso->init_rc0);

  pthread_mutex_unlock (&base->mutex);

  return pso;
}

int
net_accept (__sock spso, pmda base, pmda threadr, void *data)
{
  int fd;
  socklen_t sin_size = sizeof(struct sockaddr_storage);
  struct sockaddr_storage a;

  mutex_lock (&spso->mutex);

  spso->s_errno = 0;

  if ((fd = accept (spso->sock, (struct sockaddr *) &a, &sin_size)) == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
	{
	  /*if (!(spso->timers.flags & F_ST_MISC00_ACT))
	   {
	   spso->timers.flags |= F_ST_MISC00_ACT;
	   spso->timers.misc00 = time(NULL);
	   }
	   else
	   {
	   spso->timers.misc01 = time(NULL);
	   time_t pt_diff = (spso->timers.misc01 - spso->timers.misc00);
	   if (pt_diff > spso->policy.accept_timeout)
	   {
	   print_str(
	   "WARNING: net_accept: [%d] accept timed out after %u seconds\n",
	   spso->sock, pt_diff);
	   spso->status = -7;
	   goto f_term;
	   }
	   }*/

	  pthread_mutex_unlock (&spso->mutex);
	  return 2;
	}
      spso->status = -1;

      char err_buf[1024];
      print_str ("ERROR: net_accept: [%d]: accept: [%d]: [%s]\n", spso->sock,
      errno,
		 strerror_r (errno, err_buf, sizeof(err_buf)));

      //f_term: ;

      pthread_mutex_unlock (&spso->mutex);
      return 0;
    }

  /*if (spso->timers.flags & F_ST_MISC00_ACT)
   {
   spso->timers.flags ^= F_ST_MISC00_ACT;
   }*/

  int ret;

  if ((ret = fcntl (fd, F_SETFL, O_NONBLOCK)) == -1)
    {
      close (fd);
      spso->status = -2;

      char err_buf[1024];
      print_str (
	  "ERROR: net_accept: [%d]: fcntl F_SETFL(O_NONBLOCK): [%d]: [%s]\n",
	  spso->sock,
	  errno,
	  strerror_r (errno, err_buf, sizeof(err_buf)));

      pthread_mutex_unlock (&spso->mutex);
      return 0;
    }

  __sock pso;

  //pthread_mutex_unlock (&spso->mutex);
  pso = net_prep_acsock (base, threadr, spso, fd, (struct sockaddr *) &a);
  //mutex_lock (&spso->mutex);

  if ( NULL == pso)
    {
      /*
       if (spso->status == 23)
       {
       spso->status = 0;
       }
       else
       {
       spso->status = -3;
       }*/
      print_str ("ERROR: net_accept: [%d]: net_prep_acsock failed: [%d]\n",
		 spso->sock, spso->status);

      pthread_mutex_unlock (&spso->mutex);
      return 0;
    }

  spso->cc = (void*) pso;

  uint32_t spso_flags = spso->flags;

  pthread_mutex_unlock (&spso->mutex);

  if (!(spso_flags & F_OPSOCK_SSL))
    {
      ret = net_assign_sock (base, threadr, pso, spso);

      if (0 == ret)
	{
	  mutex_lock (&spso->mutex);
	  spso->children++;
	  pthread_mutex_unlock (&spso->mutex);
	}
      else
	{
	  return 2;
	}
    }

  return 0;
}

int
net_recv (__sock pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  ssize_t rcv_limit = pso->unit_size - pso->counters.b_read;

  if (rcv_limit <= 0)
    {
      pthread_mutex_unlock (&pso->mutex);
      return 0;
    }

  ssize_t rcvd = recv (pso->sock, (data + pso->counters.b_read), rcv_limit, 0);

  if (rcvd == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
	{
	  pthread_mutex_unlock (&pso->mutex);
	  return 2;
	}

      pso->s_errno = errno;
      pso->status = -1;
      pso->flags |= F_OPSOCK_TERM;
      pthread_mutex_unlock (&pso->mutex);
      return 1;
    }
  else if (0 == rcvd)
    {
      pso->timers.last_act = time (NULL);
      pso->flags |= F_OPSOCK_TERM | F_OPSOCK_TS_DISCONNECTED;
      goto fin;
    }

  pso->timers.last_act = time (NULL);
  pso->counters.b_read += rcvd;
  pso->counters.t_read += rcvd;

  fin: ;

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_recv_ssl (__sock pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  ssize_t rcv_limit = pso->unit_size - pso->counters.b_read;

  if (rcv_limit <= 0)
    {
      pthread_mutex_unlock (&pso->mutex);
      return 0;
    }

  int rcvd;
  ssize_t session_rcvd = 0;

  while (rcv_limit > 0
      && (rcvd = SSL_read (pso->ssl, (data + pso->counters.b_read),
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
	  pso->timers.last_act = time (NULL);
	}

      pso->s_errno = SSL_get_error (pso->ssl, rcvd);

      ERR_print_errors_fp (stderr);
      ERR_clear_error ();

      if (pso->s_errno == SSL_ERROR_WANT_READ
	  || pso->s_errno == SSL_ERROR_WANT_WRITE)
	{
	  pthread_mutex_unlock (&pso->mutex);
	  return 2;
	}

      pso->status = rcvd;

      if ((pso->s_errno == SSL_ERROR_WANT_CONNECT
	  || pso->s_errno == SSL_ERROR_WANT_ACCEPT
	  || pso->s_errno == SSL_ERROR_WANT_X509_LOOKUP))
	{
	  pthread_mutex_unlock (&pso->mutex);
	  return 2;
	}

      int ret;

      pso->flags |= F_OPSOCK_TERM;

      if (rcvd == 0)
	{
	  if (pso->s_errno == (SSL_ERROR_ZERO_RETURN))
	    {
	      pso->flags |= F_OPSOCK_TS_DISCONNECTED;
	    }

	  if (pso->counters.b_read)
	    {
	      ret = 2;
	    }
	  else
	    {
	      ret = 0;
	    }
	}
      else
	{
	  ret = 1;
	}

      pthread_mutex_unlock (&pso->mutex);

      return ret;

    }

  pso->timers.last_act = time (NULL);

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

#define T_NET_ACCEPT_SSL        (time_t) 4

int
net_accept_ssl (__sock spso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&spso->mutex);

  __sock pso = (__sock ) spso->cc;

  mutex_lock (&pso->mutex);

  int ret;

  if ((ret = SSL_accept (pso->ssl)) != 1)
    {
      int ssl_err = SSL_get_error (pso->ssl, ret);
      ERR_print_errors_fp (stderr);
      ERR_clear_error ();

      if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
	{
	  print_str (
	      "D6: net_accept_ssl: [%d]: SSL_accept not satisfied: [%d] [%d]\n",
	      pso->sock, ret, ssl_err);

	  if (!(pso->timers.flags & F_ST_MISC00_ACT))
	    {
	      pso->timers.flags |= F_ST_MISC00_ACT;
	      pso->timers.misc00 = time (NULL);
	    }
	  else
	    {
	      pso->timers.misc01 = time (NULL);
	      time_t pt_diff = (pso->timers.misc01 - pso->timers.misc00);
	      if (pt_diff > pso->policy.ssl_accept_timeout)
		{
		  print_str (
		      "WARNING: SSL_accept: [%d] timed out after %u seconds\n",
		      pso->sock, pt_diff);
		  ret = 0;
		  goto f_term;
		}
	      usleep (1000);
	    }

	  pthread_mutex_unlock (&pso->mutex);
	  pthread_mutex_unlock (&spso->mutex);
	  return -3;
	}

      if (ssl_err == SSL_ERROR_SYSCALL && ret == -1)
	{
	  char err_buf[1024];
	  print_str ("ERROR: SSL_accept: [%d]: accept: [%d]: [%s]\n",
		     spso->sock,
		     errno,
		     strerror_r (errno, err_buf, sizeof(err_buf)));
	}
      else
	{
	  print_str (
	      "ERROR: SSL_accept: socket: [%d] code: [%d] sslerr: [%d] [%d]\n",
	      pso->sock, ret, ssl_err, ret);
	}

      pso->flags |= F_OPSOCK_ERROR;
      pso->sslerr = ssl_err;

      f_term: ;

      pso->flags |= F_OPSOCK_TERM | F_OPSOCK_SKIP_SSL_SD;

    }

  if (pso->timers.flags & F_ST_MISC00_ACT)
    {
      pso->timers.flags ^= F_ST_MISC00_ACT;
    }

  pso->timers.misc00 = (time_t) 0;
  pso->timers.last_act = time (NULL);
  spso->timers.last_act = time (NULL);
  //pso->rcv_cb = pso->rcv_cb_t;

  BIO_set_buffer_size(SSL_get_rbio (pso->ssl), 16384);
  BIO_set_buffer_size(SSL_get_wbio (pso->ssl), 16384);

  //pso->limits.sock_timeout = spso->policy.idle_timeout;

  if (spso->flags & F_OPSOCK_ST_SSL_ACCEPT)
    {
      spso->flags ^= F_OPSOCK_ST_SSL_ACCEPT;
    }

  if (!(pso->flags & F_OPSOCK_TERM))
    {
      int eb;

      SSL_CIPHER_get_bits (SSL_get_current_cipher (pso->ssl), &eb);

      char cd[255];

      SSL_CIPHER_description (SSL_get_current_cipher (pso->ssl), cd,
			      sizeof(cd));

      print_str ("DEBUG: SSL_accept: %d, %s (%d) - %s\n", pso->sock,
		 SSL_get_cipher(pso->ssl), eb,
		 SSL_CIPHER_get_version (SSL_get_current_cipher (pso->ssl)));

      print_str ("D2: SSL_CIPHER_description: %d, %s", pso->sock, cd);

      ssl_show_client_certs (pso, pso->ssl);
    }

  spso->rcv_cb = spso->rcv_cb_t;

  //pso->flags |= F_OPSOCK_ACT;

  net_pop_rc (pso, &pso->init_rc0_ssl);

  pthread_mutex_unlock (&pso->mutex);
  pthread_mutex_unlock (&spso->mutex);

  ret = net_assign_sock (base, threadr, pso, spso);

  if (0 == ret)
    {
      mutex_lock (&spso->mutex);
      spso->children++;
      pthread_mutex_unlock (&spso->mutex);
    }

  return 0;
}

int
net_connect_ssl (__sock pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  int ret, f_ret = 0;

  if ((ret = SSL_connect (pso->ssl)) != 1)
    {
      int ssl_err = SSL_get_error (pso->ssl, ret);
      ERR_print_errors_fp (stderr);
      ERR_clear_error ();

      if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
	{
	  print_str (
	      "D6: net_connect_ssl: [%d]: SSL_connect not satisfied: [%d] [%d]\n",
	      pso->sock, ret, ssl_err);

	  if (!(pso->timers.flags & F_ST_MISC00_ACT))
	    {
	      pso->timers.flags |= F_ST_MISC00_ACT;
	      pso->timers.misc00 = time (NULL);
	    }
	  else
	    {
	      pso->timers.misc01 = time (NULL);
	      time_t pt_diff = (pso->timers.misc01 - pso->timers.misc00);
	      if (pt_diff > pso->policy.ssl_connect_timeout)
		{
		  print_str (
		      "WARNING: SSL_connect: [%d] timed out after %u seconds\n",
		      pso->sock, pt_diff);
		  f_ret = 2;
		  goto f_term;
		}

	      usleep (1000);
	    }

	  pthread_mutex_unlock (&pso->mutex);
	  return -3;
	}

      pso->status = ret;
      pso->s_errno = ssl_err;

      print_str ("ERROR: SSL_connect: socket:[%d] code:[%d] sslerr:[%d]\n",
		 pso->sock, ret, ssl_err);

      f_term: ;

      pso->flags |= F_OPSOCK_TERM | F_OPSOCK_SKIP_SSL_SD;

      //pthread_mutex_unlock(&pso->mutex);
      //return 1;
    }

  if (pso->timers.flags & F_ST_MISC00_ACT)
    {
      pso->timers.flags ^= F_ST_MISC00_ACT;
    }

  pso->timers.last_act = time (NULL);
  pso->rcv_cb = pso->rcv_cb_t;
  //pso->limits.sock_timeout = SOCK_DEFAULT_IDLE_TIMEOUT;

  BIO_set_buffer_size(SSL_get_rbio (pso->ssl), 16384);
  BIO_set_buffer_size(SSL_get_wbio (pso->ssl), 16384);

  if (!(pso->flags & F_OPSOCK_TERM))
    {
      int eb;

      SSL_CIPHER_get_bits (SSL_get_current_cipher (pso->ssl), &eb);

      char cd[255];

      SSL_CIPHER_description (SSL_get_current_cipher (pso->ssl), cd,
			      sizeof(cd));

      print_str ("DEBUG: SSL_connect: %d, %s (%d) - %s\n", pso->sock,
		 SSL_get_cipher(pso->ssl), eb,
		 SSL_CIPHER_get_version (SSL_get_current_cipher (pso->ssl)));

      print_str ("D1: SSL_CIPHER_description: %d, %s", pso->sock, cd);
    }

  /*if (pso->flags & F_OPSOCK_ST_SSL_CONNECT)
   {
   pso->flags ^= F_OPSOCK_ST_SSL_CONNECT;
   }*/

  pso->flags |= F_OPSOCK_PROC_READY;

  pthread_mutex_unlock (&pso->mutex);

  return f_ret;
}

int
net_ssend_b (__sock pso, void *data, size_t length)
{
  mutex_lock (&pso->mutex);

  if (0 == length)
    {
      print_str ("ERROR: net_ssend_b: [%d]: zero length input\n", pso->sock);
      abort ();
    }

  if (pso->flags & F_OPSOCK_ORPHANED)
    {
      pthread_mutex_unlock (&pso->mutex);
      return 1;
    }

  int ret = 0;
  ssize_t s_ret;
  uint32_t i = 1;

  unsigned char *in_data = (unsigned char*) data;

  time_t t00, t01;

  nssb_start: ;

  t00 = time (NULL);

  while ((s_ret = send (pso->sock, in_data, length,
  MSG_WAITALL | MSG_NOSIGNAL)) == -1)
    {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
	  char e_buffer[1024];
	  print_str ("ERROR: net_ssend_b: send failed: %s\n",
		     strerror_r (errno, e_buffer, sizeof(e_buffer)));
	  pso->s_errno = errno;
	  ret = 1;
	  break;
	}
      else
	{
	  char err_buf[1024];
	  print_str ("D6: net_ssend_b: [%d] [%d]: %s\n", pso->sock, errno,
		     strerror_r (errno, err_buf, 1024));
	}

      t01 = time (NULL);
      time_t pt_diff = (t01 - t00);

      if (pt_diff > pso->policy.send_timeout)
	{
	  print_str (
	      "WARNING: net_ssend_ssl_b: [%d] timed out after %u seconds\n",
	      pso->sock, pt_diff);
	  pthread_mutex_unlock (&pso->mutex);
	  return 1;
	}

      usleep (100000);
    }

  if (!ret)
    {
      pso->counters.session_write = (ssize_t) s_ret;
      pso->counters.total_write += (ssize_t) s_ret;

      if (s_ret < (ssize_t) length)
	{
	  print_str (
	      "D6: net_ssend_b: [%d] partial send occured: %zu / %zu [%u]\n",
	      pso->sock, s_ret, length, i);

	  in_data = (in_data + s_ret);
	  length = (length - s_ret);
	  i++;

	  goto nssb_start;

	}
    }

  pthread_mutex_unlock (&pso->mutex);

  return ret;
}

int
net_ssend_ssl_b (__sock pso, void *data, size_t length)
{

  mutex_lock (&pso->mutex);

  if (0 == length)
    {
      print_str ("ERROR: net_ssend_ssl_b: [%d]: zero length input\n",
		 pso->sock);
      abort ();
    }

  if (pso->flags & F_OPSOCK_ORPHANED)
    {
      pthread_mutex_unlock (&pso->mutex);
      return 1;
    }

  int ret, f_ret;

  time_t t00 = time (NULL), t01;

  while ((ret = SSL_write (pso->ssl, data, length)) < 1)
    {
      pso->s_errno = SSL_get_error (pso->ssl, ret);
      ERR_print_errors_fp (stderr);
      ERR_clear_error ();

      print_str (
	  "D6: net_ssend_ssl_b: [%d]: SSL_write not satisfied: [%d] [%d]\n",
	  pso->sock, ret, pso->s_errno);

      if (!(pso->s_errno == SSL_ERROR_WANT_READ
	  || pso->s_errno == SSL_ERROR_WANT_WRITE))
	{
	  pso->status = ret;

	  if (!(pso->s_errno == SSL_ERROR_WANT_CONNECT
	      || pso->s_errno == SSL_ERROR_WANT_ACCEPT
	      || pso->s_errno == SSL_ERROR_WANT_X509_LOOKUP))
	    {
	      pso->flags |= F_OPSOCK_TERM;

	      if (ret == 0)
		{
		  if (pso->s_errno == (SSL_ERROR_ZERO_RETURN))
		    {
		      pso->flags |= F_OPSOCK_TS_DISCONNECTED;
		      print_str (
			  "WARNING: net_ssend_ssl_b: [%d] socket disconnected\n",
			  pso->sock);
		    }
		  else
		    {
		      print_str (
			  "DEBUG: net_ssend_ssl_b: [%d] SSL_write returned 0\n",
			  pso->sock);
		    }
		}

	      pthread_mutex_unlock (&pso->mutex);
	      return 1;
	    }
	}

      t01 = time (NULL);
      time_t pt_diff = (t01 - t00);

      if (pt_diff > pso->policy.send_timeout)
	{
	  print_str (
	      "WARNING: net_ssend_ssl_b: [%d] SSL_write timed out after %u seconds\n",
	      pso->sock, pt_diff);
	  pthread_mutex_unlock (&pso->mutex);
	  return 1;
	}

      usleep (25000);
    }

  if (ret > 0 && ret < length)
    {
      print_str (
	  "ERROR: net_ssend_ssl_b: [%d] partial SSL_write occured on socket\n",
	  pso->sock);
      pso->flags |= F_OPSOCK_TERM;
      f_ret = 1;
    }
  else
    {
      f_ret = 0;
    }

  pso->counters.session_write = (ssize_t) ret;
  pso->counters.total_write += (ssize_t) ret;

  pthread_mutex_unlock (&pso->mutex);

  return f_ret;
}

int
net_ssend_ssl (__sock pso, void *data, size_t length)
{
  if (!length)
    {
      return -2;
    }

  mutex_lock (&pso->mutex);

  int ret;

  if ((ret = SSL_write (pso->ssl, data, length)) < 1)
    {
      pso->s_errno = SSL_get_error (pso->ssl, ret);
      ERR_print_errors_fp (stderr);
      ERR_clear_error ();

      if (pso->s_errno == SSL_ERROR_WANT_READ
	  || pso->s_errno == SSL_ERROR_WANT_WRITE)
	{
	  pthread_mutex_unlock (&pso->mutex);
	  return 2;
	}

      pso->status = ret;

      if ((pso->s_errno == SSL_ERROR_WANT_CONNECT
	  || pso->s_errno == SSL_ERROR_WANT_ACCEPT
	  || pso->s_errno == SSL_ERROR_WANT_X509_LOOKUP))
	{
	  pthread_mutex_unlock (&pso->mutex);
	  return 2;
	}

      pso->flags |= F_OPSOCK_TERM;

      if (ret == 0)
	{
	  pso->flags |= F_OPSOCK_TS_DISCONNECTED;
	}

      pthread_mutex_unlock (&pso->mutex);

      return 1;
    }

  if (ret > 0 && ret < length)
    {
      print_str (
	  "ERROR: net_ssend_ssl: [%d] partial SSL_write occured on socket\n",
	  pso->sock);
      pso->flags |= F_OPSOCK_TERM;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_ssend (__sock pso, void *data, size_t length)
{
  int ret;

  mutex_lock (&pso->mutex);

  if ((ret = send (pso->sock, data, length, MSG_WAITALL | MSG_NOSIGNAL)) == -1)
    {
      if ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
	  pthread_mutex_unlock (&pso->mutex);
	  return 2;
	}
      pso->status = -1;
      pso->s_errno = errno;
      pthread_mutex_unlock (&pso->mutex);
      return 1;
    }
  else if (ret != length)
    {
      pso->status = 11;
      pthread_mutex_unlock (&pso->mutex);
      return 1;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_proc_worker_tasks (__sock pso)
{
  pmda rc = &pso->tasks;

  mutex_lock (&rc->mutex);

  if (!rc->offset)
    {
      pthread_mutex_unlock (&rc->mutex);
      return -1;
    }

  p_md_obj ptr = rc->first;
  int c = 0;

  while (ptr)
    {
      __net_task task = (__net_task ) ptr->ptr;

      if (0 == task->net_task_proc (pso, task))
	{
	  print_str ("DEBUG: net_pop_worker_task: [%d]: processed task\n",
		     syscall (SYS_gettid));
	  p_md_obj c_ptr = ptr;
	  ptr = ptr->next;
	  md_unlink_le (rc, c_ptr);
	  c++;
	}
      else
	{
	  ptr = ptr->next;
	}
    }

  pthread_mutex_unlock (&rc->mutex);

  return c;
}

int
net_register_task (pmda host_ctx, __sock pso, pmda rt, _net_task_proc proc,
		   void *data, uint16_t flags)
{
  int ret;

  mutex_lock (&host_ctx->mutex);
  mutex_lock (&pso->mutex);
  mutex_lock (&rt->mutex);

  if (rt->offset == rt->count)
    {
      ret = 2;
      goto exit;
    }

  __net_task task = md_alloc_le (rt, sizeof(_net_task), 0, NULL);

  if ( NULL == task)
    {
      ret = 1;
      goto exit;
    }

  ret = 0;

  task->data = data;
  task->net_task_proc = proc;

  if ((pthread_t) 0 != pso->thread)
    {
      pthread_kill (pso->thread, SIGUSR1);
    }

  exit: ;

  pthread_mutex_unlock (&rt->mutex);
  pthread_mutex_unlock (&pso->mutex);
  pthread_mutex_unlock (&host_ctx->mutex);

  return ret;
}

int
net_pop_rc (__sock pso, pmda rc)
{
  mutex_lock (&rc->mutex);
  if ( NULL == rc->first)
    {
      pthread_mutex_unlock (&rc->mutex);
      return 1;
    }

  p_md_obj ptr = rc->pos;

  while (ptr)
    {
      __proc_ic_o pic = (__proc_ic_o) ptr->ptr;

      if ( NULL != pic->call )
	{
	  pic->call(pso);
	}

      ptr = ptr->prev;
    }

  pthread_mutex_unlock (&rc->mutex);

  return 0;
}

int
net_push_rc (pmda rc, _t_rcall call, uint32_t flags)
{
  mutex_lock (&rc->mutex);

  __proc_ic_o pic = md_alloc_le (rc, sizeof(_proc_ic_o), 0, NULL);

  if ( NULL == pic)
    {
      pthread_mutex_unlock (&rc->mutex);
      print_str ("ERROR: net_push_rc: could not allocate memory\n");
      return 1;
    }

  pic->call = call;
  pic->flags = flags;

  pthread_mutex_unlock (&rc->mutex);

  return 0;
}

static int
net_search_dupip (__sock pso, void *arg)
{
  __sock_cret parg = (__sock_cret) arg;
  if (pso->oper_mode == parg->pso->oper_mode )
    {
      if (!memcmp(
	      &pso->ipr.ip,
	      &parg->pso->ipr.ip,
	      16))
	{
	  parg->ret++;
	}
    }
  return 0;
}

int
net_parent_proc_rc0_destroy (__sock pso)
{
  mutex_lock (&pso->mutex);

  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;

      mutex_lock (&pso->parent->mutex);
      if (pso->parent->children > 0)
	{
	  pso->parent->children--;
	}
      pthread_mutex_unlock (&pso->parent->mutex);

      break;
    }

  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

int
net_socket_init_enforce_policy (__sock pso)
{
  switch (pso->flags & F_OPSOCK_OPER_MODE)
    {
    case F_OPSOCK_CONNECT:
      ;

      if (pso->policy.max_sim_ip)
	{
	  _sock_cret sc_ret =
	    { .pso = pso, .ret = 0 };

	  int dip_sr;
	  if ((dip_sr = net_enum_sockr (pso->host_ctx, net_search_dupip,
					(void*) &sc_ret)))
	    {
	      print_str (
		  "ERROR: net_socket_init_enforce_policy: [%d] net_enum_sockr failed: [%d]\n",
		  pso->sock, dip_sr);
	      pso->flags |= F_OPSOCK_TERM;
	      return 2;
	    }

	  if (sc_ret.ret > pso->policy.max_sim_ip)
	    {
	      print_str (
		  "WARNING: net_socket_init_enforce_policy: [%d] max_sim limit reached: [%u/%u]\n",
		  pso->sock, sc_ret.ret, pso->policy.max_sim_ip);
	      pso->flags |= F_OPSOCK_TERM;
	      return 2;
	    }
	}

      if (pso->policy.max_connects > 0)
	{
	  if (pso->parent->children >= pso->parent->policy.max_connects)
	    {
	      print_str (
		  "WARNING: net_socket_init_enforce_policy: [%d] max_connects limit reached: [%u/%u]\n",
		  pso->sock, pso->parent->children,
		  pso->parent->policy.max_connects);
	      pso->flags |= F_OPSOCK_TERM;
	      return 2;
	    }
	}
      break;

    }

  return 0;
}

int
net_exec_sock (pmda base, __ipr ipr, uint32_t flags, _ne_sock call, void *arg)
{
  mutex_lock (&base->mutex);

  p_md_obj ptr = base->first;

  int ret = -2;

  while (ptr)
    {
      __sock pso = (__sock) ptr->ptr;

      mutex_lock (&pso->mutex);

      //printf(":: %hhu.%hhu.%hhu.%hhu:%hu \n", ipr->ip[0], ipr->ip[1], ipr->ip[2], ipr->ip[3], ipr->port);
      //printf(">> %hhu.%hhu.%hhu.%hhu:%hu \n", pso->ipr.ip[0], pso->ipr.ip[1], pso->ipr.ip[2], pso->ipr.ip[3], pso->ipr.port);

      if (!(pso->flags & flags))
	{
	  goto loop_end;
	}

      if ( memcmp(ipr, &pso->ipr, sizeof(_ipr)) )
	{
	  goto loop_end;
	}

      ret = call(pso, arg);

      pthread_mutex_unlock (&pso->mutex);

      break;

      loop_end:;

      pthread_mutex_unlock (&pso->mutex);

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&base->mutex);

  return ret;
}

int
net_generic_socket_init1 (__sock pso)
{
  mutex_lock (&pso->mutex);
  char ip[128];
  uint16_t port = net_get_addrinfo_port (pso);
  net_get_addrinfo_ip_str (pso, (char*) ip, sizeof(ip));

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
  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

int
net_generic_socket_destroy0 (__sock pso)
{
  mutex_lock (&pso->mutex);
  char ip[128];
  uint16_t port = pso->ipr.port;
  snprintf (ip, sizeof(ip), "%hhu.%hhu.%hhu.%hhu", pso->ipr.ip[0],
	    pso->ipr.ip[1], pso->ipr.ip[2], pso->ipr.ip[3]);

  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      if (pso->flags & F_OPSOCK_IN)
	{
	  print_str ("INFO: [%d] client disconnected %s:%hu\n", pso->sock, ip,
		     port);

	}
      else
	{
	  print_str ("INFO: [%d] disconnected from host %s:%hu\n", pso->sock,
		     ip, port);
	}
      break;
    case SOCKET_OPMODE_LISTENER:
      ;
      print_str ("INFO: [%d]: closed listener %s:%hu\n", pso->sock, ip, port);
      break;
    }
  pthread_mutex_unlock (&pso->mutex);
  return 0;
}

int
net_join_threads (pmda base)
{
  int c = 0;

  mda pt_list =
    { 0 };

  mutex_lock (&base->mutex);

  md_init_le (&pt_list, (int) base->offset + 1);

  p_md_obj ptr = base->first;

  while (ptr)
    {
      __sock socket = (__sock) ptr->ptr;
      mutex_lock (&socket->mutex);
      pthread_t pt = socket->thread;
      pthread_mutex_unlock (&socket->mutex);

      if ( (pthread_t) 0 == pt)
	{
	  print_str ( "ERROR: net_join_threads: uninitialized thread detected : %d\n", socket->sock);

	  goto loop_end;
	}

      pthread_t *pthread;

      if ( NULL
	  == (pthread = md_alloc_le (&pt_list, sizeof(pthread_t), 0, NULL)))
	{
	  print_str ( "ERROR: net_join_threads: out of memory\n");
	  abort ();
	}

      *pthread = pt;

      loop_end:;

      ptr = ptr->next;
    }

  off_t count = base->offset;

  pthread_mutex_unlock (&base->mutex);

  ptr = pt_list.first;

  while (ptr)
    {
      pthread_t *pt = (pthread_t *) ptr->ptr;

      int r;

      if (!(r = pthread_join (*pt, NULL)))
	{
	  c++;
	}
      else
	{
	  print_str ("ERROR: net_join_threads: pthread_join failed [%d]\n", r);
	}

      ptr = ptr->next;
    }

  md_free (&pt_list);

  return (int) count - c;
}
