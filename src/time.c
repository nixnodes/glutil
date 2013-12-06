/*
 * time.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <t_glob.h>
#include <timeh.h>

#include <time.h>

struct tm *
get_localtime(void)
{
  time_t t = time(NULL);
  return localtime(&t);
}
