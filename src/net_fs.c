/*
 * net_fs.c
 *
 *  Created on: Mar 25, 2015
 *      Author: reboot
 */

#include "net_fs.h"

#include <net_io.h>
#include <string.h>
#include <g_crypto.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static void
net_fs_initialize_sts(__sock_o pso)
{
  if ( NULL == pso->va_p1)
    {
      pso->va_p1 = calloc(1, sizeof(_fs_sts));
    }
}

int
net_fs_socket_init1_req_xfer(__sock_o pso)
{
  mutex_lock(&pso->mutex);
  switch (pso->oper_mode)
    {
  case SOCKET_OPMODE_RECIEVER:
    ;
    __sock_ca ca = (__sock_ca) pso->sock_ca;

    if ( ca->ca_flags & F_CA_MISC00)
      {
        if ( gfl & F_OPT_VERBOSE3)
          {
            print_str("DEBUG: net_fs_socket_init1_req_xfer: [%d]: sending xfer request\n", pso->sock);
          }

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

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

static int
net_baseline_fsproto_gstat(char *file, __fs_hstat data)
{
  struct stat st;

  if (lstat(file, &st) == -1)
    {
      char err_buf[1024];
      print_str(
          "ERROR: net_baseline_fsproto_gstat: [%s] lstat failed: [%d] [%s]\n",
          file, errno, strerror_r(errno, err_buf, 1024));
      return 1;
    }

  data->m_tim = (int32_t) st.st_mtim.tv_sec;
  data->size = (uint64_t) st.st_size;

  return 0;
}

int
net_baseline_fsproto_xfer_stat_ok(__sock_o pso, __fs_rh_enc pkt, void *arg)
{
  __fs_sts psts = (__fs_sts ) pso->va_p1;
  __fs_hstat phst = (__fs_hstat ) arg;

  if (!(psts->state & F_FS_STSOCK_XFER_R))
    {
      return 1;
    }

  if (psts->stage != FS_STSS_XFER_R_WSTAT)
    {
      return 1;
    }

  __fs_rh_enc packet = net_fs_compile_filereq(CODE_FS_RQH_SEND,
       psts->data0,
      NULL);

  if (NULL == packet)
    {
      print_str(
          "ERROR: net_baseline_fsproto_proc_notify: [%d]: net_fs_compile_filereq failed\n",
          pso->sock);
      net_send_sock_term_sig(pso);
      return 1;
    }

  packet->body.size = phst->size;

  if (psts->hstat.file_offset >= phst->size)
    {
      print_str(
          "ERROR: net_baseline_fsproto_xfer_stat_ok: [%d] invalid seek position requested: %llu / %llu\n",
          pso->sock, psts->hstat.file_offset, phst->size);
      net_send_sock_term_sig(pso);
      free(packet);
      return 1;
    }

  if (psts->hstat.file_offset > 0)
    {
      packet->body.offset = psts->hstat.file_offset;
    }
  else
    {
      packet->body.offset = 0;
    }

  if (psts->hstat.file_size > 0)
    {
      packet->body.size = psts->hstat.file_size;
    }
  else
    {
      packet->body.size = phst->size;
      psts->hstat.file_size = phst->size;
    }

  //packet->body.offset = 1;

  print_str("DEBUG: [%d]: sending file request [%llu] [%llu] -[%llu] \n",
      pso->sock, packet->body.offset, packet->body.size, psts->hstat.file_size);

  if (!net_send_direct(pso, (const void*) packet,
      (size_t) packet->head.content_length))
    {
      psts->stage = FS_STSS_XFER_R_WDATA;
      psts->notify_cb = net_baseline_fsproto_xfer_in_ok;
    }
  else
    {
      psts->notify_cb = net_baseline_fsproto_default;
    }

  free(packet);

  return 0;
}

int
net_baseline_fsproto_xfer_in_ok(__sock_o pso, __fs_rh_enc packet, void *arg)
{
  __fs_sts psts = (__fs_sts ) pso->va_p1;

  if (!((psts->state & F_FS_STSOCK_XFER_R)
      && (psts->stage == FS_STSS_XFER_R_WDATA)))
    {
      return 1;
    }

  if (packet->body.status_flags & F_RQH_OP_OK)
    {
      print_str("DEBUG: [%d]: data stream confirmed inbound\n", pso->sock);
      //psts->notify_cb = net_baseline_fsproto_xfer_in_ok;

      pso->rcv1 = (_p_s_cb) net_baseline_fsproto_recv;
      pso->unit_size = 131072;
      free(pso->buffer0);
      pso->buffer0 = calloc(1, pso->unit_size);

      if (!SHA1_Init(&pso->sha_00.context))
        {
          print_str(
              "ERROR: net_baseline_fsproto_xfer_in_ok: [%d]: SHA1_Init failed\n",
              pso->sock);
          return 1;
        }

      if (psts->hstat.file_size < (uint64_t) pso->unit_size)
        {
          pso->unit_size = (ssize_t) psts->hstat.file_size;
        }
      pso->counters.b_read = 0;

      psts->notify_cb = net_baseline_fsproto_default;

      return -3;
    }
  else
    {
      /*if ( packet->body.status_flags & F_RQH_OP_FAILED)
       {

       }*/
      char *message = (char*) ((void*) packet + sizeof(_fs_rh_enc));

      char err_buf[1024];
      print_str(
          "ERROR: [%d]: remote host denied the transfer: [%d] [%s]: [%s]\n",
          pso->sock, packet->body.err_code,
          packet->body.err_code ?
              strerror_r(packet->body.err_code, err_buf, sizeof(err_buf)) : "",
          message);
    }

  return 0;
}

int
net_baseline_fsproto_default(__sock_o pso, __fs_rh_enc packet, void *arg)
{
  char *message = (char*) ((void*) packet + sizeof(_fs_rh_enc));
  print_str("NOTICE: [%d]: notify: '%s'\n", pso->sock, message);

  return 0;
}

static int
net_baseline_fsproto_proc_notify(__sock_o pso, __fs_rh_enc packet)
{
  net_fs_initialize_sts(pso);

  __fs_sts psts = (__fs_sts ) pso->va_p1;

  switch (packet->body.m00_8)
    {
  case CODE_FS_RQH_STAT:
    ;
    if (packet->body.status_flags & F_RQH_OP_FAILED)
      {
        char err_buf[1024];
        print_str("WARNING: [%d]: remote 'stat' failed: [%d] [%s]\n", pso->sock,
            packet->body.err_code,
            strerror_r(packet->body.err_code, err_buf, sizeof(err_buf)));
        pso->flags |= F_OPSOCK_TERM;
      }
    else
      {
        __fs_hstat phst = (__fs_hstat) ((void*) packet + sizeof(_fs_rh_enc));

        print_str("DEBUG: [%d]: remote 'stat' suceeded: [%llu] [%d]\n",
        pso->sock, phst->size, (int)phst->m_tim);

        if ( NULL != psts->notify_cb )
          {
            return psts->notify_cb(pso, packet, (void*)phst);
          }

        //pso->flags |= F_OPSOCK_TERM;

        //psts->state |= F_FS_STSOCK_FASSOC;

      }

    break;
    default:;

    if ( NULL != psts->notify_cb)
      {
        return psts->notify_cb(pso, packet, NULL);
      }

    break;
  }

  return 0;

}

static __fs_rh_enc
net_baseline_fsproto_proc_stat(__sock_o pso, void *data)
{
  __fs_rh_enc packet;
  char *st_path = (char*) (data + sizeof(_fs_rh_enc));

  //size_t st_path_len = strlen(st_path);

  _fs_hstat hst =
    { 0 };

  int ret = net_baseline_fsproto_gstat(st_path, &hst);

  int32_t r_errno = (int32_t) errno;

  packet = net_fs_compile_hstat(&hst, NULL);

  if (ret)
    {
      packet->body.status_flags |= F_RQH_OP_FAILED;
      packet->body.err_code = r_errno;
    }
  else
    {
      net_fs_initialize_sts(pso);
      __fs_sts psts = (__fs_sts ) pso->va_p1;
      psts->hstat = hst;

      snprintf(psts->data1, sizeof(psts->data1), "%s", st_path);
      psts->state |= F_FS_STSOCK_FASSOC;
    }

  return packet;
}

static int
net_fs_clean_handles(__sock_o pso)
{
  switch (pso->oper_mode)
    {
  case SOCKET_OPMODE_RECIEVER:
    ;
    __fs_sts psts = (__fs_sts ) pso->va_p1;

    if ( NULL == psts)
      {
        break;
      }

    if (close(psts->handle) == -1)
      {
        char err_buf[1024];
        print_str(
            "ERROR: net_fs_clean_handles: [%d]: could not close file descriptor [%d] [%s]\n",
            pso->sock, errno, strerror_r(errno, err_buf, sizeof(err_buf)));

        return 1;
      }
    else
      {
        print_str("D4: net_fs_clean_handles: [%d]: closed file descriptor\n",
            pso->sock);
      }

    break;
    }

  return 0;
}

static int
net_baseline_fsproto_xfer_conf(__sock_o pso, __fs_rh_enc packet, void *arg)
{
  char *message = (char*) ((void*) packet + sizeof(_fs_rh_enc));

  if (packet->body.status_flags & F_RQH_OP_OK)
    {
      print_str(
          "NOTICE: net_baseline_fsproto_xfer_conf: [%d]: peer reports transfer ok: '%s'\n",
          pso->sock, message);
      pso->flags = F_OPSOCK_TERM;
    }
  else
    {
      print_str(
          "ERROR: net_baseline_fsproto_xfer_conf: [%d]: peer reports transfer failed: '%s'\n",
          pso->sock, message);
    }

  return 0;
}

static int
net_baseline_fsproto_proc_sdata(__sock_o pso, void *data)
{
  __fs_rh_enc packet, input = (__fs_rh_enc ) data;

  char *st_path;

  //size_t st_path_len = strlen(st_path);

  net_fs_initialize_sts(pso);
  __fs_sts psts = (__fs_sts ) pso->va_p1;

  packet = net_fs_compile_filereq(CODE_FS_RESP_NOTIFY,
       BASELINE_FS_TCODE_XFER,
      NULL);

  if ( NULL == packet)
    {
      return 1;
    }

  int ret = 0;

  if (psts->state & F_FS_STSOCK_FASSOC)
    {
      st_path = psts->data1;
    }
  else
    {
      st_path = (char*) (data + sizeof(_fs_rh_enc));
      if (net_baseline_fsproto_gstat(st_path, &psts->hstat))
        {
          packet->body.status_flags |= F_RQH_OP_FAILED;
          packet->body.err_code = errno;
          ret = 2;
          goto end;
        }
    }

  psts->hstat.file_offset = input->body.offset;
  psts->hstat.file_size = input->body.size;

  if (psts->hstat.file_size + psts->hstat.file_offset > psts->hstat.size)
    {
      print_str("ERROR: [%d]: out of range file request: %s | [%llu] [%llu]\n",
          pso->sock, st_path, psts->hstat.file_offset, psts->hstat.file_size);
      packet->body.status_flags |= F_RQH_OP_FAILED;
      packet->body.err_code = ESPIPE;
      ret = 2;
      //pso->flags = F_OPSOCK_TERM;
      goto end;
    }

  if (psts->hstat.file_size == 0)
    {
      psts->hstat.file_size = psts->hstat.size - psts->hstat.file_offset;
    }

  psts->handle = open(st_path, O_RDONLY);
  if (psts->handle == -1)
    {
      print_str(
          "ERROR: net_baseline_fsproto_proc_sdata: [%d]: unable to open '%s' for reading\n",
          pso->sock, st_path);
      packet->body.status_flags |= F_RQH_OP_FAILED;
      packet->body.err_code = errno;
      //pso->flags = F_OPSOCK_TERM;
      ret = 2;
      goto end;
    }

  net_push_rc(&pso->shutdown_rc0, (_t_stocb) net_fs_clean_handles, 0);

  if (psts->hstat.file_offset > 0)
    {
      if (lseek(psts->handle, psts->hstat.file_offset, SEEK_SET) == -1)
        {
          print_str(
              "ERROR: net_baseline_fsproto_proc_sdata: [%d]: seek @[%llu] failed\n",
              pso->sock, psts->hstat.file_offset);
          packet->body.status_flags |= F_RQH_OP_FAILED;
          packet->body.err_code = errno;
          //pso->flags = F_OPSOCK_TERM;
          ret = 2;
          goto end;
        }
    }

  packet->body.status_flags |= F_RQH_OP_OK;

  pso->rcv1 = (_p_s_cb) net_baseline_fsproto_send;
  pso->flags |= F_OPSOCK_HALT_RECV;
  pso->unit_size = 131072;
  free(pso->buffer0);
  pso->buffer0 = calloc(1, pso->unit_size);

  if (!SHA1_Init(&pso->sha_00.context))
    {
      print_str(
          "ERROR: net_baseline_fsproto_proc_sdata: [%d]: SHA1_Init failed\n",
          pso->sock);
      ret = 2;
      goto end;
    }

  /*if (psts->hstat.file_size < (uint64_t) pso->unit_size)
   {
   pso->unit_size = (ssize_t) psts->hstat.file_size;

   }*/

  pso->counters.b_read = 0;

  psts->notify_cb = net_baseline_fsproto_xfer_conf;

  print_str(
      "DEBUG: net_baseline_fsproto_proc_sdata: [%d]: ack request for '%s' , [%llu] [%llu]\n",
      pso->sock, st_path, psts->hstat.file_offset, psts->hstat.file_size);

  end: ;

  net_send_direct(pso, (const void*) packet,
      (size_t) packet->head.content_length);

  free(packet);

  return ret;
}

int
net_baseline_fsproto(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  if (pso->counters.b_read < pso->unit_size)
    {
      pthread_mutex_unlock(&pso->mutex);
      return -2;
    }

  __fs_rh_enc input = (__fs_rh_enc ) data, packet = NULL;

  switch (input->body.code)
    {
  case CODE_FS_RQH_STAT:
    ;
    packet = net_baseline_fsproto_proc_stat(pso, data);
    break;
  case CODE_FS_RESP_NOTIFY:
    ;
    if (net_baseline_fsproto_proc_notify(pso, data) == -3)
      {
        goto fin_2;
      }
    else
      {

        goto fin;
      }
  case CODE_FS_RQH_SEND:
    ;
    if (net_baseline_fsproto_proc_sdata(pso, data))
      {
        goto fin;
      }
    else
      {

        goto fin_2;
      }
  default:
    print_str("ERROR: net_baseline_fsproto: unknown header code\n");
    goto fin;
    }

  if (NULL != packet)
    {
      net_send_direct(pso, (const void*) packet,
          (size_t) packet->head.content_length);

      free(packet);
    }

  fin: ;

  net_proto_reset_to_baseline(pso);

  fin_2: ;

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

int
net_baseline_fsproto_send(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  //unsigned char block[131072];

  size_t in_read, blk_sz;

  __fs_sts psts = (__fs_sts ) pso->va_p1;

  if (psts->hstat.file_size < (uint64_t) pso->unit_size)
    {
      blk_sz = (size_t) psts->hstat.file_size;
    }
  else if ((uint64_t) psts->hstat.file_size - psts->data_out
      < (uint64_t) pso->unit_size)
    {
      blk_sz = (size_t) (psts->hstat.file_size - psts->data_out);
    }
  else
    {
      blk_sz = (size_t) pso->unit_size;
    }

  in_read = read(psts->handle, pso->buffer0, blk_sz);

  if ((in_read) > 0)
    {
      SHA1_Update(&pso->sha_00.context, pso->buffer0, blk_sz);

      psts->data_out += (uint64_t) in_read;
      if (net_send_direct(pso, pso->buffer0, in_read))
        {
          pthread_mutex_unlock(&pso->mutex);
          return 1;
        }
      print_str("D4: [%d]: send: %llu/%llu     \r", pso->sock, psts->data_out,
          psts->hstat.size);
      pso->timers.last_act = time(NULL);
    }
  else if (in_read == -1)
    {
      char err_buf[1024];
      print_str(
          "ERROR: net_baseline_fsproto_send: [%d]: [%d]: read failed: [%d] [%s]\n",
          pso->sock, psts->handle, errno,
          strerror_r(errno, err_buf, sizeof(err_buf)));
    }
  else if (in_read < blk_sz)
    {
      print_str(
          "ERROR: net_baseline_fsproto_send: [%d]: [%d]: partial read occured\n",
          pso->sock, psts->handle);
      if (pso->flags & F_OPSOCK_HALT_RECV)
        {
          pso->flags ^= F_OPSOCK_HALT_RECV;
        }
      pso->flags |= F_OPSOCK_TERM;
      net_proto_reset_to_baseline(pso);
      goto finish;
    }

  if (psts->data_out == psts->hstat.file_size)
    {

      if (pso->flags & F_OPSOCK_HALT_RECV)
        {
          pso->flags ^= F_OPSOCK_HALT_RECV;
        }
      net_proto_reset_to_baseline(pso);

      if (!SHA1_Final((unsigned char*) pso->sha_00.value.data,
          &pso->sha_00.context))
        {
          print_str(
              "ERROR: net_baseline_fsproto_send: [%d]: [%d]: SHA1_Final failed\n",
              pso->sock, psts->handle);
        }
      char buffer[128];
      print_str("DEBUG: net_baseline_fsproto_send: [%d]: all data sent [%s]\n",
          pso->sock, crypto_sha1_to_ascii(&pso->sha_00.value, buffer));

    }
  else if (psts->data_out > psts->hstat.file_size)
    {
      print_str("ERROR: net_baseline_fsproto_send: [%d]: too much data sent\n",
          pso->sock);
      abort();
    }

  finish: ;

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

static int
net_baseline_fsproto_recv_validate(__sock_o pso, uint8_t status_flags,
    char *msg)
{
  __fs_rh_enc packet;

  packet = net_fs_compile_filereq(CODE_FS_RESP_NOTIFY, msg,
  NULL);

  if ( NULL == packet)
    {
      return 1;
    }

  packet->body.status_flags |= status_flags;

  net_send_direct(pso, (const void*) packet,
      (size_t) packet->head.content_length);

  return 0;
}

int
net_baseline_fsproto_recv(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  if (pso->counters.b_read < pso->unit_size)
    {
      pthread_mutex_unlock(&pso->mutex);
      return -2;
    }

  //size_t in_read, blk_sz;

  __fs_sts psts = (__fs_sts ) pso->va_p1;

  psts->data_in += (uint64_t) pso->counters.b_read;

  SHA1_Update(&pso->sha_00.context, pso->buffer0,
      (size_t) pso->counters.b_read);

  print_str("D4: [%d]: recv: %llu/%llu bytes\r", pso->sock, psts->data_in,
      psts->hstat.file_size);

  if (psts->data_in == psts->hstat.file_size)
    {
      if (!SHA1_Final((unsigned char*) pso->sha_00.value.data,
          &pso->sha_00.context))
        {
          print_str(
              "ERROR: net_baseline_fsproto_recv: [%d]: [%d]: SHA1_Final failed\n",
              pso->sock, psts->handle);
          pthread_mutex_unlock(&pso->mutex);
          return 1;
        }
      char buffer[128];

      print_str("D4: net_baseline_fsproto_recv: recieved %llu bytes [%s]\n",
          psts->data_in, crypto_sha1_to_ascii(&pso->sha_00.value, buffer));
      //pso->flags |= F_OPSOCK_TERM;
      if (net_baseline_fsproto_recv_validate(pso, F_RQH_OP_OK,
           "XFER OK"))
        {
          print_str(
              "ERROR: net_baseline_fsproto_recv: [%d] net_baseline_fsproto_recv_validate failed\n",
              pso->sock);
        }

    }
  else if (psts->data_in > psts->hstat.file_size)
    {
      print_str(
          "ERROR: net_baseline_fsproto_recv: got too much data: %llu bytes\n",
          psts->data_in);
      net_baseline_fsproto_recv_validate(pso, F_RQH_OP_OK,
           "XFER FAILED");
      net_proto_reset_to_baseline(pso);
    }
  else if (((uint64_t) psts->hstat.file_size - psts->data_in)
      < (uint64_t) pso->unit_size)
    {
      pso->unit_size = (uint64_t) psts->hstat.file_size - psts->data_in;
      print_str("D4: net_baseline_fsproto_recv: scaling unit size to %lld\n",
          pso->unit_size);

    }

  pso->counters.b_read = 0;

  /* }
   else if (pso->counters.b_read > pso->unit_size)
   {
   print_str(
   "ERROR: net_baseline_fsproto_recv: got too much data: %d bytes\n",
   pso->counters.b_read);
   net_proto_reset_to_baseline(pso);
   pso->flags |= F_OPSOCK_TERM;
   }*/

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

__fs_rh_enc
net_fs_compile_breq(int code, unsigned char *data, size_t p_len, void *arg)
{

  size_t req_len = sizeof(_fs_rh_enc) + p_len + 1;
  __fs_rh_enc request = calloc(1, req_len + 16);

  request->head.prot_code = PROT_CODE_FS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = code;

  request->body.ex_len = p_len;

  strncpy((char*) (((void*) request) + sizeof(_fs_rh_enc)), (char*) data,
      p_len);

  return request;
}

__fs_rh_enc
net_fs_compile_filereq(int code, char *data, void *arg)
{

  size_t p_len = strlen((char*) data);

  if (0 == p_len)
    {
      print_str("ERROR: net_fs_compile_request: missing path\n");
      return NULL;
    }

  if (p_len > PATH_MAX - 1)
    {
      print_str("ERROR: net_fs_compile_request: path too long\n");
      return NULL;
    }

  size_t req_len = sizeof(_fs_rh_enc) + p_len + 1;
  __fs_rh_enc request = calloc(1, req_len + 16);

  request->head.prot_code = PROT_CODE_FS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = code;

  request->body.ex_len = p_len;

  strncpy((char*) (((void*) request) + sizeof(_fs_rh_enc)), (char*) data,
      p_len);

  /*switch (code)
   {
   case CODE_FS_RQH_STATS:
   ;

   break;
   }*/

  return request;
}

__fs_rh_enc
net_fs_compile_hstat(__fs_hstat data, void *arg)
{

  size_t req_len = sizeof(_fs_rh_enc) + sizeof(_fs_hstat);
  __fs_rh_enc request = calloc(1, req_len + 16);

  request->head.prot_code = PROT_CODE_FS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = CODE_FS_RESP_NOTIFY;
  request->body.ex_len = sizeof(_fs_hstat);
  request->body.m00_8 = CODE_FS_RQH_STAT;

  __fs_hstat ptr = (__fs_hstat ) (((void*) request) + sizeof(_fs_rh_enc));

  *ptr = *data;

  return request;
}

#include <unistd.h>
#include <sys/syscall.h>

int
net_fs_socket_destroy_rc0(__sock_o pso)
{
  mutex_lock(&pso->mutex);

  if ( NULL != pso->va_p1)
    {
      free(pso->va_p1);
      pso->va_p1 = NULL;
    }

  int _tid = syscall(SYS_gettid);
  print_str("INFO: [%d] socket closed: [%d]\n", (int) _tid, pso->sock);

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}
