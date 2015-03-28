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
#include <errno_int.h>

#include <time.h>
#include <timeh.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>

#define PSTR_MAX        (V_MB/4)

int
g_print_str(const char * volatile buf, ...)
{
  char d_buffer_2[PSTR_MAX];

  uint32_t stdlog_level = get_msg_type((char*) buf);

  if (!(stdlog_level & STDLOG_LVL))
    {
      return 0;
    }

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

  char iserr = stdlog_level & (F_MSG_TYPE_EXCEPTION | F_MSG_TYPE_ERROR);

  if ((!iserr && (gfl & F_OPT_STDOUT_SILENT)))
    {
      va_end(al);
      return 0;
    }

  va_end(al);
  va_start(al, buf);

  if (gfl & F_OPT_PS_TIME)
    {
      if (iserr)
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
      if (iserr)
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

uint32_t STDLOG_LVL = F_MSG_TYPE_NORMAL;

uint32_t
get_msg_type(char *msg)
{
  switch (msg[0])
    {
  case 0x4E: // N
    return F_MSG_TYPE_NOTICE;
    break;
  case 0x45: // E
    switch (msg[1])
      {
    case 0x58: // X
      return F_MSG_TYPE_EXCEPTION;
      break;
    case 0x52: // R
      return F_MSG_TYPE_ERROR;
      break;
      }
    break;
  case 0x57: // W
    return F_MSG_TYPE_WARNING;
    break;
  case 0x4D: // M
    if (msg[1] == 0x41) //A
      {
        return F_MSG_TYPE_MACRO;
      }
    break;
  case 0x53: // S
    if (msg[1] == 0x54) //T
      {
        return F_MSG_TYPE_STATS;
      }
    break;
  case 0x49: // I
    if (msg[1] == 0x4E) //N
      {
        return F_MSG_TYPE_INFO;
      }
    break;
  case 0x44: // D

    switch (msg[1])
      {
    case 0: //1
      ;
      return F_MSG_TYPE_DEBUG0;
    case 0x31: //1
      ;
      return F_MSG_TYPE_DEBUG1;
    case 0x32: //2
      ;
      return F_MSG_TYPE_DEBUG2;
    case 0x33: //3
      ;
      return F_MSG_TYPE_DEBUG3;
    case 0x34: //4
      ;
      return F_MSG_TYPE_DEBUG4;
    case 0x35: //5
      ;
      return F_MSG_TYPE_DEBUG5;
    case 0x36: //6
      ;
      return F_MSG_TYPE_DEBUG6;
      }
    return F_MSG_TYPE_OTHER;
    }

  return F_MSG_TYPE_OTHER;
}

int
build_msg_reg(char *arg, uint32_t *opt_r)
{

  mda sp_s =
    { 0 };
  md_init(&sp_s, 8);

  int r = split_string(arg, 0x7C, &sp_s);

  if (0 == r)
    {
      r = 81451;
      goto end;
    }

  //*opt_r = 0x0;

  p_md_obj ptr = md_first(&sp_s);

  while (ptr)
    {
      char *s_ptr = (char*) ptr->ptr;

      uint32_t l_f = 0x0;

      if (s_ptr[0] == 0x2D)
        {
          s_ptr++;
          l_f |= 0x1;
        }

      uint32_t o_r = opt_get_msg_type(s_ptr);

      if (0 == o_r)
        {
          r = 81452;
          goto end;
        }

      if (l_f & 0x1)
        {
          *opt_r ^= (*opt_r & o_r);
        }
      else
        {
          *opt_r |= o_r;
        }
      ptr = ptr->next;
    }

  r = 0;

  end: ;

  md_g_free(&sp_s);

  return r;
}

uint32_t
opt_get_msg_type(char *msg)
{
  if (!strncmp(msg, "exception", 9))
    {
      return F_MSG_TYPE_EXCEPTION;
    }
  if (!strncmp(msg, "error", 5))
    {
      return F_MSG_TYPE_ERROR;
    }
  if (!strncmp(msg, "warning", 7))
    {
      return F_MSG_TYPE_WARNING;
    }
  if (!strncmp(msg, "notice", 6))
    {
      return F_MSG_TYPE_NOTICE;
    }
  if (!strncmp(msg, "macro", 5))
    {
      return F_MSG_TYPE_MACRO;
    }
  if (!strncmp(msg, "info", 4))
    {
      return F_MSG_TYPE_INFO;
    }
  if (!strncmp(msg, "stats", 5))
    {
      return F_MSG_TYPE_STATS;
    }
  if (!strncmp(msg, "other", 5))
    {
      return F_MSG_TYPE_OTHER;
    }
  if (!strncmp(msg, "debug", 5))
    {
      switch (msg[5])
        {
      case 0x30: //0
        ;
        return F_MSG_TYPE_DEBUG0;
      case 0x31: //1
        ;
        return F_MSG_TYPE_DEBUG1;
      case 0x32: //2
        ;
        return F_MSG_TYPE_DEBUG2;
      case 0x33: //3
        ;
        return F_MSG_TYPE_DEBUG3;
      case 0x34: //4
        ;
        return F_MSG_TYPE_DEBUG4;
      case 0x35: //5
        ;
        return F_MSG_TYPE_DEBUG5;
      case 0x36: //6
        ;
        return F_MSG_TYPE_DEBUG6;

        }
      return F_MSG_TYPE_DEBUG;
    }
  if (!strncmp(msg, "all", 3))
    {
      return F_MSG_TYPE_ANY;
    }
  if (!strncmp(msg, "any", 3))
    {
      return F_MSG_TYPE_ANY;
    }
  if (!strncmp(msg, "normal", 6))
    {
      return F_MSG_TYPE_NORMAL;
    }
  if (!strncmp(msg, "important", 9))
    {
      return F_MSG_TYPE_EEW;
    }

  return 0;
}

int
w_log(char *w, char *ow)
{

  /*if (ow && !(get_msg_type(ow) & log_lvl))
   {
   return 1;
   }*/

  size_t wc, wll;

  wll = strlen(w);

  if ((wc = fwrite(w, 1, wll, fd_log)) != wll)
    {
      char e_buffer[1024];
      printf("ERROR: %s: writing log failed [%d/%d] %s\n", LOGFILE, (int) wc,
          (int) wll, g_strerr_r(errno, e_buffer, 1024));
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
#include <lref_altlog.h>
#include <lref_gconf.h>
#include <lref_sconf.h>

int
g_print_info(void)
{
  print_version_long(NULL, 0, NULL);
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
  print_str(" ALTLOG         %d\t\n", AL_SZ);
  print_str(" ONLINE(SHR)    %d\t\n", OL_SZ);
  print_str(" GCONF          %d\t\n", GC_SZ);
  print_str(" SCONF          %d\t\n", SC_SZ);
  print_str(MSG_NL);
  if (gfl & F_OPT_VERBOSE)
    {
      print_str("  TYPE         SIZE(B)   \n"
          "-------------------------\n");
      print_str(" off_t            %d\t\n", (sizeof(off_t)));
      //print_str(" uintaa_t         %d\t\n", (sizeof(uintaa_t)));
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
      print_str(" DT_WHT           %d\t%s\n", DT_WHT, "What?");
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
print_version_long(void *arg, int m, void *opt)
{
  print_str("* %s-%s.%s - glFTPd general purpose utility *\n", PACKAGE_NAME,
  PACKAGE_VERSION, __STR_ARCH);
  return 0;
}
