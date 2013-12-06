/*
 * cfgv.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef CFGV_H_
#define CFGV_H_

#include <glutil.h>

#include <memory_t.h>
#include <fp_types.h>

#include <stdio.h>

#define F_CFGV_BUILD_FULL_STRING        (a32 << 1)
#define F_CFGV_RETURN_MDA_OBJECT        (a32 << 2)
#define F_CFGV_RETURN_TOKEN_EX          (a32 << 3)
#define F_CFGV_BUILD_DATA_PATH          (a32 << 10)

#define F_CFGV_MODES                            (F_CFGV_BUILD_FULL_STRING|F_CFGV_RETURN_MDA_OBJECT|F_CFGV_RETURN_TOKEN_EX)

#define MAX_CFGV_RES_LENGTH                     50000

#define LCFG_MAX_LOADED 0x200
#define LCFG_MAX_LOAD_LINES 10000
#define LCFG_MAX_LINE_SIZE      16384

#define F_LCONF_NORF                    0x1
#define MSG_INIT_PATH_OVERR             "NOTICE: %s path set to '%s'\n"
#define MSG_INIT_CMDLINE_ERROR          "ERROR: [%d] processing command line arguments failed\n"


typedef struct g_cfg_ref
{
  mda cfg;
  char file[PATH_MAX];
} cfg_r, *p_cfg_r;

typedef struct config_header
{
  char *key, *value;
  mda data;
} cfg_h, *p_cfg_h;


char *
g_rtval_ex(char *arg, char *match, size_t max_size, char *output,
    uint32_t flags);
void *
ref_to_val_get_cfgval(char *cfg, char *key, char *defpath, int flags, char *out,
    size_t max);

int
load_cfg(pmda pmd, char *file, uint32_t flags, pmda *res);
void
free_cfg(pmda md);
p_md_obj
get_cfg_opt(char *key, pmda md, pmda *ret);

__d_cfg search_cfg_rf, register_cfg_rf;

int
free_cfg_rf(pmda md);

#endif /* CFGV_H_ */
