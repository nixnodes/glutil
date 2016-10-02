/*
 * net_gfs.h
 *
 *  Created on: Mar 28, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_GFS_H_
#define SRC_NET_GFS_H_

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "net_io.h"

#define THREAD_ROLE_FS_WORKER            0x2

#define FS_GFS_JOB_DBLOCK_READY          ((uint32_t)1 << 1)
#define FS_GFS_JOB_DBLOCK_DONE           ((uint32_t)1 << 2)

typedef struct ___gfs_job_dblock
{
  ssize_t offset;
  ssize_t size;
  uint32_t status;
  __sock pso;
  pthread_mutex_t mutex;
} _gfs_jdb, *__gfs_jdb;

#define FS_GFS_JOB_LOPEN           ((uint32_t)1 << 1)
#define FS_GFS_JOB_LFAILED         ((uint32_t)1 << 2)
#define FS_GFS_JOB_STAT            ((uint32_t)1 << 3)
#define FS_GFS_JOB_STATREQUESTED   ((uint32_t)1 << 4)

#include "memory_t.h"

typedef struct ___gfs_job
{
  int id;
  char *link;
  char vb_0[PATH_MAX];
  FILE *dump;
  mda blocks;
  mda socks;
  uint32_t status;
  pthread_t thread;
  pthread_mutex_t mutex;
} _gfs, *__gfs;

mda fs_jobs;

int
fs_worker (void *args);
int
net_fs_socket_destroy_gfs (__sock pso);
int
net_fs_socket_init1 (__sock pso);
__gfs
fs_job_add (pmda jobs);
int
fs_link_socks_to_job (__gfs job, pmda psockr);

#endif /* SRC_NET_GFS_H_ */
