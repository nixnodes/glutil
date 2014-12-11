/*5
 * lref_gconf.h5
 *
 *  Created on: Dec 10, 2013
 *      Author: reboot
 */

#ifndef LREF_GCONF_H_
#define LREF_GCONF_H_

#include <glutil.h>
#include <m_general.h>
#include <fp_types.h>

#include <stdint.h>
#include <inttypes.h>
#include <regex.h>

#define _MC_GCONF_R_CLEAN               "r_path_clean"
#define _MC_GCONF_R_CLEAN_ICASE         "r_path_clean_icase"
#define _MC_GCONF_R_POSTPROC            "r_path_postproc"
#define _MC_GCONF_R_YEARM               "r_year_extract"
#define _MC_GCONF_R_SKIPBDIR            "r_skip_basedir"
#define _MC_GCONF_R_SECTS               "paths"
#define _MC_GCONF_O_SHM                 "use_shared_mem"
#define _MC_GCONF_E_LF_IMDB             "path_exec_on_lookup_fail_imdb"
#define _MC_GCONF_E_LF_TVRAGE           "path_exec_on_lookup_fail_tvrage"
#define _MC_GCONF_E_OLF                 "execute_on_lookup_fail"
#define _MC_GCONF_E_M                   "path_exec_on_match"
#define _MC_GCONF_EX_U                  "r_exclude_user"
#define _MC_GCONF_EX_UF                 "r_exclude_user_flags"
#define _MC_GCONF_STRCTNSI              "lookup_match_strictness_imdb"
#define _MC_GCONF_STRCTNST              "lookup_match_strictness_tvrage"
#define _MC_GCONF_LOGGING               "logging"
#define _MC_GCONF_LOGSTR                "log_string"
#define _MC_GCONF_IMDB_SKZERO           "imdb_skip_zero_score"

#define GCONF_MAX_REG_EXPR        16384
#define GCONF_MAX_EXEC            4096
#define GCONF_MAX_REG_USR         2048
#define GCONF_MAX_UFLAGS          256

__d_format_block gconf_format_block, gconf_format_block_batch,
    gconf_format_block_exp;

__d_ref_to_pval ref_to_val_ptr_gconf;

_d_rtv_lk ref_to_val_lk_gconf;

int
gcb_gconf(void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_gconf
{
  int8_t o_use_shared_mem;
  int8_t o_exec_on_lookup_fail;
  int8_t o_lookup_strictness_imdb;
  int8_t o_lookup_strictness_tvrage;
  int8_t o_logging;
  int8_t o_imdb_skip_zero_score;
  char r_clean[GCONF_MAX_REG_EXPR];
  char r_postproc[GCONF_MAX_REG_EXPR];
  char r_yearm[GCONF_MAX_REG_EXPR];
  char r_sects[GCONF_MAX_REG_EXPR];
  char r_skip_basedir[GCONF_MAX_REG_EXPR];
  char r_exclude_user[GCONF_MAX_REG_USR];
  char r_exclude_user_flags[GCONF_MAX_UFLAGS];
  char e_lookup_fail_imdb[GCONF_MAX_EXEC];
  char e_lookup_fail_tvrage[GCONF_MAX_EXEC];
  char e_match[GCONF_MAX_EXEC];
  int8_t o_r_clean_icase;
  char o_log_string[256];
  unsigned char ___padding[7936];
} _d_gconf, *__d_gconf;

#pragma pack(pop)

#define GC_SZ                           sizeof(_d_gconf)

#endif /* LREF_GCONF_H_ */
