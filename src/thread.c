#include <t_glob.h>
#include <thread.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

mda _net_thrd_r =
  { 0 };

int
mutex_init(pthread_mutex_t *mutex, int flags, int robust)
{
  pthread_mutexattr_t mattr;

  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr, flags);
  pthread_mutexattr_setrobust(&mattr, robust);

  return pthread_mutex_init(mutex, &mattr);
}

int
thread_create(void *call, int id, pmda thrd_r, uint16_t role,
    uint16_t oper_mode)
{

  mutex_lock(&thrd_r->mutex);

  int r;

  po_thrd object;

  if (!(object = md_alloc_le(thrd_r, sizeof(o_thrd), 0, NULL)))
    {
      r = -11;
      goto end;
    }

  if ((r = pthread_create(&object->pt, NULL, call, (void *) object)))
    {
      goto end;
    }

  object->id = id;
  object->role = role;
  object->oper_mode = oper_mode;
  md_init_le(&object->in_objects, 512);
  md_init_le(&object->proc_objects, 4096);
  object->in_objects.flags |= F_MDA_REFPTR;
  object->proc_objects.flags |= F_MDA_REFPTR;

  r = mutex_init(&object->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);

  end:

  pthread_mutex_unlock(&thrd_r->mutex);

  return r;
}

int
thread_destroy(p_md_obj ptr)
{
  int r;

  po_thrd pthrd = (po_thrd) ptr->ptr;

  mutex_lock(&pthrd->mutex);

  if ((r = pthread_cancel(pthrd->pt)) != 0)
    {
      return r;
    }

  void *res;

  if ((r = pthread_join(pthrd->pt, &res)) != 0)
    {
      return r;
    }

  if (res != PTHREAD_CANCELED)
    {
      return 1010;
    }

  pthread_mutex_unlock(&pthrd->mutex);

  if (md_unlink_le(&_net_thrd_r, ptr))
    {
      return 1012;
    }

  return 0;
}

int
thread_broadcast_kill(pmda thread_r)
{
  int c = 0;

  mutex_lock(&thread_r->mutex);

  p_md_obj ptr = thread_r->first;

  while (ptr)
    {
      po_thrd pthrd = (po_thrd) ptr->ptr;
      mutex_lock(&pthrd->mutex);
      pthrd->flags |= F_THRD_TERM;
      pthread_mutex_unlock(&pthrd->mutex);
      c++;
      ptr = ptr->next;
    }

  pthread_mutex_unlock(&thread_r->mutex);

  return c;
}

off_t
thread_register_count(pmda thread_r)
{
  mutex_lock(&thread_r->mutex);
  off_t ret = thread_r->offset;
  pthread_mutex_unlock(&thread_r->mutex);
  return ret;
}

p_md_obj
search_thrd_id(pmda thread_r, pthread_t *pt)
{
  mutex_lock(&thread_r->mutex);
  p_md_obj ptr = _net_thrd_r.first;

  while (ptr)
    {
      po_thrd pthrd = (po_thrd) ptr->ptr;
      mutex_lock(&pthrd->mutex);
      if (pthrd->pt == *pt)
        {
          pthread_mutex_unlock(&pthrd->mutex);
          pthread_mutex_unlock(&thread_r->mutex);
          return ptr;
        }
      pthread_mutex_unlock(&pthrd->mutex);
      ptr = ptr->next;
    }
  pthread_mutex_unlock(&thread_r->mutex);
  return NULL;
}

int
spawn_threads(int num, void *call, int id, pmda thread_register, uint16_t role,
    uint16_t oper_mode)
{
  int i, r = 0;
  for (i = num; i; i--)
    {
      if ((r = thread_create(call, id, thread_register, role, oper_mode)))
        {
          break;
        }
    }
  return r;
}

int
push_object_to_thread(void *object, pmda threadr, dt_score_ptp scalc)
{
  mutex_lock(&threadr->mutex);

  p_md_obj thrd_ptr = threadr->first;
  po_thrd sel_thread = NULL;
  float lowest_score = 10000000.0;

  while (thrd_ptr)
    {
      po_thrd thread = (po_thrd) thrd_ptr->ptr;

      mutex_lock(&thread->in_objects.mutex);
      mutex_lock(&thread->proc_objects.mutex);

      if (thread->in_objects.offset == thread->in_objects.count)
        {
          goto end_l;
        }

      float score = scalc(&thread->in_objects, &thread->proc_objects, object,
          (void*) thread);

      if (0.0 == score)
        {
          pthread_mutex_unlock(&thread->in_objects.mutex);
          pthread_mutex_unlock(&thread->proc_objects.mutex);
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

      pthread_mutex_unlock(&thread->in_objects.mutex);
      pthread_mutex_unlock(&thread->proc_objects.mutex);

      thrd_ptr = thrd_ptr->next;
    }

  int r;

  if (!sel_thread)
    {
      pthread_mutex_unlock(&threadr->mutex);
      return 1;
    }

  mutex_lock(&sel_thread->in_objects.mutex);

  if (!md_alloc_le(&sel_thread->in_objects, 0, 0, (void*) object))
    {
      r = 2;
    }
  else
    {
      r = 0;
    }

  pthread_mutex_unlock(&sel_thread->in_objects.mutex);

  pthread_mutex_unlock(&threadr->mutex);

  return r;
}

void
mutex_lock(pthread_mutex_t *mutex)
{
  int r;
  switch (r = pthread_mutex_lock(mutex))
    {
  case 0:
    return;
  case EOWNERDEAD:
    fprintf(stderr,
        "ERROR: %d: calling pthread_mutex_consistent [EOWNERDEAD]\n", getpid());
    if (0 == pthread_mutex_consistent(mutex))
      {
        mutex_lock(mutex);
      }
    else
      {
        fprintf(stderr, "ERROR: %d: pthread_mutex_consistent failed\n",
            getpid());
        abort();
      }
    return;
  case EAGAIN:
    usleep(10000);
    mutex_lock(mutex);
    return;
  case ENOTRECOVERABLE:
    fprintf(stderr, "ERROR: %d: pthread_mutex_lock: [ENOTRECOVERABLE]\n",
        getpid());
    abort();
    return;
  default:
    ;
    char err_b[1024];
    fprintf(stderr, "ERROR: %d: pthread_mutex_lock: [%d] [%s]\n", getpid(), r,
        strerror_r(r, (char*) err_b, sizeof(err_b)));
    abort();
    }
}

