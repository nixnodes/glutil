/*
 * lref_online.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include "lref_onliners.h"

#include <glutil.h>
#include <mc_glob.h>
#include <lref.h>
#include <lref_gen.h>

#include <stdio.h>
#include <errno.h>
#include <time.h>

void *
ref_to_val_lk_oneliners(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_oneliners_time, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_oneliners_user, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_oneliners_group, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_TAG, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_oneliners_tag, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_ONELINERS_MSG, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_oneliners_msg, (__d_drt_h ) mppd,
          "%s");
    }

  return NULL;
}


char *
dt_rval_oneliners_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct oneliner *) arg)->timestamp);
  return output;
}

char *
dt_rval_oneliners_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->uname;
}

char *
dt_rval_oneliners_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->gname;
}

char *
dt_rval_oneliners_tag(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->tagline;
}

char *
dt_rval_oneliners_msg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->message;
}

void *
ref_to_val_ptr_oneliners(void *arg, char *match, int *output)
{

  struct oneliner *data = (struct oneliner *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timestamp));
      return &data->timestamp;
    }
  return NULL;
}


int
gcb_oneliner(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  struct oneliner* ptr = (struct oneliner*) buffer;
  errno = 0;

  if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->uname, val, v_l > 23 ? 23 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GROUP, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->gname, val, v_l > 23 ? 23 : v_l);
      return 1;

    }
  else if (k_l == 3 && !strncmp(key, _MC_GLOB_TAG, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->tagline, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timestamp = v_i;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_ONELINERS_MSG, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->message, val, v_l > 99 ? 99 : v_l);
      return 1;
    }
  return 0;
}

int
oneliner_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct oneliner *data = (struct oneliner *) iarg;

  time_t t_t = (time_t) data->timestamp;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str("ONELINER: user: %s/%s [%s] - time: %s, message: %s\n",
      data->uname, data->gname, data->tagline, buffer2, data->message);

}

int
oneliner_format_block_batch(void *iarg, char *output)
{
  struct oneliner *data = (struct oneliner *) iarg;
  return printf("ONELINER\x9%s\x9%s\x9%s\x9%d\x9%s\n", data->uname, data->gname,
      data->tagline, (int32_t) data->timestamp, data->message);
}

int
oneliner_format_block_exp(void *iarg, char *output)
{
  struct oneliner *data = (struct oneliner *) iarg;
  return printf("user %s\n"
      "group %s\n"
      "tag %s\n"
      "time %d\n"
      "msg %s\n\n", data->uname, data->gname, data->tagline,
      (int32_t) data->timestamp, data->message);
}

