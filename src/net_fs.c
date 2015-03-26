/*
 * net_fs.c
 *
 *  Created on: Mar 25, 2015
 *      Author: reboot
 */

#include "net_fs.h"

#include <net_io.h>

#include <errno.h>
#include <sys/stat.h>
#include <string.h>

static void
net_fs_initialize_sts(__sock_o pso)
{
  if ( NULL == pso->va_p1)
    {
      pso->va_p1 = calloc(1, sizeof(_fs_sts));
    }
}

int
net_fs_socket_init1_rqstat(__sock_o pso)
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
            print_str("DEBUG: net_fs_socket_init1_rqstat: [%d]: sending stat request\n", pso->sock);
          }

        net_fs_initialize_sts(pso);
        __fs_sts psts = (__fs_sts ) pso->va_p1;

        snprintf(psts->data0, sizeof(psts->data0), "%s", ca->b3);

        psts->state |= F_FS_STSOCK_FASSOC|F_FS_STSOCK_XFER_R;
        psts->stage = FS_STSS_XFER_R_WSTAT;

        __fs_rh_enc packet = net_fs_compile_filereq(CODE_FS_RQH_STAT, psts->data0, NULL);

        if ( NULL == packet)
          {
            print_str("ERROR: net_fs_socket_init1_rqstat: [%d]: could not create stat request\n", pso->sock);
            pso->flags |= F_OPSOCK_TERM;
            break;
          }

        int r;
        if ( (r=net_push_to_sendq(pso, (void*) packet, packet->head.content_length, 0)) )
          {
            print_str("ERROR: net_fs_socket_init1_rqstat: [%d]: net_push_to_sendq failed: [%d]\n", pso->sock, r);
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

        print_str("NOTICE: [%d]: remote 'stat' suceeded: [%llub] [%d]\n",
        pso->sock, phst->size, (int)phst->m_tim);

        //pso->flags |= F_OPSOCK_TERM;

        if (( psts->state & F_FS_STSOCK_XFER_R))
          {
            if (psts->stage == FS_STSS_XFER_R_WSTAT)
              {
                __fs_rh_enc packet = net_fs_compile_filereq(CODE_FS_RQH_SEND, psts->data0, NULL);

                if (NULL == packet)
                  {
                    print_str("ERROR: net_baseline_fsproto_proc_notify: [%d]: net_fs_compile_filereq failed\n",
                    pso->sock);
                    net_send_sock_term_sig(pso);
                  }
                else
                  {

                    psts->stage = FS_STSS_XFER_R_WDATA;

                    packet->body.size = phst->size;
                    //packet->body.offset = 1;

                    print_str("DEBUG: [%d]: sending file request [%llu]\n",pso->sock, phst->size);

                    net_send_direct(pso, (const void*) packet,
                    (size_t) packet->head.content_length);

                    free(packet);
                  }
              }
          }

        psts->state |= F_FS_STSOCK_FASSOC|F_FS_STSOCK_XFER_R;

      }

    break;
    default:;

    if (( psts->state & F_FS_STSOCK_XFER_R) && (psts->stage == FS_STSS_XFER_R_WDATA))
      {
        if ( packet->body.status_flags & F_RQH_OP_OK)
          {
            print_str("DEBUG: [%d]: data stream confirmed inbound\n", pso->sock);
          }
        else
          {
            /*if ( packet->body.status_flags & F_RQH_OP_FAILED)
             {

             }*/
            char err_buf[1024];
            print_str("ERROR: [%d]: remote host denied the transfer: [%d] [%s]\n", pso->sock,
            packet->body.err_code, packet->body.err_code ?
            strerror_r(packet->body.err_code, err_buf, sizeof(err_buf)) : "");
          }
      }
    else
      {
        char *message = (char*) ((void*)packet + sizeof(_fs_rh_enc));
        print_str("NOTICE: [%d]: notify: '%s'\n", pso->sock, message);
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
net_baseline_fsproto_proc_sdata(__sock_o pso, void *data)
{
  __fs_rh_enc packet, input = (__fs_rh_enc ) data;

  char *st_path;

  //size_t st_path_len = strlen(st_path);

  net_fs_initialize_sts(pso);
  __fs_sts psts = (__fs_sts ) pso->va_p1;

  packet = net_fs_compile_filereq(CODE_FS_RESP_NOTIFY, "test", NULL);

  if ( NULL == packet)
    {
      return 1;
    }

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
          goto end;
        }
    }

  psts->file_offset = input->body.offset;
  psts->file_size = input->body.size;

  packet->body.status_flags |= F_RQH_OP_OK;

  print_str(
      "DEBUG: net_baseline_fsproto_proc_sdata: [%d]: request recieved for '%s' , [%llu] [%llu]\n",
      pso->sock, st_path, psts->file_offset, psts->file_size);

  end: ;

  net_send_direct(pso, (const void*) packet,
      (size_t) packet->head.content_length);

  free(packet);

  return 0;
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
    net_baseline_fsproto_proc_notify(pso, input);
    goto fin;
  case CODE_FS_RQH_SEND:
    ;
    net_baseline_fsproto_proc_sdata(pso, data);
    goto fin;
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

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

__fs_rh_enc
net_fs_compile_filereq(int code, char *path, void *arg)
{
  size_t p_len = strlen(path);

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
  __fs_rh_enc request = calloc(1, req_len);

  request->head.prot_code = PROT_CODE_FS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = code;
  request->body.ex_len = p_len;

  strncpy((char*) (((void*) request) + sizeof(_fs_rh_enc)), path, p_len);

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
  __fs_rh_enc request = calloc(1, req_len);

  request->head.prot_code = PROT_CODE_FS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = CODE_FS_RESP_NOTIFY;
  request->body.ex_len = sizeof(_fs_hstat);
  request->body.m00_8 = CODE_FS_RQH_STAT;

  __fs_hstat ptr = (__fs_hstat ) (((void*) request) + sizeof(_fs_rh_enc));

  *ptr = *data;

  return request;
}
