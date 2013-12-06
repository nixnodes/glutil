/*
 * lref_gen1.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include "lref_gen1.h"
#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <l_sb.h>

#include <errno.h>

int
gen1_format_block(void *iarg, char *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) iarg;

  return print_str("GENERIC1\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen1_format_block_batch(void *iarg, char *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) iarg;

  return printf("GENERIC1\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen1_format_block_exp(void *iarg, char *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) iarg;

  return printf("i32 %u\n"
      "ge1 %s\n"
      "ge2 %s\n"
      "ge3 %s\n"
      "ge4 %s\n"
      "ge5 %s\n"
      "ge6 %s\n"
      "ge7 %s\n"
      "ge8 %s\n\n",
      data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

void *
ref_to_val_ptr_gen1(void *arg, char *match, int *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) arg;

  if (!strncmp(match, "i32", 3))
    {
      *output = sizeof(data->i32);
      return &data->i32;
    }

  return NULL;
}


char *
dt_rval_gen1_i32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s2044) arg)->i32);
  return output;
}

char *
dt_rval_gen1_ge1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_1;
}

char *
dt_rval_gen1_ge2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_2;
}

char *
dt_rval_gen1_ge3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_3;
}

char *
dt_rval_gen1_ge4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_4;
}

char *
dt_rval_gen1_ge5(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_5;
}

char *
dt_rval_gen1_ge6(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_6;
}

char *
dt_rval_gen1_ge7(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_7;
}

char *
dt_rval_gen1_ge8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_8;
}

void *
ref_to_val_lk_gen1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, "i32", 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_i32, (__d_drt_h ) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_GE1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge1, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge2, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE3, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge3, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE4, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge4, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE5, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge5, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE6, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge6, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE7, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge7, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE8, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen1_ge8, (__d_drt_h ) mppd, "%s");
    }

  return NULL;
}



int
gcb_gen1(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_generic_s2044 ptr = (__d_generic_s2044) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GE_GE1, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_1, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE2, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_2, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE3, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_3, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE4, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_4, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE5, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_5, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE6, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_6, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE7, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_7, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE8, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_8, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, "i32", 3))
    {
      uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32 = v_ui;
      return 1;
    }

  return 0;
}
