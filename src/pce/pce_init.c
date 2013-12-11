/*
 * pce_init.c
 *
 *  Created on: Dec 8, 2013
 *      Author: reboot
 */

#include "pce_config.h"
#include <glutil.h>
#include "pce_proc.h"
#include "pce_misc.h"


#include <misc.h>

#include <stdio.h>

uint32_t pce_f = 0;
char *s_year = NULL;
int EXITVAL = 0;

int
main(int argc, char *argv[])
{
  print_str = pce_log;
  LOGLVL = LOG_LEVEL;
  setup_sighandlers();
  gfl |= F_OPT_PS_LOGGING;
  pce_enable_logging();
  //gfl |= F_OPT_PS_SILENT;
  return pce_proc(argv[1]);
}

void
pce_enable_logging(void)
{
  if ((gfl & F_OPT_PS_LOGGING) && !fd_log)
    {
      if (!(fd_log = fopen(pce_logfile, "a")))
        {
          gfl ^= F_OPT_PS_LOGGING;
        }
    }
  return;
}
