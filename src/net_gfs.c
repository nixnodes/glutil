/*
 * net_gfs.c
 *
 *  Created on: Mar 28, 2015
 *      Author: reboot
 */

#include "net_gfs.h"

#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <openssl/err.h>

#include "misc.h"
#include "net_fs.h"
#include "memory.h"

mda fs_jobs =
  { NULL };

#define FS_JOB_ADD_CLEANUP() { md_unlink_le (jobs, jobs->pos); };

__gfs
fs_job_add (pmda jobs)
{

  mutex_lock (&jobs->mutex);

  __gfs job = md_alloc_le (jobs, sizeof(_gfs), 0, NULL);

  if ( NULL == job)
    {
      return NULL;
    }

  if (mutex_init (&job->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST))
    {
      FS_JOB_ADD_CLEANUP()
      return NULL;
    }

  //job->status |= FS_GFS_JOB_LOPEN;

  pthread_mutex_unlock (&jobs->mutex);

  return job;

}

int
fs_link_socks_to_job (__gfs job, pmda psockr)
{
  mutex_lock (&job->mutex);

  md_init_le (&job->socks, 1024);

  int ret = 0;

  mutex_lock (&job->socks.mutex);

  job->socks.flags |= F_MDA_REFPTR;

  mutex_lock (&psockr->mutex);

  p_md_obj ptr = psockr->first;

  while (ptr)
    {
      __sock pso = (__sock ) ptr->ptr;

      mutex_lock (&pso->mutex);

      if (!(pso->flags & F_OPSOCK_CONNECT))
	{
	  print_str (
	      "D6: fs_link_socks_to_job: [%d]->[%d]: skipping socket (!= F_OPSOCK_CONNECT)\n",
	      pso->sock, job->id);
	  goto q_link;
	}

      __sock_ca pca = (__sock_ca ) pso->sock_ca;

      if (pca->ref_id == job->id)
	{
	  void *_pso = md_alloc_le (&job->socks, 0, 0, (void*) pso);

	  if ( NULL == _pso)
	    {
	      print_str (
		  "ERROR: fs_link_socks_to_job: [%d]->[%d]: could not allocate reference table (too many connections)\n",
		  pso->sock, job->id);
	      abort ();
	    }

	  pso->va_p2 = (void*) job;
	  job->status |= FS_GFS_JOB_LOPEN;

	  print_str ("D4: fs_link_socks_to_job: [%d] linked to job [%d]\n",
		     pso->sock, job->id);
	  ret++;
	}

      q_link: ;

      pthread_mutex_unlock (&pso->mutex);

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&psockr->mutex);

  pthread_mutex_unlock (&job->socks.mutex);

  pthread_mutex_unlock (&job->mutex);

  return ret;
}

#define SOCKOP_CLEAN(md, ptr, pso) { \
  \
  if (pso->flags & (F_OPSOCK_TERM | F_OPSOCK_ORPHANED)) { \
      print_str ("ERROR: SOCKOP_CLEAN: [%d]: dead socket\n",pso->flags & (F_OPSOCK_TERM | F_OPSOCK_ORPHANED)); \
      pthread_mutex_unlock(&pso->mutex); mutex_lock(&md.mutex);  free(ptr->ptr); ptr->ptr = NULL; md_unlink_le(&md, ptr); pthread_mutex_unlock(&md.mutex); \
  } else { \
       \
  } \
};

