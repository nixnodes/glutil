#include "errno_int.h"

#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <signal.h>

mda _net_thrd_r =
  { 0 };
mda _thrd_r_common =
  { 0 };

int
mutex_init (pthread_mutex_t *mutex, int flags, int robust)
{
  pthread_mutexattr_t mattr;

  pthread_mutexattr_init (&mattr);
  pthread_mutexattr_settype (&mattr, flags);
  pthread_mutexattr_setrobust (&mattr, robust);

  return pthread_mutex_init (mutex, &mattr);
}

int
thread_create (void *call, int id, pmda thrd_r, uint16_t role,
	       uint16_t oper_mode, uint32_t flags, uint32_t i_flags,
	       po_thrd data, po_thrd *ret, pthread_t *pt_ret)
{

  if ( NULL != thrd_r)
    {
      mutex_lock (&thrd_r->mutex);
    }

  int r;

  po_thrd object;

  if (i_flags & F_THC_USER_DATA)
    {
      flags = data->flags;
    }

  uint32_t ex_flags = 0;

  if (!(flags & F_THRD_NOREG))
    {
      if (!(object = md_alloc_le (thrd_r, sizeof(o_thrd), 0, NULL)))
	{
	  r = -11;
	  goto end;
	}
      ex_flags |= F_THRD_REGINIT;
    }
  else
    {
      object = calloc (1, sizeof(o_thrd));
      ex_flags |= F_THRD_CINIT;
    }

  if (!(i_flags & F_THC_USER_DATA))
    {
      object->id = id;
      object->role = role;
      object->oper_mode = oper_mode;
      object->host_ctx = thrd_r;
      object->flags |= flags;
      if (!(i_flags & F_THC_SKIP_IO))
	{
	  md_init_le (&object->in_objects, 512);
	  md_init_le (&object->proc_objects, 4096);
	  object->in_objects.flags |= F_MDA_REFPTR;
	  object->proc_objects.flags |= F_MDA_REFPTR;
	}

      r = mutex_init (&object->mutex, PTHREAD_MUTEX_RECURSIVE,
		      PTHREAD_MUTEX_ROBUST);
    }
  else
    {
      *object = *data;
    }

  object->caller = pthread_self ();
  object->flags |= ex_flags;

  mutex_lock (&object->mutex);

  if ((r = pthread_create (&object->pt, NULL, call, (void *) object)))
    {
      pthread_mutex_unlock (&object->mutex);
      goto end;
    }

  if ( NULL != pt_ret)
    {
      *pt_ret = object->pt;
    }

  if (flags & F_THRD_DETACH)
    {
      pthread_detach (object->pt);
    }

  if ( NULL != ret)
    {
      *ret = object;
    }

  pthread_mutex_unlock (&object->mutex);

  end:

  if ( NULL != thrd_r)
    {
      pthread_mutex_unlock (&thrd_r->mutex);
    }

  return r;
}

int
thread_destroy (p_md_obj ptr)
{
  int r;

  po_thrd pthrd = (po_thrd) ptr->ptr;

  mutex_lock (&pthrd->mutex);

  if ((r = pthread_cancel (pthrd->pt)) != 0)
    {
      return r;
    }

  void *res;

  if ((r = pthread_join (pthrd->pt, &res)) != 0)
    {
      return r;
    }

  if (res != PTHREAD_CANCELED)
    {
      return 1010;
    }

  pthread_mutex_unlock (&pthrd->mutex);

  if (md_unlink_le (&_net_thrd_r, ptr))
    {
      return 1012;
    }

  return 0;
}

void
thread_send_kill (po_thrd pthread)
{
  mutex_lock (&pthread->mutex);
  pthread->flags |= F_THRD_TERM;
  pthread_kill (pthread->pt, SIGINT);
  pthread_kill (pthread->pt, SIGUSR1);
  pthread_mutex_unlock (&pthread->mutex);
}

