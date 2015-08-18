/*
 * macros.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "macros_t.h"

#include <t_glob.h>
#include <xref.h>
#include <l_error.h>
#include <x_f.h>
#include <str.h>
#include <l_sb.h>
#include <exec_t.h>
#include <arg_proc.h>
#include <lref.h>
#include <exech.h>
#include <errno_int.h>
#include <log_io.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>
#include "lref_generic.h"

int
list_macros(void)
{
  g_setjmp(0, "list_macros", NULL, NULL);
  char buffer[PATH_MAX] =
    { 0 };

  char *dirn;

  if ((gfl0 & F_OPT_MROOT) && NULL != g_mroot)
    {
      snprintf(buffer, PATH_MAX, "%s", g_mroot);
      dirn = buffer;
    }
  else
    {
      if (self_get_path(buffer))
        {
          print_str(MSG_F_OWN_PATH);
          return 1;
        }
      else
        {
          dirn = g_dirname(buffer);
        }
    }

  _si_argv0 av =
    { 0 };
  _g_eds eds =
    { 0 };

  av.buffer = malloc(SSD_MAX_LINE_SIZE + 16);
  av.ret = -1;
  av.flags |= F_MMODE_LIST;

  int ret = 0;

  if ((ret = enum_dir(dirn, ssd_4macro, &av, F_ENUMD_NOXBLK, &eds, tp_default))
      == 0)
    {
      print_str("ERROR: %s: no macros could be found\n", av.p_buf_1);
      ret = 2;
    }
  else if (ret < 0)
    {
      print_str("ERROR: list_macros->enum_dir: %s: [%d] %s\n", dirn, ret,
          ie_tl(ret, EMR_enum_dir));
    }

  free(av.buffer);

  return ret;

}

char **
process_macro(void * arg, char **out)
{
  g_setjmp(0, "process_macro", NULL, NULL);
  if (!arg)
    {
      print_str("ERROR: missing data type argument (-m <macro name>)\n");
      return NULL;
    }

  char *a_ptr = (char*) arg;

  char buffer[PATH_MAX] =
    { 0 };

  char *dirn;

  if ((gfl0 & F_OPT_MROOT) && NULL != g_mroot)
    {
      snprintf(buffer, PATH_MAX, "%s", g_mroot);
      dirn = buffer;
    }
  else
    {
      if (self_get_path(buffer))
        {
          print_str(MSG_F_OWN_PATH);
          return NULL;
        }
      else
        {
          dirn = g_dirname(buffer);
        }
    }

  char *s_buffer = NULL;
  char **s_ptr = NULL;

  _si_argv0 av =
    { 0 };

  av.buffer = malloc(SSD_MAX_LINE_SIZE + 16);

  av.ret = -1;
  av.flags |= F_MMODE_EXEC;

  if (strlen(a_ptr) >= sizeof(av.p_buf_1))
    {
      print_str("ERROR: invalid macro name\n");
      return NULL;
    }

  snprintf(av.p_buf_1, sizeof(av.p_buf_1), "%s", a_ptr);

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("MACRO: '%s': searching for macro inside '%s' (recursive)\n",
          av.p_buf_1, dirn);
    }

  _g_handle hdl =
    { 0 };
  _g_eds eds =
    { 0 };

  int r;

  if ((r = enum_dir(dirn, ssd_4macro, &av, F_ENUMD_NOXBLK, &eds, tp_default))
      == 0)
    {
      print_str("ERROR: %s: macro not found\n", av.p_buf_1);
      return NULL;
    }
  else if (r < 0)
    {
      print_str("ERROR: process_macro->enum_dir: %s: [%d] %s\n", dirn, r,
          ie_tl(r, EMR_enum_dir));
      s_ptr = NULL;
      goto end;
    }

  if (av.ret == -1)
    {
      print_str("ERROR: %s: could not find macro\n", av.p_buf_1);
      s_ptr = NULL;
      goto end;
    }

  if (av.ret > 0)
    {
      print_str("ERROR: %s: could not run macro, error '%d'\n", av.p_buf_1,
          av.ret);
      s_ptr = NULL;
      goto end;
    }

  snprintf(b_spec1, sizeof(b_spec1), "%s", av.p_buf_2);

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("MACRO: '%s': found macro in '%s'\n", av.p_buf_1, av.p_buf_2);
    }

  s_buffer = (char*) malloc(MAX_EXEC_STR + 1);

  hdl.g_proc1_lookup = ref_to_val_lk_macro;

  if ((r = g_compile_exech(&hdl.print_mech, &hdl, (char*) av.s_ret)))
    {
      print_str("ERROR: %s: [%d]: could not compile exec string: '%s'\n",
          "MACRO", r, av.s_ret);
      goto end;
    }

  if (-1 == (g_exech_build_string(NULL, &hdl.print_mech, &hdl, s_buffer,
  MAX_EXEC_STR - 4)))
    {
      print_str("ERROR: process_macro: could not assemble print string\n");
      goto end;
    }

  char *escape_only = "{}";

  g_p_escape_once(s_buffer, escape_only);

  int c = 0;
  s_ptr = build_argv(s_buffer, 4096, &c);

  if (0 == c)
    {
      print_str("ERROR: %s: macro was declared, but no arguments found\n",
          av.p_buf_1);
      s_ptr = NULL;
      goto end;
    }

  _p_macro_argc = c;

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("MACRO: '%s': built argument string array with %d elements\n",
          av.p_buf_1, c);
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("MACRO: '%s': EXECUTING: %s\n", av.p_buf_1, s_buffer);
    }

  end:

  g_cleanup(&hdl);

  if (NULL != av.buffer)
    {
      free(av.buffer);
    }

  if (NULL != s_buffer)
    {
      free(s_buffer);
    }

  return s_ptr;
}

static int
ssd_mmode_exec(char *name, __si_argv0 ptr, char *buffer)
{
  buffer = replace_char(0xA, 0x0, buffer);
  buffer = replace_char(0xD, 0x0, buffer);

  while (buffer[0] == 0x3A)
    {
      buffer++;
    }

  char *start = buffer;

  while (buffer[0] != 0x3A && buffer[0] != 0)
    {
      if ((buffer[0] == 0x5C))
        {
          buffer++;
        }
      buffer++;
    }

  if (buffer[0] != 0x3A)
    {
      return 1;
    }

  while (buffer[0] == 0x3A)
    {
      buffer++;
    }

  size_t pb_l = strlen(ptr->p_buf_1);

  if (!(!strncmp(ptr->p_buf_1, start, pb_l)
      && (start[pb_l] == 0x3A || start[pb_l] == 0x7C)))
    {
      return -1;
    }

  //bzero(ptr->s_ret, sizeof(ptr->s_ret));

  snprintf(ptr->s_ret, sizeof(ptr->s_ret), "%s", buffer);
  snprintf(ptr->p_buf_2, PATH_MAX, "%s", name);

  if (access(name, X_OK))
    {
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str("MACRO: %s: [%s] no execute permission\n", name,
              ptr->p_buf_1);
        }
      return 2;
    }

  ptr->ret = strlen(ptr->s_ret);

  gfl |= F_OPT_TERM_ENUM;

  return 0;
}

static int
ssd_mmode_list(char *name, __si_argv0 ptr, char *buffer)
{
  char *m_n = buffer, *m_desc = NULL;

  while (buffer[0] != 0x3A && buffer[0] != 0x7C && buffer[0] != 0)
    {
      buffer++;
    }

  if (buffer[0] == 0)
    {
      return 1;
    }

  char *mid_m_n = buffer;

  if (buffer[0] == 0x7C)
    {

      while (buffer[0] == 0x7C)
        {
          buffer++;
        }
      m_desc = buffer;

      while ((buffer[0] != 0x3A && buffer[0] != 0))
        {
          if ((buffer[0] == 0x5C))
            {
              memmove(buffer, buffer + 1, strlen(buffer + 1));
              buffer++;
            }
          buffer++;
        }

      buffer[0] = 0x0;
    }

  mid_m_n[0] = 0x0;

  if (m_desc)
    {
      printf("%s - %s[ %s ] ", m_n,
          !access(name, X_OK) ? "" : "[NOT EXECUTABLE] ", m_desc);
    }
  else
    {
      printf("%s%s ", m_n, !access(name, X_OK) ? "" : " [!NOT EXECUTABLE!]");
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      printf("[%s]\n", name);
    }
  else if (gfl & F_OPT_VERBOSE)
    {
      printf("[%s]\n", g_basename(name));
    }
  else
    {
      fputs("\n", stdout);
    }

  return 0;
}

int
ssd_4macro(char *name, unsigned char type, void *arg, __g_eds eds)
{
  off_t name_sz;
  switch (type)
    {
  case DT_REG:
    ;

    if (access(name, R_OK))
      {
        if (gfl & F_OPT_VERBOSE5)
          {
            print_str("MACRO: %s: no read permission\n", name);
          }
        break;
      }

    name_sz = get_file_size(name);
    if (!name_sz)
      {
        break;
      }

    FILE *fh = fopen(name, "r");

    if (!fh)
      {
        print_str("MACRO: %s: could not open stream [%s]\n", name,
            strerror(errno));
        break;
      }

    size_t b_len, lc = 0;
    int hit = 0, i;

    __si_argv0 ptr = (__si_argv0) arg;

    char *buffer = ptr->buffer;

    while (fgets(buffer, SSD_MAX_LINE_SIZE, fh) && lc < SSD_MAX_LINE_PROC
        && !ferror(fh) && !feof(fh))
      {
        lc++;
        b_len = strlen(buffer);
        if (b_len < 8)
          {
            continue;
          }

        for (i = 0; i < b_len && i < SSD_MAX_LINE_SIZE; i++)
          {
            if (is_ascii_text((unsigned char) buffer[i]))
              {
                break;
              }
          }

        if (strncmp(buffer, "#@MACRO:", 8))
          {
            continue;
          }

        if ( ptr->flags & F_MMODE_EXEC)
          {
            if (!(ptr->ret = ssd_mmode_exec(name, ptr, &buffer[8])))
              {
                break;
              }
          }
        else if (ptr->flags & F_MMODE_LIST)
          {
            ssd_mmode_list(name, ptr, &buffer[8]);
          }

        hit++;
      }

    fclose(fh);

    if ( 0 != ptr->ret )
      {
        ptr->ret = -1;
      }

    break;
    case DT_DIR:;
    int ret = enum_dir(name, ssd_4macro, arg, 0, eds, tp_default);
    if (ret < 0)
      {
        print_str("ERROR: ssd_4macro->enum_dir: %s: [%d] %s\n", name, ret,
            ie_tl(ret, EMR_enum_dir));
      }
    break;
  }

  return 0;
}

void *
ref_to_val_lk_macro(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{

  PROC_SH_EX(match)

  void *ptr;
  if ((ptr = ref_to_val_lk_generic(arg, match, output, max_size, mppd)))
    {
      return ptr;
    }

  return NULL;
}

int
ref_to_val_macro(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strncmp(match, "m:exe", 5))
    {
      if (self_get_path(output))
        {
          output[0] = 0x0;
        }
    }
  else if (!strncmp(match, "m:glroot", 8))
    {
      strcp_s(output, max_size, GLROOT);
    }
  else if (!strncmp(match, "m:siteroot", 10))
    {
      strcp_s(output, max_size, SITEROOT);
    }
  else if (!strncmp(match, "m:siterootn", 11))
    {
      strcp_s(output, max_size, SITEROOT_N);
    }
  else if (!strncmp(match, "m:ftpdata", 9))
    {
      strcp_s(output, max_size, FTPDATA);
    }
  else if ((gfl & F_OPT_PS_LOGGING) && !strncmp(match, "m:logfile", 9))
    {
      strcp_s(output, max_size, LOGFILE);
    }
  else if (!strncmp(match, "m:PID", 5))
    {
      snprintf(output, max_size, "%d", getpid());
    }
  else if (!strncmp(match, "m:IPC", 5))
    {
      snprintf(output, max_size, "%.8X", (uint32_t) SHM_IPC);
    }
  else if (!strncmp(match, "m:spec1:dir", 10))
    {
      strcp_s(output, max_size, b_spec1);
      g_dirname(output);
    }
  else if (!strncmp(match, "m:spec1", 7))
    {
      strcp_s(output, max_size, b_spec1);
    }
  else if (!strncmp(match, "m:arg1", 6))
    {
      strcp_s(output, max_size, MACRO_ARG1);
    }
  else if (!strncmp(match, "m:arg2", 6))
    {
      strcp_s(output, max_size, MACRO_ARG2);
    }
  else if (!strncmp(match, "m:arg3", 6))
    {
      strcp_s(output, max_size, MACRO_ARG3);
    }
  else if (!strncmp(match, "m:q:", 4))
    {
      return rtv_q(&match[4], output, max_size);
    }
  else
    {
      return 1;
    }
  return 0;
}
