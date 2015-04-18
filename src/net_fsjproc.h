/*
 * net_fsjproc.h
 *
 *  Created on: Apr 10, 2015
 *      Author: reboot
 */

#ifndef SRC_NET_FSJPROC_H_
#define SRC_NET_FSJPROC_H_

#include <stdint.h>
#include <pthread.h>

#define F_FS_JENT_ST_DONE       ((uint32_t)1 << 1)

typedef struct ___fs_job_entity
{
  pthread_mutex_t mutex;
  char *path;
  uint64_t offset, blk_size;
  uint32_t status;
} _fs_jent, *__fs_jent;

#endif /* SRC_NET_FSJPROC_H_ */
