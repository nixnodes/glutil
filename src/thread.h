/*
 * thread.h
 *
 *  Created on: Sep 14, 2012
 *      Author: reboot
 */

#ifdef _G_SSYS_THREAD

#ifndef THREAD_H_
#define THREAD_H_ 1

#include <stdint.h>
#include <inttypes.h>

#include <memory_t.h>

#include <pthread.h>

#define F_THRD_TERM                     ((uint32_t)1 << 1)
#define F_THRD_NOWPID                   ((uint32_t)1 << 2)
#define F_THRD_NOREG                    ((uint32_t)1 << 3)
#define F_THRD_DETACH                   ((uint32_t)1 << 4)
#define F_THRD_NOINIT                   ((uint32_t)1 << 5)
#define F_THRD_REGINIT                  ((uint32_t)1 << 6)
#define F_THRD_CINIT                    ((uint32_t)1 << 7)
#define F_THRD_MISC00                   ((uint32_t)1 << 10)

#define THREAD_MAX	                50
#define	MAX_OBJ_GEN_WORKER_ARG	        256

#define THREAD_DEFAULT_BUFFER0_SIZE     8192

#define F_THRD_STATUS_SUSPENDED         ((uint32_t)1 << 1)
#define F_THRD_STATUS_INITIALIZED       ((uint32_t)1 << 2)

typedef struct gen_worker_arg
{
  int count;
} gwa, *p_gwa;

typedef struct object_gen_worker
{
  void *call;
  int a_count;
  void *arg;
} o_gw, *p_ogw;

#define F_TTIME_ACT_T0                  ((uint8_t)1 << 1)
#define F_TTIME_ACT_T1                  ((uint8_t)1 << 2)

typedef struct __thread_timers
{
  char act_f;
  time_t t0, t1, t2;
} _thread_tm;

typedef struct object_thrd
{
  pthread_t pt, caller;
  int id;
  int sig;
  uint32_t flags, status;
  mda in_objects, proc_objects;
  pmda pin_objects;
  uint16_t role, oper_mode;
  void *buffer0;
  size_t buffer0_size;
  _thread_tm timers;
  pmda host_ctx;
  pthread_mutex_t mutex;
} o_thrd, *po_thrd;

#define 	F_THC_SKIP_IO		(uint32_t ) 1 << 1
#define 	F_THC_USER_DATA		(uint32_t ) 1 << 2

int
thread_create (void *call, int id, pmda _thrd_r, uint16_t role,
	       uint16_t oper_mode, uint32_t flags, uint32_t i_flags,
	       po_thrd data, po_thrd *ret, pthread_t *pt_ret);
int
mutex_init (pthread_mutex_t *mutex, int flags, int robust);

int
thread_destroy (p_md_obj ptr);

mda _net_thrd_r;
mda _thrd_r_common;

p_md_obj
search_thrd_id (pmda thread_r, pthread_t *pt);
int
thread_broadcast_kill (pmda thread_r);
void
thread_send_kill (po_thrd pthread);
int
thread_join_threads (pmda thread_r);
int
thread_broadcast_sig (pmda thread_r, int sig);

int
spawn_threads (int num, void *call, int id, pmda thread_register, uint16_t role,
	       uint16_t oper_mode, uint32_t flags);

void
mutex_lock (pthread_mutex_t *mutex);
int
mutex_trylock (pthread_mutex_t *mutex);

typedef float
dt_score_pt (pmda in, pmda out, void *arg1, void *arg2);
typedef float
(*dt_score_ptp) (pmda in, pmda out, void *arg1, void *arg2);
void
ts_flag_32 (pthread_mutex_t *mutex, uint32_t flags, uint32_t *target);
void
ts_unflag_32 (pthread_mutex_t *mutex, uint32_t flags, uint32_t *target);

int
push_object_to_thread (void *object, pmda threadr, dt_score_ptp scalc,
		       pthread_t *st);

#endif /* THREAD_H_ */

#endif
