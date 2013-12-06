/*
 * lref_game.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef LREF_GAME_H_
#define LREF_GAME_H_

#pragma pack(push, 4)

#include <fp_types.h>

typedef struct ___d_game
{
  char dirname[255];
  int32_t timestamp;
  float rating;
  /* ------------- */
  uint8_t _d_unused[512]; /* Reserved for future use */
} _d_game, *__d_game;

#define GM_SZ                           sizeof(_d_game)

#pragma pack(pop)

__d_ref_to_pval ref_to_val_ptr_game;
_d_rtv_lk ref_to_val_lk_game;

__d_format_block game_format_block, game_format_block_batch,
    game_format_block_exp;

__g_proc_rv dt_rval_game_score, dt_rval_game_time, dt_rval_game_mode,
    dt_rval_game_basedir, dt_rval_game_dir;

int
gcb_game(void *buffer, char *key, char *val);

#endif /* LREF_GAME_H_ */
