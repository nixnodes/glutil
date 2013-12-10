/*
 * lref_nukelog.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_nukelog.h"

#include <mc_glob.h>
#include <lref.h>
#include <lref_gen.h>

#include <str.h>
#include <xref.h>

#include <errno.h>
#include <time.h>

void *
ref_to_val_lk_nukelog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_size, (__d_drt_h ) mppd,
          "%f");
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_time, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_status, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_NUKELOG_MULT, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_mult, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_mode_e, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_dir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_basedir_e,
          (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_NUKELOG_NUKER, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_nuker, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_NUKELOG_NUKEE, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_nukee, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_NUKELOG_UNNUKER, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_unnuker, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_NUKELOG_REASON, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_nukelog_reason, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_x_nukelog, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_xg_nukelog, (__d_drt_h ) mppd,
          "%s");
    }

  return NULL;
}


char *
dt_rval_nukelog_size(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (float) ((struct nukelog *) arg)->bytes);
  return output;
}

char *
dt_rval_nukelog_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct nukelog *) arg)->nuketime);
  return output;
}

char *
dt_rval_nukelog_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct nukelog *) arg)->status);
  return output;
}

char *
dt_rval_nukelog_mult(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct nukelog *) arg)->mult);
  return output;
}

char *
dt_rval_nukelog_mode_e(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((struct nukelog *) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_nukelog_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->dirname;
}

char *
dt_rval_nukelog_basedir_e(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((struct nukelog *) arg)->dirname);
}

char *
dt_rval_nukelog_nuker(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->nuker;
}

char *
dt_rval_nukelog_nukee(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->nukee;
}

char *
dt_rval_nukelog_unnuker(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->unnuker;
}

char *
dt_rval_nukelog_reason(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->reason;
}

char *
dt_rval_x_nukelog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((struct nukelog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_xg_nukelog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT,
      ((struct nukelog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}


int
gcb_nukelog(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  errno = 0;

  struct nukelog* ptr = (struct nukelog*) buffer;
  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_NUKELOG_REASON, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->reason, val, v_l > 59 ? 59 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_SIZE, 4))
    {
      float v_f = strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->bytes = v_f;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_NUKELOG_MULT, 4))
    {
      uint16_t v_ui = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->mult = v_ui;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_NUKELOG_NUKEE, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->nukee, val, v_l > 12 ? 12 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_NUKELOG_NUKER, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->nuker, val, v_l > 12 ? 12 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_NUKELOG_UNNUKER, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 1;
        }
      memcpy(ptr->unnuker, val, v_l > 12 ? 12 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t k_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->nuketime = k_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_STATUS, 6))
    {
      uint16_t k_us = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->status = k_us;
      return 1;
    }
  return 0;
}


int
nukelog_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct nukelog *data = (struct nukelog *) iarg;

  char *base = NULL;

  if (gfl & F_OPT_VERBOSE)
    {
      base = data->dirname;
    }
  else
    {
      base = g_basename(data->dirname);
    }

  time_t t_t = (time_t) data->nuketime;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str(
      "NUKELOG: %s - %s, reason: '%s' [%.2f MB] - factor: %hu, %s: %s, nukee: %s - %s\n",
      base, !data->status ? "NUKED" : "UNNUKED", data->reason, data->bytes,
      data->mult, !data->status ? "nuker" : "unnuker",
      !data->status ? data->nuker : data->unnuker, data->nukee, buffer2);

}

int
nukelog_format_block_batch(void *iarg, char *output)
{
  struct nukelog *data = (struct nukelog *) iarg;
  return printf("NUKELOG\x9%s\x9%s\x9%hu\x9%.2f\x9%s\x9%s\x9%s\x9%d\x9%hu\n",
      data->dirname, data->reason, data->mult, data->bytes, data->nuker,
      data->unnuker, data->nukee, (int32_t) data->nuketime, data->status);
}

int
nukelog_format_block_exp(void *iarg, char *output)
{
  struct nukelog *data = (struct nukelog *) iarg;
  return printf("dir %s\n"
      "reason %s\n"
      "mult %hu\n"
      "size %.2f\n"
      "nuker %s\n"
      "unnuker %s\n"
      "nukee %s\n"
      "time %d\n"
      "status %hu\n\n", data->dirname, data->reason, data->mult, data->bytes,
      data->nuker, data->unnuker, data->nukee, (int32_t) data->nuketime,
      data->status);
}

void *
ref_to_val_ptr_nukelog(void *arg, char *match, int *output)
{

  struct nukelog *data = (struct nukelog *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->nuketime));
      return &data->nuketime;
    }
  else if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      *output = -32;
      return &data->bytes;
    }
  else if (!strncmp(match, _MC_NUKELOG_MULT, 4))
    {
      *output = sizeof(data->mult);
      return &data->mult;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      *output = sizeof(data->status);
      return &data->status;
    }
  return NULL;
}
