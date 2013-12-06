/*
 * lref_tvrage.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_TVRAGE_H_
#define LREF_TVRAGE_H_

#define _MC_TV_STARTED          "started"
#define _MC_TV_ENDED            "ended"
#define _MC_TV_SHOWID           "showid"
#define _MC_TV_SEASONS          "seasons"
#define _MC_TV_SYEAR            "startyear"
#define _MC_TV_EYEAR            "endyear"
#define _MC_TV_AIRDAY           "airday"
#define _MC_TV_AIRTIME          "airtime"
#define _MC_TV_COUNTRY          "country"
#define _MC_TV_LINK             "link"
#define _MC_TV_NAME             "name"
#define _MC_TV_CLASS            "class"
#define _MC_TV_NETWORK          "network"

#include <fp_types.h>

__d_ref_to_pval ref_to_val_ptr_tv;
_d_rtv_lk ref_to_val_lk_tvrage;

__d_format_block tv_format_block, tv_format_block_batch,
    tv_format_block_exp;


__g_proc_rv dt_rval_tvrage_dir, dt_rval_tvrage_basedir, dt_rval_tvrage_time,
    dt_rval_tvrage_ended, dt_rval_tvrage_started, dt_rval_tvrage_started,
    dt_rval_tvrage_seasons, dt_rval_tvrage_showid, dt_rval_tvrage_runtime,
    dt_rval_tvrage_startyear, dt_rval_tvrage_endyear, dt_rval_tvrage_mode,
    dt_rval_tvrage_airday, dt_rval_tvrage_airtime, dt_rval_tvrage_country,
    dt_rval_tvrage_link, dt_rval_tvrage_name, dt_rval_tvrage_status,
    dt_rval_tvrage_class, dt_rval_tvrage_genre, dt_rval_tvrage_network;

int
gcb_tv(void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_tvrage
{
  char dirname[255];
  int32_t timestamp;
  uint32_t showid;
  char name[128];
  char link[128];
  char country[24];
  int32_t started;
  int32_t ended;
  uint16_t seasons;
  char status[64];
  uint32_t runtime;
  char airtime[6];
  char airday[32];
  char class[64];
  char genres[256];
  uint16_t startyear;
  uint16_t endyear;
  char network[72];
  uint8_t _d_unused[180]; /* Reserved for future use */
} _d_tvrage, *__d_tvrage;

#pragma pack(pop)

#define TV_SZ                           sizeof(_d_tvrage)


#endif /* LREF_TVRAGE_H_ */
