/*
 * glob.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef T_GLOB_H_
#define T_GLOB_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>
#include <limits.h>


#ifndef WEXITSTATUS
#define WEXITSTATUS(status)     (((status) & 0xff00) >> 8)
#endif


#ifndef _STDINT_H
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int uint32_t;
# define __uint32_t_defined
#endif
#if __WORDSIZE == 64
typedef unsigned long int uint64_t;
#else
__extension__
typedef unsigned long long int uint64_t;
#endif
#endif

typedef unsigned long long int ulint64_t;

#if __x86_64__ || __ppc64__
#define uintaa_t uint64_t
#define ENV_64
#define __STR_ARCH      "x86_64"
#define __AA_SPFH       "%.16X"
#else
#define uintaa_t uint32_t
#define ENV_32
#define __STR_ARCH      "i686"
#define __AA_SPFH       "%.8X"
#endif

#define MSG_NL                          "\n"
#define MSG_TAB                         "\t"

#define MAX_uint64_t                    ((uint64_t) -1)
#define MAX_uint32_t                    ((uint32_t) -1)

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define GM_MAX                          16384

#define a64                             ((ulint64_t) 1)
#define a32                             ((uint32_t) 1)

#define V_MB                            0x100000


#define CRC_FILE_READ_BUFFER_SIZE       64512

#define MAX_EXEC_STR                    262144
#define PIPE_READ_MAX                   0x2000
#define MAX_DATAIN_F                    (V_MB*32)

#define DEFPATH_LOGS                    "/logs"
#define DEFPATH_USERS                   "/users"
#define DEFPATH_GROUPS                  "/groups"

#define NUKESTR_DEF                     "NUKED-%s"

#define DB_MAX_SIZE                     ((long long int)1073741824)   /* max file size allowed to load into memory */

#define STD_FMT_TIME_STR                "%d %b %Y %T"

#define MSG_GEN_NODFILE                 "ERROR: %s: could not open data file: %s\n"
#define MSG_GEN_DFWRITE                 "ERROR: %s: [%d] [%llu] writing record to dirlog failed! (mode: %s)\n"
#define MSG_GEN_DFCORRU                 "ERROR: %s: corrupt data file detected! (data file size [%llu] is not a multiple of block size [%d])\n"
#define MSG_GEN_DFCORRUW                "WARNING: %s: data file size [%llu] is not a multiple of block size [%d]\n"
#define MSG_GEN_DFRFAIL                 "ERROR: %s: building data file failed!\n"
#define MSG_BAD_DATATYPE                "ERROR: %s: could not determine data type\n"
#define MSG_GEN_WROTE                   "STATS: %s: wrote %.2f kB in %llu records\n"
#define MSG_GEN_NO_WRITE                "STATS: %s: no data was written\n"
#define MSG_GEN_WROTE2                  "STATS: %s: wrote %.2f kB [%.2f kB] in %llu records\n"
#define MSG_GEN_FLUSHED                 "NOTICE: %s: flushed %llu records, %llu bytes\n"
#define MSG_LL_RC                       "NOTICE: %s: loaded %llu records\n"
#define MSG_F_OWN_PATH                  "ERROR: could not get own path\n"

// gfl

#define F_OPT_FORCE                     (a64 << 1)
#define F_OPT_VERBOSE                   (a64 << 2)
#define F_OPT_VERBOSE2                  (a64 << 3)
#define F_OPT_VERBOSE3                  (a64 << 4)
#define F_OPT_SFV                       (a64 << 5)
#define F_OPT_NOWRITE                   (a64 << 6)
#define F_OPT_NOBUFFER                  (a64 << 7)
#define F_OPT_UPDATE                    (a64 << 8)
#define F_OPT_FIX                       (a64 << 9)
#define F_OPT_FOLLOW_LINKS              (a64 << 10)
#define F_OPT_FORMAT_BATCH              (a64 << 11)
#define F_OPT_KILL_GLOBAL               (a64 << 12)
#define F_OPT_MODE_RAWDUMP              (a64 << 13)
#define F_OPT_HAS_G_REGEX               (a64 << 14)
#define F_OPT_VERBOSE4                  (a64 << 15)
#define F_OPT_WBUFFER                   (a64 << 16)
#define F_OPT_FORCEWSFV                 (a64 << 17)
#define F_OPT_FORMAT_COMP               (a64 << 18)
#define F_OPT_DAEMONIZE                 (a64 << 19)
#define F_OPT_LOOP                      (a64 << 20)
#define F_OPT_LOOPEXEC                  (a64 << 21)
#define F_OPT_PS_SILENT                 (a64 << 22)
#define F_OPT_PS_TIME                   (a64 << 23)
#define F_OPT_PS_LOGGING                (a64 << 24)
#define F_OPT_TERM_ENUM                 (a64 << 25)
#define F_OPT_HAS_G_MATCH               (a64 << 26)
#define F_OPT_HAS_M_ARG1                (a64 << 27)
#define F_OPT_HAS_M_ARG2                (a64 << 28)
#define F_OPT_HAS_M_ARG3                (a64 << 29)
#define F_OPT_PREEXEC                   (a64 << 30)
#define F_OPT_POSTEXEC                  (a64 << 31)
#define F_OPT_NOBACKUP                  (a64 << 32)
#define F_OPT_C_GHOSTONLY               (a64 << 33)
#define F_OPT_XDEV                      (a64 << 34)
#define F_OPT_XBLK                      (a64 << 35)
#define F_OPT_MATCHQ                    (a64 << 36)
#define F_OPT_IMATCHQ                   (a64 << 37)
#define F_OPT_CDIRONLY                  (a64 << 38)
#define F_OPT_SORT                      (a64 << 39)
#define F_OPT_HASLOM                    (a64 << 40)
#define F_OPT_HAS_G_LOM                 (a64 << 41)
#define F_OPT_FORCE2                    (a64 << 42)
#define F_OPT_VERBOSE5                  (a64 << 43)
#define F_OPT_SHAREDMEM                 (a64 << 44)
#define F_OPT_SHMRELOAD                 (a64 << 45)
#define F_OPT_LOADQ                     (a64 << 46)
#define F_OPT_SHMDESTROY                (a64 << 47)
#define F_OPT_SHMDESTONEXIT             (a64 << 48)
#define F_OPT_MODE_BINARY               (a64 << 49)
#define F_OPT_IFIRSTRES                 (a64 << 50)
#define F_OPT_IFIRSTHIT                 (a64 << 51)
#define F_OPT_HASMAXHIT                 (a64 << 52)
#define F_OPT_HASMAXRES                 (a64 << 53)
#define F_OPT_PROCREV                   (a64 << 54)
#define F_OPT_NOFQ                      (a64 << 55)
#define F_OPT_IFRH_E                    (a64 << 56)
#define F_OPT_NOGLCONF                  (a64 << 57)
#define F_OPT_MAXDEPTH                  (a64 << 58)
#define F_OPT_MINDEPTH                  (a64 << 59)
#define F_OPT_XFD                       (a64 << 60)
#define F_OPT_ZPRUNEDUP                 (a64 << 61)
#define F_OPT_DIR_FULL_REBUILD          (a64 << 62)
#define F_OPT_FORMAT_EXPORT             (a64 << 63)

