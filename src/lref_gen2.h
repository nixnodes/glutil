/*
 * lref_gen2.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_GEN2_H_
#define LREF_GEN2_H_

#include <fp_types.h>

#include <stdint.h>
#include <inttypes.h>

__d_format_block gen2_format_block, gen2_format_block_batch,
    gen2_format_block_exp;

__d_ref_to_pval ref_to_val_ptr_gen2;

_d_rtv_lk ref_to_val_lk_gen2;

int
gcb_gen2 (void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_generic_s1644
{
  uint32_t ui32_1;
  uint32_t ui32_2;
  uint32_t ui32_3;
  uint32_t ui32_4;
  int32_t i32_1;
  int32_t i32_2;
  int32_t i32_3;
  int32_t i32_4;
  float f_1;
  float f_2;
  float f_3;
  float f_4;
  uint8_t _d_unused_1[32];
  uint64_t ui64_1;
  uint64_t ui64_2;
  uint64_t ui64_3;
  uint64_t ui64_4;
  char s_1[255];
  char s_2[255];
  char s_3[255];
  char s_4[255];
  char s_5[128];
  char s_6[128];
  char s_7[128];
  char s_8[128];
} _d_generic_s1644, *__d_generic_s1644;

#pragma pack(pop)

#define G2_SZ                           sizeof(_d_generic_s1644)

#endif /* LREF_GEN2_H_ */
