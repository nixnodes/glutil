/*
 * lref_dupefile.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <mc_glob.h>

#include <lref.h>
#include <lref_gen.h>
#include "lref_dupefile.h"

#include "glconf.h"

#include <stdio.h>
#include <time.h>

#include <errno.h>

int
dupefile_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct dupefile *data = (struct dupefile *) iarg;

  time_t t_t = (time_t) data->timeup;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str("DUPEFILE: %s - uploader: %s, time: %s\n", data->filename,
      data->uploader, buffer2);

}

int
dupefile_format_block_batch(void *iarg, char *output)
{
  struct dupefile *data = (struct dupefile *) iarg;
  return printf("DUPEFILE\x9%s\x9%s\x9%d\n", data->filename, data->uploader,
      (int32_t) data->timeup);

}

int
dupefile_format_block_exp(void *iarg, char *output)
{
  struct dupefile *data = (struct dupefile *) iarg;
  return printf("file %s\n"
      "user %s\n"
      "time %d\n\n", data->filename, data->uploader, (int32_t) data->timeup);

}

char *
dt_rval_dupefile_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct dupefile *) arg)->timeup);
  return output;
}

char *
dt_rval_dupefile_file(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct dupefile *) arg)->filename;
}

char *
dt_rval_dupefile_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct dupefile *) arg)->uploader;
}

void *
ref_to_val_lk_dupefile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_dupefile_time, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_GLOB_FILE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_dupefile_file, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_dupefile_user, (__d_drt_h ) mppd,
          "%s");
    }

  return NULL;
}

int
gcb_dupefile(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  struct dupefile* ptr = (struct dupefile*) buffer;
  errno = 0;

  if (k_l == 4 && !strncmp(key, _MC_GLOB_FILE, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->filename, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->uploader, val, v_l > 23 ? 23 : v_l);
      return 1;

    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timeup = v_i;
      return 1;
    }
  return 0;
}

void *
ref_to_val_ptr_dupefile(void *arg, char *match, int *output)
{

  struct dupefile *data = (struct dupefile *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timeup));
      return &data->timeup;
    }
  return NULL;
}
