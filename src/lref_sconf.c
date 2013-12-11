/*
 * lref_sconf.c
 *
 *  Created on: Dec 7, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lref_sconf.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <l_sb.h>

#include <errno.h>

int
sconf_format_block(void *iarg, char *output)
{
  __d_sconf data = (__d_sconf) iarg;

  return print_str("SCONF\x9%u\x9%u\x9%d\x9%d\x9%d\x9%lu\x9%s\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->i32_1, data->i32_2, data->i32,
      data->ui64, data->field , data->match, data->message);

}

int
sconf_format_block_batch(void *iarg, char *output)
{
  __d_sconf data = (__d_sconf) iarg;

  return printf("SCONF\x9%u\x9%u\x9%d\x9%d\x9%d\x9%lu\x9%s\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->i32_1, data->i32_2, data->i32,
      data->ui64, data->field , data->match, data->message);

}

int
sconf_format_block_exp(void *iarg, char *output)
{
  __d_sconf data = (__d_sconf) iarg;

  return printf("u1 %u\n"
      "u2 %u\n"
      "i1 %d\n"
      "i2 %d\n"
      "int %d\n"
      "uint64 %llu\n"
      "field %s\n"
      "match %s\n\n"
      "msg %s\n"
      , data->ui32_1, data->ui32_2, data->i32_1, data->i32_2, data->i32,
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
      *output = sizeof(data->ui32_1);
      return &data->ui32_2;
    }
  else if (!strncmp(match, _MC_GE_I1, 2))
    {
      *output = sizeof(data->i32_1);
      return &data->ui32_1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      *output = sizeof(data->i32_2);
      return &data->ui32_2;
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
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->i32_1);
  return output;
}

char *
dt_rval_sconf_ui4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_sconf) arg)->i32_2);
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
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
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
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_ui3, (__d_drt_h) mppd, "%lu");
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_sconf_ui4, (__d_drt_h) mppd, "%lu");
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
          return 0;
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
          return 0;
        }
      ptr->ui64 = v_ui;
      return -1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U1, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_1 = v_ui;
      return -1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U2, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_2 = v_ui;
      return -1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I1, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_1 = v_ui;
      return -1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I2, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 1;
        }
      ptr->i32_2 = v_ui;
      return -1;
    }

  return 0;
}
