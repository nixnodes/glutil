/*
 * lref_imdb.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_IMDB_H_
#define LREF_IMDB_H_

#include <fp_types.h>

#include <stdint.h>
#include <inttypes.h>

#define _MC_IMDB_RELEASED               "released"
#define _MC_IMDB_VOTES                  "votes"
#define _MC_IMDB_YEAR                   "year"

#define _MC_IMDB_ACTORS                 "actors"
#define _MC_IMDB_TITLE                  "title"
#define _MC_IMDB_IMDBID                 "imdbid"
#define _MC_IMDB_RATED                  "rated"
#define _MC_IMDB_DIRECT                 "director"
#define _MC_IMDB_SYNOPSIS               "plot"
#define _MC_IMDB_LANGUAGE               "language"
#define _MC_IMDB_COUNTRY                "country"


#define STD_FMT_DATE_STR                "%d %b %Y"

__g_proc_rv dt_rval_imdb_time, dt_rval_imdb_score, dt_rval_imdb_votes,
    dt_rval_imdb_runtime, dt_rval_imdb_released, dt_rval_imdb_year,
    dt_rval_imdb_mode, dt_rval_imdb_basedir, dt_rval_imdb_dir,
    dt_rval_imdb_imdbid, dt_rval_imdb_genre, dt_rval_imdb_rated,
    dt_rval_imdb_title, dt_rval_imdb_director, dt_rval_imdb_actors,
    dt_rval_imdb_synopsis, dt_rval_imdb_x, dt_rval_imdb_xg;

__d_ref_to_pval ref_to_val_ptr_imdb;

_d_rtv_lk ref_to_val_lk_imdb;

__d_format_block imdb_format_block, imdb_format_block_batch,
    imdb_format_block_exp;

int
gcb_imdbh(void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_imdb
{
  char dirname[255];
  int32_t timestamp;
  char imdb_id[64]; /* IMDB ID */
  float rating; /* IMDB Rating */
  uint32_t votes; /* IMDB Votes */
  char genres[255]; /* List of genres (comma delimited) */
  uint16_t year;
  uint8_t _d_unused_m[3];
  char title[128];
  int32_t released;
  uint32_t runtime;
  char rated[32];
  char actors[128];
  char director[64];
  char synopsis[384];
  char language[128];
  char country[128];
  uint8_t _padding[32];
} _d_imdb, *__d_imdb;

#pragma pack(pop)

#define ID_SZ                           sizeof(_d_imdb)

#endif /* LREF_IMDB_H_ */
