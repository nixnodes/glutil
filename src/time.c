/*
 * time.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "timeh.h"

#include <str.h>
#include <t_glob.h>

#include <time.h>

struct tm *
get_localtime (void)
{
  time_t t = time (NULL);
  return localtime (&t);
}
