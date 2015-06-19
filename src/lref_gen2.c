/*
 * lref_gen2.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"
#include "lref_gen2.h"

#include <str.h>
#include <lref.h>
#include <lref_gen.h>
#include <l_sb.h>
#include <omfp.h>

#include <errno.h>

void
dt_set_gen2(__g_handle hdl)
{
  hdl->flags |= F_GH_ISGENERIC2;
  hdl->block_sz = G2_SZ;
  hdl->d_memb = 1;
  hdl->g_proc0 = gcb_gen2;
  hdl->g_proc1_lookup = ref_to_val_lk_gen2;
  hdl->g_proc2 = ref_to_val_ptr_gen2;
  hdl->g_proc3 = gen2_format_block;
  hdl->g_proc3_batch = gen2_format_block_batch;
  hdl->g_proc3_export = gen2_format_block_exp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_GEN2LOG;
  hdl->jm_offset = (size_t) &((__d_generic_s1644) NULL)->s_1;
}

int
gen2_format_block(void *iarg, char *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) iarg;

  return print_str("GENERIC2\x9%llu\x9%llu\x9%llu\x9%llu\x9%f\x9%f\x9%f\x9%f\x9%d\x9%d\x9%d\x9%d\x9%u\x9%u\x9%u\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      (ulint64_t)data->ui64_1,(ulint64_t)data->ui64_2,(ulint64_t)data->ui64_3,(ulint64_t)data->ui64_4, data->f_1,data->f_2,data->f_3,data->f_4,
      data->i32_1, data->i32_2, data->i32_3, data->i32_4,data->ui32_1,
      data->ui32_2, data->ui32_3, data->ui32_4, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen2_format_block_batch(void *iarg, char *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) iarg;

  return printf("GENERIC2\x9%llu\x9%llu\x9%llu\x9%llu\x9%f\x9%f\x9%f\x9%f\x9%d\x9%d\x9%d\x9%d\x9%u\x9%u\x9%u\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      (ulint64_t)data->ui64_1,(ulint64_t)data->ui64_2,(ulint64_t)data->ui64_3,(ulint64_t)data->ui64_4, data->f_1,data->f_2,data->f_3,data->f_4,
      data->i32_1, data->i32_2, data->i32_3, data->i32_4,data->ui32_1,
      data->ui32_2, data->ui32_3, data->ui32_4, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen2_format_block_exp(void *iarg, char *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) iarg;

  return printf("ul1 %llu\n"
      "ul2 %llu\n"
      "ul3 %llu\n"
      "ul4 %llu\n"
      "f1 %f\n"
      "f2 %f\n"
      "f3 %f\n"
      "f4 %f\n"
      "i1 %d\n"
      "i2 %d\n"
      "i3 %d\n"
      "i4 %d\n"
      "u1 %u\n"
      "u2 %u\n"
      "u3 %u\n"
      "u4 %u\n"
      "ge1 %s\n"
      "ge2 %s\n"
      "ge3 %s\n"
      "ge4 %s\n"
      "ge5 %s\n"
      "ge6 %s\n"
      "ge7 %s\n"
      "ge8 %s\n\n",
      (ulint64_t)data->ui64_1,(ulint64_t)data->ui64_2,(ulint64_t)data->ui64_3,(ulint64_t)data->ui64_4, data->f_1,data->f_2,data->f_3,data->f_4,
      data->i32_1, data->i32_2, data->i32_3, data->i32_4,data->ui32_1,
      data->ui32_2, data->ui32_3, data->ui32_4, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);
}


void *
ref_to_val_ptr_gen2_e(void *arg, char *match, int *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) arg;
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
  else if (!strncmp(match, _MC_GE_I3, 2))
    {
      *output = ~((int) sizeof(data->i32_3));
      return &data->i32_3;
    }
  else if (!strncmp(match, _MC_GE_I4, 2))
    {
      *output = ~((int) sizeof(data->i32_4));
      return &data->i32_4;
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
  else if (!strncmp(match, _MC_GE_U3, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_3;
    }
  else if (!strncmp(match, _MC_GE_U4, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_4;
    }
  else if (!strncmp(match, _MC_GE_F1, 2))
    {
      *output = -32;
      return &data->f_1;
    }
  else if (!strncmp(match, _MC_GE_F2, 2))
    {
      *output = -32;
      return &data->f_2;
    }
  else if (!strncmp(match, _MC_GE_F3, 2))
    {
      *output = -32;
      return &data->f_3;
    }
  else if (!strncmp(match, _MC_GE_F4, 2))
    {
      *output = -32;
      return &data->f_4;
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
  else if (!strncmp(match, _MC_GE_UL3, 3))
    {
      *output = sizeof(data->ui64_3);
      return &data->ui64_3;
    }
  else if (!strncmp(match, _MC_GE_UL4, 3))
    {
      *output = sizeof(data->ui64_4);
      return &data->ui64_4;
    }
  return NULL;
}

void *
ref_to_val_ptr_gen2(void *arg, char *match, int *output)
{
  REF_TO_VAL_RESOLVE(arg, match, output, ref_to_val_ptr_gen2_e)
}

char *
dt_rval_gen2_i1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->i32_1);
  return output;
}

char *
dt_rval_gen2_i2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->i32_2);
  return output;
}

char *
dt_rval_gen2_i3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->i32_3);
  return output;
}

char *
dt_rval_gen2_i4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->i32_4);
  return output;
}

char *
dt_rval_gen2_ui1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->ui32_1);
  return output;
}

char *
dt_rval_gen2_ui2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->ui32_2);
  return output;
}

char *
dt_rval_gen2_ui3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->ui32_3);
  return output;
}

char *
dt_rval_gen2_ui4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->ui32_4);
  return output;
}

char *
dt_rval_gen2_uli1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((__d_generic_s1644) arg)->ui64_1);
  return output;
}

char *
dt_rval_gen2_uli2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((__d_generic_s1644) arg)->ui64_2);
  return output;
}

char *
dt_rval_gen2_uli3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((__d_generic_s1644) arg)->ui64_3);
  return output;
}

char *
dt_rval_gen2_uli4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((__d_generic_s1644) arg)->ui64_4);
  return output;
}

char *
dt_rval_gen2_f1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->f_1);
  return output;
}

char *
dt_rval_gen2_f2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->f_2);
  return output;
}

char *
dt_rval_gen2_f3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->f_3);
  return output;
}

char *
dt_rval_gen2_f4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_generic_s1644) arg)->f_4);
  return output;
}

char *
dt_rval_gen2_ge1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_1;
}

char *
dt_rval_gen2_ge2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_2;
}

char *
dt_rval_gen2_ge3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_3;
}

char *
dt_rval_gen2_ge4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_4;
}

char *
dt_rval_gen2_ge5(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_5;
}

char *
dt_rval_gen2_ge6(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_6;
}

char *
dt_rval_gen2_ge7(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_7;
}

char *
dt_rval_gen2_ge8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_8;
}

void *
ref_to_val_lk_gen2(void *arg, char *match, char *output, size_t max_size,
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
      return as_ref_to_val_lk(match, dt_rval_gen2_i1, (__d_drt_h ) mppd, "%d");
      return dt_rval_gen2_i1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_i2, (__d_drt_h ) mppd, "%d");
      return dt_rval_gen2_i2;
    }
  else if (!strncmp(match, _MC_GE_I3, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_i3, (__d_drt_h ) mppd, "%d");
    }
  else if (!strncmp(match, _MC_GE_I4, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_i4, (__d_drt_h ) mppd, "%d");
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ui1, (__d_drt_h ) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ui2, (__d_drt_h ) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_U3, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ui3, (__d_drt_h ) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_U4, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ui4, (__d_drt_h ) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_F1, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_f1, (__d_drt_h ) mppd, "%f");
    }
  else if (!strncmp(match, _MC_GE_F2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_f2, (__d_drt_h ) mppd, "%f");
    }
  else if (!strncmp(match, _MC_GE_F3, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_f3, (__d_drt_h ) mppd, "%f");
    }
  else if (!strncmp(match, _MC_GE_F4, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_f4, (__d_drt_h ) mppd, "%f");
    }
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_uli1, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_uli2, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_GE_UL3, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_uli3, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_GE_UL4, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_uli4, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_GE_GE1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge1, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge2, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE3, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge3, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE4, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge4, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE5, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge5, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE6, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge6, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE7, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge7, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GE_GE8, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen2_ge8, (__d_drt_h ) mppd, "%s");
    }

  return NULL;
}


int
gcb_gen2(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_generic_s1644 ptr = (__d_generic_s1644) buffer;
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
      memcpy(ptr->s_5, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE6, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_6, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE7, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_7, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE8, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_8, val, v_l > 127 ? 127 : v_l);
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
  else if (k_l == 2 && !strncmp(key, _MC_GE_I3, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_3 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I4, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_4 = v_ui;
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
  else if (k_l == 2 && !strncmp(key, _MC_GE_U3, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_3 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U4, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_4 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F1, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_1 = v_f;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F2, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_2 = v_f;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F3, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_3 = v_f;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F4, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_4 = v_f;
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
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL3, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_3 = v_ul;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL4, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_4 = v_ul;
      return 1;
    }
  return 0;
}



