/*
 * misc.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <t_glob.h>
#include <misc.h>
#include <l_sb.h>
#include <l_error.h>
#include <m_general.h>
#include <xref.h>
#include <arg_opts.h>
#include <str.h>

#include <time.h>
#include <timeh.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>

#define PSTR_MAX        (V_MB/4)

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
g_print_str(const char * volatile buf, ...)
{
  char d_buffer_2[PSTR_MAX];
  va_list al;
  va_start(al, buf);

  if ((gfl & F_OPT_PS_LOGGING) || (gfl & F_OPT_PS_TIME))
    {
      struct tm tm = *get_localtime();
      snprintf(d_buffer_2, PSTR_MAX, "[%.2u/%.2u/%.2u %.2u:%.2u:%.2u] %s",
          tm.tm_mday, tm.tm_mon + 1, (tm.tm_year + 1900) % 100, tm.tm_hour,
          tm.tm_min, tm.tm_sec, buf);
      if (fd_log)
        {
          char wl_buffer[PSTR_MAX];
          vsnprintf(wl_buffer, PSTR_MAX, d_buffer_2, al);
          w_log(wl_buffer, (char*) buf);
        }
    }

  char iserr = !(buf[0] == 0x45 && (buf[1] == 0x52 || buf[1] == 0x58));

  if (iserr && (gfl & F_OPT_PS_SILENT))
    {
      return 0;
    }

  va_end(al);
  va_start(al, buf);

  if (gfl & F_OPT_PS_TIME)
    {
      if (!iserr)
        {
          vfprintf(stderr, d_buffer_2, al);
        }
      else
        {
          vprintf(d_buffer_2, al);
        }

    }
  else
    {
      if (!iserr)
        {
          vfprintf(stderr, buf, al);
        }
      else
        {
          vprintf(buf, al);
        }
    }

  va_end(al);

  fflush(stdout);

  return 0;
}

uint32_t LOGLVL = F_MSG_TYPE_EEW;

uint32_t
get_msg_type(char *msg)
{
  if (!strncmp(msg, "INIT:", 5))
    {
      return F_MSG_TYPE_ANY;
    }
  if (!strncmp(msg, "EXCEPTION:", 10))
    {
      return F_MSG_TYPE_EXCEPTION;
    }
  if (!strncmp(msg, "ERROR:", 6))
    {
      return F_MSG_TYPE_ERROR;
    }
  if (!strncmp(msg, "WARNING:", 8))
    {
      return F_MSG_TYPE_WARNING;
    }
  if (!strncmp(msg, "NOTICE:", 7))
    {
      return F_MSG_TYPE_NOTICE;
    }
  if (!strncmp(msg, "MACRO:", 6))
    {
      return F_MSG_TYPE_NOTICE;
    }
  if (!strncmp(msg, "STATS:", 6))
    {
      return F_MSG_TYPE_STATS;
    }

  return F_MSG_TYPE_NORMAL;
}

int
w_log(char *w, char *ow)
{

  if (ow && !(get_msg_type(ow) & LOGLVL))
    {
      return 1;
    }

  size_t wc, wll;

  wll = strlen(w);

  if ((wc = fwrite(w, 1, wll, fd_log)) != wll)
    {
      printf("ERROR: %s: writing log failed [%d/%d]\n", LOGFILE, (int) wc,
          (int) wll);
    }

  fflush(fd_log);

  return 0;
}

void
enable_logging(void)
{
  if ((gfl & F_OPT_PS_LOGGING) && !fd_log)
    {
      if (!(ofl & F_OVRR_LOGFILE))
        {
          build_data_path(DEFF_DULOG, LOGFILE, DEFPATH_LOGS);
        }
      if (!(fd_log = fopen(LOGFILE, "a")))
        {
          gfl ^= F_OPT_PS_LOGGING;
          print_str(
              "ERROR: %s: [%s]: could not open file for writing, logging disabled\n",
              LOGFILE, strerror(errno));
        }
    }
  return;
}

#include <libgen.h>

char *
build_data_path(char *file, char *path, char *sd)
{
  char *ret = path;

  size_t p_l = strlen(path);

  char *p_d = NULL;

  if (p_l)
    {
      p_d = strdup(path);
      char *b_pd = dirname(p_d);

      if (access(b_pd, R_OK))
        {
          if (gfl & F_OPT_VERBOSE4)
            {
              print_str(
                  "NOTICE: %s: data path was not found, building default using GLROOT '%s'..\n",
                  path, GLROOT);
            }
        }
      else
        {
          goto end;
        }

    }

  snprintf(path, PATH_MAX, "%s/%s/%s/%s", GLROOT, FTPDATA, sd, file);
  remove_repeating_chars(path, 0x2F);

  end:

  if (p_d)
    {
      free(p_d);
    }

  return ret;
}

#include <lref_imdb.h>
#include <lref_tvrage.h>
#include <lref_game.h>
#include <lref_gen1.h>
#include <lref_gen2.h>
#include <lref_gen3.h>
#include <lref_gen4.h>

int
g_print_info(void)
{
  print_version_long(NULL, 0);
  print_str(MSG_NL);
  print_str(" DATA SRC   BLOCK SIZE(B)   \n"
      "--------------------------\n");
  print_str(" DIRLOG         %d\t\n", DL_SZ);
  print_str(" NUKELOG        %d\t\n", NL_SZ);
  print_str(" DUPEFILE       %d\t\n", DF_SZ);
  print_str(" LASTONLOG      %d\t\n", LO_SZ);
  print_str(" ONELINERS      %d\t\n", LO_SZ);
  print_str(" IMDBLOG        %d\t\n", ID_SZ);
  print_str(" GAMELOG        %d\t\n", GM_SZ);
  print_str(" TVLOG          %d\t\n", TV_SZ);
  print_str(" GE1            %d\t\n", G1_SZ);
  print_str(" GE2            %d\t\n", G2_SZ);
  print_str(" GE3            %d\t\n", G3_SZ);
  print_str(" GE4            %d\t\n", G4_SZ);
  print_str(" ONLINE(SHR)    %d\t\n", OL_SZ);
  print_str(MSG_NL);
  if (gfl & F_OPT_VERBOSE)
    {
      print_str("  TYPE         SIZE(B)   \n"
          "-------------------------\n");
      print_str(" off_t            %d\t\n", (sizeof(off_t)));
      print_str(" uintaa_t         %d\t\n", (sizeof(uintaa_t)));
      print_str(" uint8_t          %d\t\n", (sizeof(uint8_t)));
      print_str(" uint16_t         %d\t\n", (sizeof(uint16_t)));
      print_str(" uint32_t         %d\t\n", (sizeof(uint32_t)));
      print_str(" uint64_t         %d\t\n", (sizeof(uint64_t)));
      print_str(" ulong int        %d\t\n", (sizeof(unsigned long int)));
      print_str(" ulong long int   %d\t\n", (sizeof(unsigned long long int)));
      print_str(" int32_t          %d\t\n", (sizeof(int32_t)));
      print_str(" int64_t          %d\t\n", (sizeof(int64_t)));
      print_str(" size_t           %d\t\n", (sizeof(size_t)));
      print_str(" float            %d\t\n", (sizeof(float)));
      print_str(" double           %d\t\n", (sizeof(double)));
      print_str(MSG_NL);
      print_str(" void *           %d\t\n", sizeof(void*));
      print_str(MSG_NL);
      print_str(" mda              %d\t\n", sizeof(mda));
      print_str(" md_obj           %d\t\n", sizeof(md_obj));
      print_str(" _g_handle        %d\t\n", sizeof(_g_handle));
      print_str(" _g_match         %d\t\n", sizeof(_g_match));
      print_str(" _g_lom           %d\t\n", sizeof(_g_lom));
      print_str(" _d_xref          %d\t\n", sizeof(_d_xref));
      print_str(" _d_drt_h         %d\t\n", sizeof(_d_drt_h));

      print_str(MSG_NL);
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str(" FILE TYPE     DECIMAL\tDESCRIPTION\n"
          "-------------------------\n");
      print_str(" DT_UNKNOWN       %d\t\n", DT_UNKNOWN);
      print_str(" DT_FIFO          %d\t%s\n", DT_FIFO, "named pipe (FIFO)");
      print_str(" DT_CHR           %d\t%s\n", DT_CHR, "character device");
      print_str(" DT_DIR           %d\t%s\n", DT_DIR, "directory");
      print_str(" DT_BLK           %d\t%s\n", DT_BLK, "block device");
      print_str(" DT_REG           %d\t%s\n", DT_REG, "regular file");
      print_str(" DT_LNK           %d\t%s\n", DT_LNK, "symbolic link");
      print_str(" DT_SOCK          %d\t%s\n", DT_SOCK, "UNIX domain socket");
#ifdef DT_WHT
      print_str(" DT_WHT           %d\t\n", DT_WHT);
#endif
      print_str(MSG_NL);
    }

  return 0;
}

int
g_memcomp(const void *p1, const void *p2, off_t size)
{
  unsigned char *ptr1 = (unsigned char *) p1 + (size - 1), *ptr2 =
      (unsigned char *) p2 + (size - 1);

  while (ptr1[0] == ptr2[0] && ptr1 >= (unsigned char *) p1)
    {
      ptr1--;
      ptr2--;
    }
  if (ptr1 < (unsigned char *) p1)
    {
      return 0;
    }
  return 1;
}

char *
g_bitstr(uint64_t value, uint8_t bits, char *buffer)
{
  int16_t i = (int64_t) bits - 1, p;

  buffer[i] = 0x0;

  for (p = 0; p < bits; i--, p++)
    {
      buffer[i] = 0x30 + (value >> p & 1);
    }

  return buffer;
}

int
print_version_long(void *arg, int m)
{
  print_str("* %s_%s-%s - glFTPd binary logs tool *\n", PACKAGE_NAME,
  PACKAGE_VERSION, __STR_ARCH);
  return 0;
}
