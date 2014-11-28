/*
 * lref_online.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_online.h"

#include <lref.h>
#include <lref_gen.h>
#include <cfgv.h>
#include <str.h>
#include <omfp.h>

#include <stdio.h>
#include <time.h>

void
dt_set_online(__g_handle hdl)
{
  hdl->flags |= F_GH_ISONLINE;
  hdl->d_memb = 3;
  hdl->g_proc1_lookup = ref_to_val_lk_online;
  hdl->g_proc2 = ref_to_val_ptr_online;
  hdl->g_proc3 = online_format_block;
  hdl->g_proc3_batch = online_format_block_batch;
  hdl->g_proc3_extra = online_format_block_comp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->jm_offset = (size_t) &((struct ONLINE*) NULL)->username;
}

char *
dt_rval_online_ssl(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct ONLINE *) arg)->ssl_flag);
  return output;
}

char *
dt_rval_online_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct ONLINE *) arg)->groupid);
  return output;
}

char *
dt_rval_online_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct ONLINE *) arg)->login_time);
  return output;
}

char *
dt_rval_online_lupdt(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct ONLINE *) arg)->tstart.tv_sec);
  return output;
}

char *
dt_rval_online_lxfrt(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct ONLINE *) arg)->txfer.tv_sec);
  return output;
}

char *
dt_rval_online_bxfer(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu",
      (ulint64_t) ((struct ONLINE *) arg)->bytes_xfer);
  return output;
}

char *
dt_rval_online_btxfer(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu",
      (ulint64_t) ((struct ONLINE *) arg)->bytes_txfer);
  return output;
}

char *
dt_rval_online_pid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct ONLINE *) arg)->procid);
  return output;
}

char *
dt_rval_online_rate(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  int32_t tdiff = (int32_t) time(NULL) - ((struct ONLINE *) arg)->tstart.tv_sec;
  uint32_t kbps = 0;

  if (tdiff > 0 && ((struct ONLINE *) arg)->bytes_xfer > 0)
    {
      kbps = ((struct ONLINE *) arg)->bytes_xfer / tdiff;
    }
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, kbps);
  return output;
}

char *
dt_rval_online_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((struct ONLINE *) arg)->currentdir);
}

char *
dt_rval_online_ndir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct ONLINE *) arg)->currentdir);
  g_dirname(output);

  return output;
}

char *
dt_rval_online_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->username;
}

char *
dt_rval_online_tag(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->tagline;
}

char *
dt_rval_online_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->status;
}

char *
dt_rval_online_host(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->host;
}

char *
dt_rval_online_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->currentdir;
}

void *
ref_to_val_lk_online(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  PROC_SH_EX(match)

  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_ONLINE_SSL, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_online_ssl, (__d_drt_h ) mppd,
          "%hd");
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_online_group, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_online_time, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_ONLINE_LUPDT, 8))
    {
      return as_ref_to_val_lk(match, dt_rval_online_lupdt, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_ONLINE_LXFRT, 9))
    {
      return as_ref_to_val_lk(match, dt_rval_online_lxfrt, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_ONLINE_BXFER, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_online_bxfer, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_ONLINE_BTXFER, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_online_btxfer, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_GLOB_PID, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_online_pid, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, "rate", 4))
    {
      return as_ref_to_val_lk(match, dt_rval_online_rate, (__d_drt_h ) mppd,
          "%u");
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_online_basedir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_DIRNAME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_online_ndir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_online_user, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_TAG, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_online_tag, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_online_status, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_ONLINE_HOST, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_online_host, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_online_dir, (__d_drt_h ) mppd,
          "%s");
    }

  return NULL;
}

void *
ref_to_val_ptr_online(void *arg, char *match, int *output)
{

  struct ONLINE *data = (struct ONLINE *) arg;

  if (!strncmp(match, _MC_ONLINE_BTXFER, 6))
    {
      *output = sizeof(data->bytes_txfer);
      return &data->bytes_txfer;
    }
  else if (!strncmp(match, _MC_ONLINE_BXFER, 5))
    {
      *output = sizeof(data->bytes_xfer);
      return &data->bytes_xfer;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      *output = ~((int) sizeof(data->groupid));
      return &data->groupid;
    }
  else if (!strncmp(match, _MC_GLOB_PID, 3))
    {
      *output = ~((int) sizeof(data->procid));
      return &data->procid;
    }
  else if (!strncmp(match, _MC_ONLINE_SSL, 3))
    {
      *output = ~((int) sizeof(data->ssl_flag));
      return &data->ssl_flag;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->login_time));
      return &data->login_time;
    }
  else if (!strncmp(match, _MC_ONLINE_LUPDT, 8))
    {
      *output = ~((int) sizeof(data->tstart.tv_sec));
      return &data->tstart.tv_sec;
    }
  else if (!strncmp(match, _MC_ONLINE_LXFRT, 9))
    {
      *output = ~((int) sizeof(data->txfer.tv_sec));
      return &data->txfer.tv_sec;
    }

  return NULL;
}

int
online_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct ONLINE *data = (struct ONLINE *) iarg;

  time_t t_t = (time_t) data->login_time;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  if (!strlen(data->username))
    {
      return 0;
    }

  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }

  time_t ltime = time(NULL) - (time_t) data->login_time;

  if (ltime < 0)
    {
      ltime = 0;
    }

  return print_str("[ONLINE]\n"
      "    User:            %s\n"
      "    Host:            %s\n"
      "    GID:             %u\n"
      "    Login:           %s (%us)\n"
      "    Tag:             %s\n"
      "    SSL:             %s\n"
      "    PID:             %u\n"
      "    XFER:            %lld Bytes\n"
      "    Rate:            %.2f KB/s\n"
      "    Status:          %s\n"
      "    CWD:             %s\n\n", data->username, data->host,
      (uint32_t) data->groupid, buffer2, (uint32_t) ltime, data->tagline,
      (!data->ssl_flag ?
          "NO" :
          (data->ssl_flag == 1 ?
              "YES" : (data->ssl_flag == 2 ? "YES (DATA)" : "UNKNOWN"))),
      (uint32_t) data->procid, (ulint64_t) data->bytes_xfer, kbps, data->status,
      data->currentdir);

}

int
online_format_block_batch(void *iarg, char *output)
{
  struct ONLINE *data = (struct ONLINE *) iarg;
  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }
  return printf(
      "ONLINE\x9%s\x9%s\x9%d\x9%d\x9%s\x9%hd\x9%d\x9%llu\x9%llu\x9%llu\x9%s\x9%s\x9%d\x9%d\n",
      data->username, data->host, (int32_t) data->groupid,
      (int32_t) data->login_time, data->tagline, (int16_t) data->ssl_flag,
      (int32_t) data->procid, (ulint64_t) data->bytes_xfer,
      (ulint64_t) data->bytes_txfer, (ulint64_t) kbps, data->status,
      data->currentdir, (int32_t) data->tstart.tv_sec,
      (int32_t) data->txfer.tv_sec);

}

int
online_format_block_exp(void *iarg, char *output)
{
  struct ONLINE *data = (struct ONLINE *) iarg;
  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }
  return printf("user %s\n"
      "host %s\n"
      "group %d\n"
      "time %d\n"
      "tag %s\n"
      "ssl %hd\n"
      "pid %d\n"
      "bxfer %llu\n"
      "btxfer %llu\n"
      "kbps %llu\n"
      "status %s\n"
      "dir %s\n"
      "lupdtime %d\n"
      "lxfertime %d\n\n", data->username, data->host, (int32_t) data->groupid,
      (int32_t) data->login_time, data->tagline, (int16_t) data->ssl_flag,
      (int32_t) data->procid, (ulint64_t) data->bytes_xfer,
      (ulint64_t) data->bytes_txfer, (ulint64_t) kbps, data->status,
      data->currentdir, (int32_t) data->tstart.tv_sec,
      (int32_t) data->txfer.tv_sec);

}

int
online_format_block_comp(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct ONLINE *data = (struct ONLINE *) iarg;

  time_t t_t = (time_t) data->login_time;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  if (!strlen(data->username))
    {
      return 0;
    }

  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }

  time_t ltime = time(NULL) - (time_t) data->login_time;

  if (ltime < 0)
    {
      ltime = 0;
    }

  char sp_buffer[255], sp_buffer2[255], sp_buffer3[255];
  char d_buffer[255] =
    { 0 };

  snprintf(d_buffer, 254, "%u", (uint32_t) ltime);
  size_t d_len1 = strlen(d_buffer);
  snprintf(d_buffer, 254, "%.2f", kbps);
  size_t d_len2 = strlen(d_buffer);
  snprintf(d_buffer, 254, "%u", data->procid);
  size_t d_len3 = strlen(d_buffer);
  generate_chars(
      54 - (strlen(data->username) + strlen(data->host) + d_len3 + 1), 0x20,
      sp_buffer);
  generate_chars(10 - d_len1, 0x20, sp_buffer2);
  generate_chars(13 - d_len2, 0x20, sp_buffer3);
  return printf("| %s!%s/%u%s |        %us%s |     %.2fKB/s%s |  %s\n",
      data->username, data->host, data->procid, sp_buffer, (uint32_t) ltime,
      sp_buffer2, kbps, sp_buffer3, data->status);

}

