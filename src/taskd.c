/*
 * taskd.c
 *
 *  Created on: Oct 25, 2015
 *      Author: reboot
 */

#include "memory_t.h"
#include "thread.h"
#include "net_io.h"
#include "misc.h"

#include "taskd.h"

#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <openssl/err.h>
#include <pthread.h>

mda tasks_in =
  { 0 };
pthread_t task_worker_pt = 0;

void *
task_worker (void *args)
{
  int r, s;

  task_worker_pt = pthread_self ();

  po_thrd thrd = (po_thrd) args;

  char buffer0[1024];

  mutex_lock (&thrd->mutex);

  uint32_t thread_flags = thrd->flags;
  sigset_t set;

  sigfillset (&set);

  s = pthread_sigmask (SIG_SETMASK, &set, NULL);

  if (s != 0)
    {
      print_str ("ERROR: task_worker: pthread_sigmask (SIG_BLOCK) failed: %d\n",
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
	  "ERROR: task_worker: pthread_sigmask (SIG_UNBLOCK) failed: %d\n", s);
      abort ();
    }

  thrd->timers.t1 = time (NULL);
  thrd->timers.t0 = time (NULL);
  thrd->timers.act_f |= (F_TTIME_ACT_T0 | F_TTIME_ACT_T1);

  pthread_t _pt = thrd->pt;

  pid_t _tid = (pid_t) syscall (SYS_gettid);

  thrd->status |= F_THRD_STATUS_INITIALIZED;

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
      print_str (
	  "WARNING: task_worker: [%d]: sigwait (SIGINT) failed with %d\n", _tid,
	  re);
    }

  mutex_lock (&thrd->mutex);

  init_done: ;

  print_str ("DEBUG: task_worker: [%d]: thread online [%d]\n", _tid,
	     thrd->oper_mode);

  pthread_t parent = thrd->caller;

  pthread_mutex_unlock (&thrd->mutex);

  //pthread_kill(parent, SIGINT);

  mda tasks_out =
    { 0 };
  md_init_le (&tasks_out, 40000);

  for (;;)
    {
      mutex_lock (&thrd->mutex);

      if (thrd->flags & F_THRD_TERM)
	{
	  pthread_mutex_unlock (&thrd->mutex);
	  break;
	}

      pthread_mutex_unlock (&thrd->mutex);

      unsigned int run_int = -1;
      p_md_obj ptr;

      mutex_lock (&tasks_in.mutex);

      ptr = tasks_in.first;

      while (ptr)
	{
	  if (tasks_out.offset == tasks_out.count)
	    {
	      break;
	    }

	  __task task = md_alloc_le (&tasks_out, sizeof(_task), 0, NULL);

	  if ( NULL == task)
	    {
	      print_str ("ERROR: task_worker: [%d]: out of memory\n", _tid);
	      abort ();
	    }

	  *task = *((__task ) ptr->ptr);

	  p_md_obj c_ptr = ptr;
	  ptr = ptr->next;
	  md_unlink_le (&tasks_in, c_ptr);
	}

      pthread_mutex_unlock (&tasks_in.mutex);

      ptr = tasks_out.first;

      while (ptr)
	{
	  __task task = (__task ) ptr->ptr;

	  if (0 == task->task_proc (task))
	    {
	      print_str ("DEBUG: task_worker: [%d]: processed task\n", _tid);
	      p_md_obj c_ptr = ptr;
	      ptr = ptr->next;
	      md_unlink_le (&tasks_out, c_ptr);
	    }
	  else
	    {
	      run_int = 100000;
	      ptr = ptr->next;
	    }
	}

      usleep (run_int);
    }

  shutdown: ;

  print_str ("DEBUG: task_worker: [%d]: thread shutting down..\n", _tid);

  if (thread_flags & F_THRD_MISC00)
    {

    }

  ERR_remove_state (0);

  kill (getpid (), SIGUSR2);

  free (thrd);

  return NULL;
}

int
register_task (_task_proc proc, void *data, uint16_t flags)
{
  int ret;

  mutex_lock (&tasks_in.mutex);

  if (tasks_in.offset == tasks_in.count)
    {
      pthread_mutex_unlock (&tasks_in.mutex);
      return 2;
    }

  __task task = md_alloc_le (&tasks_in, sizeof(_task), 0, NULL);

  if ( NULL == task)
    {
      pthread_mutex_unlock (&tasks_in.mutex);
      return 1;
    }

  task->data = data;
  task->task_proc = proc;

  pthread_mutex_unlock (&tasks_in.mutex);

  if ((pthread_t) 0 != task_worker_pt)
    {
      pthread_kill (task_worker_pt, SIGUSR1);
    }

  return 0;
}

