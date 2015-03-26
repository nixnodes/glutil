/*
 * net_fs.c
 *
 *  Created on: Mar 25, 2015
 *      Author: reboot
 */

#include "net_fs.h"

#include <net_io.h>

int
net_baseline_fsproto(__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock(&pso->mutex);

  if (pso->counters.b_read < pso->unit_size)
    {
      pthread_mutex_unlock(&pso->mutex);
      return -2;
    }



  pthread_mutex_unlock(&pso->mutex);

  return 0;
}