static int
fs_send_stat_req (__gfs job, p_md_obj ptr)
{
  __sock pso = (__sock) ptr->ptr;

  if ( mutex_trylock(&pso->mutex) == EBUSY )
    {

      return 2;
    }

  if (pso->flags & (F_OPSOCK_TERM | F_OPSOCK_ORPHANED))
    {
      SOCKOP_CLEAN(job->socks, ptr, pso)
      print_str ("WARNING: fs_send_stat_req: dead socket\n");
      return 1;
    }

  if (!(pso->flags & F_OPSOCK_ACT) || NULL == pso->va_p1)
    {
      print_str (
	  "WARNING: fs_send_stat_req: [%d]: not active yet\n",
	  pso->sock);
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  __fs_rh_enc packet = net_fs_compile_filereq (CODE_FS_RQH_STAT, job->link,
      NULL);

  if ( NULL == packet)
    {
      print_str (
	  "ERROR: fs_send_stat_req: [%d]: could not create stat request\n",
	  pso->sock);
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  __fs_sts psts = (__fs_sts ) pso->va_p1;
  snprintf(psts->data0, sizeof(psts->data0), "%s", job->link);

  //psts->notify_cb =

  int r;
  if ((r = net_push_to_sendq (pso, (void*) packet, packet->head.content_length,
	      0)))
    {
      print_str (
	  "ERROR: fs_send_stat_req: [%d]: net_push_to_sendq failed: [%d]\n",
	  pso->sock, r);
      free (packet);
      pthread_mutex_unlock(&pso->mutex);
      return 1;
    }

  free (packet);

  print_str (
      "D5: fs_send_stat_req: [%d]: sending remote stat request for '%s'\n",
      pso->sock, job->link);

  pthread_mutex_unlock(&pso->mutex);

  return 0;
}

int
fs_worker (void *args)
{
  po_thrd thrd = (po_thrd) args;

  mutex_lock (&thrd->mutex);

  sigset_t set;

  sigfillset (&set);

  int r;

  r = pthread_sigmask (SIG_BLOCK, &set, NULL);

  if (r != 0)
    {
      print_str ("ERROR: fs_worker: pthread_sigmask (SIG_BLOCK) failed: %d\n",
		 r);
      abort ();
    }

  sigemptyset (&set);
  sigaddset (&set, SIGURG);
  sigaddset (&set, SIGIO);
  sigaddset (&set, SIGUSR1);

  r = pthread_sigmask (SIG_UNBLOCK, &set, NULL);

  if (r != 0)
    {
      print_str ("ERROR: fs_worker: pthread_sigmask (SIG_UNBLOCK) failed: %d\n",
		 r);
      abort ();
    }

  pid_t _tid = (pid_t) syscall (SYS_gettid);

  pmda jobs = &fs_jobs;

  thrd->status |= F_THRD_STATUS_INITIALIZED;

  pthread_mutex_unlock (&thrd->mutex);

  unsigned int sleeptime = -1;

  print_str ("DEBUG: fs_worker: [%d]: thread coming online [%d]\n", _tid,
	     thrd->oper_mode);

  for (;;)
    {
      if (thrd->flags & F_THRD_TERM)
	{
	  break;
	}

      sleeptime = -1;

      mutex_lock (&jobs->mutex);

      p_md_obj ptr = jobs->first;

      while (ptr)
	{
	  __gfs job = ptr->ptr;

	  mutex_lock (&job->mutex);

	  if (!(job->status & FS_GFS_JOB_LOPEN))
	    {
	      goto q_job;
	    }

	  if (!(job->status & FS_GFS_JOB_STATREQUESTED))
	    {
	      job->thread = thrd->pt;

	      mutex_lock (&job->socks.mutex);

	      if ((r = fs_send_stat_req (job, job->socks.first)))
		{
		  if (r == 2)
		    {
		      sleeptime = 1;
		    }
		  pthread_mutex_unlock (&job->socks.mutex);
		  goto q_job;
		}
	      else
		{
		  job->status |= FS_GFS_JOB_STATREQUESTED;
		}
	      pthread_mutex_unlock (&job->socks.mutex);

	    }

	  if (!(job->status & FS_GFS_JOB_STAT))
	    {
	      goto q_job;
	    }

	  q_job: ;

	  pthread_mutex_unlock (&job->mutex);

	  ptr = ptr->next;
	}

      pthread_mutex_unlock (&jobs->mutex);

      sleep (sleeptime);

      print_str ("D6: [%d]: thread waking up [%hu]\n", _tid, thrd->oper_mode);
    }

  print_str ("DEBUG: fs_worker: [%d]: thread shutting down..\n", _tid);

  pmda thread_host_ctx = thrd->host_ctx;

  mutex_lock (&thread_host_ctx->mutex);

  mutex_lock (&thrd->mutex);

  if (thrd->buffer0)
    {
      free (thrd->buffer0);
    }

  pthread_t pt = thrd->pt;

  pthread_mutex_unlock (&thrd->mutex);

  p_md_obj ptr_thread = search_thrd_id (thread_host_ctx, &pt);

  if (NULL == ptr_thread)
    {
      print_str (
	  "ERROR: fs_worker: [%d]: thread already unregistered on exit (search_thrd_id failed)\n",
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

  ERR_remove_state (0);

  kill (getpid (), SIGUSR2);

  return 0;

}

int
net_fs_socket_init1 (__sock pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;
      __sock_ca ca = (__sock_ca) pso->sock_ca;

      if ( ca->ca_flags & F_CA_MISC02)
	{

	  print_str("DEBUG: net_fs_socket_init1: [%d]: initializing fs..\n", pso->sock);

	  net_fs_initialize_sts(pso);

	  __gfs job = (__gfs ) pso->va_p2;

	  if (NULL != job)
	    {
	      mutex_lock(&job->mutex);

	      if ( job->thread )
		{
		  print_str (
		      "D3: net_fs_socket_init1: [%d]: sending fs_thread SIGUSR1..\n",
		      pso->sock);
		  pthread_kill (job->thread, SIGUSR1);

		}

	      pthread_mutex_unlock(&job->mutex);
	    }

	}

      break;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_fs_socket_destroy_gfs (__sock pso)
{
  mutex_lock (&pso->mutex);

  __gfs job = (__gfs ) pso->va_p2;

  if (NULL != job)
    {
      mutex_lock (&job->mutex);

      if (job->thread)
	{
	  print_str (
	      "D3: net_fs_socket_destroy_gfs: [%d]: sending fs_thread SIGUSR1..\n",
	      pso->sock);
	  pthread_kill (job->thread, SIGUSR1);

	}

      pthread_mutex_unlock (&job->mutex);
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}
