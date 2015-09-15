/*
 * lref_imdb.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"
#include "lref_imdb.h"

#include <str.h>
#include <lref.h>
#include <xref.h>
#include <l_sb.h>
#include <omfp.h>

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "lref_generic.h"

void
dt_set_imdb (__g_handle hdl)
{
  hdl->flags |= F_GH_ISIMDB;
  hdl->block_sz = ID_SZ;
  hdl->d_memb = 14;
  hdl->g_proc0 = gcb_imdbh;
  hdl->g_proc1_lookup = ref_to_val_lk_imdb;
  hdl->g_proc2 = ref_to_val_ptr_imdb;
  hdl->g_proc3 = imdb_format_block;
  hdl->g_proc3_batch = imdb_format_block_batch;
  hdl->g_proc3_export = imdb_format_block_exp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_IMDBLOG;
  hdl->jm_offset = (size_t) &((__d_imdb) NULL)->dirname;
}

void *
ref_to_val_ptr_imdb_e (void *arg, char *match, int *output)
{
  __d_imdb data = (__d_imdb) arg;

  if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      *output = -32;
      return &data->rating;
    }
  else if (!strncmp(match, _MC_IMDB_RELEASED, 8))
    {
      *output = ~((int) sizeof(data->released));
      return &data->released;
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
  else if (!strncmp(match, _MC_IMDB_VOTES, 5))
    {
      *output = sizeof(data->votes);
      return &data->votes;
    }
  else if (!strncmp(match, _MC_IMDB_YEAR, 4))
    {
      *output = sizeof(data->year);
      return &data->year;
    }
  else if (!strncmp(match, _MC_IMDB_SCREENS, 7))
    {
      *output = sizeof(data->screens);
      return &data->screens;
    }

  return NULL;
}

void *
ref_to_val_ptr_imdb (void *arg, char *match, int *output)
{
  REF_TO_VAL_RESOLVE(arg, match, output, ref_to_val_ptr_imdb_e)
}

static char *
dt_rval_imdb_lang (void *arg, char *match, char *output, size_t max_size,
		   void *mppd)
{
  return ((__d_imdb) arg)->language;
}

static char *
dt_rval_imdb_country (void *arg, char *match, char *output, size_t max_size,
		      void *mppd)
{
  return ((__d_imdb) arg)->country;
}

static char *
dt_rval_imdb_type (void *arg, char *match, char *output, size_t max_size,
		   void *mppd)
{
  return ((__d_imdb) arg)->type;
}

static char *
dt_rval_imdb_screens (void *arg, char *match, char *output, size_t max_size,
		      void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_imdb) arg)->screens);
  return output;
}

void *
ref_to_val_lk_imdb (void *arg, char *match, char *output, size_t max_size,
		    void *mppd)
{
  PROC_SH_EX(match)

  void *ptr;
  if ((ptr = ref_to_val_lk_generic (arg, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp (match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_time, (__d_drt_h ) mppd,
			       "%d");
    }
  else if (!strncmp (match, _MC_GLOB_SCORE, 5))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_score, (__d_drt_h ) mppd,
			       "%.1f");
    }
  else if (!strncmp (match, _MC_IMDB_VOTES, 5))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_votes, (__d_drt_h ) mppd,
			       "%u");
    }
  else if (!strncmp (match, _MC_GLOB_RUNTIME, 7))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_runtime, (__d_drt_h ) mppd,
			       "%u");
    }
  else if (!strncmp (match, _MC_IMDB_RELEASED, 8))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_released, (__d_drt_h ) mppd,
			       "%u");
    }
  else if (!strncmp (match, _MC_IMDB_YEAR, 4))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_year, (__d_drt_h ) mppd,
			       "%hu");
    }
  else if (!strncmp (match, _MC_GLOB_MODE, 4))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_mode, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_GLOB_BASEDIR, 7))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_basedir, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_IMDBID, 6))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_imdbid, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_GLOB_GENRE, 5))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_genre, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_RATED, 5))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_rated, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_TITLE, 5))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_title, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_DIRECT, 8))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_director, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_GLOB_DIR, 3))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_dir, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp (match, _MC_IMDB_ACTORS, 6))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_actors, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_SYNOPSIS, 4))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_synopsis, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_GLOB_XREF, 2))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_x, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp (match, _MC_GLOB_XGREF, 3))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_xg, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp (match, _MC_IMDB_LANGUAGE, 8))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_lang, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_COUNTRY, 7))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_country, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_TYPE, 4))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_type, (__d_drt_h ) mppd,
			       "%s");
    }
  else if (!strncmp (match, _MC_IMDB_SCREENS, 7))
    {
      return as_ref_to_val_lk (match, dt_rval_imdb_screens, (__d_drt_h ) mppd,
			       "%u");
    }
  return NULL;
}

char *
dt_rval_imdb_time (void *arg, char *match, char *output, size_t max_size,
		   void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_imdb) arg)->timestamp);
  return output;
}

char *
dt_rval_imdb_score (void *arg, char *match, char *output, size_t max_size,
		    void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_imdb) arg)->rating);
  return output;
}

char *
dt_rval_imdb_votes (void *arg, char *match, char *output, size_t max_size,
		    void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_imdb) arg)->votes);
  return output;
}

char *
dt_rval_imdb_runtime (void *arg, char *match, char *output, size_t max_size,
		      void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_imdb) arg)->runtime);
  return output;
}

char *
dt_rval_imdb_released (void *arg, char *match, char *output, size_t max_size,
		       void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_imdb) arg)->released);
  return output;
}

char *
dt_rval_imdb_year (void *arg, char *match, char *output, size_t max_size,
		   void *mppd)
{
  snprintf (output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_imdb) arg)->year);
  return output;
}

char *
dt_rval_imdb_mode (void *arg, char *match, char *output, size_t max_size,
		   void *mppd)
{
  g_l_fmode (((__d_imdb) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_imdb_basedir (void *arg, char *match, char *output, size_t max_size,
		      void *mppd)
{
  return g_basename (((__d_imdb) arg)->dirname);
}

char *
dt_rval_imdb_dir (void *arg, char *match, char *output, size_t max_size,
		  void *mppd)
{
  return ((__d_imdb) arg)->dirname;
}

char *
dt_rval_imdb_imdbid (void *arg, char *match, char *output, size_t max_size,
		     void *mppd)
{
  return ((__d_imdb) arg)->imdb_id;
}

char *
dt_rval_imdb_genre (void *arg, char *match, char *output, size_t max_size,
		    void *mppd)
{
  return ((__d_imdb) arg)->genres;
}

char *
dt_rval_imdb_rated (void *arg, char *match, char *output, size_t max_size,
		    void *mppd)
{
  return ((__d_imdb) arg)->rated;
}

char *
dt_rval_imdb_title (void *arg, char *match, char *output, size_t max_size,
		    void *mppd)
{
  return ((__d_imdb) arg)->title;
}

char *
dt_rval_imdb_director (void *arg, char *match, char *output, size_t max_size,
		       void *mppd)
{
  return ((__d_imdb) arg)->director;
}

char *
dt_rval_imdb_actors (void *arg, char *match, char *output, size_t max_size,
		     void *mppd)
{
  return ((__d_imdb) arg)->actors;
}

char *
dt_rval_imdb_synopsis (void *arg, char *match, char *output, size_t max_size,
		       void *mppd)
{
  return ((__d_imdb) arg)->synopsis;
}

char *
dt_rval_imdb_x (void *arg, char *match, char *output, size_t max_size,
		void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s (xref_t.name, sizeof(xref_t.name), ((__d_imdb) arg)->dirname);
  ref_to_val_x ((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_imdb_xg (void *arg, char *match, char *output, size_t max_size,
		 void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf (xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT, ((__d_imdb) arg)->dirname);
  ref_to_val_x ((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

int
gcb_imdbh (void *buffer, char *key, char *val)
{
  size_t k_l = strlen (key), v_l;
  __d_imdb ptr = (__d_imdb) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp (key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp (key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol (val, NULL, 10);
      if ( errno == ERANGE)
	{
	  return 0;
	}
      ptr->timestamp = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp (key, _MC_IMDB_IMDBID, 6))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->imdb_id, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp (key, _MC_GLOB_SCORE, 5))
    {
      float v_f = strtof (val, NULL);
      if ( errno == ERANGE)
	{
	  return 0;
	}
      ptr->rating = v_f;
      return 1;
    }
  else if (k_l == 5 && !strncmp (key, _MC_IMDB_VOTES, 5))
    {
      uint32_t v_ui = (uint32_t) strtol (val, NULL, 10);
      if ( errno == ERANGE)
	{
	  return 0;
	}
      ptr->votes = v_ui;
      return 1;
    }
  else if (k_l == 4 && !strncmp (key, _MC_IMDB_YEAR, 4))
    {
      uint16_t v_ui = (uint16_t) strtol (val, NULL, 10);
      if ( errno == ERANGE)
	{
	  return 0;
	}
      ptr->year = v_ui;
      return 1;
    }
  else if (k_l == 5 && !strncmp (key, _MC_GLOB_GENRE, 5))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->genres, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp (key, _MC_IMDB_TITLE, 5))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->title, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 8 && !strncmp (key, _MC_IMDB_RELEASED, 8))
    {
      int32_t v_i = (int32_t) strtol (val, NULL, 10);
      if ( errno == ERANGE)
	{
	  return 0;
	}
      ptr->released = v_i;
      return 1;
    }
  else if (k_l == 7 && !strncmp (key, _MC_GLOB_RUNTIME, 7))
    {
      uint32_t v_ui = (uint32_t) strtoul (val, NULL, 10);
      if ( errno == ERANGE)
	{
	  return 0;
	}
      ptr->runtime = v_ui;
      return 1;
    }
  else if (k_l == 5 && !strncmp (key, _MC_IMDB_RATED, 5))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->rated, val,
	      v_l > sizeof(ptr->rated) - 1 ? sizeof(ptr->rated) - 1 : v_l);
      return 1;
    }
  else if (k_l == 6 && !strncmp (key, _MC_IMDB_ACTORS, 6))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->actors, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 8 && !strncmp (key, _MC_IMDB_DIRECT, 8))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (ptr->director, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp (key, _MC_IMDB_SYNOPSIS, 4))
    {
      if (!(v_l = strlen (val)))
	{
	  return 0;
	}
      memcpy (
	  ptr->synopsis, val,
	  v_l > sizeof(ptr->synopsis) - 1 ? sizeof(ptr->synopsis) - 1 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp (key, _MC_IMDB_COUNTRY, 7))
    {
      if (!(v_l = strlen (val)))
	{
	  return 1;
	}
      memcpy (ptr->country, val,
	      v_l > sizeof(ptr->country) - 1 ? sizeof(ptr->country) - 1 : v_l);
      return 1;
    }
  else if (k_l == 8 && !strncmp (key, _MC_IMDB_LANGUAGE, 8))
    {
      if (!(v_l = strlen (val)))
	{
	  return 1;
	}
      memcpy (
	  ptr->language, val,
	  v_l > sizeof(ptr->language) - 1 ? sizeof(ptr->language) - 1 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp (key, _MC_IMDB_TYPE, 4))
    {
      if (!(v_l = strlen (val)))
	{
	  return 1;
	}
      memcpy (ptr->type, val,
	      v_l > sizeof(ptr->type) - 1 ? sizeof(ptr->type) - 1 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp (key, _MC_IMDB_SCREENS, 7))
    {
      uint32_t v_ui = (uint32_t) strtol (val, NULL, 10);
      if ( errno == ERANGE)
	{
	  return 1;
	}
      ptr->screens = v_ui;
      return 1;
    }

  return 0;
}

int
imdb_format_block (void *iarg, char *output)
{
  char buffer2[255] =
    { 0 }, buffer3[255] =
    { 0 };

  __d_imdb data = (__d_imdb) iarg;

  time_t t_t = (time_t) data->timestamp, t_t2 = (time_t) data->released;

  strftime (buffer2, sizeof(buffer2), STD_FMT_TIME_STR, localtime (&t_t));
  strftime (buffer3, sizeof(buffer3), STD_FMT_DATE_STR, localtime (&t_t2));

  return print_str (
      "IMDB: %s (%hu): created: %s - iMDB ID: %s - rating: %.1f - votes: %u - genres: %s - released: %s - runtime: %u min - rated: %s - director: %s\n",
      data->title, data->year, buffer2, data->imdb_id, data->rating,
      data->votes, data->genres, buffer3, data->runtime, data->rated,
      data->director);

}

int
imdb_format_block_batch (void *iarg, char *output)
{
  __d_imdb data = (__d_imdb) iarg;
  return printf(
      "IMDB\x9%s\x9%s\x9%d\x9%s\x9%.1f\x9%u\x9%s\x9%hu\x9%d\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%u\n",
      data->dirname, data->title, data->timestamp, data->imdb_id,
      data->rating, data->votes, data->genres, data->year, data->released,
      data->runtime, data->rated, data->actors, data->director,
      data->synopsis, data->language, data->country, data->type, data->screens);
}

int
imdb_format_block_exp (void *iarg, char *output)
{
  __d_imdb data = (__d_imdb) iarg;
  return printf(
      "dir %s\n"
      "title %s\n"
      "time %d\n"
      "imdbid %s\n"
      "score %.1f\n"
      "votes %u\n"
      "genre %s\n"
      "year %hu\n"
      "released %d\n"
      "runtime %u\n"
      "rated %s\n"
      "actors %s\n"
      "director %s\n"
      "plot %s\n"
      "language %s\n"
      "country %s\n"
      "type %s\n"
      "screens %u\n\n",
      data->dirname, data->title, data->timestamp, data->imdb_id,
      data->rating, data->votes, data->genres, data->year, data->released,
      data->runtime, data->rated, data->actors, data->director,
      data->synopsis, data->language, data->country, data->type, data->screens);
}
