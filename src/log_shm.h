/*
 * log_shm.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LOG_SHM_H_
#define LOG_SHM_H_

#define MSG_DEF_SHM     "SHARED MEMORY"

#include <glutil.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include <im_hdr.h>
#include <log_io.h>

#define R_SHMAP_ALREADY_EXISTS  (a32 << 1)
#define R_SHMAP_FAILED_ATTACH   (a32 << 2)
#define R_SHMAP_FAILED_SHMAT    (a32 << 3)

int
g_shmap_data (__g_handle, key_t);
int
g_map_shm (__g_handle, key_t);
void *
shmap (key_t ipc, struct shmid_ds *ipcret, size_t size, uint32_t *ret,
       int *shmid, int cflags, int shmflg);
int
g_shm_cleanup (__g_handle hdl);

#endif /* LOG_SHM_H_ */
