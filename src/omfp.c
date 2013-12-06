/*
 * omfp.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <t_glob.h>
#include "omfp.h"

#include <m_general.h>
#include <sort_hdr.h>
#include <glutil.h>
#include <l_error.h>
#include <log_io.h>
#include <glutil.h>
#include <exech.h>
#include <misc.h>

#include <setjmp.h>

int
g_print_stats(char *file, uint32_t flags, size_t block_sz)
{
  g_setjmp(0, "g_print_stats", NULL, NULL);

  if (block_sz)
    {
      g_act_1.block_sz = block_sz;
    }

  if (g_fopen(file, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
    {
      return 2;
    }
printf("term\n");
  if (gfl & F_OPT_LOADQ)
    {
      goto rc_end;
    }

  void *buffer = calloc(1, g_act_1.block_sz);

  int r;

  if (gfl & F_OPT_SORT)
    {
      if ((gfl & F_OPT_NOBUFFER))
        {
          print_str("ERROR: %s: unable to sort with buffering disabled\n",
              g_act_1.file);
          goto r_end;
        }

      void *s_exec = (void*) g_act_1.exec_args.exc;

      g_act_1.exec_args.exc = NULL;

      r = g_filter(&g_act_1, &g_act_1.buffer);

      g_act_1.exec_args.exc = (__d_exec) s_exec;

      if (gfl & F_OPT_KILL_GLOBAL)
        {
          goto r_end;
        }

      if (r == 1)
        {
          print_str("WARNING: %s: all records were filtered\n", g_act_1.file);
          goto r_end;
        }
      else if (r)
        {
          print_str("ERROR: %s: [%d]: filtering failed\n", g_act_1.file, r);
          goto r_end;
        }

      if (do_sort(&g_act_1, g_sort_field, g_sort_flags))
        {
          goto r_end;
        }

      if (gfl & F_OPT_KILL_GLOBAL)
        {
          goto r_end;
        }

      g_act_1.max_hits = 0;
      g_act_1.max_results = 0;

      if (g_act_1.j_offset == 2)
        {
          g_act_1.buffer.r_pos = md_last(&g_act_1.buffer);
        }
      else
        {
          g_act_1.buffer.r_pos = md_first(&g_act_1.buffer);
        }

      p_md_obj lm_ptr = md_first(&g_act_1._match_rr), lm_s_ptr;

      while (lm_ptr)
        {
          __g_match gm_ptr = (__g_match ) lm_ptr->ptr;
          if (gm_ptr->flags & F_GM_TYPES)
            {
              lm_s_ptr = lm_ptr;
              lm_ptr = lm_ptr->next;
              md_unlink(&g_act_1._match_rr, lm_s_ptr);
            }
          else
            {
              lm_ptr = lm_ptr->next;
            }
        }

    }

  void *ptr;

  size_t c = 0;

  g_setjmp(0, "g_print_stats(loop)", NULL, NULL);

  g_act_1.buffer.offset = 0;

  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      while ((ptr = g_read(buffer, &g_act_1, g_act_1.block_sz)))
        {
          if ((gfl & F_OPT_KILL_GLOBAL))
            {
              break;
            }

          if ((r = g_bmatch(ptr, &g_act_1, &g_act_1.buffer)))
            {
              if (r == -1)
                {
                  print_str("ERROR: %s: [%d] matching record failed\n",
                      g_act_1.file, r);
                  break;
                }
              continue;
            }

          c++;
          g_act_1.g_proc4((void*) &g_act_1, ptr, NULL);

        }
    }

  if (gfl & F_OPT_MODE_RAWDUMP)
    {
      fflush(stdout);
    }

  // g_setjmp(0, "dirlog_print_stats(2)", NULL, NULL);

  if (!(g_act_1.flags & F_GH_ISONLINE))
    {
      print_str("STATS: %s: processed %llu/%llu records\n", file,
          (unsigned long long int) c,
          !g_act_1.buffer.count ?
              (unsigned long long int) c : g_act_1.buffer.count);
    }

  if (!c)
    {
      EXITVAL = 1;
    }

  r_end:

  free(buffer);

  rc_end:

  g_close(&g_act_1);

  return EXITVAL;
}

void
g_omfp_norm(void *hdl, void *ptr, char *sbuffer)
{
  ((__g_handle ) hdl)->g_proc3(ptr, sbuffer);
}

void
g_omfp_raw(void * hdl, void *ptr, char *sbuffer)
{
  fwrite(ptr, ((__g_handle ) hdl)->block_sz, 1, stdout);
}

void
g_omfp_ocomp(void * hdl, void *ptr, char *sbuffer)
{
  ((__g_handle ) hdl)->g_proc3(ptr, NULL);
}

void
g_omfp_eassemble(void *hdl, void *ptr, char *sbuffer)
{
  char *s_ptr;
  if (!(s_ptr = g_exech_build_string(ptr, &((__g_handle ) hdl)->print_mech,
      (__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      print_str("ERROR: could not assemble print string\n");
      gfl |= F_OPT_KILL_GLOBAL;
      return;
    }

  printf("%s\n", b_glob);
}

void
g_omfp_eassemblef(void *hdl, void *ptr, char *sbuffer)
{
  char *s_ptr;
  if (!(s_ptr = g_exech_build_string(ptr, &((__g_handle ) hdl)->print_mech,
      (__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      print_str("ERROR: could not assemble print string\n");
      gfl |= F_OPT_KILL_GLOBAL;
      return;
    }

  printf("%s", b_glob);
}
