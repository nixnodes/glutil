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

#define _MC_GCONF_R_CLEAN         "r_clean"
#define _MC_GCONF_R_POSTPROC      "r_postproc"
#define _MC_GCONF_R_YEARM         "r_yearm"
#define _MC_GCONF_R_SECTS         "r_sects"
#define _MC_GCONF_O_SHM           "o_use_shared"
#define _MC_GCONF_E_LF            "e_lookup_fail"
#define _MC_GCONF_E_OLF           "o_exec_on_lookup_fail"
#define _MC_GCONF_E_M             "e_match"

#define GCONF_MAX_REG_EXPR        16384
#define GCONF_MAX_EXEC            32768


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
  char r_clean[GCONF_MAX_REG_EXPR];
  char r_postproc[GCONF_MAX_REG_EXPR];
  char r_yearm[GCONF_MAX_REG_EXPR];
  char r_sects[GCONF_MAX_REG_EXPR];
  char e_lookup_fail[GCONF_MAX_EXEC];
  char e_match[GCONF_MAX_EXEC];
} _d_gconf, *__d_gconf;

#pragma pack(pop)

#define GC_SZ                           sizeof(_d_gconf)


#endif /* LREF_GCONF_H_ */
