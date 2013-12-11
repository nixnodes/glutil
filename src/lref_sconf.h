/*
 * lref_sconf.h
 *
 *  Created on: Dec 7, 2013
 *      Author: reboot
 */

#ifndef LREF_SCONF_H_
#define LREF_SCONF_H_

#include <glutil.h>
#include <m_general.h>
#include <fp_types.h>

#include <stdint.h>
#include <inttypes.h>
#include <regex.h>

#define _MC_SCONF_FIELD       "field"
#define _MC_SCONF_MATCH       "match"
#define _MC_SCONF_INT32       "int"
#define _MC_SCONF_INVERTM     "invert"
#define _MC_SCONF_TYPE        "type"
#define _MC_SCONF_UINT64      "uint64"
#define _MC_SCONF_MSG         "msg"
#define _MC_SCONF_LCOMP       "lcomp"
#define _MC_SCONF_ICASE       "icase"

#define SCONF_MAX_MSG          512
#define SCONF_MAX_REG_MATCH    32768

__d_format_block sconf_format_block, sconf_format_block_batch,
    sconf_format_block_exp;

__d_ref_to_pval ref_to_val_ptr_sconf;

_d_rtv_lk ref_to_val_lk_sconf;

int
gcb_sconf(void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_sconf
{
  int32_t i32;
  uint32_t ui32_1;
  uint32_t ui32_2;
  uint8_t invert;
  uint8_t type;
  uint8_t lcomp;
  uint8_t icase;
  uint64_t ui64;
  char match[SCONF_MAX_REG_MATCH];
  char field[MAX_VAR_LEN];
  char message[SCONF_MAX_MSG];
} _d_sconf, *__d_sconf;

#pragma pack(pop)

#define SC_SZ                           sizeof(_d_sconf)

#endif /* LREF_SCONF_H_ */
