/*
 * lref_gconf.c
 *
 *  Created on: Dec 10, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_gconf.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <l_sb.h>

#include <errno.h>

int
gconf_format_block(void *iarg, char *output)
{
  __d_gconf data = (__d_gconf) iarg;

  return print_str("GCONF\x9%s\x9%s\x9%s\x9%s\x9%hhu\n",
      data->r_clean, data->r_postproc, data->r_yearm, data->r_sects, data->o_use_shared_mem);

}

int
gconf_format_block_batch(void *iarg, char *output)
{
  __d_gconf data = (__d_gconf) iarg;

  return printf("GCONF\x9%s\x9%s\x9%s\x9%s\x9%hhu\n",
      data->r_clean, data->r_postproc, data->r_yearm, data->r_sects, data->o_use_shared_mem);

}

int
gconf_format_block_exp(void *iarg, char *output)
{
  __d_gconf data = (__d_gconf) iarg;

  return printf("r_clean %s\n"
      "r_postproc %s\n"
      "r_yearm %s\n"
      "r_sects %s\n"
      "o_use_shared_mem %hhu\n"
      , data->r_clean, data->r_postproc, data->r_yearm, data->r_sects,data->o_use_shared_mem);

}

void *
ref_to_val_ptr_gconf(void *arg, char *match, int *output)
{
  __d_gconf data = (__d_gconf) arg;
  if (!strncmp(match, _MC_GCONF_O_SHM, 3))
    {
      *output = ~((uint8_t) sizeof(data->o_use_shared_mem));
      return &data->o_use_shared_mem;
    }

  return NULL;
}

char *
dt_rval_gconf_shm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_gconf) arg)->o_use_shared_mem);
  return output;
}

char *
dt_rval_gconf_r_clean(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_gconf) arg)->r_clean;
}

char *
dt_rval_gconf_r_postproc(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_gconf) arg)->r_postproc;
}

char *
dt_rval_gconf_r_yearm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_gconf) arg)->r_yearm;
}

char *
dt_rval_gconf_r_sects(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_gconf) arg)->r_sects;
}

void *
ref_to_val_lk_gconf(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GCONF_R_CLEAN, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_gconf_r_clean, (__d_drt_h) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GCONF_R_POSTPROC, 10))
    {
      return as_ref_to_val_lk(match, dt_rval_gconf_r_postproc, (__d_drt_h) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GCONF_R_YEARM, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_gconf_r_yearm, (__d_drt_h) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GCONF_R_SECTS, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_gconf_r_sects, (__d_drt_h) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GCONF_O_SHM, 12))
    {
      return as_ref_to_val_lk(match, dt_rval_gconf_shm , (__d_drt_h) mppd, "%hhu");
    }

  return NULL;
}

int
gcb_gconf(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_gconf ptr = (__d_gconf) buffer;
  errno = 0;

  if (k_l == 7 && !strncmp(key, _MC_GCONF_R_CLEAN, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->r_clean, val,
          v_l >= GCONF_MAX_REG_EXPR ? GCONF_MAX_REG_EXPR - 1 : v_l);
      return 1;
    }
  else if (k_l == 10 && !strncmp(key, _MC_GCONF_R_POSTPROC, 10))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->r_postproc, val,
          v_l >= GCONF_MAX_REG_EXPR ? GCONF_MAX_REG_EXPR - 1 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_GCONF_R_YEARM, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->r_yearm, val,
          v_l >= GCONF_MAX_REG_EXPR ? GCONF_MAX_REG_EXPR - 1 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_GCONF_R_SECTS, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->r_sects, val,
          v_l >= GCONF_MAX_REG_EXPR ? GCONF_MAX_REG_EXPR - 1 : v_l);
      return 1;
    }
  else if (k_l == 12 && !strncmp(key, _MC_GCONF_O_SHM, 12))
    {
      uint8_t v_ui = (uint8_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->o_use_shared_mem = v_ui;
      return 1;
    }

  return 0;
}
