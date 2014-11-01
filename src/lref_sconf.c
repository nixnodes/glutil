/*
 * lref_sconf.c
 *
 *  Created on: Dec 7, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"
#include "lref_sconf.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <l_sb.h>
#include <omfp.h>

#include <errno.h>

void
dt_set_sconf(__g_handle hdl)
{
  hdl->flags |= F_GH_ISSCONF;
  hdl->block_sz = SC_SZ;
  hdl->d_memb = 3;
  hdl->g_proc0 = gcb_sconf;
  hdl->g_proc1_lookup = ref_to_val_lk_sconf;
  hdl->g_proc2 = ref_to_val_ptr_sconf;
  hdl->g_proc3 = sconf_format_block;
  hdl->g_proc3_batch = sconf_format_block_batch;
  hdl->g_proc3_export = sconf_format_block_exp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_SCONFLOG;
  hdl->jm_offset = (size_t) &((__d_sconf) NULL)->field;
}

int
sconf_format_block(void *iarg, char *output)
{
  __d_sconf data = (__d_sconf) iarg;

  return print_str("SCONF\x9%u\x9%u\x9%hhu\x9%hhu\x9%hhu\x9%hhu\x9%d\x9%llu\x9%s\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->invert, data->type, data->icase, data->lcomp, data->i32,
      (ulint64_t)data->ui64, data->field , data->match, data->message);
}

int
sconf_format_block_batch(void *iarg, char *output)
{
  __d_sconf data = (__d_sconf) iarg;

  return printf("SCONF\x9%u\x9%u\x9%hhu\x9%hhu\x9%hhu\x9%hhu\x9%d\x9%llu\x9%s\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->invert, data->type, data->icase, data->lcomp, data->i32,
      (ulint64_t)data->ui64, data->field , data->match, data->message);
}

int
sconf_format_block_exp(void *iarg, char *output)
{
  __d_sconf data = (__d_sconf) iarg;

  return printf("u1 %u\n"
      "u2 %u\n"
      "invert %hhu\n"
      "type %hhu\n"
      "icase %hhu\n"
      "lcomp %hhu\n"
      "int %d\n"
      "uint64 %llu\n"
      "field %s\n"
      "match %s\n"
      "msg %s\n\n"
      , data->ui32_1, data->ui32_2, data->invert, data->type, data->icase, data->lcomp, data->i32,
      (ulint64_t)data->ui64, data->field , data->match, data->message);
}

void *
ref_to_val_ptr_sconf(void *arg, char *match, int *output)
{
  __d_sconf data = (__d_sconf) arg;
  if (!strncmp(match, _MC_SCONF_INT32, 3))
    {
      *output = ~((int) sizeof(data->i32));
      return &data->i32;
    }
  else if (!strncmp(match, _MC_SCONF_UINT64, 6))
    {
      *output = sizeof(data->ui64);
      return &data->ui64;
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_1;
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      *output = sizeof(data->ui32_2);
      return &data->ui32_2;
    }
  else if (!strncmp(match, _MC_SCONF_INVERTM, 6))
    {
      *output = sizeof(data->invert);
      return &data->invert;
    }
  else if (!strncmp(match, _MC_SCONF_TYPE, 4))
    {
      *output = sizeof(data->type);
      return &data->type;
    }
  else if (!strncmp(match, _MC_SCONF_LCOMP, 5))
    {
      *output = sizeof(data->type);
      return &data->type;
    }
  else if (!strncmp(match, _MC_SCONF_ICASE, 5))
    {
      *output = sizeof(data->icase);
      return &data->icase;
    }
  return NULL;
}

char *
dt_rval_sconf_int(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->i32);
  return output;
}

char *
dt_rval_sconf_uint64(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->ui64);
  return output;
}

char *
dt_rval_sconf_ui1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->ui32_1);
  return output;
}

char *
dt_rval_sconf_ui2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->ui32_2);
  return output;
}
char *
dt_rval_sconf_ui3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->invert);
  return output;
}

char *
dt_rval_sconf_ui4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->type);
  return output;
}

char *
dt_rval_sconf_lcomp(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->lcomp);
  return output;
}

char *
dt_rval_sconf_icase(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->icase);
  return output;
}

char *
dt_rval_sconf_match(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_sconf) arg)->match;
}

char *
dt_rval_sconf_field(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_sconf) arg)->field;
}

char *
dt_rval_gconf_msg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_sconf) arg)->message;
}

void *
ref_to_val_lk_sconf(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  PROC_SH_EX(match)

  void *ptr;
  if ((ptr = ref_to_val_lk_generic(arg, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_SCONF_INT32, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_int, (__d_drt_h) mppd, "%d");
    }
  else if (!strncmp(match, _MC_SCONF_UINT64, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_uint64, (__d_drt_h) mppd, "%lu");
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_ui1, (__d_drt_h) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_ui2 , (__d_drt_h) mppd, "%u");
    }
  else if (!strncmp(match, _MC_SCONF_INVERTM, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_ui3, (__d_drt_h) mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_SCONF_TYPE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_ui4, (__d_drt_h) mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_SCONF_MATCH, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_match, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_SCONF_MSG, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gconf_msg, (__d_drt_h) mppd, "%s");
    }
  else if (!strncmp(match, _MC_SCONF_FIELD, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_field, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_SCONF_LCOMP, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_lcomp, (__d_drt_h ) mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_SCONF_ICASE, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_icase, (__d_drt_h ) mppd, "%hhu");
    }

  return NULL;
}

int
gcb_sconf(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_sconf ptr = (__d_sconf) buffer;
  errno = 0;

  if (k_l == 5 && !strncmp(key, _MC_SCONF_MATCH, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->match, val, v_l > 4095 ? 4095 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_SCONF_FIELD, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return -1;
        }
      memcpy(ptr->field, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_SCONF_MSG, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return -1;
        }
      memcpy(ptr->message, val, v_l >= SCONF_MAX_MSG ? SCONF_MAX_MSG - 1 : v_l);
      return -1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_SCONF_INT32, 3))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32 = v_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_SCONF_UINT64, 6))
    {
      uint64_t v_ui = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->ui64 = v_ui;
      return -1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U1, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->ui32_1 = v_ui;
      return -1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U2, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->ui32_2 = v_ui;
      return -1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_SCONF_INVERTM, 6))
    {
      int8_t v_ui = (int8_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->invert = v_ui;
      return -1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_SCONF_TYPE, 4))
    {
      int8_t v_ui = (int8_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->type = v_ui;
      return -1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_SCONF_LCOMP, 5))
    {
      int8_t v_ui = (int8_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->lcomp = v_ui;
      return -1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_SCONF_ICASE, 5))
    {
      int8_t v_ui = (int8_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return -1;
        }
      ptr->icase = v_ui;
      return -1;
    }

  return 0;
}
