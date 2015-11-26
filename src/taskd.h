/*
 * taskd.h
 *
 *  Created on: Oct 25, 2015
 *      Author: reboot
 */

#ifndef SRC_TASKD_H_
#define SRC_TASKD_H_

#define		MAX_TASKS_GLOBAL	40000

#include "memory_t.h"

#include <stdlib.h>
#include <stdint.h>

typedef struct ___task
{
  int
  (*task_proc) (struct ___task *task);
  void *data;
  uint16_t flags;
} _task, *__task;

typedef int
(*_task_proc) (struct ___task *task);

mda tasks_in;
pthread_t task_worker_pt;

int
register_task (_task_proc proc, void *data, uint16_t flags);
void *
task_worker (void *args);

#endif /* SRC_TASKD_H_ */
