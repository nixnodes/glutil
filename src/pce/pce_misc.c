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
#include <errno.h>

#define PCE_PSTR_MAX        16384

static void
w_log_p (char *w)
{

  /*if (ow && !(get_msg_type(ow) & log_lvl))
   {
   return 1;
   }*/

  size_t wc, wll;

  wll = strlen (w);

  if ((wc = fwrite (w, 1, wll, fd_log_pce)) != wll)
    {
      char e_buffer[1024];
      printf ("ERROR: %s: writing log failed [%d/%d] %s\n", LOGFILE, (int) wc,
	      (int) wll, g_strerr_r (errno, e_buffer, 1024));
      return;
    }

  fflush (fd_log_pce);

}

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

  if (NULL != fd_log_pce)
    {
      struct tm tm = *get_localtime ();
      snprintf (d_buffer_2, PCE_PSTR_MAX,
		"[%.2u/%.2u/%.2u %.2u:%.2u:%.2u] [%d] %s", tm.tm_mday,
		tm.tm_mon + 1, (tm.tm_year + 1900) % 100, tm.tm_hour, tm.tm_min,
		tm.tm_sec, getpid (), buf);

      char wl_buffer[PCE_PSTR_MAX];
      vsnprintf (wl_buffer, PCE_PSTR_MAX, d_buffer_2, al);
      w_log_p (wl_buffer);

    }

  va_end(al);

  return 0;
}

