/*
 * lref_tvrage.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"
#include "lref_tvrage.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <xref.h>
#include <mc_glob.h>
#include <l_sb.h>
#include <omfp.h>

#include <errno.h>
#include <time.h>

void
dt_set_tvrage(__g_handle hdl)
{
  hdl->flags |= F_GH_ISTVRAGE;
  hdl->block_sz = TV_SZ;
  hdl->d_memb = 18;
  hdl->g_proc0 = gcb_tv;
  hdl->g_proc1_lookup = ref_to_val_lk_tvrage;
  hdl->g_proc2 = ref_to_val_ptr_tv;
  hdl->g_proc3 = tv_format_block;
  hdl->g_proc3_batch = tv_format_block_batch;
  hdl->g_proc3_export = tv_format_block_exp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_TVRAGELOG;
  hdl->jm_offset = (size_t) &((__d_tvrage) NULL)->dirname;
}


void *
ref_to_val_ptr_tv(void *arg, char *match, int *output)
{

  __d_tvrage data = (__d_tvrage) arg;

  if (!strncmp(match, _MC_TV_STARTED, 7))
    {
      *output = ~((int)sizeof(data->started));
      return &data->started;
    }
  else if (!strncmp(match, _MC_GLOB_RUNTIME, 7))
    {
      *output = sizeof(data->runtime);
      return &data->runtime;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timestamp));
      return &data->timestamp;
    }
  else if (!strncmp(match, _MC_TV_ENDED, 5))
    {
      *output = ~((int) sizeof(data->ended));
      return &data->ended;
    }
  else if (!strncmp(match, _MC_TV_SHOWID, 6))
    {
      *output = sizeof(data->showid);
      return &data->showid;
    }
  else if (!strncmp(match, _MC_TV_SEASONS, 7))
    {
      *output = sizeof(data->seasons);
      return &data->seasons;
    }
  else if (!strncmp(match, _MC_TV_SYEAR, 9))
    {
      *output = sizeof(data->startyear);
      return &data->startyear;
    }
  else if (!strncmp(match, _MC_TV_EYEAR, 7))
    {
      *output = sizeof(data->endyear);
      return &data->endyear;
    }

  return NULL;
}


char *
dt_rval_tvrage_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->dirname;
}

char *
dt_rval_tvrage_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((__d_tvrage) arg)->dirname);
}

char *
dt_rval_tvrage_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_tvrage) arg)->timestamp);
  return output;
}

char *
dt_rval_tvrage_ended(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_tvrage) arg)->ended);
  return output;
}

char *
dt_rval_tvrage_started(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_tvrage) arg)->started);
  return output;
}

char *
dt_rval_tvrage_seasons(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_tvrage) arg)->seasons);
  return output;
}

char *
dt_rval_tvrage_showid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_tvrage) arg)->showid);
  return output;
}

char *
dt_rval_tvrage_runtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_tvrage) arg)->runtime);
  return output;
}

char *
dt_rval_tvrage_startyear(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_tvrage) arg)->startyear);
  return output;
}

char *
dt_rval_tvrage_endyear(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_tvrage) arg)->endyear);
  return output;
}

char *
dt_rval_tvrage_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((__d_tvrage) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_tvrage_airday(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->airday;
}

char *
dt_rval_tvrage_airtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->airtime;
}

char *
dt_rval_tvrage_country(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->country;
}

char *
dt_rval_tvrage_link(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->link;
}

char *
dt_rval_tvrage_name(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->name;
}

char *
dt_rval_tvrage_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->status;
}

char *
dt_rval_tvrage_class(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->class;
}

char *
dt_rval_tvrage_genre(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->genres;
}

char *
dt_rval_tvrage_network(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->network;
}

char *
dt_rval_tvrage_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((__d_tvrage) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_tvrage_xg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT, ((__d_tvrage) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_tvrage(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_time, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_TV_ENDED, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_ended, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_TV_STARTED, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_started, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_TV_SEASONS, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_seasons, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_TV_SHOWID, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_showid, (__d_drt_h ) mppd,
          "%u");
    }
  else if (!strncmp(match, _MC_GLOB_RUNTIME, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_runtime, (__d_drt_h ) mppd,
          "%u");
    }
  else if (!strncmp(match, _MC_TV_SYEAR, 9))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_startyear,
          (__d_drt_h ) mppd, "%hu");
    }
  else if (!strncmp(match, _MC_TV_EYEAR, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_endyear, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_mode, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_dir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_basedir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_AIRDAY, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_airday, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_AIRTIME, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_airtime, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_COUNTRY, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_country, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_LINK, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_link, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_NAME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_name, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_status, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_CLASS, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_class, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_GENRE, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_genre, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_TV_NETWORK, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_network, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_x, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_tvrage_xg, (__d_drt_h ) mppd, "%s");
    }

  return NULL;
}


int
gcb_tv(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_tvrage ptr = (__d_tvrage) buffer;
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
  else if (k_l == 6 && !strncmp(key, _MC_TV_AIRDAY, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->airday, val, v_l > 31 ? 31 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_AIRTIME, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->airtime, val, v_l > 5 ? 5 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_COUNTRY, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->country, val, v_l > 23 ? 23 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_TV_LINK, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->link, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_TV_NAME, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->name, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_TV_ENDED, 5))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ended = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_STARTED, 7))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->started = v_i;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_GLOB_RUNTIME, 7))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->runtime = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_SEASONS, 7))
    {
      uint16_t v_ui = (uint16_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->seasons = v_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_TV_SHOWID, 6))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->showid = v_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_STATUS, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->status, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_TV_CLASS, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->class, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GENRE, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->genres, val, v_l > 255 ? 255 : v_l);
      return 1;
    }
  else if (k_l == 9 && !strncmp(key, _MC_TV_SYEAR, 9))
    {
      uint16_t v_ui = (uint16_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->startyear = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_EYEAR, 7))
    {
      uint16_t v_ui = (uint16_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->endyear = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_NETWORK, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->network, val, v_l > 255 ? 255 : v_l);
      return 1;
    }
  return 0;
}


int
tv_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  __d_tvrage data = (__d_tvrage) iarg;

  time_t t_t = (time_t) data->timestamp;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str(
      "TVRAGE: %s: created: %s - %s/%s [%u] - runtime: %u min - status: %s - country: %s - genres: %s\n",
      data->dirname, buffer2, data->name, data->class, data->showid,
      data->runtime, data->status, data->country, data->genres);

}

int
tv_format_block_batch(void *iarg, char *output)
{
  __d_tvrage data = (__d_tvrage) iarg;
  return printf("TVRAGE\x9%s\x9%d\x9%s\x9%s\x9%u\x9%s\x9%s\x9%s\x9%s\x9%u\x9%d\x9%d\x9%s\x9%s\x9%hu\x9%hu\x9%hu\x9%s\n",
      data->dirname, data->timestamp, data->name, data->class,
      data->showid, data->link, data->status, data->airday,
      data->airtime, data->runtime, data->started,
      data->ended, data->genres, data->country, data->seasons,
      data->startyear, data->endyear, data->network);

}

int
tv_format_block_exp(void *iarg, char *output)
{
  __d_tvrage data = (__d_tvrage) iarg;
  return printf("dir %s\n"
      "time %d\n"
      "name %s\n"
      "class %s\n"
      "showid %u\n"
      "link %s\n"
      "status %s\n"
      "airday %s\n"
      "airtime %s\n"
      "runtime %u\n"
      "started %d\n"
      "ended %d\n"
      "genre %s\n"
      "country %s\n"
      "seasons %hu\n"
      "startyear %hu\n"
      "endyear %hu\n"
      "network %s\n\n",
      data->dirname, data->timestamp, data->name, data->class,
      data->showid, data->link, data->status, data->airday,
      data->airtime, data->runtime, data->started,
      data->ended, data->genres, data->country, data->seasons,
      data->startyear, data->endyear, data->network);

}
