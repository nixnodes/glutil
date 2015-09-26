/*
 * pce_misc.c
 *
 *  Created on: Dec 11, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "pce_misc.h"

#include <misc.h>
#include <timeh.h>

#include <stdio.h>
#include <time.h>

#define PCE_PSTR_MAX        16384

int
pce_log (const char * volatile buf, ...)
{
  char d_buffer_2[PCE_PSTR_MAX];

  if (!(get_msg_type ((char*) buf) & STDLOG_LVL))
    {
      return 0;
    }

  va_list al;
  va_start(al, buf);

  if (NULL != fd_log)
    {
      struct tm tm = *get_localtime ();
      snprintf (d_buffer_2, PCE_PSTR_MAX,
		"[%.2u/%.2u/%.2u %.2u:%.2u:%.2u] [%d] %s", tm.tm_mday,
		tm.tm_mon + 1, (tm.tm_year + 1900) % 100, tm.tm_hour, tm.tm_min,
		tm.tm_sec, getpid (), buf);

      char wl_buffer[PCE_PSTR_MAX];
      vsnprintf (wl_buffer, PCE_PSTR_MAX, d_buffer_2, al);
      p_log_write (wl_buffer, (char*) buf);

    }

  va_end(al);

  return 0;
}
