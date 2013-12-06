/*
 * lref_gen1.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_GEN1_H_
#define LREF_GEN1_H_

#include <fp_types.h>

__d_format_block gen1_format_block, gen1_format_block_batch,
    gen1_format_block_exp;

__d_ref_to_pval ref_to_val_ptr_gen1;

_d_rtv_lk ref_to_val_lk_gen1;

int
gcb_gen1(void *buffer, char *key, char *val);

#pragma pack(push, 4)

typedef struct ___d_generic_s2044
{
  uint32_t i32;
  char s_1[255];
  char s_2[255];
  char s_3[255];
  char s_4[255];
  char s_5[255];
  char s_6[255];
  char s_7[255];
  char s_8[255];
} _d_generic_s2044, *__d_generic_s2044;

#pragma pack(pop)


#define G1_SZ                           sizeof(_d_generic_s2044)


#endif /* LREF_GEN1_H_ */
