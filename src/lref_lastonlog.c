/*
 * lref_lastonlog.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_lastonlog.h"

#include <mc_glob.h>
#include <cfgv.h>
#include <str.h>
#include <lref.h>
#include <lref_gen.h>

#include <time.h>
#include <errno.h>

char *
dt_rval_lastonlog_logon(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct lastonlog *) arg)->logon);
  return output;
}

char *
dt_rval_lastonlog_logoff(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct lastonlog *) arg)->logoff);
  return output;
}

char *
dt_rval_lastonlog_upload(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (unsigned long) ((struct lastonlog *) arg)->upload);
  return output;
}

char *
dt_rval_lastonlog_download(void *arg, char *match, char *output,
    size_t max_size, void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (unsigned long) ((struct lastonlog *) arg)->download);
  return output;
}

char *
dt_rval_lastonlog_config(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char p_b0[64];
  int ic = 0;

  while (match[ic] != 0x7D && ic < 64)
    {
      p_b0[ic] = match[ic];
      ic++;
    }

  p_b0[ic] = 0x0;

  void *ptr = ref_to_val_get_cfgval(((struct lastonlog *) arg)->uname, p_b0,
  DEFPATH_USERS,
  F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, output, max_size);
  if (ptr)
    {
      return ptr;
    }

  return ref_to_val_get_cfgval(((struct lastonlog *) arg)->gname, p_b0,
  DEFPATH_GROUPS,
  F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, output, max_size);
}

char *
dt_rval_lastonlog_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->uname;
}

char *
dt_rval_lastonlog_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->gname;
}

char *
dt_rval_lastonlog_stats(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->stats;
}

char *
dt_rval_lastonlog_tag(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->tagline;
}

void *
ref_to_val_lk_lastonlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_LOGON, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_logon, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_GLOB_LOGOFF, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_logoff,
          (__d_drt_h ) mppd, "%d");
    }
  else if (!strncmp(match, _MC_GLOB_UPLOAD, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_upload,
          (__d_drt_h ) mppd, "%lu");
    }
  else if (!strncmp(match, _MC_GLOB_DOWNLOAD, 8))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_download,
          (__d_drt_h ) mppd, "%lu");
    }
  else if (!is_char_uppercase(match[0]))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_config,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_user, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_group, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_LASTONLOG_STATS, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_stats, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_TAG, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_lastonlog_tag, (__d_drt_h ) mppd,
          "%s");
    }

  return NULL;
}


int
lastonlog_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 }, buffer3[255] =
    { 0 }, buffer4[12] =
    { 0 };

  struct lastonlog *data = (struct lastonlog *) iarg;

  time_t t_t_ln = (time_t) data->logon;
  time_t t_t_lf = (time_t) data->logoff;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t_ln));
  strftime(buffer3, 255, STD_FMT_TIME_STR, localtime(&t_t_lf));

  memcpy(buffer4, data->stats, sizeof(data->stats));

  return print_str(
      "LASTONLOG: user: %s/%s [%s] - logon: %s, logoff: %s - up/down: %lu/%lu B, changes: %s\n",
      data->uname, data->gname, data->tagline, buffer2, buffer3,
      (unsigned long) data->upload, (unsigned long) data->download, buffer4);

}

int
lastonlog_format_block_batch(void *iarg, char *output)
{
  struct lastonlog *data = (struct lastonlog *) iarg;
  char buffer4[12] =
    { 0 };
  memcpy(buffer4, data->stats, sizeof(data->stats));
  return printf("LASTONLOG\x9%s\x9%s\x9%s\x9%d\x9%d\x9%lu\x9%lu\x9%s\n",
      data->uname, data->gname, data->tagline, (int32_t) data->logon,
      (int32_t) data->logoff, (unsigned long) data->upload,
      (unsigned long) data->download, buffer4);
}

int
lastonlog_format_block_exp(void *iarg, char *output)
{
  struct lastonlog *data = (struct lastonlog *) iarg;
  char buffer4[12] =
    { 0 };
  memcpy(buffer4, data->stats, sizeof(data->stats));
  return printf("user %s\n"
      "group %s\n"
      "tag %s\n"
      "logon %d\n"
      "logoff %d\n"
      "upload %lu\n"
      "download %lu\n"
      "stats %s\n\n", data->uname, data->gname, data->tagline,
      (int32_t) data->logon, (int32_t) data->logoff,
      (unsigned long) data->upload, (unsigned long) data->download, buffer4);
}


int
gcb_lastonlog(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  struct lastonlog* ptr = (struct lastonlog*) buffer;
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
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_LOGON, 5))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->logon = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_LOGOFF, 6))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->logoff = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_UPLOAD, 6))
    {
      unsigned long v_ul = (unsigned long) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->upload = v_ul;
      return 1;
    }
  else if (k_l == 8 && !strncmp(key, _MC_GLOB_DOWNLOAD, 8))
    {
      unsigned long v_ul = (unsigned long) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->download = v_ul;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_LASTONLOG_STATS, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->stats, val, v_l > 6 ? 6 : v_l);
      return 1;
    }
  return 0;
}

void *
ref_to_val_ptr_lastonlog(void *arg, char *match, int *output)
{
  struct lastonlog *data = (struct lastonlog *) arg;

  if (!strncmp(match, _MC_GLOB_LOGON, 5))
    {
      *output = ~((int) sizeof(data->logon));
      return &data->logon;
    }
  else if (!strncmp(match, _MC_GLOB_LOGOFF, 6))
    {
      *output = ~((int) sizeof(data->logoff));
      return &data->logoff;
    }
  else if (!strncmp(match, _MC_GLOB_DOWNLOAD, 8))
    {
      *output = sizeof(data->download);
      return &data->download;
    }
  else if (!strncmp(match, _MC_GLOB_UPLOAD, 6))
    {
      *output = sizeof(data->upload);
      return &data->upload;
    }
  return NULL;
}