int
thread_broadcast_kill (pmda thread_r)
{
  int c = 0;

  mutex_lock (&thread_r->mutex);

  p_md_obj ptr = thread_r->first;

  while (ptr)
    {
      po_thrd pthrd = (po_thrd) ptr->ptr;
      mutex_lock (&pthrd->mutex);
      pthrd->flags |= F_THRD_TERM;
      if (pthrd->status & F_THRD_STATUS_INITIALIZED)
	{
	  pthread_kill (pthrd->pt, SIGINT);
	  pthread_kill (pthrd->pt, SIGUSR1);
	}
      pthread_mutex_unlock (&pthrd->mutex);
      c++;
      ptr = ptr->next;
    }

  pthread_mutex_unlock (&thread_r->mutex);

  return c;
}

int
thread_join_threads (pmda thread_r)
{
  int c = 0;

  mda pt_list =
    { 0 };

  mutex_lock (&thread_r->mutex);

  md_init_le (&pt_list, (int) thread_r->offset + 1);

  p_md_obj ptr = thread_r->first;

  while (ptr)
    {
      po_thrd pthrd = (po_thrd) ptr->ptr;
      mutex_lock (&pthrd->mutex);
      pthread_t pt = pthrd->pt;
      pthread_mutex_unlock (&pthrd->mutex);

      pthread_t *pthread;

      if ( NULL
	  == (pthread = md_alloc_le (&pt_list, sizeof(pthread_t), 0, NULL)))
	{
	  fprintf (stderr, "ERROR: thread_join_threads: out of memory\n");
	  abort ();
	}

      *pthread = pt;

      ptr = ptr->next;
    }

  off_t count = thread_r->offset;

  pthread_mutex_unlock (&thread_r->mutex);

  ptr = pt_list.first;

  while (ptr)
    {
      pthread_t *pt = (pthread_t *) ptr->ptr;

      if (!pthread_join (*pt, NULL))
	{
	  c++;
	}

      ptr = ptr->next;
    }

  md_free (&pt_list);

  return (int) count - c;
}

int
thread_broadcast_sig (pmda thread_r, int sig)
{
  int c = 0;

  mutex_lock (&thread_r->mutex);

  p_md_obj ptr = thread_r->first;

  while (ptr)
    {
      po_thrd pthrd = (po_thrd) ptr->ptr;
      mutex_lock (&pthrd->mutex);
      if (pthrd->status & F_THRD_STATUS_INITIALIZED)
	{
	  pthread_kill (pthrd->pt, sig);
	}
      pthread_mutex_unlock (&pthrd->mutex);
      c++;
      ptr = ptr->next;
    }

  pthread_mutex_unlock (&thread_r->mutex);

  return c;
}

p_md_obj
search_thrd_id (pmda thread_r, pthread_t *pt)
{
  mutex_lock (&thread_r->mutex);
  p_md_obj ptr = thread_r->first;

  while (ptr)
    {
      po_thrd pthrd = (po_thrd) ptr->ptr;
      mutex_lock (&pthrd->mutex);
      if (pthrd->pt == *pt)
	{
	  pthread_mutex_unlock (&pthrd->mutex);
	  pthread_mutex_unlock (&thread_r->mutex);
	  return ptr;
	}
      pthread_mutex_unlock (&pthrd->mutex);
      ptr = ptr->next;
    }
  pthread_mutex_unlock (&thread_r->mutex);
  return NULL;
}

int
spawn_threads (int num, void *call, int id, pmda thread_register, uint16_t role,
	       uint16_t oper_mode, uint32_t flags)
{
  int r;
  while (num--)
    {
      if ((r = thread_create (call, id, thread_register, role, oper_mode, flags,
			      0,
			      NULL,
			      NULL, NULL)))
	{
	  return r;
	}
    }
  return 0;
}

