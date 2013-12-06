/*
 * lref_game.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include "lref_game.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <xref.h>
#include <mc_glob.h>
#include <l_sb.h>

#include <errno.h>
#include <time.h>

char *
dt_rval_game_score(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_game) arg)->rating);
  return output;
}

char *
dt_rval_game_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_game) arg)->timestamp);
  return output;
}

char *
dt_rval_game_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((__d_game) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_game_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((__d_game) arg)->dirname);
}

char *
dt_rval_game_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_game) arg)->dirname;
}

char *
dt_rval_game_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((__d_game) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_game_xg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT, ((__d_game) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_game(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_game_score, (__d_drt_h ) mppd,
          "%.1f");
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_game_time, (__d_drt_h ) mppd, "%d");
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_game_mode, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_game_basedir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_game_dir, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_game_x, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_game_xg, (__d_drt_h ) mppd, "%s");
    }
  return NULL;
}

void *
ref_to_val_ptr_game(void *arg, char *match, int *output)
{
  __d_game data = (__d_game) arg;

  if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      *output = -32;
      return &data->rating;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int)sizeof(data->timestamp));
      return &data->timestamp;
    }

  return NULL;
}

int
gcb_game(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_game ptr = (__d_game) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
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
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_SCORE, 5))
    {
      float v_f = strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->rating = v_f;
      return 1;
    }
  return 0;
}


int
game_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  __d_game data = (__d_game) iarg;

  time_t t_t = (time_t) data->timestamp;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str("GAMELOG: %s: created: %s - rating: %.1f\n", data->dirname,
      buffer2, data->rating);

}

int
game_format_block_batch(void *iarg, char *output)
{
  __d_game data = (__d_game) iarg;
  return printf("GAMELOG\x9%s\x9%d\x9%.1f\n", data->dirname,
      (int32_t) data->timestamp, data->rating);

}

int
game_format_block_exp(void *iarg, char *output)
{
  __d_game data = (__d_game) iarg;
  return printf("dir %s\n"
      "time %d\n"
      "score %.1f\n\n", data->dirname,
      (int32_t) data->timestamp, data->rating);

}
