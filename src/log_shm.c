/*
 * log_shm.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "log_shm.h"
#include <im_hdr.h>
#include <l_sb.h>
#include <l_error.h>
#include <lref_online.h>
#include <omfp.h>

#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

int
g_shmap_data(__g_handle hdl, key_t ipc)
{
  g_setjmp(0, "g_shmap_data", NULL, NULL);
  if (hdl->shmid)
    {
      return 1001;
    }

  if ((hdl->shmid = shmget(ipc, 0, 0)) == -1)
    {
      return 1002;
    }

  if ((hdl->data = shmat(hdl->shmid, NULL, SHM_RDONLY)) == (void*) -1)
    {
      hdl->data = NULL;
      return 1003;
    }

  if (shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf) == -1)
    {
      return 1004;
    }

  if (!hdl->ipcbuf.shm_segsz)
    {
      return 1005;
    }

  hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;

  return 0;
}

void *
shmap(key_t ipc, struct shmid_ds *ipcret, size_t size, uint32_t *ret,
    int *shmid, int cflags, int shmflg)
{
  g_setjmp(0, "shmap", NULL, NULL);

  void *ptr;
  int ir = 0;

  int i_shmid = -1;

  if (!shmid)
    {
      shmid = &i_shmid;
    }

  if (*ret & R_SHMAP_ALREADY_EXISTS)
    {
      ir = 1;
      if (gfl0 & F_OPT_SHMRO)
        {
          shmflg |= SHM_RDONLY;
        }
    }
  else if ((*shmid = shmget(ipc, size,
  IPC_CREAT | IPC_EXCL | cflags)) == -1)
    {
      if ( errno == EEXIST)
        {

          if (ret)
            {
              *ret |= R_SHMAP_ALREADY_EXISTS;
            }
          if ((*shmid = shmget(ipc, 0, 0)) == -1)
            {
              if (ret)
                {
                  *ret |= R_SHMAP_FAILED_ATTACH;
                }
              return NULL;
            }
          if (gfl0 & F_OPT_SHMRO)
            {
              shmflg |= SHM_RDONLY;
            }
        }
      else
        {
          return NULL;
        }
    }

  if ((ptr = shmat(*shmid, NULL, shmflg)) == (void*) -1)
    {
      if (ret)
        {
          *ret |= R_SHMAP_FAILED_SHMAT;
        }
      return NULL;
    }

  if (!ipcret)
    {
      return ptr;
    }

  if (ir != 1)
    {
      if (shmctl(*shmid, IPC_STAT, ipcret) == -1)
        {
          return NULL;
        }
    }

  return ptr;
}

int
g_map_shm(__g_handle hdl, key_t ipc)
{
  hdl->flags |= F_GH_ONSHM;

  if (hdl->buffer.count)
    {
      return 0;
    }

  if (!SHM_IPC)
    {
      print_str(
          "ERROR: %s: could not get IPC key, set manually (--ipc <key>)\n",
          MSG_DEF_SHM);
      return 101;
    }
  int r;
  if ((r = load_data_md(&hdl->buffer, NULL, hdl)))
    {
      if (((gfl & F_OPT_VERBOSE) && r != 1002) || (gfl & F_OPT_VERBOSE4))
        {
          print_str(
              "ERROR: %s: [%u/%u] [%u] [%u] could not map shared memory segment! [%d] [%s]\n",
              MSG_DEF_SHM, (uint32_t) hdl->buffer.count,
              (uint32_t) (hdl->total_sz / hdl->block_sz),
              (uint32_t) hdl->total_sz, hdl->block_sz, r,
              strerror_r(errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
        }
      return r;
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("NOTICE: %s: mapped %u records\n",
      MSG_DEF_SHM, (uint32_t) hdl->buffer.count);
    }

#ifndef _MAKE_SBIN
  pdt_set_online(hdl);
#endif

  return 0;
}

int
g_shm_cleanup(__g_handle hdl)
{
  int r = 0;

  if (shmdt(hdl->data) == -1)
    {
      r++;
    }

  if ((hdl->flags & F_GH_SHM) && (hdl->flags & F_GH_SHMDESTONEXIT))
    {
      if (shmctl(hdl->shmid, IPC_RMID, NULL) == -1)
        {
          r++;
        }
    }

  hdl->data = NULL;
  hdl->shmid = 0;

  return r;
}
