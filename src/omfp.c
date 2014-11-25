/*
 * omfp.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "omfp.h"

#include <glutil.h>
#include <m_general.h>
#include <sort_hdr.h>
#include <l_error.h>
#include <log_io.h>
#include <glutil.h>
#include <exech.h>
#include <misc.h>

#include <net_io.h>

#include <setjmp.h>

uint32_t g_omfp_sto = 0, g_omfp_suto = 0;
int fd_out;

int
(*int_printf)(const char *__restrict __format, ...) = printf;

static int
g_print_do_filter(__g_handle hdl, void *s_exec)
{
  int r;
  hdl->exec_args.exc = NULL;
  hdl->flags |= F_GH_NO_ACCU;

  r = g_filter(hdl, &hdl->buffer);

  hdl->flags ^= F_GH_NO_ACCU;
  hdl->exec_args.exc = (__d_exec) s_exec;

  if (r == 1)
    {
      print_str("WARNING: %s: all records were filtered\n", hdl->file);
    }
  else if (r)
    {
      print_str("ERROR: %s: [%d]: filtering failed\n", hdl->file, r);
    }

  return r;
}

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

  if (gfl & F_OPT_LOADQ)
    {
      goto rc_end;
    }

  void *buffer = calloc(1, g_act_1.block_sz);

  pt_g_bmatch proc_match = g_bmatch;

  int r = 0;

  if (gfl & F_OPT_SORT)
    {
      if ((gfl & F_OPT_NOBUFFER))
        {
          print_str("ERROR: %s: unable to sort with buffering disabled\n",
              g_act_1.file);
          goto r_end;
        }

      void *s_exec = (void*) g_act_1.exec_args.exc;

      if (l_sfo == L_STFO_SORT)
        {
          if (g_print_do_filter(&g_act_1, s_exec))
            {
              goto r_end;
            }
        }

      if (gfl & F_OPT_KILL_GLOBAL)
        {
          goto r_end;
        }

      if (do_sort(&g_act_1, g_sort_field, g_sort_flags))
        {
          goto r_end;
        }

      if (l_sfo == L_STFO_FILTER)
        {
          if (g_print_do_filter(&g_act_1, s_exec))
            {
              goto r_end;
            }
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

      proc_match = g_bmatch_dummy;
    }

  g_do_ppprint(&g_act_1, F_GH_PRE_PRINT, &g_act_1.pre_print_mech,
      g_act_1.g_proc4_pr);

  if (gfl0 & F_OPT_LOADQA)
    {
      goto r_end;
    }

  void *ptr;

  size_t c = 0;

  g_setjmp(F_SIGERR_CONTINUE, "g_print_stats(loop)", NULL, NULL);

  g_act_1.buffer.offset = 0;

  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      while ((ptr = g_read(buffer, &g_act_1, g_act_1.block_sz)))
        {
          if ((gfl & F_OPT_KILL_GLOBAL))
            {
              break;
            }

          if ((r = proc_match(ptr, &g_act_1, &g_act_1.buffer)))
            {
              if (r == -1)
                {
                  print_str("ERROR: %s: [%d] matching record failed\n",
                      g_act_1.file, r);
                  break;
                }
              continue;
            }

          omfp_timeout;

          c++;
          g_act_1.g_proc4((void*) &g_act_1, ptr, NULL);

        }

    }
  else
    {
      print_str(
          "ERROR: %s: an exception has occured, terminating enumeration and attempt cleanup..\n",
          g_act_1.file);
      EXITVAL = 2;
      goto r_end;
    }

  g_do_ppprint(&g_act_1, F_GH_POST_PRINT, &g_act_1.post_print_mech,
      g_act_1.g_proc4_po);

  if (gfl & F_OPT_MODE_RAWDUMP)
    {
#ifdef HAVE_ZLIB_H
      if ((g_act_1.flags & F_GH_IO_GZIP) && g_act_1.gz_fh1)
        {
          gzflush(g_act_1.gz_fh1, Z_FINISH);
        }
#endif
      fflush(stdout);
    }

  // g_setjmp(0, "dirlog_print_stats(2)", NULL, NULL);

  if (!(g_act_1.flags & F_GH_ISONLINE) && (gfl0 & F_OPT_STATS))
    {
      fprintf(stderr, "STATS: %s: processed %llu/%llu records\n", file,
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

#define g_p_print(hdl) { \
    void *t_ptr = calloc(1, hdl->block_sz); \
    hdl->g_proc4_l((void*) hdl, t_ptr, NULL); \
    free(t_ptr); \
}

void
g_do_ppprint(__g_handle hdl, uint64_t t_flags, pmda p_mech, _d_omfp g_proc)
{
  if (hdl->flags & t_flags)
    {
      void *s_act_mech = hdl->act_mech;
      hdl->act_mech = p_mech;
      void *t_ptr = calloc(1, hdl->block_sz);
      g_proc((void*) hdl, t_ptr, NULL);
      free(t_ptr);
      hdl->act_mech = (pmda) s_act_mech;
    }
}

void
g_omfp_write(int fd, char *buffer, size_t max_size, void *arg)
{
  write(fd_out, buffer, max_size);
}

void
g_omfp_write_nl(int fd, char *buffer, size_t max_size, void *arg)
{
  write(fd_out, buffer, max_size);
  write(fd_out, "\n", 1);
}

#ifdef _G_SSYS_NET

void
g_omfp_q_nssys(int fd, char *buffer, size_t max_size, void *arg)
{
  __sock_o pso = (__sock_o) arg;

  int ret;
  if ((ret = net_push_to_sendq(pso, buffer, max_size, 0)) == -1)
    {
      printf(
          "ERROR: g_omfp_q_nssys: net_push_to_sendq failed, socket: [%d], code: [%d]\n",
          pso->sock, ret);
    }
}

#endif

void
g_omfp_norm(void *hdl, void *ptr, char *sbuffer)
{
  ((__g_handle ) hdl)->g_proc3(ptr, sbuffer);
}

void
g_omfp_raw(void * hdl, void *ptr, char *sbuffer)
{

#ifdef HAVE_ZLIB_H
  if (((__g_handle ) hdl)->flags & F_GH_IO_GZIP)
    {
      gzwrite(((__g_handle ) hdl)->gz_fh1, ptr, ((__g_handle ) hdl)->block_sz);
    }
  else
    {
      write(fd_out, ptr, ((__g_handle ) hdl)->block_sz);
    }
#else
  write(fd_out, ptr, ((__g_handle ) hdl)->block_sz);
#endif
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
  if (!(s_ptr = g_exech_build_string(ptr, ((__g_handle ) hdl)->act_mech,
      (__g_handle) hdl, b_glob, MAX_EXEC_STR - 4)))
    {
      print_str("ERROR: g_omfp_eassemble: could not assemble print string\n");
      gfl |= F_OPT_KILL_GLOBAL;
      return;
    }

  ((__g_handle) hdl)->w_d(fd_out, b_glob, strlen(b_glob), NULL);

}

