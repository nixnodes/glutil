/*
 * main.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */



#include <glutil.h>
#include <l_error.h>
#include <cfgv.h>
#include <g_modes.h>
#include <macros_t.h>
#include <arg_proc.h>
#include <misc.h>
#include <signal_t.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>


int
main(int argc, char *argv[])
{
  char **p_argv = (char**) argv;
  int r;

  g_setjmp(0, "main", NULL, NULL);
  if ((r = setup_sighandlers()))
    {
      print_str(
          "WARNING: UNABLE TO SETUP SIGNAL HANDLERS! (this is weird, please report it!) [%d]\n",
          r);
      sleep(5);
    }

  _p_macro_argc = argc;

  if ((r = parse_args(argc, argv, prio_f_ref)) > 0)
    {
      print_str(MSG_INIT_CMDLINE_ERROR, r);
      EXITVAL = 2;
      g_shutdown(NULL);
    }

  enable_logging();

  switch (updmode)
    {
  case PRIO_UPD_MODE_MACRO:
    ;
    uint64_t gfl_s = (gfl & (F_OPT_WBUFFER | F_OPT_PS_LOGGING | F_OPT_NOGLCONF));
    char **ptr;
    ptr = process_macro(prio_argv_off, NULL);
    if (ptr)
      {
        _p_macro_argv = p_argv = ptr;
        gfl = gfl_s;
      }
    else
      {
        g_shutdown(NULL);
      }
    break;
  case PRIO_UPD_MODE_INFO:
    g_print_info();
    g_shutdown(NULL);
    break;
    }

  updmode = 0;

  g_init(_p_macro_argc, p_argv);

  g_shutdown(NULL);

  return EXITVAL;
}