int
push_object_to_thread (void *object, pmda threadr, dt_score_ptp scalc,
		       pthread_t *st)
{
  mutex_lock (&threadr->mutex);

  p_md_obj thrd_ptr = threadr->first;
  po_thrd sel_thread = NULL;
  float lowest_score = 10000000.0;

  while (thrd_ptr)
    {
      po_thrd thread = (po_thrd) thrd_ptr->ptr;

      mutex_lock (&thread->in_objects.mutex);
      mutex_lock (&thread->proc_objects.mutex);

      if (thread->in_objects.offset == thread->in_objects.count)
	{
	  goto end_l;
	}

      float score = scalc (&thread->in_objects, &thread->proc_objects, object,
			   (void*) thread);

      if (0.0 == score)
	{
	  pthread_mutex_unlock (&thread->in_objects.mutex);
	  pthread_mutex_unlock (&thread->proc_objects.mutex);
	  sel_thread = thread;
	  break;
	}

      if (score == -1.0)
	{
	  goto end_l;
	}

      if (score < lowest_score)
	{
	  sel_thread = thread;
	  lowest_score = score;
	}

      end_l: ;

      pthread_mutex_unlock (&thread->in_objects.mutex);
      pthread_mutex_unlock (&thread->proc_objects.mutex);

      thrd_ptr = thrd_ptr->next;
    }

  int r;

  if (NULL == sel_thread)
    {
      pthread_mutex_unlock (&threadr->mutex);
      return 1;
    }

  mutex_lock (&sel_thread->in_objects.mutex);

  if (NULL == md_alloc_le (&sel_thread->in_objects, 0, 0, (void*) object))
    {
      r = 2;
    }
  else
    {
      r = 0;
      //if (sel_thread->status & F_THRD_STATUS_SUSPENDED)
      // {
      *st = sel_thread->pt;

      pthread_kill (sel_thread->pt, SIGUSR1);

      // }

    }

  pthread_mutex_unlock (&sel_thread->in_objects.mutex);

  pthread_mutex_unlock (&threadr->mutex);

  return r;
}

void
mutex_lock (pthread_mutex_t *mutex)
{
  int r;
  switch (r = pthread_mutex_lock (mutex))
    {
    case 0:
      return;
    case EOWNERDEAD:
      fprintf (stderr,
	       "WARNING: %ld: calling pthread_mutex_consistent [EOWNERDEAD]\n",
	       syscall (SYS_gettid));
      if (0 == pthread_mutex_consistent (mutex))
	{
	  mutex_lock (mutex);
	}
      else
	{
	  fprintf (stderr, "ERROR: %ld: pthread_mutex_consistent failed\n",
		   syscall (SYS_gettid));
	  abort ();
	}
      return;
    case EAGAIN:
      usleep (10000);
      mutex_lock (mutex);
      return;
    case ENOTRECOVERABLE:
      fprintf (stderr, "ERROR: %u: pthread_mutex_lock: [ENOTRECOVERABLE]\n",
	       (unsigned int) getpid ());
      abort ();
      return;
    default:
      ;
      char err_b[1024];
      fprintf (stderr, "ERROR: %ld: pthread_mutex_lock: [%d] [%s]\n",
	       syscall (SYS_gettid), r,
	       g_strerr_r (r, (char*) err_b, sizeof(err_b)));
      abort ();
    }
}

int
mutex_trylock (pthread_mutex_t *mutex)
{
  int r;
  switch (r = pthread_mutex_trylock (mutex))
    {
    case 0:
      return 0;
    case EBUSY:
      return EBUSY;
    case EOWNERDEAD:
      fprintf (stderr,
	       "ERROR: %d: calling pthread_mutex_consistent [EOWNERDEAD]\n",
	       getpid ());
      if (0 == pthread_mutex_consistent (mutex))
	{
	  return mutex_trylock (mutex);
	}
      else
	{
	  fprintf (stderr, "ERROR: %d: pthread_mutex_consistent failed\n",
		   getpid ());
	  abort ();
	}

    case EAGAIN:
      usleep (10000);
      return mutex_trylock (mutex);

    case ENOTRECOVERABLE:
      fprintf (stderr, "ERROR: %d: pthread_mutex_lock: [ENOTRECOVERABLE]\n",
	       getpid ());
      abort ();

    default:
      ;
      char err_b[1024];
      fprintf (stderr, "ERROR: %d: pthread_mutex_lock: [%d] [%s]\n", getpid (),
	       r, g_strerr_r (r, (char*) err_b, sizeof(err_b)));
      abort ();
    }
  return r;
}

void
ts_flag_32 (pthread_mutex_t *mutex, uint32_t flags, uint32_t *target)
{
  mutex_lock (mutex);
  *target |= flags;
  pthread_mutex_unlock (mutex);
}

void
ts_unflag_32 (pthread_mutex_t *mutex, uint32_t flags, uint32_t *target)
{
  mutex_lock (mutex);
  *target ^= flags;
  pthread_mutex_unlock (mutex);
}
