/*
 * lref_glob.c
 *
 *  Created on: Aug 27, 2014
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_glob.h"

#include <l_error.h>
#include <mc_glob.h>

int64_t glob_curtime = 0;

void *
g_get_glob_ptr(__g_handle hdl, char *field, int *output)
{
  g_setjmp(0, "g_get_glob_ptr", NULL, NULL);

  int l_off = 7;

  int idx = (int) strtol(&field[l_off], NULL, 10);

  if (!(idx >= 0 && idx <= MAX_GLOB_STOR_AR_COUNT))
    {
      return NULL;
    }

  if (!strncmp(field, _MC_GLOB_U64G, 7))
    {
      *output = 8;
      memset((void*) &glob_ui64_stor[idx], 0x0, sizeof(uint64_t));
      return (void*) &glob_ui64_stor[idx];
    }
  else if (!strncmp(field, _MC_GLOB_S64G, 7))
    {
      *output = -8;
      memset((void*) &glob_si64_stor[idx], 0x0, sizeof(int64_t));
      return (void*) &glob_si64_stor[idx];
    }
  else if (!strncmp(field, _MC_GLOB_F32G, 7))
    {
      *output = -32;
      memset((void*) &glob_float_stor[idx], 0x0, sizeof(float));
      return (void*) &glob_float_stor[idx];
    }
  else if (!strncmp(field, _MC_GLOB_CURTIME, 7))
    {
      *output = -33;
      memset((void*) &glob_curtime, 0x0, sizeof(int64_t));
      return (void*) &glob_curtime;
    }

  return NULL;
}
