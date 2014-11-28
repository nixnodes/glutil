/*
 * lref_gen4.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"
#include "lref_gen4.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <l_sb.h>
#include <omfp.h>

#include <errno.h>

void
dt_set_gen4(__g_handle hdl)
{
  hdl->flags |= F_GH_ISGENERIC4;
  hdl->block_sz = G4_SZ;
  hdl->d_memb = 1;
  hdl->g_proc0 = gcb_gen4;
  hdl->g_proc1_lookup = ref_to_val_lk_gen4;
  hdl->g_proc2 = ref_to_val_ptr_gen4;
  hdl->g_proc3 = gen4_format_block;
  hdl->g_proc3_batch = gen4_format_block_batch;
  hdl->g_proc3_export = gen4_format_block_exp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_GEN4LOG;
  hdl->jm_offset = (size_t) &((__d_generic_s4640) NULL)->s_1;
}

int
gen4_format_block(void *iarg, char *output)
{
  __d_generic_s4640 data = (__d_generic_s4640) iarg;

  return print_str("GENERIC3\x9%u\x9%u\x9%s\x9%s\x9%d\x9%d\x9%llu\x9%llu\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->s_1, data->s_2, data->i32_1,
      data->i32_2, (ulint64_t)data->ui64_1, (ulint64_t)data->ui64_2, data->s_3, data->s_4);

}

int
gen4_format_block_batch(void *iarg, char *output)
{
  __d_generic_s4640 data = (__d_generic_s4640) iarg;

  return printf("GENERIC3\x9%u\x9%u\x9%s\x9%s\x9%d\x9%d\x9%llu\x9%llu\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->s_1, data->s_2, data->i32_1,
      data->i32_2, (ulint64_t)data->ui64_1, (ulint64_t)data->ui64_2, data->s_3, data->s_4);

}

int
gen4_format_block_exp(void *iarg, char *output)
{
  __d_generic_s4640 data = (__d_generic_s4640) iarg;

  return printf("u1 %u\n"
      "u2 %u\n"
      "ge1 %s\n"
      "ge2 %s\n"
      "i1 %d\n"
      "i2 %d\n"
      "ul1 %llu\n"
      "ul2 %llu\n"
      "ge3 %s\n"
      "ge4 %s\n\n"
      , data->ui32_1, data->ui32_2, data->s_1, data->s_2, data->i32_1,
      data->i32_2, (ulint64_t)data->ui64_1, (ulint64_t)data->ui64_2, data->s_3, data->s_4);

}


void *
ref_to_val_ptr_gen4(void *arg, char *match, int *output)
{
  __d_generic_s4640 data = (__d_generic_s4640) arg;
  if (!strncmp(match, _MC_GE_I1, 2))
    {
      *output = ~((int) sizeof(data->i32_1));
      return &data->i32_1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      *output = ~((int) sizeof(data->i32_2));
      return &data->i32_2;
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
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      *output = sizeof(data->ui64_1);
      return &data->ui64_1;
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      *output = sizeof(data->ui64_2);
      return &data->ui64_2;
    }

  return NULL;
}


char *
dt_rval_gen4_i1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s4640) arg)->i32_1);
  return output;
}

char *
dt_rval_gen4_i2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s4640) arg)->i32_2);
  return output;
}

char *
dt_rval_gen4_ui1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s4640) arg)->ui32_1);
  return output;
}

char *
dt_rval_gen4_ui2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s4640) arg)->ui32_2);
  return output;
}

char *
dt_rval_gen4_uli1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((__d_generic_s4640) arg)->ui64_1);
  return output;
}

char *
dt_rval_gen4_uli2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((__d_generic_s4640) arg)->ui64_2);
  return output;
}

char *
dt_rval_gen4_ge1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s4640) arg)->s_1;
}

char *
dt_rval_gen4_ge2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s4640) arg)->s_2;
}

char *
dt_rval_gen4_ge3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s4640) arg)->s_3;
}

char *
dt_rval_gen4_ge4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s4640) arg)->s_4;
}

void *
ref_to_val_lk_gen4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  PROC_SH_EX(match)

  void *ptr;
  if ((ptr = ref_to_val_lk_generic(arg, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GE_I1, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_i1, (__d_drt_h) mppd, "%d");
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_i2, (__d_drt_h) mppd, "%d");
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_ui1, (__d_drt_h) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_ui2 , (__d_drt_h) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_uli1, (__d_drt_h) mppd, "%llu");
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_uli2, (__d_drt_h) mppd, "%llu");
    }
  else if (!strncmp(match, _MC_GE_GE1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_ge1, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_ge2, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE3, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_ge3, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE4, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen4_ge4, (__d_drt_h ) mppd, "%s");
    }

  return NULL;
}


int
gcb_gen4(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_generic_s4640 ptr = (__d_generic_s4640) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GE_GE1, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_1, val, v_l > 4095 ? 4095 : v_l);
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
  else if (k_l == 2 && !strncmp(key, _MC_GE_I1, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_1 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I2, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_2 = v_ui;
      return 1;
    }

  else if (k_l == 2 && !strncmp(key, _MC_GE_U1, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_1 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U2, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_2 = v_ui;
      return 1;
    }

  else if (k_l == 3 && !strncmp(key, _MC_GE_UL1, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_1 = v_ul;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL2, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_2 = v_ul;
      return 1;
    }

  return 0;
}
