/*
 * macros.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include "macros_t.h"

#include <glutil.h>
#include <t_glob.h>
#include <xref.h>
#include <l_error.h>
#include <x_f.h>
#include <str.h>
#include <l_sb.h>
#include <exec_t.h>
#include <arg_proc.h>
#include <lref.h>

#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

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

  if (self_get_path(buffer))
    {
      print_str("ERROR: could not get own path\n");
      return NULL;
    }

  char *dirn = dirname(buffer);

  _si_argv0 av =
    { 0 };

  av.ret = -1;

  if (strlen(a_ptr) > sizeof(av.p_buf_1))
    {
      print_str("ERROR: invalid macro name\n");
      return NULL;
    }

  strncpy(av.p_buf_1, a_ptr, strlen(a_ptr));

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("MACRO: '%s': searching for macro inside '%s/' (recursive)\n",
          av.p_buf_1, dirn);
    }

  _g_eds eds =
    { 0 };

  if (enum_dir(dirn, ssd_4macro, &av, F_ENUMD_NOXBLK, &eds) < 0)
    {
      print_str("ERROR: %s: recursion failed (macro not found)\n", av.p_buf_1);
      return NULL;
    }

  if (av.ret == -1)
    {
      print_str("ERROR: %s: could not find macro\n", av.p_buf_1);
      return NULL;
    }

  strncpy(b_spec1, av.p_buf_2, strlen(av.p_buf_2));

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("MACRO: '%s': found macro in '%s'\n", av.p_buf_1, av.p_buf_2);
    }

  char *s_buffer = (char*) malloc(MAX_EXEC_STR + 1), **s_ptr = NULL;
  int r;

  if ((r = process_exec_string(av.s_ret, s_buffer, MAX_EXEC_STR,
      ref_to_val_macro,
      NULL)))
    {

      print_str("ERROR: [%d]: could not process exec string: '%s'\n", r,
          av.s_ret);
      goto end;
    }

  int c = 0;
  s_ptr = build_argv(s_buffer, 4096, &c);

  if (!c)
    {
      print_str("ERROR: %s: macro was declared, but no arguments found\n",
          av.p_buf_1);
    }

  _p_macro_argc = c;

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("MACRO: '%s': built argument string array with %d elements\n",
          av.p_buf_1, c);
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("MACRO: '%s': EXECUTING: '%s'\n", av.p_buf_1, s_buffer);
    }

  end:

  free(s_buffer);

  return s_ptr;
}


int
ssd_4macro(char *name, unsigned char type, void *arg, __g_eds eds)
{
  off_t name_sz;
  switch (type)
    {
  case DT_REG:
    if (access(name, X_OK))
      {
        if (gfl & F_OPT_VERBOSE5)
          {
            print_str("MACRO: %s: could not exec (permission denied)\n", name);
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
        break;
      }

    char *buffer = malloc(SSD_MAX_LINE_SIZE + 16);

    size_t b_len, lc = 0;
    int hit = 0, i;

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

        __si_argv0 ptr = (__si_argv0) arg;

        char buffer2[4096] =
          { 0 };
        snprintf(buffer2, 4096, "%s:", ptr->p_buf_1);

        size_t pb_l = strlen(buffer2);

        if (!strncmp(buffer2, &buffer[8], pb_l))
          {
            buffer = replace_char(0xA, 0x0, buffer);
            buffer = replace_char(0xD, 0x0, buffer);
            b_len = strlen(buffer);
            size_t d_len = b_len - 8 - pb_l;
            if (d_len > sizeof(ptr->s_ret))
              {
                d_len = sizeof(ptr->s_ret);
              }
            bzero(ptr->s_ret, sizeof(ptr->s_ret));
            strncpy(ptr->s_ret, &buffer[8 + pb_l], d_len);

            snprintf(ptr->p_buf_2, PATH_MAX, "%s", name);
            ptr->ret = d_len;
            gfl |= F_OPT_TERM_ENUM;
            break;
          }
        hit++;
      }

    fclose(fh);
    free(buffer);
    break;
  case DT_DIR:
    enum_dir(name, ssd_4macro, arg, 0, eds);
    break;
    }

  return 0;
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
