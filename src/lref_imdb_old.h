/*
 * lref_imdb.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_IMDB_O_H_
#define LREF_IMDB_O_H_

#include <fp_types.h>

#include <stdint.h>
#include <inttypes.h>


#define STD_FMT_DATE_STR                "%d %b %Y"

__g_proc_rv dt_rval_imdb_o_time, dt_rval_imdb_o_score, dt_rval_imdb_o_votes,
    dt_rval_imdb_o_runtime, dt_rval_imdb_o_released, dt_rval_imdb_o_year,
    dt_rval_imdb_o_mode, dt_rval_imdb_o_basedir, dt_rval_imdb_o_dir,
    dt_rval_imdb_o_imdbid, dt_rval_imdb_o_genre, dt_rval_imdb_o_rated,
    dt_rval_imdb_o_title, dt_rval_imdb_o_director, dt_rval_imdb_o_actors,
    dt_rval_imdb_o_synopsis, dt_rval_imdb_o_x, dt_rval_imdb_o_xg;

__d_ref_to_pval ref_to_val_ptr_imdb_o;

_d_rtv_lk ref_to_val_lk_imdb_o;

__d_format_block imdb_o_format_block, imdb_o_format_block_batch,
    imdb_o_format_block_exp;

int
gcb_imdbh_o(void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_imdb_o
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
  char rated[8];
  char actors[128];
  char director[64];
  char synopsis[198];
  /* ------------- */
  uint8_t _d_unused_e[32]; /* Reserved for future use */

} _d_imdb_o, *__d_imdb_o;

#pragma pack(pop)

#define IDO_SZ                           sizeof(_d_imdb_o)

#endif /* LREF_IMDB_O_H_ */
