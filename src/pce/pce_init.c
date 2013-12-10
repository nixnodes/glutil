/*
 * pce_init.c
 *
 *  Created on: Dec 8, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "pce_proc.h"

#include <stdio.h>

uint32_t pce_f = 0;
char *s_year = NULL;
int EXITVAL = 0;

int
main(int argc, char *argv[])
{
  setup_sighandlers();
  //gfl |= F_OPT_PS_SILENT;
  return pce_proc(argv[1]);
}
