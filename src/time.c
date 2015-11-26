/*
 * time.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include "timeh.h"

#include <time.h>

struct tm *
get_localtime (struct tm * result)
{
  time_t t = time (NULL);

  return localtime_r (&t, result);
}
