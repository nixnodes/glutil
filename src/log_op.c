/*
 * log_op.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <t_glob.h>

#include "log_op.h"

#include <l_sb.h>
#include <m_general.h>
#include <omfp.h>
#include <memory_t.h>
#include <exec_t.h>
#include <str.h>
#include <l_error.h>
#include <m_string.h>
#include <m_lom.h>
#include <exech.h>
#include <x_f.h>
#include <log_io.h>

#include <lref_dirlog.h>
#include <lref_nukelog.h>
#include <lref_lastonlog.h>
#include <lref_dupefile.h>
#include <lref_onliners.h>
#include <lref_online.h>
#include <lref_tvrage.h>
#include <lref_imdb.h>
#include <lref_game.h>
#include <lref_gen1.h>
#include <lref_gen2.h>
#include <lref_gen3.h>
#include <lref_gen4.h>

#include <unistd.h>

#include "glconf.h"

char *
g_dgetf(char *str)
{
  if (!str)
    {
      return NULL;
    }
  if (!strncmp(str, "dirlog", 6))
    {
      return DIRLOG;
    }
  else if (!strncmp(str, "nukelog", 7))
    {
      return NUKELOG;
    }
  else if (!strncmp(str, "dupefile", 8))
    {
      return DUPEFILE;
    }
  else if (!strncmp(str, "lastonlog", 9))
    {
      return LASTONLOG;
    }
  else if (!strncmp(str, "oneliners", 9))
    {
      return ONELINERS;
    }
  else if (!strncmp(str, "imdb", 4))
    {
      return IMDBLOG;
    }
  else if (!strncmp(str, "game", 4))
    {
      return GAMELOG;
    }
  else if (!strncmp(str, "tvrage", 6))
    {
      return TVLOG;
    }
  else if (!strncmp(str, "ge1", 3))
    {
      return GE1LOG;
    }
  else if (!strncmp(str, "ge2", 3))
    {
      return GE2LOG;
    }
  else if (!strncmp(str, "ge3", 3))
    {
      return GE3LOG;
    }
  else if (!strncmp(str, "ge4", 3))
    {
      return GE4LOG;
    }
  return NULL;
}


int
data_backup_records(char *file)
{
  g_setjmp(0, "data_backup_records", NULL, NULL);
  int r;
  off_t r_sz;

  if (!file)
    {
      print_str("ERROR: null argument passed (this is likely a bug)\n");
      return -1;
    }

  if ((gfl & F_OPT_NOWRITE) || (gfl & F_OPT_NOBACKUP))
    {
      return 0;
    }

  if (file_exists(file))
    {
      if (gfl & F_OPT_VERBOSE3)
        {
          print_str("WARNING: BACKUP: %s: data file doesn't exist\n", file);
        }
      return 0;
    }

  if (!(r_sz = get_file_size(file)))
    {
      if ((gfl & F_OPT_VERBOSE))
        {
          print_str("WARNING: %s: refusing to backup 0-byte data file\n", file);
        }
      return 0;
    }

  char buffer[PATH_MAX];

  snprintf(buffer, PATH_MAX, "%s.bk", file);

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("NOTICE: %s: creating data backup: %s ..\n", file, buffer);
    }

  if ((r = (int) file_copy(file, buffer, "wb", F_FC_MSET_SRC)) < 1)
    {
      print_str("ERROR: %s: [%d] failed to create backup %s\n", file, r,
          buffer);
      return r;
    }
  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: %s: created data backup: %s\n", file, buffer);
    }
  return 0;
}


int
find_absolute_path(char *exec, char *output)
{
  char *env = getenv("PATH");

  if (!env)
    {
      return 1;
    }

  mda s_p =
    { 0 };

  md_init(&s_p, 64);

  int p_c = split_string(env, 0x3A, &s_p);

  if (p_c < 1)
    {
      return 2;
    }

  p_md_obj ptr = md_first(&s_p);

  while (ptr)
    {
      snprintf(output, PATH_MAX, "%s/%s", (char*) ptr->ptr, exec);
      if (!access(output, R_OK | X_OK))
        {
          md_g_free(&s_p);
          return 0;
        }
      ptr = ptr->next;
    }

  md_g_free(&s_p);

  return 3;
}

int
g_proc_mr(__g_handle hdl)
{
  g_setjmp(0, "g_proc_mr", NULL, NULL);
  int r;

  if (!(gfl & F_OPT_PROCREV))
    {
      hdl->j_offset = 1;
      if (hdl->buffer.count)
        {
          hdl->buffer.r_pos = md_first(&hdl->buffer);
        }
    }
  else
    {
      if (hdl->buffer.count)
        {
          hdl->buffer.r_pos = md_last(&hdl->buffer);
        }
      hdl->j_offset = 2;
    }

  if (!(hdl->flags & F_GH_HASMATCHES))
    {
      if ((r = md_copy(&_match_rr, &hdl->_match_rr, sizeof(_g_match))))
        {
          print_str("ERROR: %s: could not copy matches to handle\n", hdl->file);
          return 2000;
        }
      if (hdl->_match_rr.offset)
        {
          if ((gfl & F_OPT_VERBOSE4))
            {
              print_str("NOTICE: %s: commit %llu matches to handle\n",
                  hdl->file, (ulint64_t) hdl->_match_rr.offset);
            }
          hdl->flags |= F_GH_HASMATCHES;
        }

    }

  if (gfl & F_OPT_HASMAXHIT)
    {
      hdl->max_hits = max_hits;
      hdl->flags |= F_GH_HASMAXHIT;
    }

  if (gfl & F_OPT_HASMAXRES)
    {
      hdl->max_results = max_results;
      hdl->flags |= F_GH_HASMAXRES;
    }

  if (gfl & F_OPT_IFIRSTRES)
    {
      hdl->flags |= F_GH_IFRES;
    }

  if (gfl & F_OPT_IFIRSTHIT)
    {
      hdl->flags |= F_GH_IFHIT;
    }

  if (!(gfl & F_OPT_IFRH_E))
    {
      hdl->ifrh_l0 = g_ipcbm;
    }
  else
    {
      hdl->ifrh_l1 = g_ipcbm;
    }

  if ((gfl & F_OPT_HAS_G_REGEX) || (gfl & F_OPT_HAS_G_MATCH))
    {
      if ((r = g_load_strm(hdl)))
        {
          return r;
        }
    }

  if ((gfl & F_OPT_HAS_G_LOM))
    {
      if ((r = g_load_lom(hdl)))
        {
          return r;
        }
    }

  if ((exec_v || exec_str))
    {
      if (!(hdl->flags & F_GH_HASEXC))
        {
          hdl->exec_args.exc = exc;

          if (!hdl->exec_args.exc)
            {
              print_str(
                  "ERROR: %s: no exec call pointer (this is probably a bug)\n",
                  hdl->file);
              return 2002;
            }
          hdl->flags |= F_GH_HASEXC;
        }
      int r;
      if (exec_v && !hdl->exec_args.argv)
        {
          hdl->exec_args.argv = exec_v;
          hdl->exec_args.argc = exec_vc;

          if (!hdl->exec_args.argc)
            {
              print_str("ERROR: %s: no exec arguments\n", hdl->file);
              return 2001;
            }

          if ((r = g_build_argv_c(hdl)))
            {
              print_str("ERROR: %s: [%d]: failed building exec arguments\n",
                  hdl->file, r);
              return 2005;
            }

          if ((r = find_absolute_path(hdl->exec_args.argv_c[0],
              hdl->exec_args.exec_v_path)))
            {
              if (gfl & F_OPT_VERBOSE2)
                {
                  print_str(
                      "WARNING: %s: [%d]: exec unable to get absolute path\n",
                      hdl->file, r);
                }
              snprintf(hdl->exec_args.exec_v_path, PATH_MAX, "%s",
                  hdl->exec_args.argv_c[0]);
            }
        }
      else if (!hdl->exec_args.mech.offset)
        {
          if ((r = g_compile_exech(&hdl->exec_args.mech, hdl, exec_str)))
            {
              print_str("ERROR: %s: [%d]: could not compile exec string\n",
                  hdl->file, r);
              return 2008;
            }
        }
    }

  if (gfl & F_OPT_MODE_RAWDUMP)
    {
      hdl->g_proc4 = g_omfp_raw;

    }
  else if (gfl & F_OPT_FORMAT_BATCH)
    {
      hdl->g_proc3 = hdl->g_proc3_batch;
    }
  else if ((gfl0 & F_OPT_PRINT) || (gfl0 & F_OPT_PRINTF))
    {
      if (!hdl->print_mech.offset && _print_ptr)
        {
          size_t pp_l = strlen(_print_ptr);

          if (!pp_l || _print_ptr[0] == 0xA)
            {
              print_str("ERROR: %s: empty -print(f) command\n", hdl->file);
              return 2010;
            }
          if (pp_l > MAX_EXEC_STR)
            {
              print_str("ERROR: %s: -print(f) string too large\n", hdl->file);
              return 2004;
            }
          if ((r = g_compile_exech(&hdl->print_mech, hdl, _print_ptr)))
            {
              print_str("ERROR: %s: [%d]: could not compile print string\n",
                  hdl->file, r);
              return 2009;
            }
        }
      if ((gfl0 & F_OPT_PRINTF))
        {
          hdl->g_proc4 = g_omfp_eassemblef;
        }
      else
        {
          hdl->g_proc4 = g_omfp_eassemble;
        }
    }
  else if ((hdl->flags & F_GH_ISONLINE) && (gfl & F_OPT_FORMAT_COMP))
    {
      hdl->g_proc4 = g_omfp_ocomp;
      hdl->g_proc3 = online_format_block_comp;
      print_str(
          "+-------------------------------------------------------------------------------------------------------------------------------------------\n"
              "|                     USER/HOST/PID                       |    TIME ONLINE     |    TRANSFER RATE      |        STATUS       \n"
              "|---------------------------------------------------------|--------------------|-----------------------|------------------------------------\n");
    }
  else if (gfl & F_OPT_FORMAT_EXPORT)
    {
      hdl->g_proc3 = hdl->g_proc3_export;
    }

  return 0;
}


int
determine_datatype(__g_handle hdl)
{
  if (!strncmp(hdl->file, DIRLOG, strlen(DIRLOG)))
    {
      hdl->flags |= F_GH_ISDIRLOG;
      hdl->block_sz = DL_SZ;
      hdl->d_memb = 7;
      hdl->g_proc0 = gcb_dirlog;
      hdl->g_proc1_lookup = ref_to_val_lk_dirlog;
      hdl->g_proc2 = ref_to_val_ptr_dirlog;
      hdl->g_proc3 = dirlog_format_block;
      hdl->g_proc3_batch = dirlog_format_block_batch;
      hdl->g_proc3_export = dirlog_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_DIRLOG;
      hdl->jm_offset = (size_t) &((struct dirlog*) NULL)->dirname;
    }
  else if (!strncmp(hdl->file, NUKELOG, strlen(NUKELOG)))
    {
      hdl->flags |= F_GH_ISNUKELOG;
      hdl->block_sz = NL_SZ;
      hdl->d_memb = 9;
      hdl->g_proc0 = gcb_nukelog;
      hdl->g_proc1_lookup = ref_to_val_lk_nukelog;
      hdl->g_proc2 = ref_to_val_ptr_nukelog;
      hdl->g_proc3 = nukelog_format_block;
      hdl->g_proc3_batch = nukelog_format_block_batch;
      hdl->g_proc3_export = nukelog_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_NUKELOG;
      hdl->jm_offset = (size_t) &((struct nukelog*) NULL)->dirname;
    }
  else if (!strncmp(hdl->file, DUPEFILE, strlen(DUPEFILE)))
    {
      hdl->flags |= F_GH_ISDUPEFILE;
      hdl->block_sz = DF_SZ;
      hdl->d_memb = 3;
      hdl->g_proc0 = gcb_dupefile;
      hdl->g_proc1_lookup = ref_to_val_lk_dupefile;
      hdl->g_proc2 = ref_to_val_ptr_dupefile;
      hdl->g_proc3 = dupefile_format_block;
      hdl->g_proc3_batch = dupefile_format_block_batch;
      hdl->g_proc3_export = dupefile_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_DUPEFILE;
      hdl->jm_offset = (size_t) &((struct dupefile*) NULL)->filename;
    }
  else if (!strncmp(hdl->file, LASTONLOG, strlen(LASTONLOG)))
    {
      hdl->flags |= F_GH_ISLASTONLOG;
      hdl->block_sz = LO_SZ;
      hdl->d_memb = 8;
      hdl->g_proc0 = gcb_lastonlog;
      hdl->g_proc1_lookup = ref_to_val_lk_lastonlog;
      hdl->g_proc2 = ref_to_val_ptr_lastonlog;
      hdl->g_proc3 = lastonlog_format_block;
      hdl->g_proc3_batch = lastonlog_format_block_batch;
      hdl->g_proc3_export = lastonlog_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_LASTONLOG;
      hdl->jm_offset = (size_t) &((struct lastonlog*) NULL)->uname;
    }
  else if (!strncmp(hdl->file, ONELINERS, strlen(ONELINERS)))
    {
      hdl->flags |= F_GH_ISONELINERS;
      hdl->block_sz = OL_SZ;
      hdl->d_memb = 5;
      hdl->g_proc0 = gcb_oneliner;
      hdl->g_proc1_lookup = ref_to_val_lk_oneliners;
      hdl->g_proc2 = ref_to_val_ptr_oneliners;
      hdl->g_proc3 = oneliner_format_block;
      hdl->g_proc3_batch = oneliner_format_block_batch;
      hdl->g_proc3_export = oneliner_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_ONELINERS;
      hdl->jm_offset = (size_t) &((struct oneliner*) NULL)->uname;
    }
  else if (!strncmp(hdl->file, IMDBLOG, strlen(IMDBLOG)))
    {
      hdl->flags |= F_GH_ISIMDB;
      hdl->block_sz = ID_SZ;
      hdl->d_memb = 14;
      hdl->g_proc0 = gcb_imdbh;
      hdl->g_proc1_lookup = ref_to_val_lk_imdb;
      hdl->g_proc2 = ref_to_val_ptr_imdb;
      hdl->g_proc3 = imdb_format_block;
      hdl->g_proc3_batch = imdb_format_block_batch;
      hdl->g_proc3_export = imdb_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_IMDBLOG;
      hdl->jm_offset = (size_t) &((__d_imdb) NULL)->dirname;
    }
  else if (!strncmp(hdl->file, GAMELOG, strlen(GAMELOG)))
    {
      hdl->flags |= F_GH_ISGAME;
      hdl->block_sz = GM_SZ;
      hdl->d_memb = 3;
      hdl->g_proc0 = gcb_game;
      hdl->g_proc1_lookup = ref_to_val_lk_game;
      hdl->g_proc2 = ref_to_val_ptr_game;
      hdl->g_proc3 = game_format_block;
      hdl->g_proc3_batch = game_format_block_batch;
      hdl->g_proc3_export = game_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_GAMELOG;
      hdl->jm_offset = (size_t) &((__d_game) NULL)->dirname;
    }
  else if (!strncmp(hdl->file, TVLOG, strlen(TVLOG)))
    {
      hdl->flags |= F_GH_ISTVRAGE;
      hdl->block_sz = TV_SZ;
      hdl->d_memb = 18;
      hdl->g_proc0 = gcb_tv;
      hdl->g_proc1_lookup = ref_to_val_lk_tvrage;
      hdl->g_proc2 = ref_to_val_ptr_tv;
      hdl->g_proc3 = tv_format_block;
      hdl->g_proc3_batch = tv_format_block_batch;
      hdl->g_proc3_export = tv_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_TVRAGELOG;
      hdl->jm_offset = (size_t) &((__d_tvrage) NULL)->dirname;
    }
  else if (!strncmp(hdl->file, GE1LOG, strlen(GE1LOG)))
    {
      hdl->flags |= F_GH_ISGENERIC1;
      hdl->block_sz = G1_SZ;
      hdl->d_memb = 9;
      hdl->g_proc0 = gcb_gen1;
      hdl->g_proc1_lookup = ref_to_val_lk_gen1;
      hdl->g_proc2 = ref_to_val_ptr_gen1;
      hdl->g_proc3 = gen1_format_block;
      hdl->g_proc3_batch = gen1_format_block_batch;
      hdl->g_proc3_export = gen1_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_GEN1LOG;
      hdl->jm_offset = (size_t) &((__d_generic_s2044) NULL)->s_1;
    }
  else if (!strncmp(hdl->file, GE2LOG, strlen(GE2LOG)))
    {
      hdl->flags |= F_GH_ISGENERIC2;
      hdl->block_sz = G2_SZ;
      hdl->d_memb = 24;
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
  else if (!strncmp(hdl->file, GE3LOG, strlen(GE3LOG)))
    {
      hdl->flags |= F_GH_ISGENERIC3;
      hdl->block_sz = G3_SZ;
      hdl->d_memb = 10;
      hdl->g_proc0 = gcb_gen3;
      hdl->g_proc1_lookup = ref_to_val_lk_gen3;
      hdl->g_proc2 = ref_to_val_ptr_gen3;
      hdl->g_proc3 = gen3_format_block;
      hdl->g_proc3_batch = gen3_format_block_batch;
      hdl->g_proc3_export = gen3_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_GEN3LOG;
      hdl->jm_offset = (size_t) &((__d_generic_s800) NULL)->s_1;
    }
  else if (!strncmp(hdl->file, GE4LOG, strlen(GE4LOG)))
    {
      hdl->flags |= F_GH_ISGENERIC4;
      hdl->block_sz = G4_SZ;
      hdl->d_memb = 10;
      hdl->g_proc0 = gcb_gen4;
      hdl->g_proc1_lookup = ref_to_val_lk_gen4;
      hdl->g_proc2 = ref_to_val_ptr_gen4;
      hdl->g_proc3 = gen4_format_block;
      hdl->g_proc3_batch = gen4_format_block_batch;
      hdl->g_proc3_export = gen4_format_block_exp;
      hdl->g_proc4 = g_omfp_norm;
      hdl->ipc_key = IPC_KEY_GEN4LOG;
      hdl->jm_offset = (size_t) &((__d_generic_s1644) NULL)->s_1;
    }
  else
    {
      return 1;
    }

  return 0;
}

int
d_gen_dump(char *arg)
{
  char *datafile = g_dgetf(arg);

  if (!datafile)
    {
      print_str(MSG_UNRECOGNIZED_DATA_TYPE, arg);
      return 2;
    }

  return g_print_stats(datafile, 0, 0);
}


int
rebuild(void *arg)
{
  g_setjmp(0, "rebuild", NULL, NULL);
  if (!arg)
    {
      print_str("ERROR: missing data type argument (-e <log>)\n");
      return 1;
    }

  char *a_ptr = (char*) arg;
  char *datafile = g_dgetf(a_ptr);

  if (!datafile)
    {
      print_str(MSG_UNRECOGNIZED_DATA_TYPE, a_ptr);
      return 2;
    }

  if (g_fopen(datafile, "r", F_DL_FOPEN_BUFFER, &g_act_1))
    {
      return 3;
    }

  if (!g_act_1.buffer.count)
    {
      print_str(
          "ERROR: data log rebuilding requires buffering, increase mem limit (or dump with --raw --nobuffer for huge files)\n");
      return 4;
    }

  int r;

  if ((r = rebuild_data_file(datafile, &g_act_1)))
    {
      print_str(MSG_GEN_DFRFAIL, datafile);
      return 5;
    }

  if (g_act_1.bw || (gfl & F_OPT_VERBOSE4))
    {
      print_str(MSG_GEN_WROTE, datafile, (ulint64_t) g_act_1.bw,
          (ulint64_t) g_act_1.rw);
    }

  /*if ((gfl & F_OPT_NOFQ) && !(g_act_1.flags & F_GH_APFILT))
   {
   return 6;
   }*/

  return 0;
}
