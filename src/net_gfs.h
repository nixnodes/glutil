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

#define FS_GFS_JOB_DBLOCK_READY          ((uint32_t)1 << 1)
#define FS_GFS_JOB_DBLOCK_DONE           ((uint32_t)1 << 2)

typedef struct ___gfs_job_dblock
{
  uint64_t size, offset;
  uint32_t status;
} _gfs_jdb, *__gfs_jdb;

#define FS_GFS_JOB_POOL_LOPEN           ((uint32_t)1 << 1)
#define FS_GFS_JOB_POOL_LFAILED         ((uint32_t)1 << 2)

#include "memory_t.h"

typedef struct ___gfs_job
{
  char *link_remote;
  FILE *local_fh;
  mda blocks;
  uint32_t status;
} _gfs, *__gfs;

#endif /* SRC_NET_GFS_H_ */