// gfl0

#define F_OPT_PRINT                     (a64 << 1)
#define F_OPT_STDIN                     (a64 << 2)
#define F_OPT_PRINTF                    (a64 << 3)
#define F_OPT_PCE_NO_POST_EXEC          (a64 << 4)
#define F_OPT_XRETRY                    (a64 << 5)
#define F_OPT_DRINDEPTH                 (a64 << 6)
#define F_OPT_ARR_DIST                  (a64 << 7)
#define F_OPT_NO_CHECK_NUKED            (a64 << 8)
#define F_OPT_LOOP_SLEEP                (a64 << 9)
#define F_OPT_LOOP_USLEEP               (a64 << 10)
#define F_OPT_PS_ABSSILENT              (a64 << 11)
#define F_OPT_NOSTATS                   (a64 << 12)
#define F_OPT_STATS                     (a64 << 13)
#define F_OPT_GZIP                      (a64 << 14)

#define F_DL_FOPEN_BUFFER               (a32 << 1)
#define F_DL_FOPEN_FILE                 (a32 << 2)
#define F_DL_FOPEN_REWIND               (a32 << 3)
#define F_DL_FOPEN_SHM                  (a32 << 4)

#define F_OPT_HASMATCH                  (F_OPT_HAS_G_REGEX|F_OPT_HAS_G_MATCH|F_OPT_HAS_G_LOM|F_OPT_HASMAXHIT|F_OPT_HASMAXRES)
#define F_OPT_VERBMAX                   (F_OPT_VERBOSE|F_OPT_VERBOSE2|F_OPT_VERBOSE3|F_OPT_VERBOSE4|F_OPT_VERBOSE5)

#define F_OVRR_IPC                      (a32 << 1)
#define F_OVRR_GLROOT                   (a32 << 2)
#define F_OVRR_SITEROOT                 (a32 << 3)
#define F_OVRR_DUPEFILE                 (a32 << 4)
#define F_OVRR_LASTONLOG                (a32 << 5)
#define F_OVRR_ONELINERS                (a32 << 6)
#define F_OVRR_DIRLOG                   (a32 << 7)
#define F_OVRR_NUKELOG                  (a32 << 8)
#define F_OVRR_NUKESTR                  (a32 << 9)
#define F_OVRR_IMDBLOG                  (a32 << 10)
#define F_OVRR_TVLOG                    (a32 << 11)
#define F_OVRR_GAMELOG                  (a32 << 12)
#define F_OVRR_GE1LOG                   (a32 << 13)
#define F_OVRR_LOGFILE                  (a32 << 14)
#define F_ESREDIRFAILED                 (a32 << 15)
#define F_BM_TERM                       (a32 << 16)
#define F_OVRR_GE2LOG                   (a32 << 17)
#define F_SREDIRFAILED                  (a32 << 18)
#define F_OVRR_GE3LOG                   (a32 << 19)
#define F_OVRR_GE4LOG                   (a32 << 20)
#define F_OVRR_SCONF                    (a32 << 21)
#define F_OVRR_GCONF                    (a32 << 22)

#define L_STFO_SORT                     0x1
#define L_STFO_FILTER                   0x2

uint64_t gfl0, gfl;
uint32_t ofl;

uint8_t l_sfo;

#ifdef HAVE_ZLIB_H
uint8_t comp_level;
#endif

int
(*print_str)(const char * volatile buf, ...);

typedef struct ___si_argv0
{
  int ret;
  uint32_t flags;
  char p_buf_1[4096];
  char p_buf_2[PATH_MAX];
  char s_ret[262144];
  char *buffer;
} _si_argv0, *__si_argv0;

#endif /* GLOB_H_ */
