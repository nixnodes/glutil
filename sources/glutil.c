/*
 * ============================================================================
 * Name        : glutil
 * Authors     : nymfo, siska
 * Version     : 1.12-21
 * Description : glFTPd binary logs utility
 * ============================================================================
 *
 *  Copyright (C) 2013 NixNodes
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _BSD_SOURCE
#define _GNU_SOURCE

#define _LARGEFILE64_SOURCE 1
#define _LARGEFILE_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <inttypes.h>
#include <time.h>

/* requires 'native' glconf.h (/glroot/bin/sources) */
#include "glconf.h"

/* gl root  */
#ifndef glroot
#define glroot "/glftpd"
#endif

/* site root, relative to gl root */
#ifndef siteroot
#define siteroot "/site"
#endif

/* ftp-data, relative to gl root */
#ifndef ftp_data
#define ftp_data "/ftp-data"
#endif

/* glftpd's user segment ipc key */
#ifndef shm_ipc
#define shm_ipc 0x0000DEAD
#endif

/* glutil data buffers ipc keys */
#define IPC_KEY_DIRLOG		0xDEAD1000
#define IPC_KEY_NUKELOG		0xDEAD1100
#define IPC_KEY_DUPEFILE	0xDEAD1200
#define IPC_KEY_LASTONLOG	0xDEAD1300
#define IPC_KEY_ONELINERS	0xDEAD1400
#define IPC_KEY_IMDBLOG 	0xDEAD1500
#define IPC_KEY_GAMELOG 	0xDEAD1600
#define IPC_KEY_TVRAGELOG 	0xDEAD1700
#define IPC_KEY_GEN1LOG 	0xDEAD1800
#define IPC_KEY_GEN2LOG 	0xDEAD1900
#define IPC_KEY_GEN3LOG         0xDEAD2000

/*
 * log file path
 * setting this variable enables logging (default is off)
 */
#ifndef log_file
#define log_file ""
#endif

/* dirlog file path */
#ifndef dir_log
#define dir_log "/glftpd/ftp-data/logs/dirlog"
#endif

/* nukelog file path */
#ifndef nuke_log
#define nuke_log "/glftpd/ftp-data/logs/nukelog"
#endif

/* dupe file path */
#ifndef dupe_file
#define dupe_file "/glftpd/ftp-data/logs/dupefile"
#endif

/* last-on log file path */
#ifndef last_on_log
#define last_on_log "/glftpd/ftp-data/logs/laston.log"
#endif

/* oneliner file path */
#ifndef oneliner_file
#define oneliner_file "/glftpd/ftp-data/logs/oneliners.log"
#endif

/* imdb log file path */
#ifndef imdb_file
#define imdb_file "/glftpd/ftp-data/logs/imdb.log"
#endif

/* game log file path */
#ifndef game_log
#define game_log "/glftpd/ftp-data/logs/game.log"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log "/glftpd/ftp-data/logs/tv.log"
#endif

/* generic 1 log file path */
#ifndef ge1_log
#define ge1_log "/glftpd/ftp-data/logs/gen1.log"
#endif

/* generic 2 log file path */
#ifndef ge2_log
#define ge2_log "/glftpd/ftp-data/logs/gen2.log"
#endif

/* generic 3 log file path */
#ifndef ge3_log
#define ge3_log "/glftpd/ftp-data/logs/gen3.log"
#endif

/* see README file about this */
#ifndef du_fld
#define du_fld "/glftpd/bin/glutil.folders"
#endif

/* file extensions to skip generating crc32 (SFV mode)*/
#ifndef PREG_SFV_SKIP_EXT
#define PREG_SFV_SKIP_EXT "\\.(nfo|sfv(\\.tmp|))$"
#endif

/* -------------------------- */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <sys/wait.h>

#ifndef WEXITSTATUS
#define	WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#endif

#define VER_MAJOR 1
#define VER_MINOR 12
#define VER_REVISION 21
#define VER_STR ""

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
#define __STR_ARCH	"x86_64"
#define __AA_SPFH 	"%.16X"
#else
#define uintaa_t uint32_t
#define ENV_32
#define __STR_ARCH	"i686"
#define __AA_SPFH 	"%.8X"
#endif

#define MSG_NL                  "\n"
#define MSG_TAB                  "\t"

#define MAX_uint64_t 		((uint64_t) -1)
#define MAX_uint32_t 		((uint32_t) -1)

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define GM_MAX			16384

#define a64			((ulint64_t) 1)
#define a32			((uint32_t) 1)

/* -------------------------------------------- */

#pragma pack(push, 4)

typedef struct ___d_imdb
{
  char dirname[255];
  int32_t timestamp;
  char imdb_id[64]; /* IMDB ID */
  float rating; /* IMDB Rating */
  uint32_t votes; /* IMDB Votes */
  char genres[255]; /* List of genres (comma delimited) */
  uint16_t year;
  uint8_t _d_unused_m[3];
  char title[128];
  int32_t released;
  uint32_t runtime;
  char rated[8];
  char actors[128];
  char director[64];
  char synopsis[198];
  /* ------------- */
  uint8_t _d_unused_e[32]; /* Reserved for future use */

} _d_imdb, *__d_imdb;

typedef struct ___d_game
{
  char dirname[255];
  int32_t timestamp;
  float rating;
  /* ------------- */
  uint8_t _d_unused[512]; /* Reserved for future use */

} _d_game, *__d_game;

typedef struct ___d_tvrage
{
  char dirname[255];
  int32_t timestamp;
  uint32_t showid;
  char name[128];
  char link[128];
  char country[24];
  int32_t started;
  int32_t ended;
  uint16_t seasons;
  char status[64];
  uint32_t runtime;
  char airtime[6];
  char airday[32];
  char class[64];
  char genres[256];
  uint16_t startyear;
  uint16_t endyear;
  char network[72];
  uint8_t _d_unused[180]; /* Reserved for future use */
} _d_tvrage, *__d_tvrage;

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

typedef struct ___d_generic_s1644
{
  uint32_t ui32_1;
  uint32_t ui32_2;
  uint32_t ui32_3;
  uint32_t ui32_4;
  int32_t i32_1;
  int32_t i32_2;
  int32_t i32_3;
  int32_t i32_4;
  float f_1;
  float f_2;
  float f_3;
  float f_4;
  uint8_t _d_unused_1[32];
  uint64_t ui64_1;
  uint64_t ui64_2;
  uint64_t ui64_3;
  uint64_t ui64_4;
  char s_1[255];
  char s_2[255];
  char s_3[255];
  char s_4[255];
  char s_5[128];
  char s_6[128];
  char s_7[128];
  char s_8[128];
} _d_generic_s1644, *__d_generic_s1644;

typedef struct ___d_generic_s800
{
  int32_t i32_1;
  int32_t i32_2;
  uint32_t ui32_1;
  uint32_t ui32_2;
  uint64_t ui64_1;
  uint64_t ui64_2;
  char s_1[255];
  char s_2[255];
  char s_3[128];
  char s_4[128];
} _d_generic_s800, *__d_generic_s800;

#pragma pack(pop)

/* -------------------------------------------- */

typedef struct e_arg
{
  int depth;
  uint32_t flags;
  char buffer[PATH_MAX];
  char buffer2[PATH_MAX + 10];
  struct dirlog *dirlog;
  time_t t_stor;
} ear;

typedef struct option_reference_array
{
  char *option;
  void *function, *arg_cnt;
}*p_ora;

struct d_stats
{
  uint64_t bw, br, rw;
};

typedef struct mda_object
{
  void *ptr;
  void *next;
  void *prev;
//	unsigned char flags;
}*p_md_obj, md_obj;

#define F_MDA_REFPTR		        (a32 << 1)
#define F_MDA_FREE			(a32 << 2)
#define F_MDA_REUSE			(a32 << 3)
#define F_MDA_WAS_REUSED	        (a32 << 4)
#define F_MDA_EOF			(a32 << 5)
#define F_MDA_FIRST_REUSED              (a32 << 6)

typedef struct mda_header
{
  p_md_obj objects; /* holds references */
  p_md_obj pos, r_pos, c_pos, first, last;
  off_t offset, r_offset, count, hitcnt, rescnt;
  uint32_t flags;
  void *lref_ptr;
} mda, *pmda;

typedef struct config_header
{
  char *key, *value;
  mda data;
} cfg_h, *p_cfg_h;

typedef int
(*__d_exec)(void *buffer, void *callback, char *ex_str, void *hdl);

typedef struct ___execv
{
  int argc;
  char **argv, **argv_c;
  char exec_v_path[PATH_MAX];
  mda ac_ref;
  mda mech;
  __d_exec exc;
} _execv, *__execv;

#define MAX_VAR_LEN				4096

typedef void
(*__g_ipcbm)(void *hdl, pmda md, int *r_p);
typedef int
(*__g_proc_t)(void *, char *, char *, size_t);
typedef void *
(*__g_proc_v)(void *, char *, char *, size_t, void *);
typedef void *
(*__d_ref_to_pv)(void *arg, char *match, int *output);
typedef void
(*_d_omfp)(void *hdl, void *ptr, char *sbuffer);
typedef int
(*_d_proc3)(void *, char *);

typedef struct g_handle
{
  FILE *fh;
  off_t offset, bw, br, total_sz;
  off_t rw;
  uint32_t block_sz;
  uint64_t flags;
  mda buffer, w_buffer;
  mda _match_rr;
  off_t max_results, max_hits;
  __g_ipcbm ifrh_l0, ifrh_l1;
  _execv exec_args;
  mda print_mech;
  void *data;
  char s_buffer[PATH_MAX];
  char mv1_b[MAX_VAR_LEN];
  char file[PATH_MAX], mode[32];
  mode_t st_mode;
  key_t ipc_key;
  int shmid;
  struct shmid_ds ipcbuf;
  int
  (*g_proc0)(void *, char *, char *);
  __g_proc_v g_proc1_ps;
  __d_ref_to_pv g_proc2;
  __g_proc_v g_proc1_lookup;
  _d_proc3 g_proc3, g_proc3_batch, g_proc3_export;
  _d_omfp g_proc4;
  size_t j_offset, jm_offset;
  int d_memb;
  void *_x_ref;
} _g_handle, *__g_handle;

typedef struct g_cfg_ref
{
  mda cfg;
  char file[PATH_MAX];
} cfg_r, *p_cfg_r;

typedef struct ___si_argv0
{
  int ret;
  uint32_t flags;
  char p_buf_1[4096];
  char p_buf_2[PATH_MAX];
  char s_ret[262144];
} _si_argv0, *__si_argv0;

typedef struct sig_jmp_buf
{
  sigjmp_buf env, p_env;
  uint32_t flags, pflags;
  int id, pid;
  unsigned char ci, pci;
  char type[32];
  void *callback, *arg;
} sigjmp, *p_sigjmp;

typedef float
(*g_tf_p)(void *base, size_t offset);
typedef uint64_t
(*g_t_p)(void *base, size_t offset);
typedef int64_t
(*g_ts_p)(void *base, size_t offset);
typedef int
(*g_op)(int s, int d);

typedef struct ___g_lom
{
  int result;
  /* --- */
  uint32_t flags;
  g_t_p g_t_ptr_left;
  g_ts_p g_ts_ptr_left;
  g_t_p g_t_ptr_right;
  g_ts_p g_ts_ptr_right;
  g_tf_p g_tf_ptr_left;
  g_tf_p g_tf_ptr_right;
  int
  (*g_icomp_ptr)(uint64_t s, uint64_t d);
  int
  (*g_iscomp_ptr)(int64_t s, int64_t d);
  int
  (*g_fcomp_ptr)(float s, float d);
  int
  (*g_lom_vp)(void *d_ptr, void * lom);
  g_op g_oper_ptr;
  uint64_t t_left, t_right;
  int64_t ts_left, ts_right;
  float tf_left, tf_right;
  /* --- */
  size_t t_l_off, t_r_off;
} _g_lom, *__g_lom;

typedef float
(*_d_t32_f_vg)(ulint64_t *i);
typedef ulint64_t
(*_d_f_t32_vg)(float *i);

typedef struct ___d_drt_h
{
  uint32_t flags;
  char direc[128];
  __g_proc_v fp_rval1;
  uint32_t t_1;
  time_t ts_1;
  char c_1;
  size_t vp_off1;
  __g_handle hdl;
} _d_drt_h, *__d_drt_h;

typedef struct ___g_match_h
{
  uint32_t flags;
  char *match, *field;
  int reg_i_m, match_i_m, regex_flags;
  regex_t preg;
  mda lom;
  g_op g_oper_ptr;
  __g_proc_v pmstr_cb;
  _d_drt_h dtr;
  char *data;
} _g_match, *__g_match;

typedef struct ___g_eds
{
  uint32_t flags;
  uint32_t r_minor, r_major;
  struct stat st;
  off_t depth;
} _g_eds, *__g_eds;

typedef struct ___d_xref_ct
{
  uint8_t active;
  time_t curtime;
  int ct_off;
} _d_xref_ct, *__d_xref_ct;

typedef struct ___d_xref
{
  char name[PATH_MAX];
  struct stat st;
  uint8_t type;
  uint8_t r, w, x;
  uint8_t uperm, gperm, operm;
  uint16_t perm;
  uint32_t flags;
  uint32_t crc32;
  uint32_t major;
  uint32_t minor;
  float sparseness;
  _d_xref_ct ct[GM_MAX / 16];
} _d_xref, *__d_xref;

typedef struct ___d_exec_ch
{
  char *st_ptr;
  size_t len;
  __g_proc_v callback;
  _d_drt_h dtr;
} _d_exec_ch, *__d_exec_ch;

typedef struct ___d_argv_ch
{
  int cindex;
  mda mech;
} _d_argv_ch, *__d_argv_ch;

/*
 * CRC-32 polynomial 0x04C11DB7 (0xEDB88320)
 * see http://en.wikipedia.org/wiki/Cyclic_redundancy_check#Commonly_used_and_standardized_CRCs
 */

static uint32_t crc_32_tab[] =
  { 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
      0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
      0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
      0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
      0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
      0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
      0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
      0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
      0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
      0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
      0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
      0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
      0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
      0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
      0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
      0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
      0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
      0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
      0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
      0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
      0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
      0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
      0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
      0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
      0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
      0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
      0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
      0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
      0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
      0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
      0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
      0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
      0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
      0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
      0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
      0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
      0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
      0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
      0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
      0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
      0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
      0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
      0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ ((uint8_t)octet)) & 0xff] ^ ((crc) >> 8))

uint32_t
crc32(uint32_t crc32, uint8_t *buf, size_t len)
{
  for (; len; len--, buf++)
    {
      crc32 = UPDC32(*buf, crc32);
    }

  return crc32;
}

#define PTRSZ 				(sizeof(void*))

#define REG_MATCHES_MAX	 	        4

#define UPD_MODE_RECURSIVE 		0x1
#define UPD_MODE_SINGLE			0x2
#define UPD_MODE_CHECK			0x3
#define UPD_MODE_DUMP			0x4
#define UPD_MODE_DUMP_NUKE		0x5
#define UPD_MODE_DUPE_CHK		0x6
#define UPD_MODE_REBUILD		0x7
#define UPD_MODE_DUMP_DUPEF 	        0x8
#define UPD_MODE_DUMP_LON		0x9
#define UPD_MODE_DUMP_ONEL		0xA
#define UPD_MODE_DUMP_ONL		0xB
#define UPD_MODE_NOOP			0xC
#define UPD_MODE_MACRO			0xD
#define UPD_MODE_FORK			0xE
#define UPD_MODE_BACKUP			0xF
#define UPD_MODE_DUMP_USERS		0x10
#define UPD_MODE_DUMP_GRPS		0x11
#define UPD_MODE_DUMP_GEN		0x12
#define UPD_MODE_WRITE			0x13
#define UPD_MODE_DUMP_IMDB		0x14
#define UPD_MODE_DUMP_GAME		0x15
#define UPD_MODE_DUMP_TV		0x16
#define UPD_MODE_DUMP_GENERIC	        0x17

#define PRIO_UPD_MODE_MACRO 	        0x1001
#define PRIO_UPD_MODE_INFO 		0x1002

/* -- flags -- */

#define F_OPT_FORCE 			(a64 << 1)
#define F_OPT_VERBOSE 			(a64 << 2)
#define F_OPT_VERBOSE2 			(a64 << 3)
#define F_OPT_VERBOSE3 			(a64 << 4)
#define F_OPT_SFV	 		(a64 << 5)
#define F_OPT_NOWRITE			(a64 << 6)
#define F_OPT_NOBUFFER			(a64 << 7)
#define F_OPT_UPDATE			(a64 << 8)
#define F_OPT_FIX			(a64 << 9)
#define F_OPT_FOLLOW_LINKS		(a64 << 10)
#define F_OPT_FORMAT_BATCH		(a64 << 11)
#define F_OPT_KILL_GLOBAL		(a64 << 12)
#define F_OPT_MODE_RAWDUMP  	        (a64 << 13)
#define F_OPT_HAS_G_REGEX		(a64 << 14)
#define F_OPT_VERBOSE4 			(a64 << 15)
#define F_OPT_WBUFFER			(a64 << 16)
#define F_OPT_FORCEWSFV			(a64 << 17)
#define F_OPT_FORMAT_COMP   	        (a64 << 18)
#define F_OPT_DAEMONIZE			(a64 << 19)
#define F_OPT_LOOP			(a64 << 20)
#define F_OPT_LOOPEXEC			(a64 << 21)
#define F_OPT_PS_SILENT			(a64 << 22)
#define F_OPT_PS_TIME			(a64 << 23)
#define F_OPT_PS_LOGGING		(a64 << 24)
#define F_OPT_TERM_ENUM			(a64 << 25)
#define F_OPT_HAS_G_MATCH		(a64 << 26)
#define F_OPT_HAS_M_ARG1		(a64 << 27)
#define F_OPT_HAS_M_ARG2		(a64 << 28)
#define F_OPT_HAS_M_ARG3		(a64 << 29)
#define F_OPT_PREEXEC			(a64 << 30)
#define F_OPT_POSTEXEC			(a64 << 31)
#define F_OPT_NOBACKUP			(a64 << 32)
#define F_OPT_C_GHOSTONLY		(a64 << 33)
#define F_OPT_XDEV			(a64 << 34)
#define F_OPT_XBLK			(a64 << 35)
#define F_OPT_MATCHQ			(a64 << 36)
#define F_OPT_IMATCHQ			(a64 << 37)
#define F_OPT_CDIRONLY			(a64 << 38)
#define F_OPT_SORT 			(a64 << 39)
#define F_OPT_HASLOM			(a64 << 40)
#define F_OPT_HAS_G_LOM			(a64 << 41)
#define F_OPT_FORCE2 			(a64 << 42)
#define F_OPT_VERBOSE5 			(a64 << 43)
#define F_OPT_SHAREDMEM			(a64 << 44)
#define F_OPT_SHMRELOAD			(a64 << 45)
#define F_OPT_LOADQ			(a64 << 46)
#define F_OPT_SHMDESTROY		(a64 << 47)
#define F_OPT_SHMDESTONEXIT		(a64 << 48)
#define F_OPT_MODE_BINARY		(a64 << 49)
#define F_OPT_IFIRSTRES			(a64 << 50)
#define F_OPT_IFIRSTHIT			(a64 << 51)
#define F_OPT_HASMAXHIT			(a64 << 52)
#define F_OPT_HASMAXRES			(a64 << 53)
#define F_OPT_PROCREV			(a64 << 54)
#define F_OPT_NOFQ			(a64 << 55)
#define F_OPT_IFRH_E			(a64 << 56)
#define F_OPT_NOGLCONF			(a64 << 57)
#define F_OPT_MAXDEPTH 			(a64 << 58)
#define F_OPT_MINDEPTH 			(a64 << 59)
#define F_OPT_XFD 			(a64 << 60)
#define F_OPT_ZPRUNEDUP			(a64 << 61)
#define F_OPT_PEX			(a64 << 62)
#define F_OPT_FORMAT_EXPORT             (a64 << 63)

#define F_OPT_PRINT                     (a64 << 1)
#define F_OPT_STDIN                     (a64 << 2)
#define F_OPT_PRINTF                    (a64 << 3)

#define F_OPT_HASMATCH			(F_OPT_HAS_G_REGEX|F_OPT_HAS_G_MATCH|F_OPT_HAS_G_LOM|F_OPT_HASMAXHIT|F_OPT_HASMAXRES)

#define F_DL_FOPEN_BUFFER		(a32 << 1)
#define F_DL_FOPEN_FILE			(a32 << 2)
#define F_DL_FOPEN_REWIND		(a32 << 3)
#define F_DL_FOPEN_SHM			(a32 << 4)

#define F_EARG_SFV 			(a32 << 1)
#define F_EAR_NOVERB			(a32 << 2)

#define F_FC_MSET_SRC			(a32 << 1)
#define F_FC_MSET_DEST			(a32 << 2)

#define F_GH_NOMEM  			(a64 << 1)
#define F_GH_ISDIRLOG			(a64 << 2)
#define F_GH_EXEC			(a64 << 3)
#define F_GH_ISNUKELOG			(a64 << 4)
#define F_GH_FFBUFFER			(a64 << 5)
#define F_GH_WAPPEND			(a64 << 6)
#define F_GH_DFWASWIPED			(a64 << 7)
#define F_GH_DFNOWIPE			(a64 << 8)
#define F_GH_ISDUPEFILE			(a64 << 9)
#define F_GH_ISLASTONLOG		(a64 << 10)
#define F_GH_ISONELINERS		(a64 << 11)
#define F_GH_ONSHM			(a64 << 12)
#define F_GH_ISONLINE			(a64 << 13)
#define F_GH_ISIMDB			(a64 << 14)
#define F_GH_ISGAME			(a64 << 15)
#define F_GH_ISFSX			(a64 << 16)
#define F_GH_ISTVRAGE			(a64 << 17)
#define F_GH_ISGENERIC1			(a64 << 18)
#define F_GH_SHM			(a64 << 19)
#define F_GH_SHMRB			(a64 << 20)
#define F_GH_SHMDESTROY			(a64 << 21)
#define F_GH_SHMDESTONEXIT		(a64 << 22)
#define F_GH_FROMSTDIN			(a64 << 23)
#define F_GH_HASLOM			(a64 << 24)
#define F_GH_HASMATCHES			(a64 << 25)
#define F_GH_HASEXC			(a64 << 26)
#define F_GH_APFILT 			(a64 << 27)
#define F_GH_HASMAXRES			(a64 << 28)
#define F_GH_HASMAXHIT			(a64 << 29)
#define F_GH_IFRES			(a64 << 30)
#define F_GH_IFHIT			(a64 << 31)
#define F_GH_ISGENERIC2			(a64 << 32)
#define F_GH_HASSTRM			(a64 << 33)
#define F_GH_ISGENERIC3                 (a64 << 34)

/* these bits determine log type */
#define F_GH_ISTYPE			(F_GH_ISGENERIC3|F_GH_ISGENERIC2|F_GH_ISGENERIC1|F_GH_ISNUKELOG|F_GH_ISDIRLOG|F_GH_ISDUPEFILE|F_GH_ISLASTONLOG|F_GH_ISONELINERS|F_GH_ISONLINE|F_GH_ISIMDB|F_GH_ISGAME|F_GH_ISFSX|F_GH_ISTVRAGE)

#define F_GH_ISSHM			(F_GH_SHM|F_GH_ONSHM)
#define F_GH_ISMP			(F_GH_HASMATCHES|F_GH_HASMAXRES|F_GH_HASMAXHIT)

#define F_OVRR_IPC			(a32 << 1)
#define F_OVRR_GLROOT			(a32 << 2)
#define F_OVRR_SITEROOT			(a32 << 3)
#define F_OVRR_DUPEFILE			(a32 << 4)
#define F_OVRR_LASTONLOG		(a32 << 5)
#define F_OVRR_ONELINERS		(a32 << 6)
#define F_OVRR_DIRLOG			(a32 << 7)
#define F_OVRR_NUKELOG			(a32 << 8)
#define F_OVRR_NUKESTR			(a32 << 9)
#define F_OVRR_IMDBLOG			(a32 << 10)
#define F_OVRR_TVLOG			(a32 << 11)
#define F_OVRR_GAMELOG			(a32 << 12)
#define F_OVRR_GE1LOG			(a32 << 13)
#define F_OVRR_LOGFILE			(a32 << 14)
#define F_ESREDIRFAILED			(a32 << 15)
#define F_BM_TERM			(a32 << 16)
#define F_OVRR_GE2LOG			(a32 << 17)
#define F_SREDIRFAILED                  (a32 << 18)
#define F_OVRR_GE3LOG                   (a32 << 19)

#define F_PD_RECURSIVE 			(a32 << 1)
#define F_PD_MATCHDIR			(a32 << 2)
#define F_PD_MATCHREG			(a32 << 3)

#define F_PD_MATCHTYPES			(F_PD_MATCHDIR|F_PD_MATCHREG)

#define F_GSORT_DESC			(a32 << 1)
#define F_GSORT_ASC			(a32 << 2)
#define F_GSORT_RESETPOS		(a32 << 3)
#define F_GSORT_NUMERIC	    	        (a32 << 4)

#define F_GSORT_ORDER			(F_GSORT_DESC|F_GSORT_ASC)

#define F_ENUMD_ENDFIRSTOK		(a32 << 1)
#define F_ENUMD_BREAKONBAD		(a32 << 2)
#define F_ENUMD_NOXDEV			(a32 << 3)
#define F_ENUMD_NOXBLK			(a32 << 4)

#define F_GM_ISREGEX			(a32 << 1)
#define F_GM_ISMATCH			(a32 << 2)
#define F_GM_ISLOM			(a32 << 3)
#define F_GM_IMATCH			(a32 << 4)
#define F_GM_NAND			(a32 << 5)
#define F_GM_NOR			(a32 << 6)

#define F_GM_TYPES			(F_GM_ISREGEX|F_GM_ISMATCH|F_GM_ISLOM)

#define F_LOM_LVAR_KNOWN		(a32 << 1)
#define F_LOM_RVAR_KNOWN		(a32 << 2)
#define F_LOM_FLOAT			(a32 << 3)
#define F_LOM_INT			(a32 << 4)
#define F_LOM_HASOPER			(a32 << 5)
#define F_LOM_FLOAT_DBL			(a32 << 6)
#define F_LOM_INT_S			(a32 << 7)

#define F_LOM_TYPES			(F_LOM_FLOAT|F_LOM_INT|F_LOM_INT_S)
#define F_LOM_VAR_KNOWN			(F_LOM_LVAR_KNOWN|F_LOM_RVAR_KNOWN)

#define F_EDS_ROOTMINSET		(a32 << 1)
#define F_EDS_KILL			(a32 << 2)

/* -- end flags -- */

#define V_MB				0x100000

#define DL_SZ 				sizeof(struct dirlog)
#define NL_SZ 				sizeof(struct nukelog)
#define DF_SZ 				sizeof(struct dupefile)
#define LO_SZ 				sizeof(struct lastonlog)
#define OL_SZ 				sizeof(struct oneliner)
#define ON_SZ 				sizeof(struct ONLINE)
#define ID_SZ 				sizeof(_d_imdb)
#define GM_SZ 				sizeof(_d_game)
#define TV_SZ 				sizeof(_d_tvrage)
#define G1_SZ 				sizeof(_d_generic_s2044)
#define G2_SZ 				sizeof(_d_generic_s1644)
#define G3_SZ                           sizeof(_d_generic_s800)

#define CRC_FILE_READ_BUFFER_SIZE 	64512
#define	DB_MAX_SIZE 			((long long int)1073741824)   /* max file size allowed to load into memory */
#define MAX_EXEC_STR 			262144

#define	PIPE_READ_MAX			0x2000
#define MAX_DATAIN_F			(V_MB*32)
#define MAX_G_PRINT_STATS_BUFFER	8192

#define MSG_GEN_NODFILE 		"ERROR: %s: could not open data file: %s\n"
#define MSG_GEN_DFWRITE 		"ERROR: %s: [%d] [%llu] writing record to dirlog failed! (mode: %s)\n"
#define MSG_GEN_DFCORRU 		"ERROR: %s: corrupt data file detected! (data file size [%llu] is not a multiple of block size [%d])\n"
#define MSG_GEN_DFRFAIL 		"ERROR: %s: building data file failed!\n"
#define MSG_BAD_DATATYPE 		"ERROR: %s: could not determine data type\n"
#define MSG_GEN_WROTE			"STATS: %s: wrote %llu bytes in %llu records\n"

#define DEFPATH_LOGS 			"/logs"
#define DEFPATH_USERS 			"/users"
#define DEFPATH_GROUPS 			"/groups"

#define DEFF_DIRLOG 			"dirlog"
#define DEFF_NUKELOG 			"nukelog"
#define DEFF_LASTONLOG  		"laston.log"
#define DEFF_DUPEFILE 			"dupefile"
#define DEFF_ONELINERS 			"oneliners.log"
#define DEFF_DULOG	 		"glutil.log"
#define DEFF_IMDB	 		"imdb.log"
#define DEFF_GAMELOG 			"game.log"
#define DEFF_TV 			"tv.log"
#define DEFF_GEN1 			"gen1.log"
#define DEFF_GEN2 			"gen2.log"
#define DEFF_GEN3                       "gen3.log"

#define NUKESTR_DEF			"NUKED-%s"

#ifdef GLCONF
char GLCONF_I[PATH_MAX] =
  { GLCONF };
#else
char GLCONF_I[PATH_MAX] =
  { 0};
#endif

#define _MC_GLOB_DIR 			"dir"
#define _MC_GLOB_BASEDIR 		"basedir"
#define _MC_GLOB_DIRNAME 		"ndir"
#define _MC_GLOB_MODE 			"mode"
#define _MC_GLOB_PID 			"pid"
#define _MC_GLOB_USER			"user"
#define _MC_GLOB_GROUP			"group"
#define _MC_GLOB_TIME			"time"
#define _MC_GLOB_SIZE			"size"
#define _MC_GLOB_SCORE			"score"
#define _MC_GLOB_RUNTIME		"runtime"
#define _MC_GLOB_LOGON			"logon"
#define _MC_GLOB_LOGOFF			"logoff"
#define _MC_GLOB_DOWNLOAD		"download"
#define _MC_GLOB_UPLOAD			"upload"
#define _MC_GLOB_STATUS			"status"
#define _MC_GLOB_XREF                   "x:"
#define _MC_GLOB_XGREF                  "xg:"

#define F_SIGERR_CONTINUE 		0x1  /* continue after exception */

#define ID_SIGERR_UNSPEC 		0x0
#define ID_SIGERR_MEMCPY 		0x1
#define ID_SIGERR_STRCPY 		0x2
#define ID_SIGERR_FREE 			0x3
#define ID_SIGERR_FREAD 		0x4
#define ID_SIGERR_FWRITE 		0x5
#define ID_SIGERR_FOPEN 		0x6
#define ID_SIGERR_FCLOSE 		0x7
#define ID_SIGERR_MEMMOVE 		0x8

sigjmp g_sigjmp =
  {
    {
      {
        { 0 } } } };
uint64_t gfl0 = 0x0, gfl = F_OPT_WBUFFER;
uint32_t ofl = 0;
FILE *fd_log = NULL;
char LOGFILE[PATH_MAX] =
  { log_file };

void
e_pop(p_sigjmp psm)
{
  memcpy(psm->p_env, psm->env, sizeof(sigjmp_buf));
  psm->pid = psm->id;
  psm->pflags = psm->flags;
  psm->pci = psm->ci;
}

void
e_push(p_sigjmp psm)
{
  memcpy(psm->env, psm->p_env, sizeof(sigjmp_buf));
  psm->id = psm->pid;
  psm->flags = psm->pflags;
  psm->ci = psm->pci;
}

void
g_setjmp(uint32_t flags, char *type, void *callback, void *arg)
{
  if (flags)
    {
      g_sigjmp.flags = flags;
    }
  g_sigjmp.id = ID_SIGERR_UNSPEC;

  bzero(g_sigjmp.type, 32);
  memcpy(g_sigjmp.type, type, strlen(type));

  return;
}

void *
g_memcpy(void *dest, const void *src, size_t n)
{
  void *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags = 0;
  g_sigjmp.id = ID_SIGERR_MEMCPY;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = memcpy(dest, src, n);
    }
  e_push(&g_sigjmp);
  return ret;
}

void *
g_memmove(void *dest, const void *src, size_t n)
{
  void *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags = 0;
  g_sigjmp.id = ID_SIGERR_MEMMOVE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = memmove(dest, src, n);
    }
  e_push(&g_sigjmp);
  return ret;
}

char *
g_strncpy(char *dest, const char *src, size_t n)
{
  char *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags = 0;
  g_sigjmp.id = ID_SIGERR_STRCPY;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = strncpy(dest, src, n);
    }
  e_push(&g_sigjmp);
  return ret;
}

void
g_free(void *ptr)
{
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FREE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      free(ptr);
    }
  e_push(&g_sigjmp);
}

size_t
g_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t ret = 0;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FREAD;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fread(ptr, size, nmemb, stream);
    }
  e_push(&g_sigjmp);
  return ret;
}

size_t
g_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t ret = 0;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FWRITE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fwrite(ptr, size, nmemb, stream);
    }
  e_push(&g_sigjmp);
  return ret;
}

FILE *
gg_fopen(const char *path, const char *mode)
{
  FILE *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FOPEN;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fopen(path, mode);
    }
  e_push(&g_sigjmp);
  return ret;
}

int
g_fclose(FILE *fp)
{
  int ret = 0;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FCLOSE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fclose(fp);
    }
  e_push(&g_sigjmp);
  return ret;
}

#define MSG_DEF_UNKN1 	"(unknown)"

void
sighdl_error(int sig, siginfo_t* siginfo, void* context)
{

  char *s_ptr1 = MSG_DEF_UNKN1, *s_ptr2 = MSG_DEF_UNKN1, *s_ptr3 = "";
  char buffer1[4096] =
    { 0 };

  switch (sig)
    {
  case SIGSEGV:
    s_ptr1 = "SEGMENTATION FAULT";
    break;
  case SIGFPE:
    s_ptr1 = "FLOATING POINT EXCEPTION";
    break;
  case SIGILL:
    s_ptr1 = "ILLEGAL INSTRUCTION";
    break;
  case SIGBUS:
    s_ptr1 = "BUS ERROR";
    break;
  case SIGTRAP:
    s_ptr1 = "TRACE TRAP";
    break;
  default:
    s_ptr1 = "UNKNOWN EXCEPTION";
    }

  snprintf(buffer1, 4096, ", fault address: 0x%.16llX",
      (ulint64_t) (uintaa_t) siginfo->si_addr);

  switch (g_sigjmp.id)
    {
  case ID_SIGERR_MEMCPY:
    s_ptr2 = "memcpy";
    break;
  case ID_SIGERR_STRCPY:
    s_ptr2 = "strncpy";
    break;
  case ID_SIGERR_FREE:
    s_ptr2 = "free";
    break;
  case ID_SIGERR_FREAD:
    s_ptr2 = "fread";
    break;
  case ID_SIGERR_FWRITE:
    s_ptr2 = "fwrite";
    break;
  case ID_SIGERR_FCLOSE:
    s_ptr2 = "fclose";
    break;
  case ID_SIGERR_MEMMOVE:
    s_ptr2 = "memove";
    break;
    }

  if (g_sigjmp.flags & F_SIGERR_CONTINUE)
    {
      s_ptr3 = ", resuming execution..";
    }

  fprintf(stderr, "EXCEPTION: %s: [%s] [%s] [%d]%s%s\n", s_ptr1, g_sigjmp.type,
      s_ptr2, siginfo->si_errno, buffer1, s_ptr3);

  usleep(450000);

  g_sigjmp.ci++;

  if (g_sigjmp.flags & F_SIGERR_CONTINUE)
    {
      siglongjmp(g_sigjmp.env, 0);
    }

  g_sigjmp.ci = 0;
  g_sigjmp.flags = 0;

  exit(siginfo->si_errno);
}

struct tm *
get_localtime(void)
{
  time_t t = time(NULL);
  return localtime(&t);
}

#define F_MSG_TYPE_ANY		 	MAX_uint32_t
#define F_MSG_TYPE_EXCEPTION 	        (a32 << 1)
#define F_MSG_TYPE_ERROR 		(a32 << 2)
#define F_MSG_TYPE_WARNING 		(a32 << 3)
#define F_MSG_TYPE_NOTICE		(a32 << 4)
#define F_MSG_TYPE_STATS		(a32 << 5)
#define F_MSG_TYPE_NORMAL		(a32 << 6)

#define F_MSG_TYPE_EEW 			(F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_ERROR|F_MSG_TYPE_WARNING)

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

#define PSTR_MAX	(V_MB/4)

int
print_str(const char * volatile buf, ...)
{
  char d_buffer_2[PSTR_MAX + 1];
  va_list al;
  va_start(al, buf);

  if ((gfl & F_OPT_PS_LOGGING) || (gfl & F_OPT_PS_TIME))
    {
      struct tm tm = *get_localtime();
      snprintf(d_buffer_2, PSTR_MAX, "[%.2u-%.2u-%.2u %.2u:%.2u:%.2u] %s",
          (tm.tm_year + 1900) % 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
          tm.tm_min, tm.tm_sec, buf);
      if (fd_log)
        {
          char wl_buffer[PSTR_MAX + 1];
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

/* ---------------------------------------------------------------------------------- */

struct d_stats dl_stats =
  { 0 };
struct d_stats nl_stats =
  { 0 };

_g_handle g_act_1 =
  { 0 };
_g_handle g_act_2 =
  { 0 };

mda dirlog_buffer =
  { 0 };
mda nukelog_buffer =
  { 0 };

int updmode = 0;
char *argv_off = NULL;
char GLROOT[PATH_MAX] =
  { glroot };
char SITEROOT_N[PATH_MAX] =
  { siteroot };
char SITEROOT[PATH_MAX] =
  { 0 };
char DIRLOG[PATH_MAX] =
  { dir_log };
char NUKELOG[PATH_MAX] =
  { nuke_log };
char DU_FLD[PATH_MAX] =
  { du_fld };
char DUPEFILE[PATH_MAX] =
  { dupe_file };
char LASTONLOG[PATH_MAX] =
  { last_on_log };
char ONELINERS[PATH_MAX] =
  { oneliner_file };
char FTPDATA[PATH_MAX] =
  { ftp_data };
char IMDBLOG[PATH_MAX] =
  { imdb_file };
char GAMELOG[PATH_MAX] =
  { game_log };
char TVLOG[PATH_MAX] =
  { tv_log };
char GE1LOG[PATH_MAX] =
  { ge1_log };
char GE2LOG[PATH_MAX] =
  { ge2_log };
char GE3LOG[PATH_MAX] =
  { ge3_log };
char *LOOPEXEC = NULL;
long long int db_max_size = DB_MAX_SIZE;
key_t SHM_IPC = (key_t) shm_ipc;

int EXITVAL = 0;

int loop_interval = 0;
uint64_t loop_max = 0;
char *NUKESTR = NUKESTR_DEF;

char *exec_str = NULL;
char **exec_v = NULL;
int exec_vc = 0;
//char exec_v_path[PATH_MAX];
__d_exec exc = NULL;

mda glconf =
  { 0 };

char b_spec1[PATH_MAX];

void *_p_macro_argv = NULL;
int _p_macro_argc = 0;

uint32_t g_sleep = 0;
uint32_t g_usleep = 0;

void *p_argv_off = NULL;
void *prio_argv_off = NULL;

mda cfg_rf =
  { 0 };

uint32_t flags_udcfg = 0;

uint32_t g_sort_flags = 0;

off_t max_hits = 0, max_results = 0;
off_t max_depth, min_depth;

int execv_stdout_redir = -1;

int g_regex_flags = REG_EXTENDED;

size_t max_datain_f = MAX_DATAIN_F;

char b_glob[MAX_EXEC_STR + 1];

char *hpd_up =
    "glFTPd binary logs utility, version %d.%d-%d%s-%s\n"
        "\n"
        "Main options:\n"
        "\n Output:\n"
        "  -d, [--raw] [--batch] [--export|-E] [-print]\n"
        "                        Parse directory log and print to stdout in text/binary format\n"
        "                          (-vv prints dir nuke status from nukelog)\n"
        "                         --batch         Prints with simple formatting\n"
        "                         --export, -E    Prints in glutil export text format\n"
        "                         --raw           Prints raw binary data to stdout\n"
        "  -n, -||-              Parse nuke log\n"
        "  -i, -||-              Parse dupe file\n"
        "  -l, -||-              Parse last-on log\n"
        "  -o, -||-              Parse oneliners\n"
        "  -a, -||-              Parse iMDB log\n"
        "  -k, -||-              Parse game log\n"
        "  -h, -||-              Parse tvrage log\n"
        "  -w  -||- [--comp]     Parse online users data from shared memory\n"
        "  -q <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1|ge2> [--raw] [--batch] [--export|-E] [-print]\n"
        "                        Parse specified log\n"
        "  -t                    Parse all user files inside /ftp-data/users\n"
        "  -g                    Parse all group files inside /ftp-data/groups\n"
        "  -x <root dir> [-R] ([--dir]|[--file]|[--cdir]) [--maxdepth=<limit>] [--mindepth=<limit>] [--fd] [-print]\n"
        "                        Parses filesystem and processes each item found with internal filters/hooks\n"
        "                          --dir - scan directories only\n"
        "                          --file - scan files only (default is both dirs and files)\n"
        "                          --cdir - process only the root directory itself\n"
        "                          --maxdepth - limit how deep into the directory tree recursor descends\n"
        "                          --mindepth - process only when recursor depth is over <limit>\n"
        "                          --fd - apply filters before recursor descends into subdirectory\n"
        "                          --recursive (-R) - traverse the whole <root dir> directory tree\n"
        "  -print <fmt string>   Format output using {var} directives (see MANUAL for a field list)\n"
        "  -printf <fmt string>  Same as -printf, only does not print a new line character at the end\n"
        "  --stdin               Read data from stdin\n"
        "\n Input:\n"
        "  -e <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1>\n"
        "                        Rebuilds existing data file, based on filtering rules (see --exec,\n"
        "                          --[i]regex[i] and --[i]match\n"
        "  -z <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1> [--infile=/path/file] [--binary] [--prune]\n"
        "                        Creates a binary record from ASCII data, inserting it into the specified log\n"
        "                          Captures input from stdin, unless --infile is set\n"
        "                        --binary expects a normal binary log as input and merges it\n"
        "                        --prune skips importing duplicate records (full binary compare)\n"
        "\n Directory log:\n"
        "  -s <folders>          Import specific directories. Use quotation marks with multiple arguments\n"
        "                           <folders> are passed relative to SITEROOT, separated by space\n"
        "                           Use -f to overwrite existing entries\n"
        "  -r [-u]               Rebuild dirlog based on filesystem data\n"
        "                           .folders file (see MANUAL) defines a list of dirs in SITEROOT to scan\n"
        "                           -u only imports new records and does not truncate existing dirlog\n"
        "                           -f ignores .folders file and does a full recursive scan\n"
        "  -c, --check [--fix] [--ghost]\n"
        "                        Compare dirlog and filesystem records and warn on differences\n"
        "                          --fix attempts to correct dirlog/filesystem\n"
        "                          --ghost only looks for dirlog records with missing directories on filesystem\n"
        "                          Folder creation dates are ignored unless -f is given\n"
        "  -p, --dupechk         Look for duplicate records within dirlog and print to stdout\n"
        "  -b, --backup <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1>\n"
        "                        Perform backup on specified log\n"
        "\n Other:\n"
        "  -m <macro> [--arg[1-3] <string>]\n"
        "                         Searches subdirs for script that has the given macro defined, and executes\n"
        "                         --arg[1-3] sets values that fill {m:arg[1-3]} variables inside a macro\n"
        "\n Hooks:\n"
        "  --exec <command [{field}..{field}..]>\n"
        "                        While parsing data structure/filesystem, execute shell command for each record\n"
        "                          Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x, -a, -k, -h, -n, -q\n"
        "                          Operators {..} are overwritten with dirlog values\n"
        "                          If return value is non-zero, the processed record gets filtered\n"
        "                          Uses system() call (man system)\n"
        "  --execv <command [{field}..{field}..]> [--esredir=<file>]\n"
        "                        Same as --exec, only instead of calling system(), this uses execv() (man exec)\n"
        "                          It's generally much faster compared to --exec, since it doesn't fork /bin/sh\n"
        "                          for each individual processed record\n"
        "                        --esredir redirects stdout from executed command to <file>\n"
        "  --preexec <command [{field}..{field}..]>\n"
        "                        Execute shell <command> before starting main procedure\n"
        "  --postexec <command [{field}..{field}..]>\n"
        "                        Execute shell <command> after main procedure finishes\n"
        "  --loopexec <command [{field}..{field}..]>\n"
        "\n Matching:\n"
        "  --regex [<field>,]<match>\n"
        "                        Regex filter string, used during various operations\n"
        "                          If <field> is set, matching is performed against a specific data log field\n"
        "                            (field names are the same as --exec variable names for logs)\n"
        "                          Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x, -a, -k, -h, -n, -q\n"
        "  --regexi [<var>,]<match>\n"
        "                        Case insensitive variant of --regex\n"
        "  --iregex [<var>,]<match> \n"
        "                        Same as --regex with negated match\n"
        "  --iregexi [<var>,]<match>\n"
        "                        Same as --regexi with negated match\n"
        "  --noereg              Disable POSIX Extended Regular Expression syntax (enabled by default)\n"
        "  --match [<field>,]<match>\n"
        "                        Regular filter string (exact matches)\n"
        "                          Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x, -a, -k, -h, -n, -q\n"
        "  --imatch [<field>,]<match>\n"
        "                        Inverted --match\n"
        "  --lom <<field> > 5.0 && <field> != 0 || <field> ..>\n"
        "                        Compare values by logical and comparison/relational operators\n"
        "                        Applies to any integer/floating point fields from data sources\n"
        "                          Use quotation marks to avoid collisions with bash operators\n"
        "                        Valid logical operators: && (and), || (or)\n"
        "                        Valid comparison/relational operators: =, !=, >, <, <=, >=\n"
        "  --ilom <expression>   Same as --lom with negated match\n"
        "  --maxhit <limit>      Maximum number of positive filter matches (rest are forced negative)\n"
        "  --maxres <limit>      Maximum number of negative filter matches (rest are forced positive)\n"
        "  --ifhit               Ignore first match\n"
        "  --ifres               Ignore first result\n"
        "  --ifrhe               Takes -exec[v] return value into account with --max[hit|res] and --if[hit|res]\n"
        "  --matchq              Exit on first match\n"
        "  --imatchq             Exit on first result\n"
        "\n"
        "  In between match arguments, logical or|and operators apply:\n"
        "  \".. --<argument1> <or|and> --<margument2> ..\"\n"
        "\n Misc:\n"
        "  --sort <mode>,<order>,<field>\n"
        "                        Sort data log entries\n"
        "                          <mode> can only be 'num' (numeric)\n"
        "                          <order> can be 'asc' (ascending) or 'desc' (descending)\n"
        "                          Sorts by the specified data log <field>\n"
        "                          Used with -e, -d, -i, -l, -o, -w, -a, -k, -h, -n, -q\n"
        "\n"
        "Options:\n"
        "  -f                    Force operation where it applies (use -ff for greater effect)\n"
        "  -v                    Increase verbosity level (use -vv or more for greater effect)\n"
        "  --nowrite             Perform a dry run, executing normally except no writing is done\n"
        "  -b, --nobuffer        Disable data file memory buffering\n"
        "  -y, --followlinks     Follow symbolic links (default is skip)\n"
        "  --nowbuffer           Disable write pre-caching (faster but less safe), applies to -r\n"
        "  --memlimit=<bytes>    Maximum file size that can be pre-buffered into memory\n"
        "  --memlimita=<bytes>   Maximum ASCII input data file size (ignored)\n"
        "  --shmem [--shmdestroy] [--shmdestonexit] [--shmreload]\n"
        "                        Instead of internal memory, use the shared memory segment to buffer log data\n"
        "                           This is usefull as an inter-process caching mechanism, allowing other glutil\n"
        "                            instances (or other processes) rapid access to the log data, without having to\n"
        "                            allocate pages and load from filesystem each time\n"
        "                           If log file is present and it's size doesn't match segment size, glutil\n"
        "                            destroys the segment and re-creates it with new size\n"
        "                           Although this applies globally, it should only be used with dump operations (-d,-n,-i,..)\n"
        "                           DO NOT use, unless you know exactly what you're doing\n"
        "                        --shmdestroy forces the old segment (if any) be destroyed and re-loaded\n"
        "                        --shmdestonexit destroys used segments before exiting process\n"
        "                        --shmreload forces existing records (if any) be reloaded, but segment is not destroyed\n"
        "                         and it's size remains the same (when data log size doesn't match segment size,\n"
        "                         there will be junk/missing records, depending on input size being higher/lower)\n"
        "  --loadq               Quit just after loading data into memory\n"
        "                           Applies to dump operations only\n"
        "  --nofq                Abort data (re)build operation unconditionally, if nothing was filtered\n"
        "  --noglconf            Disable reading settings from glftpd.conf\n"
        "  --sfv                 Generate new SFV files inside target folders, works with -r [-u] and -s\n"
        "                           Used by itself, triggers -r (fs rebuild) dry run (does not modify dirlog)\n"
        "                           Avoid using this if doing a full recursive rebuild\n"
        "  --xdev                Ignores files/dirs on other filesystems\n"
        "                           Applies to -r, -t, -g, -x (can apply to other modes)\n"
        "  --xblk                Ignores files/dirs on non-block devices\n"
        "                           Applies to -r, -t, -g, -x (can apply to other modes)\n"
        "  --rev                 Reverses the order in which records are processed\n"
        "  --ipc <key>           Override gl's shared memory segment key setting\n"
        "  --daemon              Fork process into background\n"
        "  --loop <interval>     Loops the given operation\n"
        "                           Use caution, some operations might fail when looped\n"
        "                           This is usefull when running yourown scripts (--exec)\n"
        "                         Execute command each loop\n"
        "  --loglevel <0-6>      Log verbosity level (1: exception only..6: everything)\n"
        "                           Level 0 turns logging off\n"
        "  --silent              Silent mode\n"
        "  --ftime               Prepend formatted timestamps to output\n"
        "  --log                 Force logging enabled\n"
        "  --fork <command>      Fork process into background and execute <command>\n"
        "  --[u]sleep <timeout>  Wait for <timeout> before running\n"
        "  --version             Print version and exit\n"
        "\n"
        "Directory and file:\n"
        "  --glroot=<path>       glFTPd root path\n"
        "  --siteroot=<path>     Site root path (relative to glFTPd root)\n"
        "  --dirlog=<file>       Path to directory log\n"
        "  --nukelog=<file>      Path to nuke log\n"
        "  --dupefile=<file>     Path to dupe file\n"
        "  --lastonlog=<file>    Path to last-on log\n"
        "  --oneliners=<file>    Path to oneliners file\n"
        "  --imdblog=<file>      Path to iMDB log\n"
        "  --gamelog=<file>      Path to game log\n"
        "  --tvlog=<file>        Path to TVRAGE log\n"
        "  --ge1log=<file>       Path to GENERIC1 log\n"
        "  --ge12og=<file>       Path to GENERIC2 log\n"
        "  --glconf=<file>       Path to glftpd.conf (set using glconf.h by default)\n"
        "  --folders=<file>      Path to folders file (contains sections and depths,\n"
        "                           used on recursive imports)\n"
        "  --logfile=<file>      Log file path\n"
        "\n";

int
md_init(pmda md, int nm);
p_md_obj
md_first(pmda md);
int
split_string(char *, char, pmda);
off_t
s_string(char *input, char *m, off_t offset);
void *
md_alloc(pmda md, int b);
__g_match
g_global_register_match(void);
int
g_oper_and(int s, int d);
int
g_oper_or(int s, int d);
char **
build_argv(char *args, size_t max, int *c);
int
find_absolute_path(char *exec, char *output);
int
g_do_exec(void *, void *, char*, void *hdl);
int
g_do_exec_v(void *buffer, void *p_hdl, char *ex_str, void *hdl);
int
g_do_exec_fb(void *buffer, void *p_hdl, char *ex_str, void *hdl);

int
g_cpg(void *arg, void *out, int m, size_t sz)
{
  char *buffer;
  if (m == 2)
    {
      buffer = (char *) arg;
    }
  else
    {
      buffer = ((char **) arg)[0];
    }
  if (!buffer)
    {
      return 1;
    }

  size_t a_l = strlen(buffer);

  if (!a_l)
    {
      return 2;
    }

  a_l > sz ? a_l = sz : sz;
  char *ptr = (char*) out;
  strncpy(ptr, buffer, a_l);
  ptr[a_l] = 0x0;

  return 0;
}
void *
g_pg(void *arg, int m)
{
  if (m == 2)
    {
      return (char *) arg;
    }
  return ((char **) arg)[0];
}

char *
g_pd(void *arg, int m, size_t l)
{
  char *buffer = (char*) g_pg(arg, m);
  char *ptr = NULL;
  size_t a_l = strlen(buffer);

  if (!a_l)
    {
      return NULL;
    }

  a_l > l ? a_l = l : l;

  if (a_l)
    {
      ptr = (char*) calloc(a_l + 1, 1);
      strncpy(ptr, buffer, a_l);
    }
  return ptr;
}

int
prio_opt_g_macro(void *arg, int m)
{
  prio_argv_off = g_pg(arg, m);
  updmode = PRIO_UPD_MODE_MACRO;
  return 0;
}

int
prio_opt_g_pinfo(void *arg, int m)
{
  updmode = PRIO_UPD_MODE_INFO;
  return 0;
}

int
opt_g_loglvl(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  int lvl = atoi(buffer), i;
  uint32_t t_LOGLVL = 0;

  for (i = -1; i < lvl; i++)
    {
      t_LOGLVL <<= 1;
      t_LOGLVL |= 0x1;
    }

  LOGLVL = t_LOGLVL;
  gfl |= F_OPT_PS_LOGGING;

  return 0;
}

int
opt_g_verbose(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE;
  return 0;
}

int
opt_g_verbose2(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2;
  return 0;
}

int
opt_g_verbose3(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3;
  return 0;
}

int
opt_g_verbose4(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4;
  return 0;
}

int
opt_g_verbose5(void *arg, int m)
{
  gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4
      | F_OPT_VERBOSE5;
  return 0;
}

int
opt_g_force(void *arg, int m)
{
  gfl |= F_OPT_FORCE;
  return 0;
}

int
opt_g_force2(void *arg, int m)
{
  gfl |= (F_OPT_FORCE2 | F_OPT_FORCE);
  return 0;
}

int
opt_g_update(void *arg, int m)
{
  gfl |= F_OPT_UPDATE;
  return 0;
}

int
opt_g_loop(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  loop_interval = (int) strtol(buffer, NULL, 10);
  gfl |= F_OPT_LOOP;
  return 0;
}

int
opt_loop_max(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  errno = 0;
  loop_max = (uint64_t) strtol(buffer, NULL, 10);
  if ((errno == ERANGE && (loop_max == LONG_MAX || loop_max == LONG_MIN))
      || (errno != 0 && loop_max == 0))
    {
      return (a32 << 17);
    }

  return 0;
}

int
opt_g_udc(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_DUMP_GEN;
  return 0;
}

int
opt_g_dg(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_DUMP_GENERIC;
  return 0;
}

int
opt_g_recursive(void *arg, int m)
{
  flags_udcfg |= F_PD_RECURSIVE;
  return 0;
}

int
opt_g_udc_dir(void *arg, int m)
{
  flags_udcfg |= F_PD_MATCHDIR;
  return 0;
}

int
opt_g_udc_f(void *arg, int m)
{
  flags_udcfg |= F_PD_MATCHREG;
  return 0;
}

int
opt_g_loopexec(void *arg, int m)
{
  LOOPEXEC = g_pd(arg, m, MAX_EXEC_STR);
  if (LOOPEXEC)
    {
      gfl |= F_OPT_LOOPEXEC;
    }
  return 0;
}

int
opt_g_maxresults(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  max_results = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_HASMAXRES;
  return 0;
}

int
opt_g_maxdepth(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  max_depth = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_MAXDEPTH;
  return 0;
}

int
opt_g_mindepth(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  min_depth = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_MINDEPTH;
  return 0;
}

int
opt_g_maxhits(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  max_hits = (off_t) strtoll(buffer, NULL, 10);
  gfl |= F_OPT_HASMAXHIT;
  return 0;
}

int
opt_g_daemonize(void *arg, int m)
{
  gfl |= F_OPT_DAEMONIZE;
  return 0;
}

int
opt_g_ifres(void *arg, int m)
{
  gfl |= F_OPT_IFIRSTRES;
  return 0;
}

int
opt_g_ifhit(void *arg, int m)
{
  gfl |= F_OPT_IFIRSTHIT;
  return 0;
}

int
opt_g_ifrh_e(void *arg, int m)
{
  gfl |= F_OPT_IFRH_E;
  return 0;
}

int
opt_g_nofq(void *arg, int m)
{
  gfl |= F_OPT_NOFQ;
  return 0;
}

int
opt_g_noereg(void *arg, int m)
{
  g_regex_flags ^= REG_EXTENDED;
  return 0;
}

int
opt_g_fd(void *arg, int m)
{
  gfl |= F_OPT_XFD;
  return 0;
}

int
opt_prune(void *arg, int m)
{
  gfl |= F_OPT_ZPRUNEDUP;
  return 0;
}

int
opt_g_noglconf(void *arg, int m)
{
  gfl |= F_OPT_NOGLCONF;
  return 0;
}

int
opt_g_fix(void *arg, int m)
{
  gfl |= F_OPT_FIX;
  return 0;
}

int
opt_g_sfv(void *arg, int m)
{
  gfl |= F_OPT_SFV;
  return 0;
}

int
opt_batch_output_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_BATCH | F_OPT_PS_SILENT;
  return 0;
}

int
opt_export_output_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_EXPORT | F_OPT_PS_SILENT;
  return 0;
}

int
opt_compact_output_formatting(void *arg, int m)
{
  gfl |= F_OPT_FORMAT_COMP;
  return 0;
}

int
opt_g_shmem(void *arg, int m)
{
  gfl |= F_OPT_SHAREDMEM;
  return 0;
}

int
opt_g_loadq(void *arg, int m)
{
  gfl |= F_OPT_LOADQ;
  return 0;
}

int
opt_g_shmdestroy(void *arg, int m)
{
  gfl |= F_OPT_SHMDESTROY;
  return 0;
}

int
opt_g_shmdestroyonexit(void *arg, int m)
{
  gfl |= F_OPT_SHMDESTONEXIT;
  return 0;
}

int
opt_g_shmreload(void *arg, int m)
{
  gfl |= F_OPT_SHMRELOAD;
  return 0;
}

int
opt_g_nowrite(void *arg, int m)
{
  gfl |= F_OPT_NOWRITE;
  return 0;
}

int
opt_g_nobuffering(void *arg, int m)
{
  gfl |= F_OPT_NOBUFFER;
  return 0;
}

int
opt_g_buffering(void *arg, int m)
{
  gfl ^= F_OPT_WBUFFER;
  return 0;
}

int
opt_g_followlinks(void *arg, int m)
{
  gfl |= F_OPT_FOLLOW_LINKS;
  return 0;
}

int
opt_g_ftime(void *arg, int m)
{
  gfl |= F_OPT_PS_TIME;
  return 0;
}

int
opt_g_matchq(void *arg, int m)
{
  gfl |= F_OPT_MATCHQ;
  return 0;
}

int
opt_g_imatchq(void *arg, int m)
{
  gfl |= F_OPT_IMATCHQ;
  return 0;
}

int
opt_update_single_record(void *arg, int m)
{
  argv_off = g_pg(arg, m);
  updmode = UPD_MODE_SINGLE;
  return 0;
}

int
opt_recursive_update_records(void *arg, int m)
{
  updmode = UPD_MODE_RECURSIVE;
  return 0;
}

int
opt_raw_dump(void *arg, int m)
{
  gfl |= F_OPT_MODE_RAWDUMP | F_OPT_PS_SILENT;
  return 0;
}

int
opt_binary(void *arg, int m)
{
  gfl |= F_OPT_MODE_BINARY;
  return 0;
}

int
opt_g_reverse(void *arg, int m)
{
  gfl |= F_OPT_PROCREV;
  return 0;
}

int
opt_silent(void *arg, int m)
{
  gfl |= F_OPT_PS_SILENT;
  return 0;
}

int
opt_logging(void *arg, int m)
{
  gfl |= F_OPT_PS_LOGGING;
  return 0;
}

int
opt_nobackup(void *arg, int m)
{
  gfl |= F_OPT_NOBACKUP;
  return 0;
}

int
opt_backup(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_BACKUP;
  return 0;
}

char *_print_ptr = NULL;

int
opt_print(void *arg, int m)
{
  if ((_print_ptr = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PRINT;
      return 0;
    }
  return 4250;
}

int
opt_printf(void *arg, int m)
{
  if ((_print_ptr = g_pg(arg, m)))
    {
      gfl0 |= F_OPT_PRINTF;
      return 0;
    }
  return 4250;
}

int
opt_stdin(void *arg, int m)
{
  gfl0 |= F_OPT_STDIN;
  return 0;
}

int
opt_exec(void *arg, int m)
{
  exec_str = g_pd(arg, m, MAX_EXEC_STR);
  exc = g_do_exec_fb;
  return 0;
}

long amax = 0;

int
opt_execv(void *arg, int m)
{
  int c = 0;

#ifdef _SC_ARG_MAX
  amax = sysconf(_SC_ARG_MAX);
#else
#ifdef ARG_MAX
  val = ARG_MAX;
#endif
#endif

  if (!amax)
    {
      amax = LONG_MAX;
    }

  long count = amax / sizeof(char*);

  exec_str = g_pd(arg, m, MAX_EXEC_STR);

  if (!exec_str)
    {
      return 9008;
    }

  if (!strlen(exec_str))
    {
      return 9009;
    }

  char **ptr = build_argv(exec_str, count, &c);

  if (!c)
    {
      return 9001;
    }

  if (c > count / 2)
    {
      return 9002;
    }

  exec_vc = c;

  exec_v = ptr;
  exc = g_do_exec_v;

  return 0;
}

char infile_p[PATH_MAX];

int
opt_g_infile(void *arg, int m)
{
  g_cpg(arg, infile_p, m, PATH_MAX);

  return 0;
}

int
opt_shmipc(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (!strlen(buffer))
    {
      return (a32 << 16);
    }

  SHM_IPC = (key_t) strtoul(buffer, NULL, 16);

  if (!SHM_IPC)
    {
      return 2;
    }

  ofl |= F_OVRR_IPC;

  return 0;
}

int
opt_log_file(void *arg, int m)
{
  g_cpg(arg, LOGFILE, m, PATH_MAX);
  gfl |= F_OPT_PS_LOGGING;
  ofl |= F_OVRR_LOGFILE;
  return 0;
}

char MACRO_ARG1[4096] =
  { 0 };
char MACRO_ARG2[4096] =
  { 0 };
char MACRO_ARG3[4096] =
  { 0 };

int
opt_g_arg1(void *arg, int m)
{
  g_cpg(arg, MACRO_ARG1, m, 4095);
  gfl |= F_OPT_HAS_M_ARG1;
  return 0;
}

int
opt_g_arg2(void *arg, int m)
{
  g_cpg(arg, MACRO_ARG2, m, 4095);
  gfl |= F_OPT_HAS_M_ARG2;
  return 0;
}

int
opt_g_arg3(void *arg, int m)
{
  g_cpg(arg, MACRO_ARG3, m, 4095);
  gfl |= F_OPT_HAS_M_ARG3;
  return 0;
}

char *GLOBAL_PREEXEC = NULL;
char *GLOBAL_POSTEXEC = NULL;

int
opt_g_preexec(void *arg, int m)
{
  GLOBAL_PREEXEC = g_pd(arg, m, MAX_EXEC_STR);
  if (GLOBAL_PREEXEC)
    {
      gfl |= F_OPT_PREEXEC;
    }
  return 0;
}

int
opt_g_postexec(void *arg, int m)
{
  GLOBAL_POSTEXEC = g_pd(arg, m, MAX_EXEC_STR);
  if (GLOBAL_POSTEXEC)
    {
      gfl |= F_OPT_POSTEXEC;
    }
  return 0;
}

int
opt_glroot(void *arg, int m)
{
  if (!(ofl & F_OVRR_GLROOT))
    {
      g_cpg(arg, GLROOT, m, PATH_MAX);
      ofl |= F_OVRR_GLROOT;
    }
  return 0;
}

int
opt_siteroot(void *arg, int m)
{
  if (!(ofl & F_OVRR_SITEROOT))
    {
      g_cpg(arg, SITEROOT_N, m, PATH_MAX);
      ofl |= F_OVRR_SITEROOT;
    }
  return 0;
}

int
opt_dupefile(void *arg, int m)
{
  if (!(ofl & F_OVRR_DUPEFILE))
    {
      g_cpg(arg, DUPEFILE, m, PATH_MAX);
      ofl |= F_OVRR_DUPEFILE;
    }
  return 0;
}

int
opt_lastonlog(void *arg, int m)
{
  if (!(ofl & F_OVRR_LASTONLOG))
    {
      g_cpg(arg, LASTONLOG, m, PATH_MAX);
      ofl |= F_OVRR_LASTONLOG;
    }
  return 0;
}

int
opt_oneliner(void *arg, int m)
{
  if (!(ofl & F_OVRR_ONELINERS))
    {
      g_cpg(arg, ONELINERS, m, PATH_MAX);
      ofl |= F_OVRR_ONELINERS;
    }
  return 0;
}

int
opt_imdblog(void *arg, int m)
{
  if (!(ofl & F_OVRR_IMDBLOG))
    {
      g_cpg(arg, IMDBLOG, m, PATH_MAX);
      ofl |= F_OVRR_IMDBLOG;
    }
  return 0;
}

int
opt_tvlog(void *arg, int m)
{
  if (!(ofl & F_OVRR_TVLOG))
    {
      g_cpg(arg, TVLOG, m, PATH_MAX);
      ofl |= F_OVRR_TVLOG;
    }
  return 0;
}

int
opt_gamelog(void *arg, int m)
{
  if (!(ofl & F_OVRR_GAMELOG))
    {
      g_cpg(arg, GAMELOG, m, PATH_MAX);
      ofl |= F_OVRR_GAMELOG;
    }
  return 0;
}

int
opt_GE1LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE1LOG))
    {
      g_cpg(arg, GE1LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE1LOG;
    }
  return 0;
}

int
opt_GE2LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE2LOG))
    {
      g_cpg(arg, GE2LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE2LOG;
    }
  return 0;
}

int
opt_GE3LOG(void *arg, int m)
{
  if (!(ofl & F_OVRR_GE3LOG))
    {
      g_cpg(arg, GE3LOG, m, PATH_MAX);
      ofl |= F_OVRR_GE3LOG;
    }
  return 0;
}

int
opt_rebuild(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_REBUILD;
  return 0;
}

int
opt_dirlog_file(void *arg, int m)
{
  if (!(ofl & F_OVRR_DIRLOG))
    {
      g_cpg(arg, DIRLOG, m, PATH_MAX);
      ofl |= F_OVRR_DIRLOG;
    }
  return 0;
}

int
opt_g_sleep(void *arg, int m)
{
  g_sleep = atoi(g_pg(arg, m));
  return 0;
}

int
opt_g_usleep(void *arg, int m)
{
  g_usleep = atoi(g_pg(arg, m));
  return 0;
}

int
opt_execv_stdout_redir(void *arg, int m)
{
  char *ptr = g_pg(arg, m);
  execv_stdout_redir = open(ptr, O_RDWR | O_CREAT,
      (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
  if (execv_stdout_redir == -1)
    {
      ofl |= F_ESREDIRFAILED;
    }

  return 0;
}

mda _md_gsort =
  { 0 };
char *g_sort_field = NULL;

int
opt_g_sort(void *arg, int m)
{
  char *buffer = g_pg(arg, m);

  if (gfl & F_OPT_SORT)
    {
      return 0;
    }

  if (_md_gsort.offset >= 64)
    {
      return 4600;
    }

  md_init(&_md_gsort, 3);

  int r = split_string(buffer, 0x2C, &_md_gsort);

  if (r != 3)
    {
      return 4601;
    }

  p_md_obj ptr = md_first(&_md_gsort);

  if (!ptr)
    {
      return 4602;
    }

  char *s_ptr = (char*) ptr->ptr;

  if (!strncmp(s_ptr, "num", 3))
    {
      g_sort_flags |= F_GSORT_NUMERIC;
    }
  else
    {
      return 4603;
    }

  ptr = ptr->next;
  s_ptr = (char*) ptr->ptr;

  if (!strncmp(s_ptr, "desc", 4))
    {
      g_sort_flags |= F_GSORT_DESC;
    }
  else if (!strncmp(s_ptr, "asc", 3))
    {
      g_sort_flags |= F_GSORT_ASC;
    }
  else
    {
      return 4604;
    }

  ptr = ptr->next;
  g_sort_field = (char*) ptr->ptr;

  if (!strlen(g_sort_field))
    {
      return 4605;
    }

  g_sort_flags |= F_GSORT_RESETPOS;

  gfl |= F_OPT_SORT;

  return 0;
}

mda _match_rr =
  { 0 };

typedef struct ___lom_strings_header
{
  uint32_t flags;
  g_op g_oper_ptr;
  __g_match m_ref;
  char string[8192];
} _lom_s_h, *__lom_s_h;

#define G_MATCH		((int)0)
#define G_NOMATCH	((int)1)

typedef struct ___last_match
{
  uint32_t flags;
  void *ptr;
} _l_match, *__l_match;

#define F_LM_CPRG		(a32 << 1)
#define F_LM_LOM		(a32 << 2)

#define F_LM_TYPES		(F_LM_CPRG|F_LM_LOM)

_l_match _match_rr_l =
  { 0 };

#define MAX_CPRG_STRING		4096
#define MAX_LOM_STRING		4096

int
g_cprg(void *arg, int m, int match_i_m, int reg_i_m, int regex_flags,
    uint32_t flags)
{
  char *buffer = g_pg(arg, m);

  if (!buffer)
    {
      return 1113;
    }

  size_t a_i = strlen(buffer);

  if (!a_i)
    {
      return 0;
    }

  if (a_i > MAX_CPRG_STRING)
    {
      return 1114;
    }

  __g_match pgm = g_global_register_match();

  if (!pgm)
    {
      return 1114;
    }

  /*char *ptr = (char*) pgm->data;

   strncpy(ptr, buffer, a_i);*/
  //strncpy((char*) pgm->b_data, buffer, a_i);
  pgm->data = buffer;

  pgm->match_i_m = match_i_m;
  pgm->reg_i_m = reg_i_m;
  pgm->regex_flags = regex_flags;
  pgm->flags = flags;

  if (pgm->reg_i_m == REG_NOMATCH || pgm->match_i_m == 1)
    {
      pgm->g_oper_ptr = g_oper_or;
      pgm->flags |= F_GM_NOR;
    }
  else
    {
      pgm->g_oper_ptr = g_oper_and;
      pgm->flags |= F_GM_NAND;
    }

  bzero(&_match_rr_l, sizeof(_match_rr_l));
  _match_rr_l.ptr = (void *) pgm;
  _match_rr_l.flags = F_LM_CPRG;

  switch (flags & F_GM_TYPES)
    {
  case F_GM_ISREGEX:
    ;
    if (!(gfl & F_OPT_HAS_G_REGEX))
      {
        gfl |= F_OPT_HAS_G_REGEX;
      }
    break;
  case F_GM_ISMATCH:
    if (!(gfl & F_OPT_HAS_G_MATCH))
      {
        gfl |= F_OPT_HAS_G_MATCH;
      }
    break;
    }

  return 0;
}

int
opt_g_lom(void *arg, int m, uint32_t flags)
{
  char *buffer = g_pg(arg, m);

  size_t a_i = strlen(buffer);

  if (!a_i)
    {
      return 0;
    }

  if (a_i > MAX_LOM_STRING)
    {
      return 8900;
    }

  __g_match pgm = g_global_register_match();

  if (!pgm)
    {
      return 8901;
    }

  pgm->flags = flags | F_GM_ISLOM;
  if (pgm->flags & F_GM_IMATCH)
    {
      pgm->g_oper_ptr = g_oper_or;
      pgm->flags |= F_GM_NOR;
    }
  else
    {
      pgm->g_oper_ptr = g_oper_and;
      pgm->flags |= F_GM_NAND;
    }

  gfl |= F_OPT_HAS_G_LOM;

  bzero(&_match_rr_l, sizeof(_match_rr_l));
  _match_rr_l.ptr = (void *) pgm;
  _match_rr_l.flags = F_LM_LOM;

  //g_cpg(arg, pgm->data, m, a_i);
  pgm->data = buffer;

  return 0;
}

int
opt_g_operator_or(void *arg, int m)
{
  __g_match pgm = (__g_match) _match_rr_l.ptr;
  if (!pgm)
    {
      return 7100;
    }
  switch (_match_rr_l.flags & F_LM_TYPES)
    {

      case F_LM_CPRG:
      ;
      if ( pgm->reg_i_m == REG_NOMATCH || pgm->match_i_m == 1)
        {
          pgm->g_oper_ptr = g_oper_and;
        }
      else
        {
          pgm->g_oper_ptr = g_oper_or;
        }
      break;
      case F_LM_LOM:;
      if (pgm->flags & F_GM_IMATCH)
        {
          pgm->g_oper_ptr = g_oper_and;
        }
      else
        {
          pgm->g_oper_ptr = g_oper_or;
        }
      break;
      default:
      return 7110;
      break;
    }
  pgm->flags |= F_GM_NOR;
  return 0;
}

int
opt_g_operator_and(void *arg, int m)
{
  __g_match pgm = (__g_match) _match_rr_l.ptr;
  if (!pgm)
    {
      return 6100;
    }
  switch (_match_rr_l.flags & F_LM_TYPES)
    {

      case F_LM_CPRG:
      ;
      if ( pgm->reg_i_m == REG_NOMATCH || pgm->match_i_m == 1)
        {
          pgm->g_oper_ptr = g_oper_or;
        }
      else
        {
          pgm->g_oper_ptr = g_oper_and;
        }
      break;
      case F_LM_LOM:;
      if (pgm->flags & F_GM_IMATCH)
        {
          pgm->g_oper_ptr = g_oper_or;
        }
      else
        {
          pgm->g_oper_ptr = g_oper_and;
        }
      break;
      default:
      return 6110;
      break;
    }
  pgm->flags |= F_GM_NAND;
  return 0;
}

int
opt_g_lom_match(void *arg, int m)
{
  return opt_g_lom(arg, m, 0);
}

int
opt_g_lom_imatch(void *arg, int m)
{
  return opt_g_lom(arg, m, F_GM_IMATCH);
}

int
opt_g_regexi(void *arg, int m)
{
  return g_cprg(arg, m, 0, 0, REG_ICASE, F_GM_ISREGEX);
}

int
opt_g_match(void *arg, int m)
{
  return g_cprg(arg, m, 0, 0, 0, F_GM_ISMATCH);
}

int
opt_g_imatch(void *arg, int m)
{
  return g_cprg(arg, m, 1, 0, 0, F_GM_ISMATCH);
}

int
opt_g_regex(void *arg, int m)
{
  return g_cprg(arg, m, 0, 0, 0, F_GM_ISREGEX);
}

int
opt_g_iregexi(void *arg, int m)
{
  return g_cprg(arg, m, 0, REG_NOMATCH, REG_ICASE, F_GM_ISREGEX);
}

int
opt_g_iregex(void *arg, int m)
{
  return g_cprg(arg, m, 0, REG_NOMATCH, 0, F_GM_ISREGEX);
}

int
opt_nukelog_file(void *arg, int m)
{
  g_cpg(arg, NUKELOG, m, PATH_MAX - 1);
  ofl |= F_OVRR_NUKELOG;
  return 0;
}

int
opt_glconf_file(void *arg, int m)
{
  g_cpg(arg, GLCONF_I, m, PATH_MAX - 1);
  return 0;
}

int
opt_dirlog_sections_file(void *arg, int m)
{
  g_cpg(arg, DU_FLD, m, PATH_MAX - 1);
  return 0;
}

int
print_version(void *arg, int m)
{
  print_str("glutil_%d.%d-%d%s-%s\n", VER_MAJOR, VER_MINOR,
  VER_REVISION, VER_STR, __STR_ARCH);
  updmode = UPD_MODE_NOOP;
  return 0;
}

int
g_opt_mode_noop(void *arg, int m)
{
  updmode = UPD_MODE_NOOP;
  return 0;
}

int
print_version_long(void *arg, int m)
{
  print_str("* glutil-%d.%d-%d%s-%s - glFTPd binary logs tool *\n", VER_MAJOR,
  VER_MINOR,
  VER_REVISION, VER_STR, __STR_ARCH);
  return 0;
}

int
opt_dirlog_check(void *arg, int m)
{
  updmode = UPD_MODE_CHECK;
  return 0;
}

int
opt_check_ghost(void *arg, int m)
{
  gfl |= F_OPT_C_GHOSTONLY;
  return 0;
}

int
opt_g_xdev(void *arg, int m)
{
  gfl |= F_OPT_XDEV;
  return 0;
}

int
opt_g_xblk(void *arg, int m)
{
  gfl |= F_OPT_XBLK;
  return 0;
}

int
opt_pex(void *arg, int m)
{
  gfl |= F_OPT_PEX;
  return 0;
}

int
opt_g_cdironly(void *arg, int m)
{
  gfl |= F_OPT_CDIRONLY;
  return 0;
}

int
opt_dirlog_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP;
  return 0;
}

int
opt_dupefile_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_DUPEF;
  return 0;
}

int
opt_online_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_ONL;
  return 0;
}

int
opt_lastonlog_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_LON;
  return 0;
}

int
opt_dump_users(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_USERS;
  return 0;
}

int
opt_dump_grps(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_GRPS;
  return 0;
}

int
opt_dirlog_dump_nukelog(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_NUKE;
  return 0;
}

int
opt_g_write(void *arg, int m)
{
  updmode = UPD_MODE_WRITE;
  p_argv_off = g_pg(arg, m);
  return 0;
}

int
opt_g_dump_imdb(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_IMDB;
  return 0;
}

int
opt_g_dump_game(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_GAME;
  return 0;
}

int
opt_g_dump_tv(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_TV;
  return 0;
}

int
opt_oneliner_dump(void *arg, int m)
{
  updmode = UPD_MODE_DUMP_ONEL;
  return 0;
}

int
print_help(void *arg, int m)
{
  print_str(hpd_up, VER_MAJOR, VER_MINOR, VER_REVISION,
  VER_STR, __STR_ARCH);
  if (m != -1)
    {
      updmode = UPD_MODE_NOOP;
    }
  return 0;
}

int
opt_g_ex_fork(void *arg, int m)
{
  p_argv_off = g_pg(arg, m);
  updmode = UPD_MODE_FORK;
  gfl |= F_OPT_DAEMONIZE;
  return 0;
}

int
opt_dirlog_chk_dupe(void *arg, int m)
{
  updmode = UPD_MODE_DUPE_CHK;
  return 0;
}

int
opt_membuffer_limit(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  if (!buffer)
    {
      return 3512;
    }
  long long int l_buffer = atoll(buffer);
  if (l_buffer > 1024)
    {
      db_max_size = l_buffer;
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: max memory buffer limit set to %lld bytes\n",
              l_buffer);
        }
    }
  else
    {
      print_str(
          "NOTICE: invalid memory buffer limit, using default (%lld bytes)\n",
          db_max_size);
    }
  return 0;
}

int
opt_membuffer_limit_in(void *arg, int m)
{
  char *buffer = g_pg(arg, m);
  if (!buffer)
    {
      return 3513;
    }
  long long int l_buffer = atoll(buffer);
  if (l_buffer > 8192)
    {
      max_datain_f = l_buffer;
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: ASCII input buffer limit set to %lld bytes\n",
              l_buffer);
        }
    }
  else
    {
      print_str(
          "NOTICE: invalid ASCII input buffer limit, using default (%lld bytes)\n",
          max_datain_f);
    }
  return 0;
}

/* generic types */
typedef int
_d_ag_handle_i(__g_handle);
typedef int
_d_achar_i(char *);
typedef int
_d_avoid_i(void);
typedef int
_d_is_am(uint8_t in_c);
typedef int
(*__d_is_am)(uint8_t in_c);

/* specific types */
typedef int
__d_enum_cb(char *, unsigned char, void *, __g_eds);
typedef int
__d_ref_to_val(void *, char *, char *, size_t, void *mppd);
typedef int
__d_format_block(void *, char *);
typedef uint64_t
__d_dlfind(char *, int, uint32_t, void *);
typedef pmda
__d_cfg(pmda md, char * file);
typedef int
__d_mlref(void *buffer, char *key, char *val);
typedef uint64_t
__g_t_ptr(void *base, size_t offset);
typedef void *
__d_ref_to_pval(void *arg, char *match, int *output);
typedef char *
__g_proc_rv(void *arg, char *match, char *output, size_t max_size, void *mppd);
typedef void *
_d_rtv_lk(void *arg, char *match, char *output, size_t max_size, void *mppd);
typedef void
_d_omfp_fp(void *hdl, void *ptr, char *sbuffer);
typedef int64_t
g_sint_p(void *base, size_t offset);

g_sint_p g_ts8_ptr, g_ts16_ptr, g_ts32_ptr;

__g_t_ptr g_t8_ptr, g_t16_ptr, g_t32_ptr, g_t64_ptr;
_d_ag_handle_i g_cleanup, gh_rewind, determine_datatype, g_close, g_shm_cleanup;
_d_avoid_i dirlog_check_dupe, rebuild_dirlog, dirlog_check_records,
    g_print_info;

_d_achar_i self_get_path, file_exists, get_file_type, dir_exists,
    dirlog_update_record, g_dump_ug, g_dump_gen, d_write, d_gen_dump;

__d_enum_cb proc_section, proc_directory, ssd_4macro, g_process_directory;

__d_ref_to_val ref_to_val_dirlog, ref_to_val_nukelog, ref_to_val_dupefile,
    ref_to_val_lastonlog, ref_to_val_oneliners, ref_to_val_online,
    ref_to_val_generic, ref_to_val_macro, ref_to_val_imdb, ref_to_val_game,
    ref_to_val_tv, ref_to_val_x, ref_to_val_gen1, ref_to_val_gen2;

__d_ref_to_pval ref_to_val_ptr_dirlog, ref_to_val_ptr_nukelog,
    ref_to_val_ptr_oneliners, ref_to_val_ptr_online, ref_to_val_ptr_imdb,
    ref_to_val_ptr_game, ref_to_val_ptr_lastonlog, ref_to_val_ptr_dupefile,
    ref_to_val_ptr_tv, ref_to_val_ptr_dummy, ref_to_val_ptr_gen1,
    ref_to_val_ptr_x, ref_to_val_ptr_gen2, ref_to_val_ptr_gen3;

__d_format_block lastonlog_format_block, dupefile_format_block,
    oneliner_format_block, online_format_block, nukelog_format_block,
    dirlog_format_block, imdb_format_block, game_format_block, tv_format_block,
    gen1_format_block, gen2_format_block;

__d_format_block lastonlog_format_block_batch, dupefile_format_block_batch,
    oneliner_format_block_batch, online_format_block_batch,
    nukelog_format_block_batch, dirlog_format_block_batch,
    imdb_format_block_batch, game_format_block_batch, tv_format_block_batch,
    gen1_format_block_batch, gen2_format_block_batch;

__d_format_block lastonlog_format_block_exp, dupefile_format_block_exp,
    oneliner_format_block_exp, online_format_block_exp,
    nukelog_format_block_exp, dirlog_format_block_exp, imdb_format_block_exp,
    game_format_block_exp, tv_format_block_exp, gen1_format_block_exp,
    gen2_format_block_exp;

__d_dlfind dirlog_find, dirlog_find_old, dirlog_find_simple;
__d_cfg search_cfg_rf, register_cfg_rf;
__d_mlref gcb_dirlog, gcb_nukelog, gcb_imdbh, gcb_oneliner, gcb_dupefile,
    gcb_lastonlog, gcb_game, gcb_tv, gcb_gen1, gcb_gen2, gcb_gen3;

__g_proc_rv dt_rval_dirlog_user, dt_rval_dirlog_group, dt_rval_dirlog_files,
    dt_rval_dirlog_size, dt_rval_dirlog_status, dt_rval_dirlog_time,
    dt_rval_dirlog_mode_e, dt_rval_dirlog_dir, dt_rval_xg_dirlog,
    dt_rval_x_dirlog, dt_rval_dirlog_basedir;

__g_proc_rv dt_rval_nukelog_size, dt_rval_nukelog_time, dt_rval_nukelog_status,
    dt_rval_nukelog_mult, dt_rval_nukelog_mode_e, dt_rval_nukelog_dir,
    dt_rval_nukelog_basedir_e, dt_rval_nukelog_nuker, dt_rval_nukelog_nukee,
    dt_rval_nukelog_unnuker, dt_rval_nukelog_reason;

__g_proc_rv dt_rval_dupefile_time, dt_rval_dupefile_file, dt_rval_dupefile_user;

__g_proc_rv dt_rval_lastonlog_logon, dt_rval_lastonlog_logoff,
    dt_rval_lastonlog_upload, dt_rval_lastonlog_download,
    dt_rval_lastonlog_config, dt_rval_lastonlog_user, dt_rval_lastonlog_user,
    dt_rval_lastonlog_group, dt_rval_lastonlog_stats, dt_rval_lastonlog_tag;

__g_proc_rv dt_rval_generic_nukestr, dt_rval_generic_procid,
    dt_rval_generic_ipc, dt_rval_generic_usroot, dt_rval_generic_logroot,
    dt_rval_generic_memlimit, dt_rval_generic_curtime, dt_rval_q,
    dt_rval_generic_exe, dt_rval_generic_glroot, dt_rval_generic_siteroot,
    dt_rval_generic_siterootn, dt_rval_generic_ftpdata,
    dt_rval_generic_imdbfile, dt_rval_generic_tvfile, dt_rval_generic_gamefile,
    dt_rval_generic_spec1, dt_rval_generic_glconf, dt_rval_generic_logfile;

__g_proc_rv dt_rval_x_path, dt_rval_x_basepath, dt_rval_x_dirpath, dt_rval_c,
    dt_rval_x_size, dt_rval_x_mode, dt_rval_x_devid, dt_rval_x_minor, major,
    dt_rval_x_inode, dt_rval_x_links, dt_rval_x_uid, dt_rval_x_gid,
    dt_rval_x_blksize, dt_rval_x_blocks, dt_rval_x_atime, dt_rval_x_ctime,
    dt_rval_x_mtime, dt_rval_x_isread, dt_rval_x_iswrite, dt_rval_x_isexec,
    dt_rval_x_uperm, dt_rval_x_gperm, dt_rval_x_operm, dt_rval_x_perm,
    dt_rval_x_sparse, dt_rval_x_crc32, dt_rval_x_deccrc32;

__g_proc_rv dt_rval_spec_slen;

void *
ref_to_val_af(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd);

__g_proc_rv dt_rval_online_ssl, dt_rval_online_group, dt_rval_online_time,
    dt_rval_online_lupdt, dt_rval_online_lxfrt, dt_rval_online_bxfer,
    dt_rval_online_btxfer, dt_rval_online_pid, dt_rval_online_rate,
    dt_rval_online_basedir, dt_rval_online_ndir, dt_rval_online_user,
    dt_rval_online_tag, dt_rval_online_status, dt_rval_online_host,
    dt_rval_online_dir, dt_rval_online_config;

__g_proc_rv dt_rval_oneliners_time, dt_rval_oneliners_user,
    dt_rval_oneliners_group, dt_rval_oneliners_tag, dt_rval_oneliners_msg;

__g_proc_rv dt_rval_imdb_time, dt_rval_imdb_score, dt_rval_imdb_votes,
    dt_rval_imdb_runtime, dt_rval_imdb_released, dt_rval_imdb_year,
    dt_rval_imdb_mode, dt_rval_imdb_basedir, dt_rval_imdb_dir,
    dt_rval_imdb_imdbid, dt_rval_imdb_genre, dt_rval_imdb_rated,
    dt_rval_imdb_title, dt_rval_imdb_director, dt_rval_imdb_actors,
    dt_rval_imdb_synopsis;

__g_proc_rv dt_rval_game_score, dt_rval_game_time, dt_rval_game_mode,
    dt_rval_game_basedir, dt_rval_game_dir;

__g_proc_rv dt_rval_tvrage_dir, dt_rval_tvrage_basedir, dt_rval_tvrage_time,
    dt_rval_tvrage_ended, dt_rval_tvrage_started, dt_rval_tvrage_started,
    dt_rval_tvrage_seasons, dt_rval_tvrage_showid, dt_rval_tvrage_runtime,
    dt_rval_tvrage_startyear, dt_rval_tvrage_endyear, dt_rval_tvrage_mode,
    dt_rval_tvrage_airday, dt_rval_tvrage_airtime, dt_rval_tvrage_country,
    dt_rval_tvrage_link, dt_rval_tvrage_name, dt_rval_tvrage_status,
    dt_rval_tvrage_class, dt_rval_tvrage_genre, dt_rval_tvrage_network;

_d_rtv_lk ref_to_val_lk_dirlog, ref_to_val_lk_nukelog, ref_to_val_lk_dupefile,
    ref_to_val_lk_lastonlog, ref_to_val_lk_oneliners, ref_to_val_lk_online,
    ref_to_val_lk_generic, ref_to_val_lk_x, ref_to_val_lk_imdb,
    ref_to_val_lk_game, ref_to_val_lk_tvrage, ref_to_val_lk_gen1,
    ref_to_val_lk_gen2, ref_to_val_lk_gen3;

_d_is_am is_ascii_text, is_ascii_lowercase_text, is_ascii_alphanumeric,
    is_ascii_hexadecimal, is_ascii_uppercase_text, is_ascii_numeric;

_d_omfp_fp g_omfp_norm, g_omfp_raw, g_omfp_ocomp, g_omfp_eassemble,
    g_omfp_eassemblef, g_xproc_print_d, g_xproc_print;

void *
as_ref_to_val_lk(char *match, void *c, __d_drt_h mppd, char *defdc);

char *
g_get_stf(char *match);

off_t
file_crc32(char *, uint32_t *);
void
g_xproc_rc(char *name, void *aa_rh, __g_eds eds);
int
data_backup_records(char*);
ssize_t
file_copy(char *, char *, char *, uint32_t);

int
release_generate_block(char *, ear *);
off_t
get_file_size(char *);
char **
process_macro(void *, char **);
char *
generate_chars(size_t, char, char*);
time_t
get_file_creation_time(struct stat *);
int
dirlog_write_record(struct dirlog *, off_t, int);

char *
string_replace(char *, char *, char *, char *, size_t);
int
enum_dir(char *, void *, void *, int, __g_eds);
int
update_records(char *, int);
off_t
read_file(char *, void *, size_t, off_t, FILE *);
int
option_crc32(void *, int);

int
load_cfg(pmda md, char * file, uint32_t flags, pmda *res);
int
free_cfg_rf(pmda md);

int
reg_match(char *, char *, int);

int
delete_file(char *, unsigned char, void *);

int
write_file_text(char *, char *);

size_t
str_match(char *, char *);
size_t
exec_and_wait_for_output(char*, char*);

p_md_obj
get_cfg_opt(char *, pmda, pmda*);

uint64_t
nukelog_find(char *, int, struct nukelog *);
int
parse_args(int argc, char **argv, void*fref_t[]);
int
process_opt_n(char *opt, void *arg, void *reference_array, int m, int *ret);
int
g_fopen(char *, char *, uint32_t, __g_handle);
void *
g_read(void *buffer, __g_handle, size_t);
int
process_exec_string(char *, char *, size_t, void *, void*);
int
process_execv_args(void *data, __g_handle hdl);
int
is_char_uppercase(char);
char *
g_basename(char *input);
char *
g_dirname(char *input);
void
sig_handler(int);
void
child_sig_handler(int, siginfo_t*, void*);
int
flush_data_md(__g_handle, char *);
int
rebuild(void *);
int
rebuild_data_file(char *, __g_handle);

int
g_bmatch(void *, __g_handle, pmda md);
int
do_match(__g_handle hdl, void *d_ptr, __g_match _gm);

size_t
g_load_data_md(void *, size_t, char *, __g_handle hdl);
int
g_load_record(__g_handle, const void *);
int
remove_repeating_chars(char *string, char c);

int
g_buffer_into_memory(char *, __g_handle);
int
g_print_stats(char *, uint32_t, size_t);

int
load_data_md(pmda md, char *file, __g_handle hdl);
int
g_shmap_data(__g_handle, key_t);
int
g_map_shm(__g_handle, key_t);
int
gen_md_data_ref(__g_handle hdl, pmda md, off_t count);
int
gen_md_data_ref_cnull(__g_handle hdl, pmda md, off_t count);
int
is_memregion_null(void *addr, size_t size);

char *
build_data_path(char *, char *, char *);
void *
ref_to_val_get_cfgval(char *, char *, char *, int, char *, size_t);

int
get_relative_path(char *, char *, char *);

void
free_cfg(pmda);

int
g_init(int argc, char **argv);

char *
g_dgetf(char *str);

int
m_load_input_n(__g_handle hdl, FILE *input);

off_t
s_string_r(char *input, char *m);

int
g_bin_compare(const void *p1, const void *p2, off_t size);

typedef int
__d_icomp(uint64_t s, uint64_t d);
typedef int
__d_fcomp(float s, float d);

int
rtv_q(void *query, char *output, size_t max_size);

char *
strcp_s(char *dest, size_t max_size, char *source);

__d_icomp g_is_higher, g_is_lower, g_is_higherorequal, g_is_equal,
    g_is_not_equal, g_is_lowerorequal, g_is_not, g_is, g_is_lower_2,
    g_is_higher_2;
__d_fcomp g_is_lower_f, g_is_higher_f, g_is_higherorequal_f, g_is_equal_f,
    g_is_lower_f_2, g_is_higher_f_2, g_is_not_equal_f, g_is_lowerorequal_f,
    g_is_f, g_is_not_f;

int
g_sort(__g_handle hdl, char *field, uint32_t flags);
int
g_sortf_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2);
int
g_sorti_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2);
char *
g_rtval_ex(char *arg, char *match, size_t max_size, char *output,
    uint32_t flags);
int
do_sort(__g_handle hdl, char *field, uint32_t flags);
void
g_ipcbm(void *, pmda md, int *r_p);

int
g_filter(__g_handle hdl, pmda md);

int
g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
    size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
    uint32_t flags);

int
g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags);

int
g_load_lom(__g_handle hdl);
int
g_load_strm(__g_handle hdl);

int
g_proc_mr(__g_handle hdl);
int
md_copy(pmda source, pmda dest, size_t block_sz);
int
g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
    uint32_t flags);

#define R_SHMAP_ALREADY_EXISTS	(a32 << 1)
#define R_SHMAP_FAILED_ATTACH	(a32 << 2)
#define R_SHMAP_FAILED_SHMAT	(a32 << 3)

void *
shmap(key_t ipc, struct shmid_ds *ipcret, size_t size, uint32_t *ret,
    int *shmid);

size_t
d_xref_ct_fe(__d_xref_ct input, size_t sz);

typedef int
__d_lom_vp(void *d_ptr, void *_lom);

__d_lom_vp g_lom_var_int, g_lom_var_uint, g_lom_var_float;
int
g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm);

int
g_compile_exech(pmda mech, __g_handle hdl, char *instr);
char *
g_exech_build_string(void *d_ptr, pmda mech, __g_handle hdl, char *outstr,
    size_t maxlen);

void *prio_f_ref[] =
  { "noop", g_opt_mode_noop, (void*) 0, "--raw", opt_raw_dump, (void*) 0,
      "silent", opt_silent, (void*) 0, "--silent", opt_silent, (void*) 0,
      "-arg1", opt_g_arg1, (void*) 1, "--arg1", opt_g_arg1, (void*) 1, "-arg2",
      opt_g_arg2, (void*) 1, "--arg2", opt_g_arg2, (void*) 1, "-arg3",
      opt_g_arg3, (void*) 1, "--arg3", opt_g_arg3, (void*) 1, "-vvvvv",
      opt_g_verbose5, (void*) 0, "-vvvv", opt_g_verbose4, (void*) 0, "-vvv",
      opt_g_verbose3, (void*) 0, "-vv", opt_g_verbose2, (void*) 0, "-v",
      opt_g_verbose, (void*) 0, "-m", prio_opt_g_macro, (void*) 1, "--info",
      prio_opt_g_pinfo, (void*) 0, "--loglevel", opt_g_loglvl, (void*) 1,
      "--logfile", opt_log_file, (void*) 1, "--log", opt_logging, (void*) 0,
      "--dirlog", opt_dirlog_file, (void*) 1, "--ge1log", opt_GE1LOG, (void*) 1,
      "--ge2log", opt_GE2LOG, (void*) 1, "--ge3log", opt_GE3LOG, (void*) 1,
      "--gamelog", opt_gamelog, (void*) 1, "--tvlog", opt_tvlog, (void*) 1,
      "--imdblog", opt_imdblog, (void*) 1, "--oneliners", opt_oneliner,
      (void*) 1, "--lastonlog", opt_lastonlog, (void*) 1, "--nukelog",
      opt_nukelog_file, (void*) 1, "--siteroot", opt_siteroot, (void*) 1,
      "--glroot", opt_glroot, (void*) 1, "--noglconf", opt_g_noglconf,
      (void*) 0, "--glconf", opt_glconf_file, (void*) 1,
      NULL, NULL, NULL };

void *f_ref[] =
  { "noop", g_opt_mode_noop, (void*) 0, "and", opt_g_operator_and, (void*) 0,
      "or", opt_g_operator_or, (void*) 0, "--rev", opt_g_reverse, (void*) 0,
      "lom", opt_g_lom_match, (void*) 1, "--lom", opt_g_lom_match, (void*) 1,
      "ilom", opt_g_lom_imatch, (void*) 1, "--ilom", opt_g_lom_imatch,
      (void*) 1, "--info", prio_opt_g_pinfo, (void*) 0, "sort", opt_g_sort,
      (void*) 1, "--sort", opt_g_sort, (void*) 1, "-h", opt_g_dump_tv,
      (void*) 0, "-k", opt_g_dump_game, (void*) 0, "--cdir", opt_g_cdironly,
      (void*) 0, "--imatchq", opt_g_imatchq, (void*) 0, "--matchq",
      opt_g_matchq, (void*) 0, "-a", opt_g_dump_imdb, (void*) 0, "-z",
      opt_g_write, (void*) 1, "--infile", opt_g_infile, (void*) 1, "-xdev",
      opt_g_xdev, (void*) 0, "--xdev", opt_g_xdev, (void*) 0, "-xblk",
      opt_g_xblk, (void*) 0, "--xblk", opt_g_xblk, (void*) 0, "-file",
      opt_g_udc_f, (void*) 0, "--file", opt_g_udc_f, (void*) 0, "-dir",
      opt_g_udc_dir, (void*) 0, "--dir", opt_g_udc_dir, (void*) 0, "--loopmax",
      opt_loop_max, (void*) 1, "--ghost", opt_check_ghost, (void*) 0, "-q",
      opt_g_dg, (void*) 1, "-x", opt_g_udc, (void*) 1, "-R", opt_g_recursive,
      (void*) 0, "-recursive", opt_g_recursive, (void*) 0, "--recursive",
      opt_g_recursive, (void*) 0, "-g", opt_dump_grps, (void*) 0, "-t",
      opt_dump_users, (void*) 0, "--backup", opt_backup, (void*) 1, "-print",
      opt_print, (void*) 1, "-printf", opt_printf, (void*) 1, "-stdin",
      opt_stdin, (void*) 0, "--stdin", opt_stdin, (void*) 0, "--print",
      opt_print, (void*) 1, "--printf", opt_printf, (void*) 1, "-b", opt_backup,
      (void*) 1, "--postexec", opt_g_postexec, (void*) 1, "--preexec",
      opt_g_preexec, (void*) 1, "--usleep", opt_g_usleep, (void*) 1, "--sleep",
      opt_g_sleep, (void*) 1, "-arg1",
      NULL, (void*) 1, "--arg1", NULL, (void*) 1, "-arg2",
      NULL, (void*) 1, "--arg2", NULL, (void*) 1, "-arg3", NULL, (void*) 1,
      "--arg3", NULL, (void*) 1, "-m", NULL, (void*) 1, "--imatch",
      opt_g_imatch, (void*) 1, "imatch", opt_g_imatch, (void*) 1, "match",
      opt_g_match, (void*) 1, "--match", opt_g_match, (void*) 1, "--fork",
      opt_g_ex_fork, (void*) 1, "-vvvvv", opt_g_verbose5, (void*) 0, "-vvvv",
      opt_g_verbose4, (void*) 0, "-vvv", opt_g_verbose3, (void*) 0, "-vv",
      opt_g_verbose2, (void*) 0, "-v", opt_g_verbose, (void*) 0, "--loglevel",
      opt_g_loglvl, (void*) 1, "--ftime", opt_g_ftime, (void*) 0, "--logfile",
      opt_log_file, (void*) 0, "--log", opt_logging, (void*) 0, "silent",
      opt_silent, (void*) 0, "--silent", opt_silent, (void*) 0, "--loopexec",
      opt_g_loopexec, (void*) 1, "--loop", opt_g_loop, (void*) 1, "--daemon",
      opt_g_daemonize, (void*) 0, "-w", opt_online_dump, (void*) 0, "--ipc",
      opt_shmipc, (void*) 1, "-l", opt_lastonlog_dump, (void*) 0, "--ge1log",
      opt_GE1LOG, (void*) 1, "--ge2log", opt_GE2LOG, (void*) 1, "--ge3log",
      opt_GE3LOG, (void*) 1, "--gamelog", opt_gamelog, (void*) 1, "--tvlog",
      opt_tvlog, (void*) 1, "--imdblog", opt_imdblog, (void*) 1, "--oneliners",
      opt_oneliner, (void*) 1, "-o", opt_oneliner_dump, (void*) 0,
      "--lastonlog", opt_lastonlog, (void*) 1, "-i", opt_dupefile_dump,
      (void*) 0, "--dupefile", opt_dupefile, (void*) 1, "--nowbuffer",
      opt_g_buffering, (void*) 0, "--raw", opt_raw_dump, (void*) 0, "--binary",
      opt_binary, (void*) 0, "iregexi", opt_g_iregexi, (void*) 1, "--iregexi",
      opt_g_iregexi, (void*) 1, "iregex", opt_g_iregex, (void*) 1, "--iregex",
      opt_g_iregex, (void*) 1, "regexi", opt_g_regexi, (void*) 1, "--regexi",
      opt_g_regexi, (void*) 1, "regex", opt_g_regex, (void*) 1, "--regex",
      opt_g_regex, (void*) 1, "-e", opt_rebuild, (void*) 1, "--comp",
      opt_compact_output_formatting, (void*) 0, "--batch",
      opt_batch_output_formatting, (void*) 0, "-E",
      opt_export_output_formatting, (void*) 0, "--export",
      opt_export_output_formatting, (void*) 0, "-y", opt_g_followlinks,
      (void*) 0, "--allowsymbolic", opt_g_followlinks, (void*) 0,
      "--followlinks", opt_g_followlinks, (void*) 0, "--allowlinks",
      opt_g_followlinks, (void*) 0, "--execv", opt_execv, (void*) 1, "-execv",
      opt_execv, (void*) 1, "-exec", opt_exec, (void*) 1, "--exec", opt_exec,
      (void*) 1, "--fix", opt_g_fix, (void*) 0, "-u", opt_g_update, (void*) 0,
      "--memlimit", opt_membuffer_limit, (void*) 1, "--memlimita",
      opt_membuffer_limit_in, (void*) 1, "-p", opt_dirlog_chk_dupe, (void*) 0,
      "--dupechk", opt_dirlog_chk_dupe, (void*) 0, "--nobuffer",
      opt_g_nobuffering, (void*) 0, "-n", opt_dirlog_dump_nukelog, (void*) 0,
      "--help", print_help, (void*) 0, "--version", print_version, (void*) 0,
      "--folders", opt_dirlog_sections_file, (void*) 1, "--dirlog",
      opt_dirlog_file, (void*) 1, "--nukelog", opt_nukelog_file, (void*) 1,
      "--siteroot", opt_siteroot, (void*) 1, "--glroot", opt_glroot, (void*) 1,
      "--nowrite", opt_g_nowrite, (void*) 0, "--sfv", opt_g_sfv, (void*) 0,
      "--crc32", option_crc32, (void*) 1, "--nobackup", opt_nobackup, (void*) 0,
      "-c", opt_dirlog_check, (void*) 0, "--check", opt_dirlog_check, (void*) 0,
      "--dump", opt_dirlog_dump, (void*) 0, "-d", opt_dirlog_dump, (void*) 0,
      "-f", opt_g_force, (void*) 0, "-ff", opt_g_force2, (void*) 0, "-s",
      opt_update_single_record, (void*) 1, "-r", opt_recursive_update_records,
      (void*) 0, "--shmem", opt_g_shmem, (void*) 0, "--shmreload",
      opt_g_shmreload, (void*) 0, "--loadq", opt_g_loadq, (void*) 0,
      "--shmdestroy", opt_g_shmdestroy, (void*) 0, "--shmdestonexit",
      opt_g_shmdestroyonexit, (void*) 0, "--maxres", opt_g_maxresults,
      (void*) 1, "--maxhit", opt_g_maxhits, (void*) 1, "--ifres", opt_g_ifres,
      (void*) 0, "--ifhit", opt_g_ifhit, (void*) 0, "--ifrhe", opt_g_ifrh_e,
      (void*) 0, "--nofq", opt_g_nofq, (void*) 0, "--esredir",
      opt_execv_stdout_redir, (void*) 1, "--noglconf", opt_g_noglconf,
      (void*) 0, "--maxdepth", opt_g_maxdepth, (void*) 1, "-maxdepth",
      opt_g_maxdepth, (void*) 1, "--mindepth", opt_g_mindepth, (void*) 1,
      "-mindepth", opt_g_mindepth, (void*) 1, "--noereg", opt_g_noereg,
      (void*) 0, "--fd", opt_g_fd, (void*) 0, "-fd", opt_g_fd, (void*) 0,
      "--prune", opt_prune, (void*) 0, "--glconf", opt_glconf_file, (void*) 1,
      "--glconf", opt_pex, (void*) 0,
      NULL, NULL, NULL };

int
md_init(pmda md, int nm)
{
  if (!md || md->objects)
    {
      return 1;
    }
  bzero(md, sizeof(mda));
  md->objects = calloc(nm, sizeof(md_obj));
  md->count = nm;
  md->pos = md->objects;
  md->r_pos = md->objects;
  md->first = md->objects;
  return 0;
}

int
md_g_free(pmda md)
{
  if (!md || !md->objects)
    return 1;

  if (!(md->flags & F_MDA_REFPTR))
    {
      p_md_obj ptr = md_first(md), ptr_s;
      while (ptr)
        {
          ptr_s = ptr->next;
          if (ptr->ptr)
            {
              free(ptr->ptr);
              ptr->ptr = NULL;
            }
          ptr = ptr_s;
        }
    }

  free(md->objects);
  bzero(md, sizeof(mda));

  return 0;
}

uintaa_t
md_relink(pmda md)
{
  off_t off, l = 1;

  p_md_obj last = NULL, cur = md->objects;

  for (off = 0; off < md->count; off++)
    {
      if (cur->ptr)
        {
          if (last)
            {
              last->next = cur;
              cur->prev = last;
              l++;
            }
          else
            {
              md->first = cur;
            }
          last = cur;
        }
      cur++;
    }
  return l;
}

p_md_obj
md_first(pmda md)
{

  /*if (md->first && md->first != md->objects) {
   if (md->first->ptr) {
   return md->first;
   }
   }*/

  off_t off = 0;
  p_md_obj ptr = md->objects;

  for (off = 0; off < md->count; off++, ptr++)
    {
      if (ptr->ptr)
        {
          return ptr;
        }
    }

  return NULL;
}

p_md_obj
md_last(pmda md)
{
  p_md_obj ptr = md_first(md);

  if (!ptr)
    {
      return ptr;
    }

  while (ptr->next)
    {
      ptr = ptr->next;
    }

  return ptr;
}

#define MDA_MDALLOC_RE	0x1

void *
md_alloc(pmda md, int b)
{
  int flags = 0;

  if (md->offset >= md->count)
    {
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str(
              "NOTICE: re-allocating memory segment to increase size; current address: 0x%.16llX, current size: %llu\n",
              (ulint64_t) (uintaa_t) md->objects, (ulint64_t) md->count);
        }
      md->objects = realloc(md->objects, (md->count * sizeof(md_obj)) * 2);
      md->pos = md->objects;
      md->pos += md->count;
      bzero(md->pos, md->count * sizeof(md_obj));

      md->count *= 2;
      uintaa_t rlc = md_relink(md);
      flags |= MDA_MDALLOC_RE;
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str(
              "NOTICE: re-allocation done; new address: 0x%.16llX, new size: %llu, re-linked %llu records\n",
              (ulint64_t) (uintaa_t) md->objects, (ulint64_t) md->count,
              (ulint64_t) rlc);
        }
    }

  p_md_obj prev = md->pos;
  uintaa_t pcntr = 0;
  while (md->pos->ptr
      && (pcntr = ((md->pos - md->objects) / sizeof(md_obj))) < md->count)
    {
      md->pos++;
    }

  if (pcntr >= md->count)
    {
      return NULL;
    }

  if (md->pos > md->objects && !(md->pos - 1)->ptr)
    {
      flags |= MDA_MDALLOC_RE;
    }

  if (md->pos->ptr)
    return NULL;

  if (md->flags & F_MDA_REFPTR)
    {
      md->pos->ptr = md->lref_ptr;
    }
  else
    {
      md->pos->ptr = calloc(1, b);
    }

  if (prev != md->pos)
    {
      prev->next = md->pos;
      md->pos->prev = prev;
    }

  md->offset++;

  if (flags & MDA_MDALLOC_RE)
    {
      md_relink(md);
    }

  return md->pos->ptr;
}

void *
md_unlink(pmda md, p_md_obj md_o)
{
  if (!md_o)
    {
      return NULL;
    }

  p_md_obj c_ptr = NULL;

  if (md_o->prev)
    {
      ((p_md_obj) md_o->prev)->next = (p_md_obj) md_o->next;
      c_ptr = md_o->prev;
    }

  if (md_o->next)
    {
      ((p_md_obj) md_o->next)->prev = (p_md_obj) md_o->prev;
      c_ptr = md_o->next;
    }

  /*if (md->first == md_o && !md->first->prev) {
   if (md_o->next) {
   md->first = md_o->next;
   } else {
   md->first = md->objects;
   }
   }*/

  md->offset--;
  if (md->pos == md_o && c_ptr)
    {
      md->pos = c_ptr;
    }
  if (!(md->flags & F_MDA_REFPTR) && md_o->ptr)
    {
      free(md_o->ptr);
    }
  md_o->ptr = NULL;

  return (void*) c_ptr;
}

void *
md_swap(pmda md, p_md_obj md_o1, p_md_obj md_o2)
{
  if (!md_o1 || !md_o2)
    {
      return NULL;
    }

  void *ptr2_s;

  ptr2_s = md_o1->prev;
  md_o1->next = md_o2->next;
  md_o1->prev = md_o2;
  md_o2->next = md_o1;
  md_o2->prev = ptr2_s;

  if (md_o2->prev)
    {
      ((p_md_obj) md_o2->prev)->next = md_o2;
    }

  if (md_o1->next)
    {
      ((p_md_obj) md_o1->next)->prev = md_o1;
    }

  if (md->first == md_o1)
    {
      md->first = md_o2;
    }

  /*if (md->pos == md_o1) {
   md->pos = md_o2;
   }

   if (md->pos == md_o2) {
   md->pos = md_o1;
   }*/

  return md_o2->next;
}

void *
md_swap_s(pmda md, p_md_obj md_o1, p_md_obj md_o2)
{
  void *ptr = md_o1->ptr;
  md_o1->ptr = md_o2->ptr;
  md_o2->ptr = ptr;

  return md_o1->next;
}

int
setup_sighandlers(void)
{
  struct sigaction sa =
    {
      { 0 } }, sa_c =
    {
      { 0 } }, sa_e =
    {
      { 0 } };
  int r = 0;

  sa.sa_handler = &sig_handler;
  sa.sa_flags = SA_RESTART;

  sa_c.sa_sigaction = &child_sig_handler;
  sa_c.sa_flags = SA_RESTART | SA_SIGINFO;

  sa_e.sa_sigaction = sighdl_error;
  sa_e.sa_flags = SA_RESTART | SA_SIGINFO;

  sigfillset(&sa.sa_mask);
  sigfillset(&sa_c.sa_mask);
  sigemptyset(&sa_e.sa_mask);

  r += sigaction(SIGINT, &sa, NULL);
  r += sigaction(SIGQUIT, &sa, NULL);
  r += sigaction(SIGABRT, &sa, NULL);
  r += sigaction(SIGTERM, &sa, NULL);
  r += sigaction(SIGCHLD, &sa_c, NULL);
  r += sigaction(SIGSEGV, &sa_e, NULL);
  r += sigaction(SIGILL, &sa_e, NULL);
  r += sigaction(SIGFPE, &sa_e, NULL);
  r += sigaction(SIGBUS, &sa_e, NULL);
  r += sigaction(SIGTRAP, &sa_e, NULL);

  signal(SIGKILL, sig_handler);

  return r;
}

int
g_shutdown(void *arg)
{
  g_setjmp(0, "g_shutdown", NULL, NULL);

  g_cleanup(&g_act_1);
  g_cleanup(&g_act_2);
  free_cfg_rf(&cfg_rf);
  free_cfg(&glconf);

  if (NUKESTR && NUKESTR != (char*) NUKESTR_DEF)
    {
      free(NUKESTR);
    }

  if ((gfl & F_OPT_PS_LOGGING) && fd_log)
    {
      fclose(fd_log);
    }

  if (_p_macro_argv)
    {
      free(_p_macro_argv);
    }

  if (GLOBAL_PREEXEC)
    {
      free(GLOBAL_PREEXEC);
    }

  if (GLOBAL_POSTEXEC)
    {
      free(GLOBAL_POSTEXEC);
    }

  if (LOOPEXEC)
    {
      free(LOOPEXEC);
    }

  if (exec_str)
    {
      free(exec_str);
    }

  if (exec_v)
    {
      int i;

      for (i = 0; i < exec_vc && exec_v[i]; i++)
        {
          free(exec_v[i]);
        }
      free(exec_v);

    }

  if (execv_stdout_redir != -1)
    {
      close(execv_stdout_redir);
    }

  md_g_free(&_match_rr);
  md_g_free(&_md_gsort);

  _p_macro_argc = 0;

  exit(EXITVAL);
}

int
g_shm_cleanup(__g_handle hdl)
{
  int r = 0;

  if (shmdt(hdl->data) == -1)
    {
      r++;
    }

  if ((hdl->flags & F_GH_SHM) && (hdl->flags & F_GH_SHMDESTONEXIT))
    {
      if (shmctl(hdl->shmid, IPC_RMID, NULL) == -1)
        {
          r++;
        }
    }

  hdl->data = NULL;
  hdl->shmid = 0;

  return r;
}

int
g_cleanup(__g_handle hdl)
{
  int r = 0;

  r += md_g_free(&hdl->buffer);
  r += md_g_free(&hdl->w_buffer);

  p_md_obj ptr;

  if (hdl->_match_rr.objects)
    {
      ptr = md_first(&hdl->_match_rr);

      while (ptr)
        {
          __g_match g_ptr = (__g_match) ptr->ptr;
          if ( g_ptr->flags & F_GM_ISLOM)
            {
              md_g_free(&g_ptr->lom);

            }
          if ( g_ptr->flags & F_GM_ISREGEX)
            {
              regfree(&g_ptr->preg);
            }
          ptr = ptr->next;
        }

      r += md_g_free(&hdl->_match_rr);
    }

  if (hdl->exec_args.ac_ref.objects)
    {
      __d_argv_ch ach;
      ptr = md_first(&hdl->exec_args.ac_ref);
      while (ptr)
        {
          ach = (__d_argv_ch) ptr->ptr;
          free(hdl->exec_args.argv_c[ach->cindex]);
          md_g_free(&ach->mech);
          ptr = ptr->next;
        }

      if (hdl->exec_args.argv_c)
        {
          free(hdl->exec_args.argv_c);
        }

      r += md_g_free(&hdl->exec_args.ac_ref);
    }

  md_g_free(&hdl->exec_args.mech);

  if (!(hdl->flags & F_GH_ISSHM) && hdl->data)
    {
      free(hdl->data);
    }
  else if ((hdl->flags & F_GH_ISSHM) && hdl->data)
    {
      g_shm_cleanup(hdl);
    }
  bzero(hdl, sizeof(_g_handle));
  return r;
}

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
              "ERROR: %s: [%d]: could not open file for writing, logging disabled\n",
              LOGFILE, errno);
        }
    }
  return;
}

#define F_LCONF_NORF 	                0x1
#define MSG_INIT_PATH_OVERR 	        "NOTICE: %s path set to '%s'\n"
#define MSG_INIT_CMDLINE_ERROR          "ERROR: [%d] processing command line arguments failed\n"

int
g_init(int argc, char **argv)
{
  g_setjmp(0, "g_init", NULL, NULL);
  int r;

  if (strlen(LOGFILE))
    {
      gfl |= F_OPT_PS_LOGGING;
    }
  r = parse_args(argc, argv, f_ref);

  if (r == -2 || r == -1)
    {
      print_str("Read ./glutil --help\n");
      EXITVAL = 4;
      return EXITVAL;
    }

  if (r > 0)
    {
      print_str(MSG_INIT_CMDLINE_ERROR, r);
      EXITVAL = 2;
      return EXITVAL;
    }

  enable_logging();

  if (ofl & F_ESREDIRFAILED)
    {
      print_str("ERROR: could not open file to redirect execv stdout to\n");
      EXITVAL = 2;
      return EXITVAL;
    }

  if (updmode && updmode != UPD_MODE_NOOP && !(gfl & F_OPT_FORMAT_BATCH)
      && !(gfl & F_OPT_FORMAT_COMP) && (gfl & F_OPT_VERBOSE2))
    {
      print_str("INIT: glutil %d.%d-%d%s-%s starting [PID: %d]\n",
      VER_MAJOR,
      VER_MINOR,
      VER_REVISION, VER_STR, __STR_ARCH, getpid());
    }

  if (!(gfl & F_OPT_NOGLCONF))
    {
      if (strlen(GLCONF_I))
        {
          if ((r = load_cfg(&glconf, GLCONF_I, F_LCONF_NORF, NULL))
              && (gfl & F_OPT_VERBOSE))
            {
              print_str("WARNING: %s: could not load GLCONF file [%d]\n",
                  GLCONF_I, r);
            }

          if ((gfl & F_OPT_VERBOSE4) && glconf.offset)
            {
              print_str("NOTICE: %s: loaded %d config lines into memory\n",
                  GLCONF_I, (int) glconf.offset);
            }

          p_md_obj ptr = get_cfg_opt("ipc_key", &glconf, NULL);

          if (ptr && !(ofl & F_OVRR_IPC))
            {
              SHM_IPC = (key_t) strtol(ptr->ptr, NULL, 16);
            }

          ptr = get_cfg_opt("rootpath", &glconf, NULL);

          if (ptr && !(ofl & F_OVRR_GLROOT))
            {
              snprintf(GLROOT, PATH_MAX, "%s", (char*) ptr->ptr);
              if ((gfl & F_OPT_VERBOSE5))
                {
                  print_str("NOTICE: GLCONF: using 'rootpath': %s\n", GLROOT);
                }
            }

          ptr = get_cfg_opt("min_homedir", &glconf, NULL);

          if (ptr && !(ofl & F_OVRR_SITEROOT))
            {
              snprintf(SITEROOT_N, PATH_MAX, "%s", (char*) ptr->ptr);
              if ((gfl & F_OPT_VERBOSE5))
                {
                  print_str("NOTICE: GLCONF: using 'min_homedir': %s\n",
                      SITEROOT_N);
                }
            }

          ptr = get_cfg_opt("ftp-data", &glconf, NULL);

          if (ptr)
            {
              snprintf(FTPDATA, PATH_MAX, "%s", (char*) ptr->ptr);
              if ((gfl & F_OPT_VERBOSE5))
                {
                  print_str("NOTICE: GLCONF: using 'ftp-data': %s\n", FTPDATA);
                }
            }

          ptr = get_cfg_opt("nukedir_style", &glconf, NULL);

          if (ptr)
            {
              NUKESTR = calloc(255, 1);
              NUKESTR = string_replace(ptr->ptr, "%N", "%s", NUKESTR, 255);
              if ((gfl & F_OPT_VERBOSE5))
                {
                  print_str("NOTICE: GLCONF: using 'nukedir_style': %s\n",
                      NUKESTR);
                }
              ofl |= F_OVRR_NUKESTR;
            }
        }
      else
        {
          print_str("WARNING: GLCONF not defined in glconf.h\n");
        }
    }

  if (!strlen(GLROOT))
    {
      print_str("ERROR: glftpd root directory not specified!\n");
      return 2;
    }

  if (!strlen(SITEROOT_N))
    {
      print_str("ERROR: glftpd site root directory not specified!\n");
      return 2;
    }

  /*if (!(ofl & F_OVRR_DIRLOG))
    {
      build_data_path(DEFF_DIRLOG, DIRLOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_NUKELOG))
    {
      build_data_path(DEFF_NUKELOG, NUKELOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_DUPEFILE))
    {
      build_data_path(DEFF_DUPEFILE, DUPEFILE, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_LASTONLOG))
    {
      build_data_path(DEFF_LASTONLOG, LASTONLOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_ONELINERS))
    {
      build_data_path(DEFF_ONELINERS, ONELINERS, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_IMDBLOG))
    {
      build_data_path(DEFF_IMDB, IMDBLOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_TVLOG))
    {
      build_data_path(DEFF_TV, TVLOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_GAMELOG))
    {
      build_data_path(DEFF_GAMELOG, GAMELOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_GE1LOG))
    {
      build_data_path(DEFF_GEN1, GE1LOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_GE2LOG))
    {
      build_data_path(DEFF_GEN2, GE2LOG, DEFPATH_LOGS);
    }

  if (!(ofl & F_OVRR_GE3LOG))
    {
      build_data_path(DEFF_GEN3, GE3LOG, DEFPATH_LOGS);
    }*/

  snprintf(SITEROOT, PATH_MAX, "%s%s", GLROOT, SITEROOT_N);
  remove_repeating_chars(SITEROOT, 0x2F);

  if (dir_exists(SITEROOT) && !dir_exists(SITEROOT_N))
    {
      strcp_s(SITEROOT, PATH_MAX, SITEROOT_N);
    }

  if ((gfl & F_OPT_VERBOSE) && dir_exists(SITEROOT))
    {
      print_str("WARNING: no valid siteroot!\n");
    }

  if (!updmode && (gfl & F_OPT_SFV))
    {
      updmode = UPD_MODE_RECURSIVE;
      if (!(gfl & F_OPT_NOWRITE))
        {
          gfl |= F_OPT_FORCEWSFV | F_OPT_NOWRITE;
        }
      if (ofl & F_OVRR_GLROOT)
        {
          print_str(MSG_INIT_PATH_OVERR, "GLROOT", GLROOT);
        }

      if (ofl & F_OVRR_SITEROOT)
        {
          print_str(MSG_INIT_PATH_OVERR, "SITEROOT", SITEROOT);
        }
      if ((gfl & F_OPT_VERBOSE))
        {
          print_str(
              "NOTICE: switching to non-destructive filesystem rebuild mode\n");
        }
    }

  if ((gfl & F_OPT_VERBOSE))
    {
      if (gfl & F_OPT_NOBUFFER)
        {
          print_str("NOTICE: disabling memory buffering\n");
          if (gfl & F_OPT_SHAREDMEM)
            {
              print_str(
                  "WARNING: --shmem: shared memory segment buffering option is invalid when --nobuffer specified\n");
            }
        }
      if (SHM_IPC && SHM_IPC != shm_ipc)
        {
          print_str("NOTICE: IPC key set to '0x%.8X'\n", SHM_IPC);
        }

      if ((gfl & F_OPT_VERBOSE4) && (gfl & F_OPT_PS_LOGGING))
        {
          print_str("NOTICE: Logging enabled: %s\n", LOGFILE);
        }
    }

  if ((gfl & F_OPT_VERBOSE) && (gfl & F_OPT_NOWRITE))
    {
      print_str("WARNING: performing dry run, no writing will be done\n");
    }

  if (gfl & F_OPT_DAEMONIZE)
    {
      print_str("NOTICE: forking into background.. [PID: %d]\n", getpid());
      if (daemon(1, 0) == -1)
        {
          print_str(
              "ERROR: [%d] could not fork into background, terminating..\n",
              errno);
          EXITVAL = errno;
          return errno;
        }
    }

  if (updmode && (gfl & F_OPT_PREEXEC))
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("PREEXEC: running: '%s'\n", GLOBAL_PREEXEC);
        }
      int r_e = 0;
      if ((r_e = g_do_exec(NULL, ref_to_val_generic, GLOBAL_PREEXEC, NULL))
          == -1 || WEXITSTATUS(r_e))
        {
          if (gfl & F_OPT_VERBOSE5)
            {
              print_str("WARNING: [%d]: PREEXEC returned non-zero: '%s'\n",
                  WEXITSTATUS(r_e), GLOBAL_PREEXEC);
            }
          EXITVAL = WEXITSTATUS(r_e);
          return 1;
        }
    }

  if (g_usleep)
    {
      usleep(g_usleep);
    }
  else if (g_sleep)
    {
      sleep(g_sleep);
    }

  uint64_t mloop_c = 0;
  char m_b1[128];
  int m_f = 0x1;

  g_setjmp(0, "main(start)", NULL, NULL);

  enter:

  if ((m_f & 0x1))
    {
      snprintf(m_b1, 127, "main(loop) [c:%llu]",
          (long long unsigned int) mloop_c);
      g_setjmp(0, m_b1, NULL, NULL);
      m_f ^= 0x1;
    }

  switch (updmode)
    {
  case UPD_MODE_RECURSIVE:
    EXITVAL = rebuild_dirlog();
    break;
  case UPD_MODE_SINGLE:
    EXITVAL = dirlog_update_record(argv_off);
    break;
  case UPD_MODE_CHECK:
    EXITVAL = dirlog_check_records();
    break;
  case UPD_MODE_DUMP:
    EXITVAL = g_print_stats(DIRLOG, 0, 0);
    break;
  case UPD_MODE_DUMP_NUKE:
    EXITVAL = g_print_stats(NUKELOG, 0, 0);
    break;
  case UPD_MODE_DUMP_DUPEF:
    EXITVAL = g_print_stats(DUPEFILE, 0, 0);
    break;
  case UPD_MODE_DUMP_LON:
    EXITVAL = g_print_stats(LASTONLOG, 0, 0);
    break;
  case UPD_MODE_DUMP_ONEL:
    EXITVAL = g_print_stats(ONELINERS, 0, 0);
    break;
  case UPD_MODE_DUMP_IMDB:
    EXITVAL = g_print_stats(IMDBLOG, 0, 0);
    break;
  case UPD_MODE_DUMP_GAME:
    EXITVAL = g_print_stats(GAMELOG, 0, 0);
    break;
  case UPD_MODE_DUMP_TV:
    EXITVAL = g_print_stats(TVLOG, 0, 0);
    break;
  case UPD_MODE_DUMP_GENERIC:
    EXITVAL = d_gen_dump(p_argv_off);
    break;
  case UPD_MODE_DUPE_CHK:
    EXITVAL = dirlog_check_dupe();
    break;
  case UPD_MODE_REBUILD:
    EXITVAL = rebuild(p_argv_off);
    break;
  case UPD_MODE_DUMP_ONL:
    EXITVAL = g_print_stats("ONLINE USERS", F_DL_FOPEN_SHM, ON_SZ);
    break;
  case UPD_MODE_FORK:
    if (p_argv_off)
      {
        if ((EXITVAL = WEXITSTATUS(system(p_argv_off))))
          {
            if (gfl & F_OPT_VERBOSE)
              {
                print_str("WARNING: '%s': command failed, code %d\n",
                    p_argv_off, EXITVAL);
              }
          }
      }
    break;
  case UPD_MODE_BACKUP:
    EXITVAL = data_backup_records(g_dgetf(p_argv_off));
    break;
  case UPD_MODE_DUMP_USERS:
    EXITVAL = g_dump_ug(DEFPATH_USERS);
    break;
  case UPD_MODE_DUMP_GRPS:
    EXITVAL = g_dump_ug(DEFPATH_GROUPS);
    break;
  case UPD_MODE_DUMP_GEN:
    EXITVAL = g_dump_gen(p_argv_off);
    break;
  case UPD_MODE_WRITE:
    EXITVAL = d_write((char*) p_argv_off);
    break;
  case PRIO_UPD_MODE_INFO:
    g_print_info();
    break;
  case UPD_MODE_NOOP:
    break;
  default:
    print_help(NULL, -1);
    print_str("ERROR: no mode specified\n");
    break;
    }

  if ((gfl & F_OPT_LOOP) && !(gfl & F_OPT_KILL_GLOBAL)
      && (!loop_max || mloop_c < loop_max - 1))
    {
      g_cleanup(&g_act_1);
      g_cleanup(&g_act_2);
      free_cfg_rf(&cfg_rf);
      sleep(loop_interval);
      if (gfl & F_OPT_LOOPEXEC)
        {
          g_do_exec(NULL, ref_to_val_generic, LOOPEXEC, NULL);
        }
      mloop_c++;
      goto enter;
    }

  if (updmode && (gfl & F_OPT_POSTEXEC))
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("POSTEXEC: running: '%s'\n", GLOBAL_POSTEXEC);
        }
      if (g_do_exec(NULL, ref_to_val_generic, GLOBAL_POSTEXEC, NULL) == -1)
        {
          if (gfl & F_OPT_VERBOSE)
            {
              print_str("WARNING: POSTEXEC failed: '%s'\n", GLOBAL_POSTEXEC);
            }
        }
    }

  return EXITVAL;
}

int
main(int argc, char *argv[])
{
  char **p_argv = (char**) argv;
  int r;

  g_setjmp(0, "main", NULL, NULL);
  if ((r = setup_sighandlers()))
    {
      print_str(
          "WARNING: UNABLE TO SETUP SIGNAL HANDLERS! (this is weird, please report it!) [%d]\n",
          r);
      sleep(5);
    }

  _p_macro_argc = argc;

  if ((r = parse_args(argc, argv, prio_f_ref)) > 0)
    {
      print_str(MSG_INIT_CMDLINE_ERROR, r);
      EXITVAL = 2;
      g_shutdown(NULL);
    }

  enable_logging();

  switch (updmode)
    {
  case PRIO_UPD_MODE_MACRO:
    ;
    uint64_t gfl_s = (gfl & (F_OPT_WBUFFER | F_OPT_PS_LOGGING | F_OPT_NOGLCONF));
    char **ptr;
    ptr = process_macro(prio_argv_off, NULL);
    if (ptr)
      {
        _p_macro_argv = p_argv = ptr;
        gfl = gfl_s;
      }
    else
      {
        g_shutdown(NULL);
      }
    break;
  case PRIO_UPD_MODE_INFO:
    g_print_info();
    g_shutdown(NULL);
    break;
    }

  updmode = 0;

  g_init(_p_macro_argc, p_argv);

  g_shutdown(NULL);

  return EXITVAL;
}

int
g_print_info(void)
{
  char buffer[4096];
  print_version_long(NULL, 0);
  print_str(MSG_NL);
  snprintf(buffer, 4095, "EP: @0x%s\n", __AA_SPFH);
  print_str(buffer, (uintaa_t) main);
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
      print_str(" size_t           %d\t\n", (sizeof(size_t)));
      print_str(" float            %d\t\n", (sizeof(float)));
      print_str(" double           %d\t\n", (sizeof(double)));
      print_str(MSG_NL);
      print_str(" void *           %d\t\n", PTRSZ);
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

char **
build_argv(char *args, size_t max, int *c)
{
  char **ptr = (char **) calloc(max, sizeof(char **));

  size_t args_l = strlen(args);
  int i_0, l_p = 0, b_c = 0;
  char sp_1 = 0x20, sp_2 = 0x22, sp_3 = 0x60;

  *c = 0;

  for (i_0 = 0; i_0 <= args_l && b_c < max; i_0++)
    {
      if (i_0 == 0)
        {
          while (args[i_0] == sp_1)
            {
              i_0++;
            }
          if (args[i_0] == sp_2 || args[i_0] == sp_3)
            {
              while (args[i_0] == sp_2 || args[i_0] == sp_3)
                {
                  i_0++;
                }
              sp_1 = sp_2;
              l_p = i_0;
            }
        }

      if ((((args[i_0] == sp_1 || (args[i_0] == sp_2 || args[i_0] == sp_3))
          && args[i_0 - 1] != 0x5C && args[i_0] != 0x5C) || !args[i_0])
          && i_0 > l_p)
        {

          if (i_0 == args_l - 1)
            {
              if (!(args[i_0] == sp_1 || args[i_0] == sp_2 || args[i_0] == sp_3))
                {
                  i_0++;
                }

            }

          size_t ptr_b_l = i_0 - l_p;

          ptr[b_c] = (char*) calloc(ptr_b_l + 1, 1);
          strncpy((char*) ptr[b_c], &args[l_p], ptr_b_l);

          b_c++;
          *c += 1;

          int ii_l = 1;
          while (args[i_0] == sp_1 || args[i_0] == sp_2 || args[i_0] == sp_3)
            {
              if (sp_1 != sp_2 && sp_1 != sp_3)
                {
                  if (args[i_0] == sp_2 || args[i_0] == sp_3)
                    {
                      i_0++;
                      break;
                    }
                }
              i_0++;
            }
          l_p = i_0;
          if (sp_1 == sp_2 || sp_1 == sp_3)
            {
              sp_1 = 0x20;
              sp_2 = 0x22;
              sp_3 = 0x60;
              while (args[i_0] == sp_1 || args[i_0] == sp_2 || args[i_0] == sp_3)
                {
                  i_0++;
                }
              l_p = i_0;
            }
          else
            {
              if (args[i_0 - ii_l] == 0x22 || args[i_0 - ii_l] == 0x60)
                {
                  sp_1 = args[i_0 - ii_l];
                  sp_2 = args[i_0 - ii_l];
                  sp_3 = args[i_0 - ii_l];
                  while (args[i_0] == sp_1 || args[i_0] == sp_2
                      || args[i_0] == sp_3)
                    {
                      i_0++;
                    }
                  l_p = i_0;
                }

            }
        }

    }

  return ptr;
}

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
  return NULL;
}

#define MSG_UNRECOGNIZED_DATA_TYPE 	"ERROR: [%s] unrecognized data type\n"

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
d_write(char *arg)
{
  g_setjmp(0, "d_write", NULL, NULL);

  int ret = 0;

  if (!arg)
    {
      print_str("ERROR: missing data type argument\n");
      return 1;
    }

  char *a_ptr = (char*) arg;
  char *datafile = g_dgetf(a_ptr);

  if (!datafile)
    {
      print_str(MSG_UNRECOGNIZED_DATA_TYPE, a_ptr);
      return 2;
    }

  off_t f_sz = get_file_size(datafile);

  g_act_1.flags |= F_GH_FFBUFFER;

  if (f_sz > 0)
    {
      g_act_1.flags |= F_GH_WAPPEND | F_GH_DFNOWIPE;
    }

  strncpy(g_act_1.file, datafile, strlen(datafile));

  if (determine_datatype(&g_act_1))
    {
      print_str(MSG_BAD_DATATYPE, datafile);
      return 3;
    }

  FILE *in, *pf_infile = NULL;
  struct stat st;

  errno = 0;
  if (!lstat(infile_p, &st))
    {
      pf_infile = fopen(infile_p, "rb");
    }

  if (pf_infile && !errno)
    {
      in = pf_infile;
    }
  else
    {
      in = stdin;
      g_act_1.flags |= F_GH_FROMSTDIN;
    }

  int r;

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: %s: loading data..\n", datafile);
    }

  if (!(gfl & F_OPT_MODE_BINARY))
    {
      if ((r = m_load_input_n(&g_act_1, in)))
        {
          print_str("ERROR: %s: [%d]: could not parse input data\n", datafile,
              r);
          ret = 5;
          goto end;
        }
    }
  else
    {
      if (!(g_act_1.flags & F_GH_FROMSTDIN))
        {
          g_act_1.total_sz = get_file_size(infile_p);
        }
      else
        {
          g_act_1.total_sz = db_max_size;
        }
      if ((r = load_data_md(&g_act_1.w_buffer, infile_p, &g_act_1)))
        {
          print_str(
              "ERROR: %s: [%d]: could not load input data (binary source)\n",
              datafile, r);
          ret = 12;
          goto end;
        }
    }

  if (g_act_1.flags & F_GH_FROMSTDIN)
    {
      g_act_1.flags ^= F_GH_FROMSTDIN;
    }

  if (!g_act_1.w_buffer.offset)
    {
      print_str("ERROR: %s: no records were loaded, aborting..\n", datafile);
      ret = 15;
      goto end;
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: '%s': parsed and loaded %llu records\n", a_ptr,
          (unsigned long long int) g_act_1.w_buffer.offset);
    }

  if ((gfl & F_OPT_ZPRUNEDUP) && !access(datafile, R_OK)
      && !g_fopen(datafile, "rb", F_DL_FOPEN_BUFFER, &g_act_1)
      && g_act_1.buffer.count)
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: '%s': pruning exact data duplicates..\n", a_ptr);
        }
      p_md_obj ptr_w = md_first(&g_act_1.w_buffer), ptr_r;
      int m = 1;
      while (ptr_w)
        {
          ptr_r = g_act_1.buffer.first;
          m = 1;
          while (ptr_r)
            {
              if (!(m = g_bin_compare(ptr_r->ptr, ptr_w->ptr,
                  (off_t) g_act_1.block_sz)))
                {
                  if (!md_unlink(&g_act_1.w_buffer, ptr_w))
                    {
                      print_str("%s: %s: [%llu]: %s, aborting build..\n",
                          g_act_1.w_buffer.offset ? "ERROR" : "WARNING",
                          datafile,
                          (unsigned long long int) g_act_1.w_buffer.offset,
                          g_act_1.w_buffer.offset ?
                              "could not unlink existing record" :
                              "all records already exist (nothing to do)");
                      ret = 11;
                      goto end;
                    }
                  break;
                }

              ptr_r = ptr_r->next;
            }

          ptr_w = ptr_w->next;
        }
    }

  if (rebuild_data_file(datafile, &g_act_1))
    {
      print_str(MSG_GEN_DFRFAIL, datafile);
      ret = 7;
      goto end;
    }
  else
    {
      if (g_act_1.bw || (gfl & F_OPT_VERBOSE4))
        {
          print_str(MSG_GEN_WROTE, datafile, g_act_1.bw, g_act_1.rw);
        }
    }

  end:

  if (pf_infile)
    {
      fclose(pf_infile);
    }

  return ret;
}

int
g_bin_compare(const void *p1, const void *p2, off_t size)
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

#define F_XRF_DO_STAT		(a32 << 1)
#define F_XRF_GET_DT_MODE	(a32 << 2)
#define F_XRF_GET_READ		(a32 << 3)
#define F_XRF_GET_WRITE		(a32 << 4)
#define F_XRF_GET_EXEC		(a32 << 5)
#define F_XRF_GET_UPERM		(a32 << 6)
#define F_XRF_GET_GPERM		(a32 << 7)
#define F_XRF_GET_OPERM		(a32 << 8)
#define F_XRF_GET_PERM		(a32 << 9)
#define F_XRF_GET_CRC32		(a32 << 10)
#define F_XRF_GET_CTIME		(a32 << 11)
#define F_XRF_GET_MINOR		(a32 << 12)
#define F_XRF_GET_MAJOR		(a32 << 13)
#define F_XRF_GET_SPARSE	(a32 << 14)
#define F_XRF_GET_STCTIME       (a32 << 15)

#define F_XRF_ACCESS_TYPES  (F_XRF_GET_READ|F_XRF_GET_WRITE|F_XRF_GET_EXEC)
#define F_XRF_PERM_TYPES	(F_XRF_GET_UPERM|F_XRF_GET_GPERM|F_XRF_GET_OPERM|F_XRF_GET_PERM)

typedef void
(*__d_xproc_rc)(char *name, void* aa_rh, __g_eds eds);
typedef void
(*__d_xproc_out)(char *desc, char *name);

typedef struct ___std_rh_0
{
  uint8_t rt_m;
  uint32_t flags;
  uint64_t st_1, st_2;
  _g_handle hdl;
  _d_xref p_xref;
  __d_xproc_rc xproc_rcl0, xproc_rcl1;
  _d_omfp xproc_out;
  char root[PATH_MAX];
} _std_rh, *__std_rh;

void
g_xproc_print_d(void *hdl, void *ptr, char *sbuffer)
{
  print_str("%s\n", sbuffer);
}

void
g_xproc_print(void *hdl, void *ptr, char *sbuffer)
{
  printf("%s\n", sbuffer);
}

void
g_preproc_xhdl(__std_rh ret)
{
  if (ret->flags & F_PD_RECURSIVE)
    {
      if (!(gfl & F_OPT_XFD))
        {
          ret->xproc_rcl0 = g_xproc_rc;
        }
      else
        {
          ret->xproc_rcl1 = g_xproc_rc;
        }
    }

  if ((gfl & F_OPT_FORMAT_BATCH))
    {
      ret->xproc_out = g_xproc_print;
    }
  else if ((gfl0 & F_OPT_PRINT))
    {
      ret->xproc_out = g_omfp_eassemble;
    }
  else if ((gfl0 & F_OPT_PRINTF))
    {
      ret->xproc_out = g_omfp_eassemblef;
    }
  else
    {
      if ((gfl & F_OPT_VERBOSE))
        {
          ret->xproc_out = g_xproc_print_d;
        }
      else
        {
          ret->xproc_out = NULL;
        }
    }

  ret->hdl.flags |= F_GH_ISFSX;
  ret->hdl.g_proc1_lookup = ref_to_val_lk_x;
  ret->hdl.jm_offset = (size_t) ((__d_xref ) NULL)->name;
  ret->hdl.g_proc2 = ref_to_val_ptr_x;
  ret->hdl._x_ref = &ret->p_xref;
  ret->hdl.block_sz = sizeof(_d_xref);
}

int
g_dump_ug(char *ug)
{
  g_setjmp(0, "g_dump_ug", NULL, NULL);
  _std_rh ret =
    { 0 };
  _g_eds eds =
    { 0 };
  char buffer[PATH_MAX] =
    { 0 };

  ret.flags = flags_udcfg | F_PD_MATCHREG;
  g_preproc_xhdl(&ret);

  if (g_proc_mr(&ret.hdl))
    {
      return 1;
    }

  snprintf(buffer, PATH_MAX, "%s/%s/%s", GLROOT, FTPDATA, ug);

  remove_repeating_chars(buffer, 0x2F);

  return enum_dir(buffer, g_process_directory, &ret, 0, &eds);
}

int
g_dump_gen(char *root)
{
  g_setjmp(0, "g_dump_gen", NULL, NULL);
  if (!root)
    {
      return 1;
    }

  _std_rh ret =
    { 0 };
  _g_eds eds =
    { 0 };

  ret.flags = flags_udcfg;
  g_preproc_xhdl(&ret);

  if (g_proc_mr(&ret.hdl))
    {
      return 1;
    }

  if (!(ret.flags & F_PD_MATCHTYPES))
    {
      ret.flags |= F_PD_MATCHTYPES;
    }

  ret.rt_m = 1;

  if (!file_exists(root))
    {
      ret.flags ^= F_PD_MATCHTYPES;
      ret.flags |= F_PD_MATCHREG;
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: %s is a file\n", root);
        }
      g_process_directory(root, DT_REG, &ret, &eds);
      goto end;
    }

  if ((gfl & F_OPT_CDIRONLY) && !dir_exists(root))
    {
      ret.flags ^= F_PD_MATCHTYPES;
      ret.flags |= F_PD_MATCHDIR;
      ret.xproc_rcl0 = NULL;
      ret.xproc_rcl1 = NULL;
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: %s is a directory\n", root);
        }
      g_process_directory(root, DT_DIR, &ret, &eds);
      goto end;
    }

  snprintf(ret.root, PATH_MAX, "%s", root);
  remove_repeating_chars(ret.root, 0x2F);

  enum_dir(ret.root, g_process_directory, &ret, 0, &eds);

  end:

  if (!(gfl & F_OPT_FORMAT_BATCH))
    {
      print_str("STATS: %s: OK: %llu/%llu\n", ret.root,
          (unsigned long long int) ret.st_1,
          (unsigned long long int) ret.st_1 + ret.st_2);
    }

  return ret.rt_m;
}

void
g_preproc_dm(char *name, __std_rh aa_rh, unsigned char type)
{
  size_t s_l = strlen(name);
  s_l >= PATH_MAX ? s_l = PATH_MAX - 1 : s_l;
  strncpy(aa_rh->p_xref.name, name, s_l);
  aa_rh->p_xref.name[s_l] = 0x0;
  if (aa_rh->p_xref.flags & F_XRF_DO_STAT)
    {
      if (lstat(name, &aa_rh->p_xref.st))
        {
          bzero(&aa_rh->p_xref.st, sizeof(struct stat));
        }
      else
        {
          if (aa_rh->p_xref.flags & F_XRF_GET_STCTIME)
            {
              aa_rh->p_xref.st.st_ctime = get_file_creation_time(
                  &aa_rh->p_xref.st);
            }
          if (aa_rh->p_xref.flags & F_XRF_GET_UPERM)
            {
              aa_rh->p_xref.uperm = (aa_rh->p_xref.st.st_mode & S_IRWXU) >> 6;
            }
          if (aa_rh->p_xref.flags & F_XRF_GET_GPERM)
            {
              aa_rh->p_xref.gperm = (aa_rh->p_xref.st.st_mode & S_IRWXG) >> 3;
            }
          if (aa_rh->p_xref.flags & F_XRF_GET_OPERM)
            {
              aa_rh->p_xref.operm = (aa_rh->p_xref.st.st_mode & S_IRWXO);
            }
          if (aa_rh->p_xref.flags & F_XRF_GET_MINOR)
            {
              aa_rh->p_xref.minor = minor(aa_rh->p_xref.st.st_dev);
            }
          if (aa_rh->p_xref.flags & F_XRF_GET_MAJOR)
            {
              aa_rh->p_xref.major = major(aa_rh->p_xref.st.st_dev);
            }
          if (aa_rh->p_xref.flags & F_XRF_GET_SPARSE)
            {
              aa_rh->p_xref.sparseness = ((float) aa_rh->p_xref.st.st_blksize
                  * (float) aa_rh->p_xref.st.st_blocks
                  / (float) aa_rh->p_xref.st.st_size);
            }
        }
    }

  if (aa_rh->p_xref.flags & F_XRF_GET_DT_MODE)
    {
      aa_rh->p_xref.type = type;
    }
  if (aa_rh->p_xref.flags & F_XRF_GET_READ)
    {
      aa_rh->p_xref.r = (uint8_t) !(access(aa_rh->p_xref.name, R_OK));
    }
  if (aa_rh->p_xref.flags & F_XRF_GET_WRITE)
    {
      aa_rh->p_xref.w = (uint8_t) !(access(aa_rh->p_xref.name, W_OK));
    }
  if (aa_rh->p_xref.flags & F_XRF_GET_EXEC)
    {
      aa_rh->p_xref.x = (uint8_t) !(access(aa_rh->p_xref.name, X_OK));
    }
  if (aa_rh->p_xref.flags & F_XRF_GET_CRC32)
    {
      file_crc32(aa_rh->p_xref.name, &aa_rh->p_xref.crc32);
    }
}

int
g_xproc_m(unsigned char type, char *name, __std_rh aa_rh, __g_eds eds)
{
  if ((gfl & F_OPT_MINDEPTH) && eds->depth < min_depth)
    {
      return 1;
    }
  g_preproc_dm(name, aa_rh, type);
  if ((g_bmatch((void*) &aa_rh->p_xref, &aa_rh->hdl, &aa_rh->hdl.buffer)))
    {
      aa_rh->st_2++;
      return 1;
    }

  aa_rh->rt_m = 0;
  aa_rh->st_1++;
  return 0;
}

void
g_xproc_rc(char *name, void *aa_rh, __g_eds eds)
{
  if (!((gfl & F_OPT_MAXDEPTH) && eds->depth >= max_depth))
    {
      eds->depth++;
      enum_dir(name, g_process_directory, aa_rh, 0, eds);
      eds->depth--;
    }
}

int
g_process_directory(char *name, unsigned char type, void *arg, __g_eds eds)
{
  __std_rh aa_rh = (__std_rh) arg;

  switch (type)
    {
      case DT_REG:;
      if (aa_rh->flags & F_PD_MATCHREG)
        {
          if (g_xproc_m(type, name, aa_rh, eds))
            {
              break;
            }
          if (aa_rh->xproc_out)
            {
              aa_rh->xproc_out(&aa_rh->hdl, (void*) &aa_rh->p_xref, (void*)aa_rh->p_xref.name);
            }
        }
      break;
      case DT_DIR:;

      if (aa_rh->xproc_rcl0)
        {
          aa_rh->xproc_rcl0(name, (void*)aa_rh, eds);
        }

      if ((gfl & F_OPT_KILL_GLOBAL) )
        {
          break;
        }

      if (aa_rh->flags & F_PD_MATCHDIR)
        {
          if (g_xproc_m(type, name, aa_rh, eds))
            {
              break;
            }
          if (aa_rh->xproc_out)
            {
              aa_rh->xproc_out(&aa_rh->hdl, (void*) &aa_rh->p_xref, (void*)aa_rh->p_xref.name);
            }
        }

      if (aa_rh->xproc_rcl1)
        {
          aa_rh->xproc_rcl1(name, (void*)aa_rh, eds);
        }

      break;
      case DT_LNK:;
      if (!(gfl & F_OPT_FOLLOW_LINKS))
        {
          if (g_xproc_m(type, name, aa_rh, eds))
            {
              break;
            }
          if (aa_rh->xproc_out)
            {
              aa_rh->xproc_out(&aa_rh->hdl, (void*) &aa_rh->p_xref, (void*)aa_rh->p_xref.name);
            }
        }
      else
        {
          char b_spl[PATH_MAX];
          ssize_t b_spl_l;
          if ( (b_spl_l=readlink(name, b_spl, PATH_MAX)) > 0 )
            {
              b_spl[b_spl_l] = 0x0;

              char *p_spl;

              if (stat(name, &aa_rh->p_xref.st))
                {
                  break;
                }

              uint8_t dt_mode=IFTODT(aa_rh->p_xref.st.st_mode);

              if (dt_mode == DT_DIR && (p_spl=strstr(name, b_spl)) && p_spl == name)
                {
                  print_str("ERROR: %s: filesystem loop detected inside '%s'\n", name, b_spl);
                  break;
                }

              g_process_directory(name, dt_mode, arg, eds);
            }

        }
      break;
    }

  return 0;
}

void
g_progress_stats(time_t s_t, time_t e_t, off_t total, off_t done)
{
  register float diff = (float) (e_t - s_t);
  register float rate = ((float) done / diff);

  fprintf(stderr, "PROCESSING: %llu/%llu [ %.2f%s ] | %.2f r/s | ETA: %.1f s\r",
      (long long unsigned int) done, (long long unsigned int) total,
      ((float) done / ((float) total / 100.0)), "%", rate,
      (float) (total - done) / rate);

}

int
dirlog_check_dupe(void)
{
  g_setjmp(0, "dirlog_check_dupe", NULL, NULL);
  struct dirlog buffer, buffer2;
  struct dirlog *d_ptr = NULL, *dd_ptr = NULL;
  char *s_pb, *ss_pb;

  if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &g_act_1))
    {
      return 2;
    }
  off_t st1, st2 = 0, st3 = 0;
  p_md_obj pmd_st1 = NULL, pmd_st2 = NULL;
  g_setjmp(0, "dirlog_check_dupe(loop)", NULL, NULL);
  time_t s_t = time(NULL), e_t = time(NULL), d_t = time(NULL);

  off_t nrec = g_act_1.total_sz / g_act_1.block_sz;

  if (g_act_1.buffer.count)
    {
      nrec = g_act_1.buffer.count;
    }

  if (gfl & F_OPT_VERBOSE)
    {
      g_progress_stats(s_t, e_t, nrec, st3);
    }
  off_t rtt;
  while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, g_act_1.block_sz)))
    {
      st3++;
      if (gfl & F_OPT_KILL_GLOBAL)
        {
          break;
        }

      if (g_bmatch(d_ptr, &g_act_1, &g_act_1.buffer))
        {
          continue;
        }

      rtt = s_string_r(d_ptr->dirname, "/");
      s_pb = &d_ptr->dirname[rtt + 1];
      size_t s_pb_l = strlen(s_pb);

      if (s_pb_l < 4)
        {
          continue;
        }

      if (gfl & F_OPT_VERBOSE)
        {
          e_t = time(NULL);

          if (e_t - d_t)
            {
              d_t = time(NULL);
              g_progress_stats(s_t, e_t, nrec, st3);
            }
        }

      st1 = g_act_1.offset;

      if (!g_act_1.buffer.count)
        {
          st2 = (off_t) ftello(g_act_1.fh);
        }
      else
        {

          pmd_st1 = g_act_1.buffer.r_pos;
          pmd_st2 = g_act_1.buffer.pos;
        }

      int ch = 0;

      while ((dd_ptr = (struct dirlog *) g_read(&buffer2, &g_act_1,
          g_act_1.block_sz)))
        {
          rtt = s_string_r(dd_ptr->dirname, "/");
          ss_pb = &dd_ptr->dirname[rtt + 1];
          size_t ss_pb_l = strlen(ss_pb);

          if (ss_pb_l == s_pb_l && !strncmp(s_pb, ss_pb, s_pb_l))
            {
              if (!ch)
                {
                  printf("\r%s               \n", d_ptr->dirname);
                  ch++;
                }
              printf("\r%s               \n", dd_ptr->dirname);
              if (gfl & F_OPT_VERBOSE)
                {
                  e_t = time(NULL);
                  g_progress_stats(s_t, e_t, nrec, st3);
                }
            }
        }

      g_act_1.offset = st1;
      if (!g_act_1.buffer.count)
        {
          fseeko(g_act_1.fh, (off_t) st2, SEEK_SET);
        }
      else
        {
          g_act_1.buffer.r_pos = pmd_st1;
          g_act_1.buffer.pos = pmd_st2;
        }

    }
  if (gfl & F_OPT_VERBOSE)
    {
      d_t = time(NULL);
      g_progress_stats(s_t, e_t, nrec, st3);
      print_str("\nSTATS: processed %llu/%llu records\n", st3, nrec);
    }
  return 0;
}

int
gh_rewind(__g_handle hdl)
{
  g_setjmp(0, "gh_rewind", NULL, NULL);
  if (hdl->buffer.count)
    {
      hdl->buffer.r_pos = hdl->buffer.objects;
      hdl->buffer.pos = hdl->buffer.r_pos;
      hdl->buffer.offset = 0;
      hdl->offset = 0;
    }
  else
    {
      if (hdl->fh)
        {
          rewind(hdl->fh);
          hdl->offset = 0;
        }
    }
  return 0;
}

int
dirlog_update_record(char *argv)
{
  g_setjmp(0, "dirlog_update_record", NULL, NULL);

  int r, seek = SEEK_END, ret = 0, dr;
  off_t offset = 0;
  uint64_t rl = MAX_uint64_t;
  struct dirlog dl =
    { 0 };
  ear arg =
    { 0 };
  arg.dirlog = &dl;

  if (gfl & F_OPT_SFV)
    {
      gfl |= F_OPT_NOWRITE | F_OPT_FORCE | F_OPT_FORCEWSFV;
    }

  mda dirchain =
    { 0 };
  p_md_obj ptr;

  md_init(&dirchain, 1024);

  if ((r = split_string(argv, 0x20, &dirchain)) < 1)
    {
      print_str("ERROR: [dirlog_update_record]: missing arguments\n");
      ret = 1;
      goto r_end;
    }

  data_backup_records(DIRLOG);

  char s_buffer[PATH_MAX];
  ptr = dirchain.objects;
  while (ptr)
    {
      snprintf(s_buffer, PATH_MAX, "%s/%s", SITEROOT, (char*) ptr->ptr);
      remove_repeating_chars(s_buffer, 0x2F);
      size_t s_buf_len = strlen(s_buffer);
      if (s_buffer[s_buf_len - 1] == 0x2F)
        {
          s_buffer[s_buf_len - 1] = 0x0;
        }

      rl = dirlog_find(s_buffer, 0, 0, NULL);

      char *mode = "a";

      if (!(gfl & F_OPT_FORCE) && rl < MAX_uint64_t)
        {
          print_str(
              "WARNING: %s: [%llu] already exists in dirlog (use -f to overwrite)\n",
              (char*) ptr->ptr, rl);
          ret = 4;
          goto end;
        }
      else if (rl < MAX_uint64_t)
        {
          if (gfl & F_OPT_VERBOSE)
            {
              print_str(
                  "WARNING: %s: [%llu] overwriting existing dirlog record\n",
                  (char*) ptr->ptr, rl);
            }
          offset = rl;
          seek = SEEK_SET;
          mode = "r+";
        }

      if (g_fopen(DIRLOG, mode, 0, &g_act_1))
        {
          goto r_end;
        }

      if ((r = release_generate_block(s_buffer, &arg)))
        {
          if (r < 5)
            {
              print_str("ERROR: %s: [%d] generating dirlog data chunk failed\n",
                  (char*) ptr->ptr, r);
            }
          ret = 3;
          goto end;
        }

      if ((dr = dirlog_write_record(arg.dirlog, offset, seek)))
        {
          print_str(
          MSG_GEN_DFWRITE, (char*) ptr->ptr, dr, (ulint64_t) offset, mode);
          ret = 6;
          goto end;
        }

      char buffer[MAX_G_PRINT_STATS_BUFFER] =
        { 0 };

      dirlog_format_block(arg.dirlog, buffer);

      end:

      g_close(&g_act_1);
      ptr = ptr->next;
    }
  r_end:

  md_g_free(&dirchain);

  if (dl_stats.bw || (gfl & F_OPT_VERBOSE4))
    {
      print_str(MSG_GEN_WROTE, DIRLOG, dl_stats.bw, dl_stats.rw);
    }

  return ret;
}

int
option_crc32(void *arg, int m)
{
  g_setjmp(0, "option_crc32", NULL, NULL);
  char *buffer;
  if (m == 2)
    {
      buffer = (char *) arg;
    }
  else
    {
      buffer = ((char **) arg)[0];
    }

  updmode = UPD_MODE_NOOP;

  if (!buffer)
    return 1;

  uint32_t crc32;

  off_t read = file_crc32(buffer, &crc32);

  if (read > 0)
    print_str("%.8X\n", (uint32_t) crc32);
  else
    {
      print_str("ERROR: %s: [%d] could not get CRC32\n", buffer,
      errno);
      EXITVAL = 1;
    }

  return 0;
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
dirlog_check_records(void)
{
  g_setjmp(0, "dirlog_check_records", NULL, NULL);
  struct dirlog buffer, buffer4;
  ear buffer3 =
    { 0 };
  char s_buffer[PATH_MAX], s_buffer2[PATH_MAX], s_buffer3[PATH_MAX] =
    { 0 };
  buffer3.dirlog = &buffer4;
  int r = 0, r2;
  char *mode = "r";
  uint32_t flags = 0;
  off_t dsz;

  if ((dsz = get_file_size(DIRLOG)) % DL_SZ)
    {
      print_str(MSG_GEN_DFCORRU, DIRLOG, (ulint64_t) dsz, (int) DL_SZ);
      print_str("NOTICE: use -r to rebuild (see --help)\n");
      return -1;
    }

  if (g_fopen(DIRLOG, mode, F_DL_FOPEN_BUFFER | flags, &g_act_1))
    {
      return 2;
    }

  if (!g_act_1.buffer.count && (gfl & F_OPT_FIX))
    {
      print_str(
          "ERROR: internal buffering must be enabled when fixing, increase limit with --memlimit (see --help)\n");
    }

  struct dirlog *d_ptr = NULL;
  int ir;

  while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ)))
    {
      if (!sigsetjmp(g_sigjmp.env, 1))
        {
          g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_records(loop)",
          NULL,
          NULL);

          if (gfl & F_OPT_KILL_GLOBAL)
            {
              break;
            }
          snprintf(s_buffer, PATH_MAX, "%s/%s", GLROOT, d_ptr->dirname);
          remove_repeating_chars(s_buffer, 0x2F);

          if (d_ptr->status == 1)
            {
              char *c_nb, *base, *c_nd, *dir;
              c_nb = strdup(d_ptr->dirname);
              base = basename(c_nb);
              c_nd = strdup(d_ptr->dirname);
              dir = dirname(c_nd);

              snprintf(s_buffer2, PATH_MAX, NUKESTR, base);
              snprintf(s_buffer3, PATH_MAX, "%s/%s/%s", GLROOT, dir, s_buffer2);
              remove_repeating_chars(s_buffer3, 0x2F);
              free(c_nb);
              free(c_nd);
            }

          if ((d_ptr->status != 1 && dir_exists(s_buffer))
              || (d_ptr->status == 1 && dir_exists(s_buffer3)))
            {
              print_str(
                  "WARNING: %s: listed in dirlog but does not exist on filesystem\n",
                  s_buffer);
              if (gfl & F_OPT_FIX)
                {
                  if (!md_unlink(&g_act_1.buffer, g_act_1.buffer.pos))
                    {
                      print_str("ERROR: %s: unlinking ghost record failed\n",
                          s_buffer);
                    }
                  r++;
                }
              continue;
            }

          if (gfl & F_OPT_C_GHOSTONLY)
            {
              continue;
            }

          struct nukelog n_buffer;
          ir = r;
          if (d_ptr->status == 1 || d_ptr->status == 2)
            {
              if (nukelog_find(d_ptr->dirname, 2, &n_buffer) == MAX_uint64_t)
                {
                  print_str(
                      "WARNING: %s: was marked as '%sNUKED' in dirlog but not found in nukelog\n",
                      s_buffer, d_ptr->status == 2 ? "UN" : "");
                }
              else
                {
                  if ((d_ptr->status == 1 && n_buffer.status != 0)
                      || (d_ptr->status == 2 && n_buffer.status != 1)
                      || (d_ptr->status == 0))
                    {
                      print_str(
                          "WARNING: %s: MISMATCH: was marked as '%sNUKED' in dirlog, but nukelog reads '%sNUKED'\n",
                          s_buffer, d_ptr->status == 2 ? "UN" : "",
                          n_buffer.status == 1 ? "UN" : "");
                    }
                }
              continue;
            }
          buffer3.flags |= F_EAR_NOVERB;

          if ((r2 = release_generate_block(s_buffer, &buffer3)))
            {
              if (r2 == 5)
                {
                  if ((gfl & F_OPT_FIX) && (gfl & F_OPT_FORCE))
                    {
                      if (remove(s_buffer))
                        {
                          print_str(
                              "WARNING: %s: failed removing empty directory\n",
                              s_buffer);

                        }
                      else
                        {
                          if (gfl & F_OPT_VERBOSE)
                            {
                              print_str("FIX: %s: removed empty directory\n",
                                  s_buffer);
                            }
                        }
                    }
                }
              else
                {
                  print_str(
                      "WARNING: [%s] - could not get directory information from the filesystem\n",
                      s_buffer);
                }
              r++;
              continue;
            }
          if (d_ptr->files != buffer4.files)
            {
              print_str(
                  "WARNING: [%s] file counts in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
                  d_ptr->dirname, d_ptr->files, buffer4.files);
              r++;
              if (gfl & F_OPT_FIX)
                {
                  d_ptr->files = buffer4.files;
                }
            }

          if (d_ptr->bytes != buffer4.bytes)
            {
              print_str(
                  "WARNING: [%s] directory sizes in dirlog and on disk do not match ( dirlog: %llu , filesystem: %llu )\n",
                  d_ptr->dirname, (ulint64_t) d_ptr->bytes,
                  (ulint64_t) buffer4.bytes);
              r++;
              if (gfl & F_OPT_FIX)
                {
                  d_ptr->bytes = buffer4.bytes;
                }
            }

          if (d_ptr->group != buffer4.group)
            {
              print_str(
                  "WARNING: [%s] group ids in dirlog and on disk do not match (dirlog:%hu filesystem:%hu)\n",
                  d_ptr->dirname, d_ptr->group, buffer4.group);
              r++;
              if (gfl & F_OPT_FIX)
                {
                  d_ptr->group = buffer4.group;
                }
            }

          if (d_ptr->uploader != buffer4.uploader)
            {
              print_str(
                  "WARNING: [%s] user ids in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
                  d_ptr->dirname, d_ptr->uploader, buffer4.uploader);
              r++;
              if (gfl & F_OPT_FIX)
                {
                  d_ptr->uploader = buffer4.uploader;
                }
            }

          if ((gfl & F_OPT_FORCE) && d_ptr->uptime != buffer4.uptime)
            {
              print_str(
                  "WARNING: [%s] folder creation dates in dirlog and on disk do not match (dirlog:%u, filesystem:%u)\n",
                  d_ptr->dirname, d_ptr->uptime, buffer4.uptime);
              r++;
              if (gfl & F_OPT_FIX)
                {
                  d_ptr->uptime = buffer4.uptime;
                }
            }
          if (r == ir)
            {
              if (gfl & F_OPT_VERBOSE2)
                {
                  print_str("OK: %s\n", d_ptr->dirname);
                }
            }
          else
            {
              if (gfl & F_OPT_VERBOSE2)
                {
                  print_str("BAD: %s\n", d_ptr->dirname);
                }

            }
        }

    }

  if (!(gfl & F_OPT_KILL_GLOBAL) && (gfl & F_OPT_FIX) && r)
    {
      if (rebuild_data_file(DIRLOG, &g_act_1))
        {
          print_str(MSG_GEN_DFRFAIL, DIRLOG);
        }
      else
        {
          if (g_act_1.bw || (gfl & F_OPT_VERBOSE4))
            {
              print_str(MSG_GEN_WROTE, DIRLOG, (ulint64_t) g_act_1.bw,
                  (ulint64_t) g_act_1.rw);
            }
        }
    }

  g_close(&g_act_1);

  return r;
}

int
do_match(__g_handle hdl, void *d_ptr, __g_match _gm)
{
  char *mstr;

  if (_gm->pmstr_cb)
    {
      mstr = _gm->pmstr_cb(d_ptr, _gm->field, hdl->mv1_b, MAX_VAR_LEN,
          &_gm->dtr);
    }
  else
    {
      mstr = (char*) (d_ptr + hdl->jm_offset);
    }

  if (!mstr)
    {
      print_str("ERROR: could not get match string\n");
      gfl |= F_OPT_KILL_GLOBAL;
      ofl |= F_BM_TERM;
      return 0;
    }

  int r = 0;
  if ((_gm->flags & F_GM_ISMATCH))
    {
      size_t mstr_l = strlen(mstr);

      int irl = strlen(_gm->match) != mstr_l, ir = strncmp(mstr, _gm->match,
          mstr_l);

      if ((_gm->match_i_m && (ir || irl)) || (!_gm->match_i_m && (!ir && !irl)))
        {
          r = 1;
        }
      goto end;
    }
  int rr;
  if ((_gm->flags & F_GM_ISREGEX)
      && (rr = regexec(&_gm->preg, mstr, 0, NULL, 0)) == _gm->reg_i_m)
    {
      r = 1;
    }

  end:

  return r;
}

int
g_lom_var_float(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;
  if (lom->g_tf_ptr_left)
    {
      lom->tf_left = lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_t_ptr_left)
    {
      lom->tf_left = (float) lom->g_t_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_ts_ptr_left)
    {
      lom->tf_left = (float) lom->g_ts_ptr_left(d_ptr, lom->t_l_off);
    }

  if (lom->g_tf_ptr_right)
    {
      lom->tf_right = lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_t_ptr_right)
    {
      lom->tf_right = (float) lom->g_t_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_ts_ptr_right)
    {
      lom->tf_right = (float) lom->g_ts_ptr_right(d_ptr, lom->t_r_off);
    }

  lom->result = lom->g_fcomp_ptr(lom->tf_left, lom->tf_right);

  return 0;
}

int
g_lom_var_int(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;

  if (lom->g_ts_ptr_left)
    {
      lom->ts_left = lom->g_ts_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_t_ptr_left)
    {
      lom->ts_left = (int64_t) lom->g_t_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_tf_ptr_left)
    {
      lom->ts_left = (int64_t) lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
    }

  if (lom->g_ts_ptr_right)
    {
      lom->ts_right = lom->g_ts_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_tf_ptr_right)
    {
      lom->ts_right = (int64_t) lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_t_ptr_right)
    {
      lom->ts_right = (int64_t) lom->g_t_ptr_right(d_ptr, lom->t_r_off);
    }

  lom->result = lom->g_iscomp_ptr(lom->ts_left, lom->ts_right);

  return 0;
}

int
g_lom_var_uint(void *d_ptr, void *_lom)
{
  __g_lom lom = _lom;
  if (lom->g_t_ptr_left)
    {
      lom->t_left = lom->g_t_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_tf_ptr_left)
    {
      lom->t_left = (uint64_t) lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
    }
  else if (lom->g_ts_ptr_left)
    {
      lom->t_left = (uint64_t) lom->g_ts_ptr_left(d_ptr, lom->t_l_off);
    }

  if (lom->g_t_ptr_right)
    {
      lom->t_right = lom->g_t_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_tf_ptr_right)
    {
      lom->t_right = (uint64_t) lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
    }
  else if (lom->g_ts_ptr_right)
    {
      lom->t_right = (uint64_t) lom->g_ts_ptr_right(d_ptr, lom->t_r_off);
    }

  lom->result = lom->g_icomp_ptr(lom->t_left, lom->t_right);

  return 0;
}

int
g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm)
{
  p_md_obj ptr = md_first(&_gm->lom);
  __g_lom lom, p_lom = NULL;

  int r_p = 1, i = 0;

  while (ptr)
    {
      lom = (__g_lom) ptr->ptr;
      lom->result = 0;
      i++;
      lom->g_lom_vp(d_ptr, (void*)lom);

      if (!ptr->next && !p_lom && lom->result)
        {
          return !lom->result;
        }

      if ( p_lom && p_lom->g_oper_ptr )
        {
          if (!(r_p = p_lom->g_oper_ptr(r_p, lom->result)))
            {
              if ( !ptr->next)
                {
                  return 1;
                }
            }
          else
            {
              if (!ptr->next)
                {
                  return !r_p;
                }
            }
        }
      else
        {
          r_p = lom->result;
        }

      p_lom = lom;
      ptr = ptr->next;
    }

  return 1;
}

int
g_bmatch(void *d_ptr, __g_handle hdl, pmda md)
{
  if (md)
    {
      if (hdl->max_results && md->rescnt >= hdl->max_results)
        {
          return 1;
        }
      if (hdl->max_hits && md->hitcnt >= hdl->max_hits)
        {
          return 0;
        }
    }

  p_md_obj ptr = md_first(&hdl->_match_rr);
  int r, r_p = 0;
  __g_match _gm, _p_gm = NULL;

  while (ptr)
    {
      r = 0;
      _gm = (__g_match) ptr->ptr;

      if ((_gm->flags & F_GM_ISLOM))
        {
          if ((g_lom_match(hdl, d_ptr, _gm)) == _gm->match_i_m)
            {
              r = 1;
              goto l_end;
            }
        }

      r = do_match(hdl, d_ptr, _gm);

      l_end:

      if (_p_gm && _p_gm->g_oper_ptr)
        {
          r_p = _p_gm->g_oper_ptr(r_p, r);
        }
      else
        {
          r_p = r;
        }

      _p_gm = _gm;
      ptr = ptr->next;
    }

  if (hdl->ifrh_l0)
    {
      hdl->ifrh_l0((void*) hdl, md, &r_p);
    }

  if (!r_p)
    {
      int r_e;

      if (hdl->exec_args.exc
          && WEXITSTATUS(
              r_e = hdl->exec_args.exc(d_ptr, (void*) NULL, NULL, (void*)hdl)))
        {
          r_p = 1;
        }
    }

  if (hdl->ifrh_l1)
    {
      hdl->ifrh_l1((void*) hdl, md, &r_p);
    }

  if (((gfl & F_OPT_MATCHQ) && r_p) || ((gfl & F_OPT_IMATCHQ) && !r_p))
    {
      ofl |= F_BM_TERM;
      gfl |= F_OPT_KILL_GLOBAL;
    }

  return r_p;
}

void
g_ipcbm(void *ptr, pmda md, int *r_p)
{
  __g_handle hdl = (__g_handle) ptr;

  if (!*r_p)
    {
      if (hdl->flags & F_GH_IFRES)
        {
          hdl->flags ^= F_GH_IFRES;
          *r_p = 1;
        }
      md->rescnt++;
    }
  else
    {
      if (hdl->flags & F_GH_IFHIT)
        {
          hdl->flags ^= F_GH_IFHIT;
          *r_p = 0;
        }
      md->hitcnt++;
    }

}

int
do_sort(__g_handle hdl, char *field, uint32_t flags)
{
  if (!(gfl & F_OPT_SORT))
    {
      return 0;
    }

  if (!field)
    {
      print_str("ERROR: %s: sorting requested but no field was set\n",
          hdl->file);
      return 1;
    }

  if ((gfl & F_OPT_VERBOSE))
    {
      print_str("NOTICE: %s: sorting %llu records..\n", hdl->file,
          (uint64_t) hdl->buffer.offset);
    }

  int r = g_sort(hdl, field, flags);

  if (r)
    {
      print_str("ERROR: %s: [%d]: could not sort data\n", hdl->file, r);
    }
  else
    {
      if (gfl & F_OPT_VERBOSE4)
        {
          print_str("NOTICE: %s: sorting done\n", hdl->file);
        }
    }

  return r;
}

int
g_filter(__g_handle hdl, pmda md)
{
  g_setjmp(0, "g_filter", NULL, NULL);
  if (!((hdl->exec_args.exc || (hdl->flags & F_GH_ISMP))) || !md->count)
    {
      return 0;
    }

  if (g_proc_mr(hdl))
    {
      return -1;
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: %s: passing %llu records through filters..\n",
          hdl->file, (uint64_t) hdl->buffer.offset);
    }

  off_t s_offset = md->offset;

  p_md_obj ptr = NULL, o_ptr;

  if (hdl->j_offset == 2)
    {
      ptr = md_last(md);
    }
  else
    {
      ptr = md_first(md);
      hdl->j_offset = 1;
    }

  int r = 0;

  while (ptr)
    {
      if (ofl & F_BM_TERM)
        {
          if (gfl & F_OPT_KILL_GLOBAL)
            {
              gfl ^= F_OPT_KILL_GLOBAL;
            }
          break;
        }
      if (g_bmatch(ptr->ptr, hdl, md))
        {
          o_ptr = ptr;
          ptr = (p_md_obj) *((void**) ptr + hdl->j_offset);
          if (!(md_unlink(md, o_ptr)))
            {
              r = 2;
              break;
            }
          continue;
        }
      ptr = (p_md_obj) *((void**) ptr + hdl->j_offset);
    }

  if (gfl & F_OPT_VERBOSE3)
    {
      print_str("NOTICE: %s: filtered %llu records..\n", hdl->file,
          (uint64_t) (s_offset - md->offset));
    }

  if (s_offset != md->offset)
    {
      hdl->flags |= F_GH_APFILT;
    }

  if (!md->offset)
    {
      return 1;
    }

  return r;
}

#define F_GPS_NONUKELOG				0x1

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
      print_str("STATS: %s: read %llu/%llu records\n", file,
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

#define 	ACT_WRITE_BUFFER_MEMBERS	10000

int
rebuild_dirlog(void)
{
  g_setjmp(0, "rebuild_dirlog", NULL, NULL);
  char mode[255] =
    { 0 };
  uint32_t flags = 0;
  int rt = 0;

  if (!(ofl & F_OVRR_NUKESTR))
    {
      print_str(
          "WARNING: failed extracting nuke string from glftpd.conf, nuked dirs might not get detected properly\n");
    }

  if (gfl & F_OPT_NOWRITE)
    {
      strncpy(mode, "r", 1);
      flags |= F_DL_FOPEN_BUFFER;
    }
  else if (gfl & F_OPT_UPDATE)
    {
      strncpy(mode, "a+", 2);
      flags |= F_DL_FOPEN_BUFFER | F_DL_FOPEN_FILE;
    }
  else
    {
      strncpy(mode, "w+", 2);
    }

  if (gfl & F_OPT_WBUFFER)
    {
      strncpy(g_act_1.mode, "r", 1);
      if (gfl & F_OPT_VERBOSE3)
        {
          print_str(
              "NOTICE: %s: allocating %u bytes for references (overhead)\n",
              DIRLOG, (uint32_t) (ACT_WRITE_BUFFER_MEMBERS * sizeof(md_obj)));
        }
      md_init(&g_act_1.w_buffer, ACT_WRITE_BUFFER_MEMBERS);
      g_act_1.block_sz = DL_SZ;
      g_act_1.flags |= F_GH_FFBUFFER | F_GH_WAPPEND
          | ((gfl & F_OPT_UPDATE) ? F_GH_DFNOWIPE : 0);
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("NOTICE: %s: explicit write pre-caching enabled\n", DIRLOG);
        }
    }
  else
    {
      strncpy(g_act_1.mode, mode, strlen(mode));
      data_backup_records(DIRLOG);
    }
  g_act_1.block_sz = DL_SZ;
  g_act_1.flags |= F_GH_ISDIRLOG;

  int dfex = file_exists(DIRLOG);

  if ((gfl & F_OPT_UPDATE) && dfex)
    {
      print_str(
          "WARNING: %s: requested update, but no dirlog exists - removing update flag..\n",
          DIRLOG);
      gfl ^= F_OPT_UPDATE;
      flags ^= F_DL_FOPEN_BUFFER;
    }

  if (!strncmp(g_act_1.mode, "r", 1) && dfex)
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str(
              "WARNING: %s: requested read mode access but file not there\n",
              DIRLOG);
        }
    }
  else if (g_fopen(DIRLOG, g_act_1.mode, flags, &g_act_1))
    {
      print_str("ERROR: could not open dirlog, mode '%s', flags %u\n",
          g_act_1.mode, flags);
      return errno;
    }

  mda dirchain =
    { 0 }, buffer2 =
    { 0 };

  if (g_proc_mr(&g_act_1))
    {
      return 12;
    }

  if (gfl & F_OPT_FORCE)
    {
      print_str("NOTICE: performing a full siteroot rescan\n");
      print_str("SCANNING: '%s'\n", SITEROOT);
      update_records(SITEROOT, 0);
      goto rw_end;
    }

  char buffer[V_MB + 1] =
    { 0 };

  md_init(&dirchain, 128);

  if (read_file(DU_FLD, buffer, V_MB, 0, NULL) < 1)
    {
      print_str(
          "ERROR: unable to read folders file '%s', read MANUAL on how to set it up, or use -f (force) to do a full rescan (not compatible with -u (update))..\n",
          DU_FLD, SITEROOT);
      goto r_end;
    }

  int r, r2;

  if ((r = split_string(buffer, 0x13, &dirchain)) < 1)
    {
      print_str("ERROR: [%d] could not parse input from %s\n", r, DU_FLD);
      rt = 5;
      goto r_end;
    }

  int i = 0, ib;
  char s_buffer[PATH_MAX] =
    { 0 };
  p_md_obj ptr = dirchain.objects;

  while (ptr)
    {
      if (!sigsetjmp(g_sigjmp.env, 1))
        {
          g_setjmp(F_SIGERR_CONTINUE, "rebuild_dirlog(loop)", NULL,
          NULL);
          if (gfl & F_OPT_KILL_GLOBAL)
            {
              rt = EXITVAL;
              break;
            }

          md_init(&buffer2, 6);
          i++;
          if ((r2 = split_string((char*) ptr->ptr, 0x20, &buffer2)) != 2)
            {
              print_str("ERROR: [%d] could not parse line %d from %s\n", r2, i,
                  DU_FLD);
              goto lend;
            }
          bzero(s_buffer, PATH_MAX);
          snprintf(s_buffer, PATH_MAX, "%s/%s", SITEROOT,
              (char*) buffer2.objects->ptr);
          remove_repeating_chars(s_buffer, 0x2F);

          size_t s_buf_len = strlen(s_buffer);
          if (s_buffer[s_buf_len - 1] == 0x2F)
            {
              s_buffer[s_buf_len - 1] = 0x0;
            }

          ib = strtol((char*) ((p_md_obj) buffer2.objects->next)->ptr,
          NULL, 10);

          if (errno == ERANGE)
            {
              print_str("ERROR: could not get depth from line %d\n", i);
              goto lend;
            }
          if (dir_exists(s_buffer))
            {
              print_str("ERROR: %s: directory doesn't exist (line %d)\n",
                  s_buffer, i);
              goto lend;
            }
          char *ndup = strdup(s_buffer);
          char *nbase = basename(ndup);

          print_str("SCANNING: '%s', depth: %d\n", nbase, ib);
          if (update_records(s_buffer, ib) < 1)
            {
              print_str("WARNING: %s: nothing was processed\n", nbase);
            }

          free(ndup);
          lend:

          md_g_free(&buffer2);
        }
      ptr = ptr->next;
    }

  rw_end:

  if (g_act_1.flags & F_GH_FFBUFFER)
    {
      if (rebuild_data_file(DIRLOG, &g_act_1))
        {
          print_str(MSG_GEN_DFRFAIL, DIRLOG);
        }
    }

  r_end:

  md_g_free(&dirchain);

  g_close(&g_act_1);

  if (dl_stats.bw || (gfl & F_OPT_VERBOSE4))
    {
      print_str(MSG_GEN_WROTE, DIRLOG, dl_stats.bw, dl_stats.rw);
    }

  return rt;
}

int
process_opt_n(char *opt, void *arg, void *reference_array, int m, int *ret)
{
  int
  (*proc_opt_generic)(void *arg, int m);
  int i = 0;
  p_ora ora = (p_ora) reference_array;

  while (ora->option)
    {
      size_t oo_l = strlen(ora->option);
      if (oo_l == strlen(opt) && !strncmp(ora->option, opt, oo_l))
        {
          if (ora->function)
            {
              proc_opt_generic = ora->function;
              *ret = i;
              return proc_opt_generic(arg, m);
            }
          else
            {
              return -4;
            }
        }

      ora++;
      i++;
    }
  return -2;
}

int
parse_args(int argc, char **argv, void*fref_t[])
{
  g_setjmp(0, "parse_args", NULL, NULL);
  int vi, ret, c = 0;

  int i;

  char *c_arg;

  p_ora ora = (p_ora) fref_t;

  for (i = 1, ret = 0; i < argc; i++, vi = -1)
    {
      c_arg = argv[i];

      char *p_iseq = strchr(c_arg, 0x3D);

      if (p_iseq)
        {
          char bp_opt[64];
          size_t p_isl = p_iseq - c_arg;
          p_isl > 63 ? p_isl = 63 : 0;
          strncpy(bp_opt, c_arg, p_isl);
          bp_opt[p_isl] = 0x0;
          c_arg = bp_opt;
          p_iseq++;

          if ((ret = process_opt_n(c_arg, p_iseq, fref_t, 2, &vi)))
            {
              if (fref_t != prio_f_ref)
                {
                  print_str("ERROR: [%d] malformed/invalid argument '%s'\n",
                      ret, c_arg);
                  c = -2;
                  goto end;
                }
            }
          goto ll_end;

        }
      else
        {
          if ((ret = process_opt_n(c_arg, (char*) &argv[i + 1], fref_t, 0, &vi)))
            {
              if (fref_t != prio_f_ref)
                {
                  print_str("ERROR: [%d] alformed/invalid argument '%s'\n", ret,
                      c_arg);
                  c = -2;
                  goto end;
                }

              //goto ll_end;

            }
          else
            {
              c++;
            }

        }

      if (vi > -1)
        {
          i += (int) (uintaa_t) ora[vi].arg_cnt;
        }

      ll_end: ;
    }
  end:

  if (!c)
    {
      return -1;
    }

  return ret;
}

int
update_records(char *dirname, int depth)
{
  g_setjmp(0, "update_records", NULL, NULL);
  struct dirlog buffer =
    { 0 };
  ear arg =
    { 0 };

  if (dir_exists(dirname))
    return 2;

  arg.depth = depth;
  arg.dirlog = &buffer;

  _g_eds eds =
    { 0 };

  return enum_dir(dirname, proc_section, &arg, 0, &eds);

}

int
proc_directory(char *name, unsigned char type, void *arg, __g_eds eds)
{
  ear *iarg = (ear*) arg;
  uint32_t crc32 = 0;
  char buffer[PATH_MAX] =
    { 0 };
  char *fn, *fn2, *base;

  if (!reg_match("\\/[.]{1,2}$", name, 0))
    return 1;

  if (!reg_match("\\/[.].*$", name, REG_NEWLINE))
    return 1;

  switch (type)
    {
  case DT_REG:
    fn2 = strdup(name);
    base = g_basename(name);
    if ((gfl & F_OPT_SFV)
        && (updmode == UPD_MODE_RECURSIVE || updmode == UPD_MODE_SINGLE)
        && reg_match(PREG_SFV_SKIP_EXT, fn2,
        REG_ICASE | REG_NEWLINE) && file_crc32(name, &crc32) > 0)
      {
        fn = strdup(name);
        char *dn = g_basename(g_dirname(fn));
        free(fn2);
        fn2 = strdup(name);
        snprintf(iarg->buffer, PATH_MAX, "%s/%s.sfv.tmp", g_dirname(fn2), dn);
        snprintf(iarg->buffer2, PATH_MAX, "%s/%s.sfv", fn2, dn);
        char buffer2[PATH_MAX + 10];
        snprintf(buffer2, 1024, "%s %.8X\n", base, (uint32_t) crc32);
        if (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV))
          {
            if (!write_file_text(buffer2, iarg->buffer))
              {
                print_str("ERROR: %s: failed writing to SFV file: '%s'\n", name,
                    iarg->buffer);
              }
          }
        iarg->flags |= F_EARG_SFV;
        snprintf(buffer, PATH_MAX, "  %.8X", (uint32_t) crc32);
        free(fn);
      }
    off_t fs = get_file_size(name);
    iarg->dirlog->bytes += fs;
    iarg->dirlog->files++;
    if (gfl & F_OPT_VERBOSE4)
      {
        print_str("     %s  %.2fMB%s\n", base, (double) fs / 1024.0 / 1024.0,
            buffer);
      }

    free(fn2);
    break;
  case DT_DIR:
    if ((gfl & F_OPT_SFV)
        && (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV)))
      {
        enum_dir(name, delete_file, (void*) "\\.sfv(\\.tmp|)$", 0, NULL);
      }

    enum_dir(name, proc_directory, iarg, 0, eds);
    break;
    }

  return 0;
}

#define MSG_PS_UWSFV	"WARNING: %s: unable wiping temp. SFV file (remove it manually)\n"

int
proc_section(char *name, unsigned char type, void *arg, __g_eds eds)
{
  ear *iarg = (ear*) arg;
  int r;
  uint64_t rl;

  if (!reg_match("\\/[.]{1,2}", name, 0))
    {
      return 1;
    }

  if (!reg_match("\\/[.].*$", name, REG_NEWLINE))
    {
      return 1;
    }

  switch (type)
    {
  case DT_DIR:
    iarg->depth--;
    if (!iarg->depth || (gfl & F_OPT_FORCE))
      {
        if (gfl & F_OPT_UPDATE)
          {
            if (((rl = dirlog_find(name, 1, F_DL_FOPEN_REWIND, NULL))
                < MAX_uint64_t))
              {
                if (gfl & F_OPT_VERBOSE2)
                  {
                    print_str(
                        "WARNING: %s: [%llu] record already exists, not importing\n",
                        name, rl);
                  }
                goto end;
              }
          }
        bzero(iarg->buffer, PATH_MAX);
        iarg->flags = 0;
        if ((r = release_generate_block(name, iarg)))
          {
            if (r < 5)
              print_str("ERROR: %s: [%d] generating dirlog data chunk failed\n",
                  name, r);
            goto end;
          }

        if (g_bmatch(iarg->dirlog, &g_act_1, &g_act_1.buffer))
          {
            if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV))
              {
                if (remove(iarg->buffer))
                  {
                    print_str( MSG_PS_UWSFV, iarg->buffer);
                  }
              }
            goto end;
          }

        if (gfl & F_OPT_KILL_GLOBAL)
          {
            if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV))
              {
                if (remove(iarg->buffer))
                  {
                    print_str( MSG_PS_UWSFV, iarg->buffer);
                  }
              }
            goto end;
          }

        if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV))
          {
            iarg->dirlog->bytes += (uint64_t) get_file_size(iarg->buffer);
            iarg->dirlog->files++;
            if (!rename(iarg->buffer, iarg->buffer2))
              {
                if (gfl & F_OPT_VERBOSE)
                  {
                    print_str("NOTICE: '%s': succesfully generated SFV file\n",
                        name);
                  }
              }
            else
              {
                print_str("ERROR: '%s': failed renaming '%s' to '%s'\n",
                basename(iarg->buffer2), basename(iarg->buffer));
              }
          }

        if (g_act_1.flags & F_GH_FFBUFFER)
          {
            if ((r = g_load_record(&g_act_1, (const void*) iarg->dirlog)))
              {
                print_str(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
                    (ulint64_t) g_act_1.w_buffer.offset, "wbuffer");
              }
          }
        else
          {
            if ((r = dirlog_write_record(iarg->dirlog, 0, SEEK_END)))
              {
                print_str(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
                    (ulint64_t) g_act_1.offset - 1, "w");
                goto end;
              }
          }

        char buffer[MAX_G_PRINT_STATS_BUFFER] =
          { 0 };

        if ((gfl & F_OPT_VERBOSE))
          {
            dirlog_format_block(iarg->dirlog, buffer);
          }

        if (gfl & F_OPT_FORCE)
          {
            enum_dir(name, proc_section, iarg, 0, eds);
          }
      }
    else
      {
        enum_dir(name, proc_section, iarg, 0, eds);
      }
    end: iarg->depth++;
    break;
    }
  return 0;
}

int
release_generate_block(char *name, ear *iarg)
{
  bzero(iarg->dirlog, sizeof(struct dirlog));

  int r, ret = 0;
  struct stat st =
    { 0 }, st2 =
    { 0 };

  if (gfl & F_OPT_FOLLOW_LINKS)
    {
      if (stat(name, &st))
        {
          return 1;
        }
    }
  else
    {
      if (lstat(name, &st))
        {
          return 1;
        }

      if ((st.st_mode & S_IFMT) == S_IFLNK)
        {
          if (gfl & F_OPT_VERBOSE2)
            {
              print_str("WARNING: %s - is symbolic link, skipping..\n", name);
            }
          return 6;
        }
    }

  time_t orig_ctime = get_file_creation_time(&st);

  if ((gfl & F_OPT_SFV) && (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV)))
    {
      enum_dir(name, delete_file, (void*) "\\.sfv(\\.tmp|)$", 0, NULL);
    }

  if ((gfl & F_OPT_VERBOSE2) && !(iarg->flags & F_EAR_NOVERB))
    print_str("ENTERING: %s\n", name);

  _g_eds eds =
    { 0 };

  if ((r = enum_dir(name, proc_directory, iarg, 0, &eds)) < 1
      || !iarg->dirlog->files)
    {
      if (gfl & F_OPT_VERBOSE)
        {
          print_str("WARNING: %s: [%d] - empty directory\n", name, r);
        }
      ret = 5;
    }
  g_setjmp(0, "release_generate_block(2)", NULL, NULL);

  if ((gfl & F_OPT_VERBOSE2) && !(iarg->flags & F_EAR_NOVERB))
    print_str("EXITING: %s\n", name);

  if ((gfl & F_OPT_SFV) && !(gfl & F_OPT_NOWRITE))
    {
      if (gfl & F_OPT_FOLLOW_LINKS)
        {
          if (stat(name, &st2))
            {
              ret = 1;
            }
        }
      else
        {
          if (lstat(name, &st2))
            {
              ret = 1;
            }
        }
      time_t c_ctime = get_file_creation_time(&st2);
      if (c_ctime != orig_ctime)
        {
          if (gfl & F_OPT_VERBOSE4)
            {
              print_str(
                  "NOTICE: %s: restoring original folder modification date\n",
                  name);
            }
          struct utimbuf utb;
          utb.actime = 0;
          utb.modtime = orig_ctime;
          if (utime(name, &utb))
            {
              print_str(
                  "WARNING: %s: SFVGEN failed to restore original modification date\n",
                  name);
            }
        }

    }

  if (ret)
    {
      goto end;
    }

  char *bn = g_basename(name);

  iarg->dirlog->uptime = orig_ctime;
  iarg->dirlog->uploader = (uint16_t) st.st_uid;
  iarg->dirlog->group = (uint16_t) st.st_gid;
  char buffer[PATH_MAX] =
    { 0 };

  if ((r = get_relative_path(name, GLROOT, buffer)))
    {
      print_str("ERROR: [%s] could not get relative to root directory name\n",
          bn);
      ret = 2;
      goto end;
    }

  struct nukelog n_buffer =
    { 0 };
  if (nukelog_find(buffer, 2, &n_buffer) < MAX_uint64_t)
    {
      iarg->dirlog->status = n_buffer.status + 1;
      strncpy(iarg->dirlog->dirname, n_buffer.dirname,
          strlen(n_buffer.dirname));
    }
  else
    {
      strncpy(iarg->dirlog->dirname, buffer, strlen(buffer));
    }

  end:

  return ret;
}

int
get_relative_path(char *subject, char *root, char *output)
{
  char *root_dir = root;

  if (!root_dir)
    return 11;

  int i, root_dir_len = strlen(root_dir);

  for (i = 0; i < root_dir_len; i++)
    {
      if (subject[i] != root_dir[i])
        break;
    }

  while (subject[i] != 0x2F && i > 0)
    {
      i--;
    }

  snprintf(output, PATH_MAX, "%s", &subject[i]);
  return 0;
}

#define STD_FMT_TIME_STR  	"%d %b %Y %T"

int
dirlog_format_block(void *iarg, char *output)
{
  char buffer2[255];

  struct dirlog *data = (struct dirlog *) iarg;

  char *base = NULL;

  if (gfl & F_OPT_VERBOSE)
    {
      base = data->dirname;
    }
  else
    {
      base = g_basename(data->dirname);
    }

  time_t t_t = (time_t) data->uptime;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str(
      "DIRLOG: %s - %llu Mbytes in %hu files - created %s by %hu.%hu [%hu]\n",
      base, (ulint64_t) (data->bytes / 1024 / 1024), data->files, buffer2,
      data->uploader, data->group, data->status);
}

int
dirlog_format_block_batch(void *iarg, char *output)
{
  struct dirlog *data = (struct dirlog *) iarg;
  return printf("DIRLOG\x9%s\x9%llu\x9%hu\x9%d\x9%hu\x9%hu\x9%hu\n",
      data->dirname, (ulint64_t) data->bytes, data->files,
      (int32_t) data->uptime, data->uploader, data->group, data->status);

}

int
dirlog_format_block_exp(void *iarg, char *output)
{
  struct dirlog *data = (struct dirlog *) iarg;
  return printf("dir %s\n"
      "size %llu\n"
      "files %hu\n"
      "time %d\n"
      "user %hu\n"
      "group %hu\n"
      "status %hu\n\n", data->dirname, (ulint64_t) data->bytes, data->files,
      (int32_t) data->uptime, data->uploader, data->group, data->status);
}

int
nukelog_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct nukelog *data = (struct nukelog *) iarg;

  char *base = NULL;

  if (gfl & F_OPT_VERBOSE)
    {
      base = data->dirname;
    }
  else
    {
      base = g_basename(data->dirname);
    }

  time_t t_t = (time_t) data->nuketime;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str(
      "NUKELOG: %s - %s, reason: '%s' [%.2f MB] - factor: %hu, %s: %s, nukee: %s - %s\n",
      base, !data->status ? "NUKED" : "UNNUKED", data->reason, data->bytes,
      data->mult, !data->status ? "nuker" : "unnuker",
      !data->status ? data->nuker : data->unnuker, data->nukee, buffer2);

}

int
nukelog_format_block_batch(void *iarg, char *output)
{
  struct nukelog *data = (struct nukelog *) iarg;
  return printf("NUKELOG\x9%s\x9%s\x9%hu\x9%.2f\x9%s\x9%s\x9%s\x9%d\x9%hu\n",
      data->dirname, data->reason, data->mult, data->bytes, data->nuker,
      data->unnuker, data->nukee, (int32_t) data->nuketime, data->status);
}

int
nukelog_format_block_exp(void *iarg, char *output)
{
  struct nukelog *data = (struct nukelog *) iarg;
  return printf("dir %s\n"
      "reason %s\n"
      "mult %hu\n"
      "size %.2f\n"
      "nuker %s\n"
      "unnuker %s\n"
      "nukee %s\n"
      "time %d\n"
      "status %hu\n\n", data->dirname, data->reason, data->mult, data->bytes,
      data->nuker, data->unnuker, data->nukee, (int32_t) data->nuketime,
      data->status);
}

int
dupefile_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct dupefile *data = (struct dupefile *) iarg;

  time_t t_t = (time_t) data->timeup;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str("DUPEFILE: %s - uploader: %s, time: %s\n", data->filename,
      data->uploader, buffer2);

}

int
dupefile_format_block_batch(void *iarg, char *output)
{
  struct dupefile *data = (struct dupefile *) iarg;
  return printf("DUPEFILE\x9%s\x9%s\x9%d\n", data->filename, data->uploader,
      (int32_t) data->timeup);

}

int
dupefile_format_block_exp(void *iarg, char *output)
{
  struct dupefile *data = (struct dupefile *) iarg;
  return printf("file %s\n"
      "user %s\n"
      "time %d\n\n", data->filename, data->uploader, (int32_t) data->timeup);

}

int
lastonlog_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 }, buffer3[255] =
    { 0 }, buffer4[12] =
    { 0 };

  struct lastonlog *data = (struct lastonlog *) iarg;

  time_t t_t_ln = (time_t) data->logon;
  time_t t_t_lf = (time_t) data->logoff;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t_ln));
  strftime(buffer3, 255, STD_FMT_TIME_STR, localtime(&t_t_lf));

  memcpy(buffer4, data->stats, sizeof(data->stats));

  return print_str(
      "LASTONLOG: user: %s/%s [%s] - logon: %s, logoff: %s - up/down: %lu/%lu B, changes: %s\n",
      data->uname, data->gname, data->tagline, buffer2, buffer3,
      (unsigned long) data->upload, (unsigned long) data->download, buffer4);

}

int
lastonlog_format_block_batch(void *iarg, char *output)
{
  struct lastonlog *data = (struct lastonlog *) iarg;
  char buffer4[12] =
    { 0 };
  memcpy(buffer4, data->stats, sizeof(data->stats));
  return printf("LASTONLOG\x9%s\x9%s\x9%s\x9%d\x9%d\x9%lu\x9%lu\x9%s\n",
      data->uname, data->gname, data->tagline, (int32_t) data->logon,
      (int32_t) data->logoff, (unsigned long) data->upload,
      (unsigned long) data->download, buffer4);
}

int
lastonlog_format_block_exp(void *iarg, char *output)
{
  struct lastonlog *data = (struct lastonlog *) iarg;
  char buffer4[12] =
    { 0 };
  memcpy(buffer4, data->stats, sizeof(data->stats));
  return printf("user %s\n"
      "group %s\n"
      "tag %s\n"
      "logon %d\n"
      "logoff %d\n"
      "upload %lu\n"
      "download %lu\n"
      "stats %s\n\n", data->uname, data->gname, data->tagline,
      (int32_t) data->logon, (int32_t) data->logoff,
      (unsigned long) data->upload, (unsigned long) data->download, buffer4);
}

int
oneliner_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct oneliner *data = (struct oneliner *) iarg;

  time_t t_t = (time_t) data->timestamp;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str("ONELINER: user: %s/%s [%s] - time: %s, message: %s\n",
      data->uname, data->gname, data->tagline, buffer2, data->message);

}

int
oneliner_format_block_batch(void *iarg, char *output)
{
  struct oneliner *data = (struct oneliner *) iarg;
  return printf("ONELINER\x9%s\x9%s\x9%s\x9%d\x9%s\n", data->uname, data->gname,
      data->tagline, (int32_t) data->timestamp, data->message);
}

int
oneliner_format_block_exp(void *iarg, char *output)
{
  struct oneliner *data = (struct oneliner *) iarg;
  return printf("user %s\n"
      "group %s\n"
      "tag %s\n"
      "time %d\n"
      "msg %s\n\n", data->uname, data->gname, data->tagline,
      (int32_t) data->timestamp, data->message);
}

#define FMT_SP_OFF 	30

int
online_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct ONLINE *data = (struct ONLINE *) iarg;

  time_t t_t = (time_t) data->login_time;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  if (!strlen(data->username))
    {
      return 0;
    }

  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }

  time_t ltime = time(NULL) - (time_t) data->login_time;

  if (ltime < 0)
    {
      ltime = 0;
    }

  return print_str("[ONLINE]\n"
      "    User:            %s\n"
      "    Host:            %s\n"
      "    GID:             %u\n"
      "    Login:           %s (%us)\n"
      "    Tag:             %s\n"
      "    SSL:             %s\n"
      "    PID:             %u\n"
      "    XFER:            %lld Bytes\n"
      "    Rate:            %.2f KB/s\n"
      "    Status:          %s\n"
      "    CWD:             %s\n\n", data->username, data->host,
      (uint32_t) data->groupid, buffer2, (uint32_t) ltime, data->tagline,
      (!data->ssl_flag ?
          "NO" :
          (data->ssl_flag == 1 ?
              "YES" : (data->ssl_flag == 2 ? "YES (DATA)" : "UNKNOWN"))),
      (uint32_t) data->procid, (ulint64_t) data->bytes_xfer, kbps, data->status,
      data->currentdir);

}

int
online_format_block_batch(void *iarg, char *output)
{
  struct ONLINE *data = (struct ONLINE *) iarg;
  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }
  return printf(
      "ONLINE\x9%s\x9%s\x9%d\x9%d\x9%s\x9%hd\x9%d\x9%llu\x9%llu\x9%llu\x9%s\x9%s\x9%d\x9%d\n",
      data->username, data->host, (int32_t) data->groupid,
      (int32_t) data->login_time, data->tagline, (int16_t) data->ssl_flag,
      (int32_t) data->procid, (ulint64_t) data->bytes_xfer,
      (ulint64_t) data->bytes_txfer, (ulint64_t) kbps, data->status,
      data->currentdir, (int32_t) data->tstart.tv_sec,
      (int32_t) data->txfer.tv_sec);

}

int
online_format_block_exp(void *iarg, char *output)
{
  struct ONLINE *data = (struct ONLINE *) iarg;
  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }
  return printf("user %s\n"
      "host %s\n"
      "group %d\n"
      "time %d\n"
      "tag %s\n"
      "ssl %hd\n"
      "pid %d\n"
      "bxfer %llu\n"
      "btxfer %llu\n"
      "kbps %llu\n"
      "status %s\n"
      "dir %s\n"
      "lupdtime %d\n"
      "lxfertime %d\n\n", data->username, data->host, (int32_t) data->groupid,
      (int32_t) data->login_time, data->tagline, (int16_t) data->ssl_flag,
      (int32_t) data->procid, (ulint64_t) data->bytes_xfer,
      (ulint64_t) data->bytes_txfer, (ulint64_t) kbps, data->status,
      data->currentdir, (int32_t) data->tstart.tv_sec,
      (int32_t) data->txfer.tv_sec);

}

int
online_format_block_comp(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  struct ONLINE *data = (struct ONLINE *) iarg;

  time_t t_t = (time_t) data->login_time;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  if (!strlen(data->username))
    {
      return 0;
    }

  int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
  float kb = (data->bytes_xfer / 1024), kbps = 0.0;

  if (tdiff > 0 && kb > 0)
    {
      kbps = kb / (float) tdiff;
    }

  time_t ltime = time(NULL) - (time_t) data->login_time;

  if (ltime < 0)
    {
      ltime = 0;
    }

  char sp_buffer[255], sp_buffer2[255], sp_buffer3[255];
  char d_buffer[255] =
    { 0 };

  snprintf(d_buffer, 254, "%u", (uint32_t) ltime);
  size_t d_len1 = strlen(d_buffer);
  snprintf(d_buffer, 254, "%.2f", kbps);
  size_t d_len2 = strlen(d_buffer);
  snprintf(d_buffer, 254, "%u", data->procid);
  size_t d_len3 = strlen(d_buffer);
  generate_chars(
      54 - (strlen(data->username) + strlen(data->host) + d_len3 + 1), 0x20,
      sp_buffer);
  generate_chars(10 - d_len1, 0x20, sp_buffer2);
  generate_chars(13 - d_len2, 0x20, sp_buffer3);
  return printf("| %s!%s/%u%s |        %us%s |     %.2fKB/s%s |  %s\n",
      data->username, data->host, data->procid, sp_buffer, (uint32_t) ltime,
      sp_buffer2, kbps, sp_buffer3, data->status);

}

#define STD_FMT_DATE_STR  	"%d %b %Y"

int
imdb_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 }, buffer3[255] =
    { 0 };

  __d_imdb data = (__d_imdb) iarg;

  time_t t_t = (time_t) data->timestamp, t_t2 = (time_t) data->released;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));
  strftime(buffer3, 255, STD_FMT_DATE_STR, localtime(&t_t2));

  return print_str(
      "IMDB: %s: %s (%hu): created: %s - iMDB ID: %s - rating: %.1f - votes: %u - genres: %s - released: %s - runtime: %u min - rated: %s - actors: %s - director: %s\n",
      data->dirname, data->title, data->year, buffer2, data->imdb_id,
      data->rating, data->votes, data->genres, buffer3, data->runtime,
      data->rated, data->actors, data->director);

}

int
imdb_format_block_batch(void *iarg, char *output)
{
  __d_imdb data = (__d_imdb) iarg;
  return printf(
      "IMDB\x9%s\x9%s\x9%d\x9%s\x9%.1f\x9%u\x9%s\x9%hu\x9%d\x9%u\x9%s\x9%s\x9%s\x9%s\n",
      data->dirname, data->title, data->timestamp, data->imdb_id,
      data->rating, data->votes, data->genres, data->year, data->released,
      data->runtime, data->rated, data->actors, data->director,
      data->synopsis);
}

int
imdb_format_block_exp(void *iarg, char *output)
{
  __d_imdb data = (__d_imdb) iarg;
  return printf(
      "dir %s\n"
      "title %s\n"
      "time %d\n"
      "imdbid %s\n"
      "score %.1f\n"
      "votes %u\n"
      "genre %s\n"
      "year %hu\n"
      "released %d\n"
      "runtime %u\n"
      "rated %s\n"
      "actors %s\n"
      "director %s\n"
      "plot %s\n\n",
      data->dirname, data->title, data->timestamp, data->imdb_id,
      data->rating, data->votes, data->genres, data->year, data->released,
      data->runtime, data->rated, data->actors, data->director,
      data->synopsis);
}

int
game_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  __d_game data = (__d_game) iarg;

  time_t t_t = (time_t) data->timestamp;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str("GAMELOG: %s: created: %s - rating: %.1f\n", data->dirname,
      buffer2, data->rating);

}

int
game_format_block_batch(void *iarg, char *output)
{
  __d_game data = (__d_game) iarg;
  return printf("GAMELOG\x9%s\x9%d\x9%.1f\n", data->dirname,
      (int32_t) data->timestamp, data->rating);

}

int
game_format_block_exp(void *iarg, char *output)
{
  __d_game data = (__d_game) iarg;
  return printf("dir %s\n"
      "time %d\n"
      "score %.1f\n\n", data->dirname,
      (int32_t) data->timestamp, data->rating);

}

int
tv_format_block(void *iarg, char *output)
{
  char buffer2[255] =
    { 0 };

  __d_tvrage data = (__d_tvrage) iarg;

  time_t t_t = (time_t) data->timestamp;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str(
      "TVRAGE: %s: created: %s - %s/%s [%u] - runtime: %u min - status: %s - country: %s - genres: %s\n",
      data->dirname, buffer2, data->name, data->class, data->showid,
      data->runtime, data->status, data->country, data->genres);

}

int
tv_format_block_batch(void *iarg, char *output)
{
  __d_tvrage data = (__d_tvrage) iarg;
  return printf("TVRAGE\x9%s\x9%d\x9%s\x9%s\x9%u\x9%s\x9%s\x9%s\x9%s\x9%u\x9%d\x9%d\x9%s\x9%s\x9%hu\x9%hu\x9%hu\x9%s\n",
      data->dirname, data->timestamp, data->name, data->class,
      data->showid, data->link, data->status, data->airday,
      data->airtime, data->runtime, data->started,
      data->ended, data->genres, data->country, data->seasons,
      data->startyear, data->endyear, data->network);

}

int
tv_format_block_exp(void *iarg, char *output)
{
  __d_tvrage data = (__d_tvrage) iarg;
  return printf("dir %s\n"
      "time %d\n"
      "name %s\n"
      "class %s\n"
      "showid %u\n"
      "link %s\n"
      "status %s\n"
      "airday %s\n"
      "airtime %s\n"
      "runtime %u\n"
      "started %d\n"
      "ended %d\n"
      "genre %s\n"
      "country %s\n"
      "seasons %hu\n"
      "startyear %hu\n"
      "endyear %hu\n"
      "network %s\n\n",
      data->dirname, data->timestamp, data->name, data->class,
      data->showid, data->link, data->status, data->airday,
      data->airtime, data->runtime, data->started,
      data->ended, data->genres, data->country, data->seasons,
      data->startyear, data->endyear, data->network);

}

int
gen1_format_block(void *iarg, char *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) iarg;

  return print_str("GENERIC1\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen1_format_block_batch(void *iarg, char *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) iarg;

  return printf("GENERIC1\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen1_format_block_exp(void *iarg, char *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) iarg;

  return printf("i32 %u\n"
      "ge1 %s\n"
      "ge2 %s\n"
      "ge3 %s\n"
      "ge4 %s\n"
      "ge5 %s\n"
      "ge6 %s\n"
      "ge7 %s\n"
      "ge8 %s\n\n",
      data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen2_format_block(void *iarg, char *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) iarg;

  return print_str("GENERIC2\x9%llu\x9%llu\x9%llu\x9%llu\x9%f\x9%f\x9%f\x9%f\x9%d\x9%d\x9%d\x9%d\x9%u\x9%u\x9%u\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      (ulint64_t)data->ui64_1,(ulint64_t)data->ui64_2,(ulint64_t)data->ui64_3,(ulint64_t)data->ui64_4, data->f_1,data->f_2,data->f_3,data->f_4,
      data->i32_1, data->i32_2, data->i32_3, data->i32_4,data->ui32_1,
      data->ui32_2, data->ui32_3, data->ui32_4, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen2_format_block_batch(void *iarg, char *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) iarg;

  return printf("GENERIC2\x9%llu\x9%llu\x9%llu\x9%llu\x9%f\x9%f\x9%f\x9%f\x9%d\x9%d\x9%d\x9%d\x9%u\x9%u\x9%u\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
      (ulint64_t)data->ui64_1,(ulint64_t)data->ui64_2,(ulint64_t)data->ui64_3,(ulint64_t)data->ui64_4, data->f_1,data->f_2,data->f_3,data->f_4,
      data->i32_1, data->i32_2, data->i32_3, data->i32_4,data->ui32_1,
      data->ui32_2, data->ui32_3, data->ui32_4, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen2_format_block_exp(void *iarg, char *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) iarg;

  return printf("ul1 %llu\n"
      "ul2 %llu\n"
      "ul3 %llu\n"
      "ul4 %llu\n"
      "f1 %f\n"
      "f2 %f\n"
      "f3 %f\n"
      "f4 %f\n"
      "i1 %d\n"
      "i2 %d\n"
      "i3 %d\n"
      "i4 %d\n"
      "u1 %u\n"
      "u2 %u\n"
      "u3 %u\n"
      "u4 %u\n"
      "ge1 %s\n"
      "ge2 %s\n"
      "ge3 %s\n"
      "ge4 %s\n"
      "ge5 %s\n"
      "ge6 %s\n"
      "ge7 %s\n"
      "ge8 %s\n\n",
      (ulint64_t)data->ui64_1,(ulint64_t)data->ui64_2,(ulint64_t)data->ui64_3,(ulint64_t)data->ui64_4, data->f_1,data->f_2,data->f_3,data->f_4,
      data->i32_1, data->i32_2, data->i32_3, data->i32_4,data->ui32_1,
      data->ui32_2, data->ui32_3, data->ui32_4, data->s_1, data->s_2, data->s_3, data->s_4,
      data->s_5, data->s_6, data->s_7, data->s_8);

}

int
gen3_format_block(void *iarg, char *output)
{
  __d_generic_s800 data = (__d_generic_s800) iarg;

  return print_str("GENERIC3\x9%u\x9%u\x9%s\x9%s\x9%d\x9%d\x9%llu\x9%llu\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->s_1, data->s_2, data->i32_1,
      data->i32_2, (ulint64_t)data->ui64_1, (ulint64_t)data->ui64_2, data->s_3, data->s_4);

}

int
gen3_format_block_batch(void *iarg, char *output)
{
  __d_generic_s800 data = (__d_generic_s800) iarg;

  return printf("GENERIC3\x9%u\x9%u\x9%s\x9%s\x9%d\x9%d\x9%llu\x9%llu\x9%s\x9%s\n",
      data->ui32_1, data->ui32_2, data->s_1, data->s_2, data->i32_1,
      data->i32_2, (ulint64_t)data->ui64_1, (ulint64_t)data->ui64_2, data->s_3, data->s_4);

}

int
gen3_format_block_exp(void *iarg, char *output)
{
  __d_generic_s800 data = (__d_generic_s800) iarg;

  return printf("u1 %u\n"
      "u2 %u\n"
      "ge1 %s\n"
      "ge2 %s\n"
      "i1 %d\n"
      "i2 %d\n"
      "ul1 %llu\n"
      "ul2 %llu\n"
      "ge3 %s\n"
      "ge4 %s\n\n"
      , data->ui32_1, data->ui32_2, data->s_1, data->s_2, data->i32_1,
      data->i32_2, (ulint64_t)data->ui64_1, (ulint64_t)data->ui64_2, data->s_3, data->s_4);

}

char *
generate_chars(size_t num, char chr, char*buffer)
{
  g_setjmp(0, "generate_chars", NULL, NULL);
  bzero(buffer, 255);
  if (num < 1 || num > 254)
    {
      return buffer;
    }
  memset(buffer, (int) chr, num);

  return buffer;
}

off_t
get_file_size(char *file)
{
  struct stat st;

  if (stat(file, &st) == -1)
    return 0;

  return st.st_size;
}

#ifdef HAVE_ST_BIRTHTIME
#define birthtime(x) x->st_birthtime
#else
#define birthtime(x) x->st_ctime
#endif

time_t
get_file_creation_time(struct stat *st)
{
  return (time_t) birthtime(st);
}

uint64_t
dirlog_find(char *dirn, int mode, uint32_t flags, void *callback)
{
  if (!(ofl & F_OVRR_NUKESTR))
    {
      return dirlog_find_old(dirn, mode, flags, callback);
    }

  int
  (*callback_f)(struct dirlog *) = callback;

  if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
    {
      return MAX_uint64_t;
    }

  struct dirlog buffer;

  int r;
  uint64_t ur = MAX_uint64_t;

  char buffer_s[PATH_MAX] =
    { 0 }, buffer_s2[PATH_MAX], buffer_s3[PATH_MAX];
  char *dup2, *base, *dir;

  if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
    {
      snprintf(buffer_s, PATH_MAX, "%s", dirn);
    }

  size_t d_l = strlen(buffer_s);

  struct dirlog *d_ptr = NULL;

  while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ)))
    {
      if (!strncmp(buffer_s, d_ptr->dirname, d_l))
        {
          goto match;
        }
      base = g_basename(d_ptr->dirname);
      dup2 = strdup(d_ptr->dirname);
      dir = dirname(dup2);
      snprintf(buffer_s2, PATH_MAX, NUKESTR, base);
      snprintf(buffer_s3, PATH_MAX, "%s/%s", dir, buffer_s2);
      remove_repeating_chars(buffer_s3, 0x2F);

      free(dup2);
      if (!strncmp(buffer_s3, buffer_s, d_l))
        {
          match: ur = g_act_1.offset - 1;
          if (mode == 2 && callback)
            {
              if (callback_f(&buffer))
                {
                  break;
                }

            }
          else
            {
              break;
            }
        }
    }

  if (mode != 1)
    {
      g_close(&g_act_1);
    }

  return ur;
}

uint64_t
dirlog_find_old(char *dirn, int mode, uint32_t flags, void *callback)
{
  int
  (*callback_f)(struct dirlog *data) = callback;

  if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
    {
      return MAX_uint64_t;
    }

  struct dirlog buffer;

  int r;
  uint64_t ur = MAX_uint64_t;

  char buffer_s[PATH_MAX] =
    { 0 };
  char *dup2, *base, *dir;
  int gi1, gi2;

  if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
    {
      strncpy(buffer_s, dirn, strlen(dirn));
    }

  gi2 = strlen(buffer_s);

  struct dirlog *d_ptr = NULL;

  while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ)))
    {

      base = g_basename(d_ptr->dirname);
      gi1 = strlen(base);
      dup2 = strdup(d_ptr->dirname);
      dir = dirname(dup2);
      if (!strncmp(&buffer_s[gi2 - gi1], base, gi1)
          && !strncmp(buffer_s, d_ptr->dirname, strlen(dir)))
        {

          ur = g_act_1.offset - 1;
          if (mode == 2 && callback)
            {
              if (callback_f(&buffer))
                {

                  free(dup2);
                  break;
                }

            }
          else
            {

              free(dup2);
              break;
            }
        }

      free(dup2);
    }

  if (mode != 1)
    {
      g_close(&g_act_1);
    }

  return ur;
}

size_t
str_match(char *input, char *match)
{
  size_t i_l = strlen(input), m_l = strlen(match);

  size_t i;

  for (i = 0; i < i_l - m_l + 1; i++)
    {
      if (!strncmp(&input[i], match, m_l))
        {
          return i;
        }
    }

  return -1;
}

char *
string_replace(char *input, char *match, char *with, char *output,
    size_t max_out)
{
  size_t i_l = strlen(input), w_l = strlen(with), m_l = strlen(match);

  size_t m_off = str_match(input, match);

  if ((int) m_off < 0)
    {
      return output;
    }

  bzero(output, max_out);

  strncpy(output, input, m_off);
  strncpy(&output[m_off], with, w_l);
  strncpy(&output[m_off + w_l], &input[m_off + m_l], i_l - m_off - m_l);

  return output;
}

uint64_t
nukelog_find(char *dirn, int mode, struct nukelog *output)
{
  struct nukelog buffer =
    { 0 };

  uint64_t r = MAX_uint64_t;
  char *dup2, *base, *dir;

  if (g_fopen(NUKELOG, "r", F_DL_FOPEN_BUFFER, &g_act_2))
    {
      goto r_end;
    }

  int gi1, gi2;
  gi2 = strlen(dirn);

  struct nukelog *n_ptr = NULL;

  while ((n_ptr = (struct nukelog *) g_read(&buffer, &g_act_2, NL_SZ)))
    {

      base = g_basename(n_ptr->dirname);
      gi1 = strlen(base);
      dup2 = strdup(n_ptr->dirname);
      dir = dirname(dup2);

      if (gi1 >= gi2 || gi1 < 2)
        {
          goto l_end;
        }

      if (!strncmp(&dirn[gi2 - gi1], base, gi1)
          && !strncmp(dirn, n_ptr->dirname, strlen(dir)))
        {
          if (output)
            {
              memcpy(output, n_ptr, NL_SZ);
            }
          r = g_act_2.offset - 1;
          if (mode != 2)
            {

              free(dup2);
              break;
            }
        }
      l_end:

      free(dup2);
    }

  if (mode != 1)
    {
      g_close(&g_act_2);
    }

  r_end:

  return r;
}

#define MSG_REDF_ABORT "WARNING: %s: aborting rebuild (will not be writing what was done up to here)\n"

int
rebuild_data_file(char *file, __g_handle hdl)
{
  g_setjmp(0, "rebuild_data_file", NULL, NULL);
  int ret = 0, r;
  off_t sz_r;
  struct stat st;
  char buffer[PATH_MAX] =
    { 0 };

  if (strlen(file) + 4 > PATH_MAX)
    {
      return 1;
    }

  snprintf(hdl->s_buffer, PATH_MAX - 1, "%s.%d.dtm", file, getpid());
  snprintf(buffer, PATH_MAX - 1, "%s.bk", file);

  pmda p_ptr;

  if (hdl->flags & F_GH_FFBUFFER)
    {
      p_ptr = &hdl->w_buffer;

    }
  else
    {
      p_ptr = &hdl->buffer;
    }

  if (updmode != UPD_MODE_RECURSIVE)
    {
      g_setjmp(0, "rebuild_data_file(2)", NULL, NULL);
      if ((r = g_filter(hdl, p_ptr)))
        {
          if (r == 1)
            {
              if (!(gfl & F_OPT_FORCE))
                {
                  print_str(
                      "WARNING: %s: everything got filtered, refusing to write 0-byte data file\n",
                      file);
                  return 11;
                }
            }
          else if (r == 2)
            {
              print_str(
                  "ERROR: %s: failed unlinking record entry after match!\n",
                  file);
              return 12;
            }
        }
      if ((gfl & F_OPT_NOFQ) && (hdl->exec_args.exc || (gfl & F_OPT_HASMATCH))
          && !(hdl->flags & F_GH_APFILT))
        {
          return 0;
        }
    }

  if (do_sort(&g_act_1, g_sort_field, g_sort_flags))
    {
      ret = 11;
      goto end;
    }

  g_setjmp(0, "rebuild_data_file(3)", NULL, NULL);

  if ((gfl & F_OPT_KILL_GLOBAL) && !(gfl & F_OPT_FORCE))
    {
      print_str(MSG_REDF_ABORT, file);
      return 0;
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: %s: flushing data to disk..\n", hdl->s_buffer);
    }

  if (!lstat(file, &st))
    {
      hdl->st_mode = st.st_mode;
    }
  else
    {
      hdl->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("NOTICE: %s: using mode %o\n", hdl->s_buffer, hdl->st_mode);
    }

  off_t rw_o = hdl->rw, bw_o = hdl->bw;

  if ((r = flush_data_md(hdl, hdl->s_buffer)))
    {
      if (r == 1)
        {
          if (gfl & F_OPT_VERBOSE)
            {
              print_str("WARNING: %s: empty buffer (nothing to flush)\n",
                  hdl->s_buffer);
            }
          ret = 0;
        }
      else
        {
          print_str("ERROR: %s: [%d] flushing data failed!\n", hdl->s_buffer,
              r);
          ret = 2;
        }
      goto end;
    }

  g_setjmp(0, "rebuild_data_file(4)", NULL, NULL);

  if ((gfl & F_OPT_KILL_GLOBAL) && !(gfl & F_OPT_FORCE))
    {
      print_str( MSG_REDF_ABORT, file);
      if (!(gfl & F_OPT_NOWRITE))
        {
          remove(hdl->s_buffer);
        }
      return 0;
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("NOTICE: %s: flushed %llu records, %llu bytes\n", hdl->s_buffer,
          (ulint64_t) (hdl->rw - rw_o), (ulint64_t) (hdl->bw - bw_o));
    }

  g_setjmp(0, "rebuild_data_file(5)", NULL, NULL);

  if (!(gfl & F_OPT_FORCE2) && !(gfl & F_OPT_NOWRITE)
      && (sz_r = get_file_size(hdl->s_buffer)) < hdl->block_sz)
    {
      print_str(
          "ERROR: %s: [%u/%u] generated data file is smaller than a single record!\n",
          hdl->s_buffer, (uint32_t) sz_r, (uint32_t) hdl->block_sz);
      ret = 7;
      if (!(gfl & F_OPT_NOWRITE))
        {
          remove(hdl->s_buffer);
        }
      goto cleanup;
    }

  g_setjmp(0, "rebuild_data_file(6)", NULL, NULL);

  if (!(gfl & F_OPT_NOWRITE)
      && !((hdl->flags & F_GH_WAPPEND) && (hdl->flags & F_GH_DFWASWIPED)))
    {
      if (data_backup_records(file))
        {
          ret = 3;
          if (!(gfl & F_OPT_NOWRITE))
            {
              remove(hdl->s_buffer);
            }
          goto cleanup;
        }
    }

  g_setjmp(0, "rebuild_data_file(7)", NULL, NULL);

  if (!(gfl & F_OPT_NOWRITE))
    {

      if (hdl->fh)
        {
          fclose(hdl->fh);
          hdl->fh = NULL;
        }

      if (!file_exists(file) && !(hdl->flags & F_GH_DFNOWIPE)
          && (hdl->flags & F_GH_WAPPEND) && !(hdl->flags & F_GH_DFWASWIPED))
        {
          if (remove(file))
            {
              print_str("ERROR: %s: could not clean old data file\n", file);
              ret = 9;
              goto cleanup;
            }
          hdl->flags |= F_GH_DFWASWIPED;
        }

      g_setjmp(0, "rebuild_data_file(8)", NULL, NULL);

      if (!strncmp(hdl->mode, "a", 1) || (hdl->flags & F_GH_WAPPEND))
        {
          if ((r = (int) file_copy(hdl->s_buffer, file, "ab",
          F_FC_MSET_SRC)) < 1)
            {
              print_str("ERROR: %s: [%d] merging temp file failed!\n",
                  hdl->s_buffer, r);
              ret = 4;
            }

        }
      else
        {
          if ((r = rename(hdl->s_buffer, file)))
            {
              print_str("ERROR: %s: [%d] renaming temporary file failed!\n",
                  hdl->s_buffer,
                  errno);
              ret = 4;
            }
          goto end;
        }

      cleanup:

      if ((r = remove(hdl->s_buffer)))
        {
          print_str(
              "WARNING: %s: [%d] deleting temporary file failed (remove manually)\n",
              hdl->s_buffer,
              errno);
          ret = 5;
        }

    }
  end:

  return ret;
}

#define MAX_WBUFFER_HOLD	100000

int
g_load_record(__g_handle hdl, const void *data)
{
  g_setjmp(0, "g_load_record", NULL, NULL);
  void *buffer = NULL;

  if (hdl->w_buffer.offset == MAX_WBUFFER_HOLD)
    {
      hdl->w_buffer.flags |= F_MDA_FREE;
      rebuild_data_file(hdl->file, hdl);
      p_md_obj ptr = hdl->w_buffer.objects, ptr_s;
      if (gfl & F_OPT_VERBOSE3)
        {
          print_str("NOTICE: scrubbing write cache..\n");
        }
      while (ptr)
        {
          ptr_s = ptr->next;
          free(ptr->ptr);
          bzero(ptr, sizeof(md_obj));
          ptr = ptr_s;
        }
      hdl->w_buffer.pos = hdl->w_buffer.objects;
      hdl->w_buffer.offset = 0;
    }

  buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);

  if (!buffer)
    {
      return 2;
    }

  memcpy(buffer, data, hdl->block_sz);

  return 0;
}

int
flush_data_md(__g_handle hdl, char *outfile)
{
  g_setjmp(0, "flush_data_md", NULL, NULL);

  if (gfl & F_OPT_NOWRITE)
    {
      return 0;
    }

  FILE *fh = NULL;
  size_t bw = 0;
  unsigned char *buffer = NULL;
  char *mode = "w";

  int ret = 0;

  if (!(gfl & F_OPT_FORCE))
    {
      if (hdl->flags & F_GH_FFBUFFER)
        {
          if (!hdl->w_buffer.offset)
            {
              return 1;
            }
        }
      else
        {
          if (!hdl->buffer.count)
            {
              return 1;
            }
        }
    }

  if ((fh = fopen(outfile, mode)) == NULL)
    {
      return 2;
    }

  size_t v = (V_MB * 8) / hdl->block_sz;

  buffer = calloc(v, hdl->block_sz);

  p_md_obj ptr;

  if (hdl->flags & F_GH_FFBUFFER)
    {
      ptr = md_first(&hdl->w_buffer);
    }
  else
    {
      ptr = md_first(&hdl->buffer);
    }

  g_setjmp(0, "flush_data_md(loop)", NULL, NULL);

  while (ptr)
    {
      if ((bw = fwrite(ptr->ptr, hdl->block_sz, 1, fh)) != 1)
        {
          ret = 3;
          break;
        }
      hdl->bw += hdl->block_sz;
      hdl->rw++;
      ptr = ptr->next;
    }

  if (!hdl->bw && !(gfl & F_OPT_FORCE))
    {
      ret = 5;
    }

  g_setjmp(0, "flush_data_md(2)", NULL, NULL);

  free(buffer);
  fclose(fh);

  if (hdl->st_mode)
    {
      chmod(outfile, hdl->st_mode);
    }

  return ret;
}

size_t
g_load_data_md(void *output, size_t max, char *file, __g_handle hdl)
{
  g_setjmp(0, "g_load_data_md", NULL, NULL);
  size_t fr = 0;
  off_t c_fr = 0;
  FILE *fh;

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      if (!(fh = fopen(file, "rb")))
        {
          return 0;
        }
    }
  else
    {
      fh = stdin;
    }

  uint8_t *b_output = (uint8_t*) hdl->data;
  while (!feof(fh) && !ferror(fh))
    {
      if ((hdl->flags & F_GH_FROMSTDIN))
        {
          if (!fr && !(hdl->total_sz - c_fr))
            {
//              printf("Data still waiting..\n");
              hdl->total_sz *= 2;
              hdl->data = realloc(hdl->data, hdl->total_sz);
              b_output = hdl->data;
            }
        }
      else
        {
          if (!(hdl->total_sz - c_fr))
            {
              break;
            }
        }
      fr = fread(&b_output[c_fr], 1, hdl->total_sz - c_fr, fh);
      if (fr > 0)
        {
          c_fr += fr;
        }

    }

  if (ferror(fh))
    {
      //c_fr = 0;
    }

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      fclose(fh);
    }

  return c_fr;
}

int
gen_md_data_ref(__g_handle hdl, pmda md, off_t count)
{

  unsigned char *w_ptr = (unsigned char*) hdl->data;

  off_t i;

  for (i = 0; i < count; i++)
    {
      md->lref_ptr = (void*) w_ptr;
      w_ptr += hdl->block_sz;
      if (!md_alloc(md, hdl->block_sz))
        {
          md_g_free(md);
          return -5;
        }

      //hdl->buffer_count++;
    }

  return 0;
}

int
is_memregion_null(void *addr, size_t size)
{
  size_t i = size - 1;
  unsigned char *ptr = (unsigned char*) addr;
  while (!ptr[i] && i)
    {
      i--;
    }
  return i;
}

int
gen_md_data_ref_cnull(__g_handle hdl, pmda md, off_t count)
{

  unsigned char *w_ptr = (unsigned char*) hdl->data;

  off_t i;

  for (i = 0; i < count; i++)
    {
      if (is_memregion_null((void*) w_ptr, hdl->block_sz))
        {
          md->lref_ptr = (void*) w_ptr;
          if (!md_alloc(md, hdl->block_sz))
            {
              md_g_free(md);
              return -5;
            }
        }
      w_ptr += hdl->block_sz;
    }

  return 0;
}

typedef int
(*__g_mdref)(__g_handle hdl, pmda md, off_t count);

int
load_data_md(pmda md, char *file, __g_handle hdl)
{
  g_setjmp(0, "load_data_md", NULL, NULL);
  errno = 0;
  int r = 0;
  off_t count = 0;

  if (!hdl->block_sz)
    {
      return -2;
    }

  uint32_t sh_ret = 0;

  if (hdl->flags & F_GH_ONSHM)
    {
      if ((r = g_shmap_data(hdl, SHM_IPC)))
        {
          md_g_free(md);
          return r;
        }
      count = hdl->total_sz / hdl->block_sz;
    }
  else if (hdl->flags & F_GH_SHM)
    {
      if (hdl->shmid != -1)
        {
          sh_ret |= R_SHMAP_ALREADY_EXISTS;
        }
      hdl->data = shmap(hdl->ipc_key, &hdl->ipcbuf, (size_t) hdl->total_sz,
          &sh_ret, &hdl->shmid);

      if (sh_ret & R_SHMAP_FAILED_ATTACH)
        {
          return -22;
        }
      if (sh_ret & R_SHMAP_FAILED_SHMAT)
        {
          return -23;
        }
      if (!hdl->data)
        {
          return -12;
        }

      if (sh_ret & R_SHMAP_ALREADY_EXISTS)
        {
          errno = 0;
          if (hdl->flags & F_GH_SHMRB)
            {
              bzero(hdl->data, hdl->ipcbuf.shm_segsz);
            }

          hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;
        }
      count = hdl->total_sz / hdl->block_sz;
    }
  else if (hdl->flags & F_GH_FROMSTDIN)
    {
      count = 32;
      hdl->total_sz = count * hdl->block_sz;
      hdl->data = malloc(count * hdl->block_sz);
    }
  else
    {
      if (!hdl->total_sz)
        {
          return -3;
        }
      count = hdl->total_sz / hdl->block_sz;
      hdl->data = malloc(count * hdl->block_sz);
    }

  size_t b_read = 0;

  //hdl->buffer_count = 0;

  __g_mdref cb = NULL;

  if ((hdl->flags & F_GH_ONSHM))
    {
      cb = (__g_mdref ) gen_md_data_ref_cnull;
    }
  else
    {
      if (!((sh_ret & R_SHMAP_ALREADY_EXISTS)) || ((hdl->flags & F_GH_SHMRB)))
        {
          if ((b_read = g_load_data_md(hdl->data, hdl->total_sz, file, hdl))
              % hdl->block_sz || !b_read)
            {

              md_g_free(md);
              return -9;
            }
          if (b_read != hdl->total_sz)
            {
              hdl->total_sz = b_read;
              count = hdl->total_sz / hdl->block_sz;
            }
        }
      cb = (__g_mdref ) gen_md_data_ref;
    }

  if (md_init(md, count))
    {
      return -4;
    }

  md->flags |= F_MDA_REFPTR;
  cb(hdl, md, count);

  g_setjmp(0, "load_data_md", NULL, NULL);

  if (!md->count)
    {
      return -5;
    }

  return 0;
}

#define MSG_DEF_SHM "SHARED MEMORY"

int
g_map_shm(__g_handle hdl, key_t ipc)
{
  hdl->flags |= F_GH_ONSHM;

  if (hdl->buffer.count)
    {
      return 0;
    }

  if (!SHM_IPC)
    {
      print_str(
          "ERROR: %s: could not get IPC key, set manually (--ipc <key>)\n",
          MSG_DEF_SHM);
      return 1;
    }

  int r;

  if ((r = load_data_md(&hdl->buffer, NULL, hdl)))
    {
      if (((gfl & F_OPT_VERBOSE) && r != 1002) || (gfl & F_OPT_VERBOSE4))
        {
          print_str(
              "ERROR: %s: [%u/%u] [%u] [%u] could not map shared memory segment! [%d] [%d]\n",
              MSG_DEF_SHM, (uint32_t) hdl->buffer.count,
              (uint32_t) (hdl->total_sz / hdl->block_sz),
              (uint32_t) hdl->total_sz, hdl->block_sz, r, errno);
        }
      return 9;
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      print_str("NOTICE: %s: mapped %u records\n",
      MSG_DEF_SHM, (uint32_t) hdl->buffer.count);
    }

  hdl->flags |= F_GH_ISONLINE;
  hdl->d_memb = 3;

  hdl->g_proc1_lookup = ref_to_val_lk_online;
  hdl->g_proc2 = ref_to_val_ptr_online;
  hdl->g_proc3 = online_format_block;
  hdl->g_proc3_batch = online_format_block_batch;
  hdl->g_proc4 = g_omfp_norm;
  hdl->jm_offset = (size_t) &((struct ONLINE*) NULL)->username;

  return 0;
}

#define F_GBM_SHM_NO_DATAFILE		(a32 << 1)

int
g_buffer_into_memory(char *file, __g_handle hdl)
{
  g_setjmp(0, "g_buffer_into_memory", NULL, NULL);

  uint32_t flags = 0;

  if (hdl->buffer.count)
    {
      return 0;
    }

  if (gfl0 & F_OPT_STDIN)
    {
      hdl->flags |= F_GH_FROMSTDIN;
    }

  struct stat st =
    { 0 };

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      if (stat(file, &st) == -1)
        {
          if (!(gfl & F_OPT_SHAREDMEM))
            {
              print_str(
                  "ERROR: %s: [%d] unable to get information from data file\n",
                  file,
                  errno);
              return 2;
            }
          else
            {
              flags |= F_GBM_SHM_NO_DATAFILE;
            }
        }

      if (!(flags & F_GBM_SHM_NO_DATAFILE))
        {
          if (!st.st_size)
            {
              print_str("WARNING: %s: 0-byte data file\n", file);
              return 3;
            }

          if (st.st_size > db_max_size)
            {
              print_str(
                  "WARNING: %s: disabling memory buffering, file too big (%lld MB max)\n",
                  file, db_max_size / 1024 / 1024);
              hdl->flags |= F_GH_NOMEM;
              return 5;
            }

          hdl->total_sz = st.st_size;
        }
    }

  strncpy(hdl->file, file, strlen(file) + 1);

  if (determine_datatype(hdl))
    {
      print_str(MSG_BAD_DATATYPE, file);
      return 6;
    }

  if (gfl & F_OPT_SHMRELOAD)
    {
      hdl->flags |= F_GH_SHMRB;
    }

  if (gfl & F_OPT_SHMDESTROY)
    {
      hdl->flags |= F_GH_SHMDESTROY;
    }

  if (gfl & F_OPT_SHMDESTONEXIT)
    {
      hdl->flags |= F_GH_SHMDESTONEXIT;
    }

  off_t tot_sz = 0;

  if (!(flags & F_GBM_SHM_NO_DATAFILE))
    {
      if (st.st_size % hdl->block_sz)
        {
          print_str(MSG_GEN_DFCORRU, file, (ulint64_t) st.st_size,
              hdl->block_sz);
          return 12;
        }

      tot_sz = hdl->total_sz;

    }
  else
    {
      strncpy(hdl->file, "SHM", 4);
    }

  if (gfl0 & F_OPT_STDIN)
    {
      strncpy(hdl->file, "stdin", 7);
    }

  if (gfl & F_OPT_SHAREDMEM)
    {
      hdl->shmid = -1;
      hdl->flags |= F_GH_SHM;
      if ((hdl->shmid = shmget(hdl->ipc_key, 0, 0)) != -1)
        {
          if (gfl & F_OPT_VERBOSE2)
            {
              print_str(
                  "NOTICE: %s: [IPC: 0x%.8X]: [%d]: attached to existing shared memory segment\n",
                  hdl->file, (uint32_t) hdl->ipc_key, hdl->shmid);
            }
          if (shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf) != -1)
            {
              if (flags & F_GBM_SHM_NO_DATAFILE)
                {
                  hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;
                }
              if ((off_t) hdl->ipcbuf.shm_segsz != hdl->total_sz
                  || ((hdl->flags & F_GH_SHMDESTROY)
                      && !(flags & F_GBM_SHM_NO_DATAFILE)))
                {
                  if (gfl & F_OPT_VERBOSE2)
                    {
                      print_str(
                          "NOTICE: %s: [IPC: 0x%.8X]: destroying existing shared memory segment [%zd]%s\n",
                          hdl->file, hdl->ipc_key, hdl->ipcbuf.shm_segsz,
                          ((off_t) hdl->ipcbuf.shm_segsz != hdl->total_sz) ?
                              ": segment and data file sizes differ" : "");
                    }

                  if (shmctl(hdl->shmid, IPC_RMID, NULL) == -1)
                    {
                      print_str(
                          "WARNING: %s: [IPC: 0x%.8X] [%d] unable to destroy shared memory segment\n",
                          hdl->file, hdl->ipc_key, errno);
                    }
                  else
                    {
                      if (gfl & F_OPT_VERBOSE2)
                        {
                          print_str(
                              "NOTICE: %s: [IPC: 0x%.8X]: [%d]: marked segment to be destroyed\n",
                              hdl->file, hdl->ipc_key, hdl->shmid);
                        }
                      hdl->shmid = -1;
                    }
                }
            }
          else
            {
              print_str(
                  "ERROR: %s: [IPC: 0x%.8X]: [%d]: could not get shared memory segment information from kernel\n",
                  hdl->file, hdl->ipc_key, errno);
              return 21;
            }
          if ((gfl & F_OPT_VERBOSE2) && hdl->shmid != -1
              && (hdl->flags & F_GH_SHMRB))
            {
              print_str(
                  "NOTICE: %s: [IPC: 0x%.8X]: [%d]: segment data will be reloaded from file\n",
                  hdl->file, hdl->ipc_key, hdl->shmid);
            }

        }
      else if ((flags & F_GBM_SHM_NO_DATAFILE))
        {
          print_str(
              "ERROR: %s: [IPC: 0x%.8X]: failed loading data into shared memory segment: [%d]: no shared memory segment or data file available to load\n",
              hdl->file, hdl->ipc_key, errno);
          return 22;
        }
    }

  if (gfl & F_OPT_VERBOSE2)
    {
      if (gfl0 & F_OPT_STDIN)
        {
          print_str("NOTICE: loading from stdin..\n");
        }
      else
        {
          print_str("NOTICE: %s: %s [%llu records] [%llu bytes]\n", hdl->file,
              (hdl->flags & F_GH_SHM) ?
                  hdl->shmid == -1 && !(hdl->flags & F_GH_SHMRB) ?
                      "loading data into shared memory segment" :
                  (hdl->flags & F_GH_SHMRB) ?
                      "re-loading data into shared memory segment" :
                      "mapping shared memory segment"
                  :
                  "loading data file into memory",
              (hdl->flags & F_GH_SHM) && hdl->shmid != -1 ?
                  (uint64_t) (hdl->ipcbuf.shm_segsz / hdl->block_sz) :
                  (uint64_t) (hdl->total_sz / hdl->block_sz),
              (hdl->flags & F_GH_SHM) && hdl->shmid != -1 ?
                  (ulint64_t) hdl->ipcbuf.shm_segsz :
                  (ulint64_t) hdl->total_sz);
        }
    }

  errno = 0;
  int r;
  if ((r = load_data_md(&hdl->buffer, hdl->file, hdl)))
    {
      print_str(
          "ERROR: %s: [%llu/%llu] [%llu] [%u] could not load data!%s [%d] [%d]\n",
          hdl->file, (ulint64_t) hdl->buffer.count,
          (ulint64_t) (hdl->total_sz / hdl->block_sz), hdl->total_sz,
          hdl->block_sz,
          (hdl->flags & F_GH_SHM) ? " [shared memory segment]" : "", r,
          errno);
      return 4;
    }
  else
    {
      if (!(flags & F_GBM_SHM_NO_DATAFILE) && !(hdl->flags & F_GH_FROMSTDIN))
        {
          if (tot_sz != hdl->total_sz)
            {
              print_str(
                  "WARNING: %s: [%llu/%llu] actual data loaded was not the same size as source data file\n",
                  hdl->file, (uint64_t) hdl->total_sz, (uint64_t) tot_sz);
            }
        }
      if (gfl & F_OPT_VERBOSE2)
        {
          print_str("NOTICE: %s: loaded %llu records\n", hdl->file,
              (uint64_t) hdl->buffer.count);
        }
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
  else
    {
      return 1;
    }

  return 0;
}

int
g_fopen(char *file, char *mode, uint32_t flags, __g_handle hdl)
{
  g_setjmp(0, "g_fopen", NULL, NULL);

  if (flags & F_DL_FOPEN_SHM)
    {
      if (g_map_shm(hdl, SHM_IPC))
        {
          return 12;
        }
      if (g_proc_mr(hdl))
        {
          return 14;
        }
      return 0;
    }

  if (flags & F_DL_FOPEN_REWIND)
    {
      gh_rewind(hdl);
    }

  if (!(gfl & F_OPT_NOBUFFER) && (flags & F_DL_FOPEN_BUFFER)
      && !(hdl->flags & F_GH_NOMEM))
    {
      if (!g_buffer_into_memory(file, hdl))
        {
          if (g_proc_mr(hdl))
            {
              return 24;
            }

          if (!(flags & F_DL_FOPEN_FILE))
            {
              return 0;
            }
        }
      else
        {
          if (!(hdl->flags & F_GH_NOMEM))
            {
              return 11;
            }
        }
    }

  if (hdl->fh)
    {
      return 0;
    }

  if (strlen(file) > PATH_MAX)
    {
      print_str(MSG_GEN_NODFILE, file, "file path too large");
      return 2;
    }

  strncpy(hdl->file, file, strlen(file) + 1);

  if (determine_datatype(hdl))
    {
      print_str(MSG_GEN_NODFILE, file, "could not determine data-type");
      return 3;
    }

  FILE *fd;

  if (!(gfl0 & F_OPT_STDIN))
    {
      hdl->total_sz = get_file_size(file);

      if (!hdl->total_sz)
        {
          print_str(MSG_GEN_NODFILE, file, "zero-byte data file");
        }

      if (hdl->total_sz % hdl->block_sz)
        {
          print_str(MSG_GEN_DFCORRU, file, (ulint64_t) hdl->total_sz,
              hdl->block_sz);
          return 4;
        }

      if (!(fd = fopen(file, mode)))
        {
          print_str(MSG_GEN_NODFILE, file, "not available");
          return 1;
        }
    }
  else
    {
      fd = stdin;
      strncpy(hdl->file, "stdin", 7);
    }

  if (g_proc_mr(hdl))
    {
      return 32;
    }

  hdl->fh = fd;

  return 0;
}

int
g_close(__g_handle hdl)
{
  g_setjmp(0, "g_close", NULL, NULL);
  bzero(&dl_stats, sizeof(struct d_stats));
  dl_stats.br += hdl->br;
  dl_stats.bw += hdl->bw;
  dl_stats.rw += hdl->rw;

  if (hdl->fh && !(hdl->flags & F_GH_FROMSTDIN))
    {
      fclose(hdl->fh);
      hdl->fh = NULL;
    }

  if (hdl->buffer.count)
    {
      hdl->buffer.r_pos = hdl->buffer.objects;
      hdl->buffer.pos = hdl->buffer.r_pos;
      hdl->buffer.offset = 0;
      hdl->offset = 0;
      if ((hdl->flags & F_GH_ONSHM))
        {
          md_g_free(&hdl->buffer);
          md_g_free(&hdl->w_buffer);
        }
    }

  hdl->br = 0;
  hdl->bw = 0;
  hdl->rw = 0;

  return 0;
}

static int
prep_for_exec(void)
{
  const char inputfile[] = "/dev/null";

  if (close(0) < 0)
    {
      fprintf(stdout, "ERROR: could not close stdin\n");
      return 1;
    }
  else
    {
      if (open(inputfile, O_RDONLY
#if defined O_LARGEFILE
          | O_LARGEFILE
#endif
          ) < 0)
        {
          fprintf(stdout, "ERROR: could not open %s\n", inputfile);
        }
    }

  if (execv_stdout_redir != -1)
    {
      dup2(execv_stdout_redir, STDOUT_FILENO);
    }

  return 0;
}

int
l_execv(char *exec, char **argv)
{
  pid_t c_pid;

  fflush(stdout);
  fflush(stderr);

  c_pid = fork();

  if (c_pid == (pid_t) -1)
    {
      fprintf(stderr, "ERROR: %s: fork failed\n", exec);
      return 1;
    }

  if (!c_pid)
    {
      if (prep_for_exec())
        {
          _exit(1);
        }
      else
        {
          execv(exec, argv);
          fprintf(stderr, "ERROR: %s: execv failed to execute [%d]\n", exec,
          errno);
          _exit(1);
        }
    }
  int status;
  while (waitpid(c_pid, &status, 0) == (pid_t) -1)
    {
      if (errno != EINTR)
        {
          fprintf(stderr,
              "ERROR: %s:failed waiting for child process to finish [%d]\n",
              exec, errno);
          return 2;
        }
    }

  return status;
}

int
g_do_exec_v(void *buffer, void *callback, char *ex_str, void * p_hdl)
{
  __g_handle hdl = (__g_handle) p_hdl;
  process_execv_args(buffer, hdl);
  return l_execv(hdl->exec_args.exec_v_path, hdl->exec_args.argv_c);
}

int
g_do_exec(void *buffer, void *callback, char *ex_str, void *hdl)
{
  if (callback)
    {
      char *e_str;
      if (ex_str)
        {
          e_str = ex_str;
        }
      else
        {
          if (!exec_str)
            {
              return -1;
            }
          e_str = exec_str;
        }

      if (process_exec_string(e_str, b_glob, MAX_EXEC_STR, callback, buffer))
        {
          bzero(b_glob, MAX_EXEC_STR + 1);
          return -2;
        }

      return system(b_glob);
    }
  else if (ex_str)
    {
      if (strlen(ex_str) > MAX_EXEC_STR)
        {
          return -1;
        }
      return system(ex_str);
    }
  else
    {
      return -1;
    }
}

int
g_do_print(void *buffer, void *callback, char *ex_str, void *hdl)
{
  char *ptr;
  if (!(ptr = g_exech_build_string(buffer, &((__g_handle ) hdl)->exec_args.mech,
      (__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      b_glob[0] = 0x0;
      return -2;
    }

  return system(b_glob);
}

int
g_do_exec_fb(void *buffer, void *callback, char *ex_str, void *hdl)
{
  char *ptr;
  if (!(ptr = g_exech_build_string(buffer, &((__g_handle ) hdl)->exec_args.mech,
      (__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      b_glob[0] = 0x0;
      return -2;
    }

  return system(b_glob);
}

int
process_execv_args(void *data, __g_handle hdl)
{
  g_setjmp(0, "process_execv_args", NULL, NULL);

  p_md_obj ptr = md_first(&hdl->exec_args.ac_ref);

  __d_argv_ch ach;
  char *s_ptr;
  while (ptr)
    {

      ach = (__d_argv_ch) ptr->ptr;

      if (!(s_ptr = g_exech_build_string(data, &ach->mech,
                  hdl, hdl->exec_args.argv_c[ach->cindex], 8191)))
        {

          hdl->exec_args.argv_c[ach->cindex][0] = 0x0;
        }

      /*if (process_exec_string(hdl->exec_args.argv[ach->cindex],
       hdl->exec_args.argv_c[ach->cindex], 8191, (void*) hdl->g_proc1, data))
       {
       }*/

      ptr = ptr->next;
    }

  return 0;
}

void *
g_read(void *buffer, __g_handle hdl, size_t size)
{
  if (hdl->buffer.count)
    {
      hdl->buffer.pos = hdl->buffer.r_pos;
      if (!hdl->buffer.pos)
        {
          return NULL;
        }
      hdl->buffer.r_pos = *((void**) hdl->buffer.r_pos + hdl->j_offset);
      hdl->buffer.offset++;
      hdl->offset++;
      hdl->br += hdl->block_sz;
      return (void *) hdl->buffer.pos->ptr;
    }

  if (!buffer)
    {
      print_str("IO ERROR: %s: no buffer to write to\n", hdl->file);
      return NULL;
    }

  if (!hdl->fh)
    {
      print_str("IO ERROR: %s: data file handle not open\n", hdl->file);
      return NULL;
    }

  if (feof(hdl->fh))
    {
      return NULL;
    }

  size_t fr;

  if ((fr = fread(buffer, 1, size, hdl->fh)) != size)
    {
      if (fr == 0)
        {
          return NULL;
        }
      return NULL;
    }

  hdl->br += fr;
  hdl->offset++;

  return buffer;
}

size_t
read_from_pipe(char *buffer, FILE *pipe)
{
  size_t read = 0, r;

  while (!feof(pipe))
    {
      if ((r = fread(&buffer[read], 1, PIPE_READ_MAX - read, pipe)) <= 0)
        {
          break;
        }
      read += r;
    }

  return read;
}

size_t
exec_and_wait_for_output(char *command, char *output)
{
  char buf[PIPE_READ_MAX] =
    { 0 };
  size_t r = 0;
  FILE *pipe = NULL;

  if (!(pipe = popen(command, "r")))
    {
      return 0;
    }

  r = read_from_pipe(buf, pipe);

  pclose(pipe);

  if (output && r)
    {

      strncpy(output, buf, strlen(buf));
    }

  return r;
}

int
dirlog_write_record(struct dirlog *buffer, off_t offset, int whence)
{
  g_setjmp(0, "dirlog_write_record", NULL, NULL);
  if (gfl & F_OPT_NOWRITE)
    {
      return 0;
    }

  if (!buffer)
    {
      return 2;
    }

  if (!g_act_1.fh)
    {
      print_str("ERROR: dirlog handle is not open\n");
      return 1;
    }

  if (whence == SEEK_SET && fseeko(g_act_1.fh, offset * DL_SZ, SEEK_SET) < 0)
    {
      print_str("ERROR: seeking dirlog failed!\n");
      return 1;
    }

  int fw;

  if ((fw = fwrite(buffer, 1, DL_SZ, g_act_1.fh)) < DL_SZ)
    {
      print_str("ERROR: could not write dirlog record! %d/%d\n", fw,
          (int) DL_SZ);
      return 1;
    }

  g_act_1.bw += (off_t) fw;
  g_act_1.rw++;

  if (whence == SEEK_SET)
    g_act_1.offset = offset;
  else
    g_act_1.offset++;

  return 0;
}

typedef int
(*__d_edscb)(char *, unsigned char, void *, __g_eds);

int
enum_dir(char *dir, void *cb, void *arg, int f, __g_eds eds)
{
  __d_edscb callback_f = (__d_edscb ) cb;
  struct dirent *dirp, _dirp =
    { 0 };
  int r = 0, ir;

  DIR *dp = opendir(dir);

  if (!dp)
    {
      return -2;
    }

  char buf[PATH_MAX];
  struct stat st;

  if (stat(dir, &st))
    {
      return -3;
    }

  if (eds)
    {
      if (!(eds->flags & F_EDS_ROOTMINSET))
        {
          eds->r_minor = minor(st.st_dev);
          eds->flags |= F_EDS_ROOTMINSET;
          memcpy(&eds->st, &st, sizeof(struct stat));
        }

      if (!(f & F_ENUMD_NOXBLK) && (gfl & F_OPT_XBLK)
          && major(eds->st.st_dev) != 8)
        {
          r = -6;
          goto end;
        }

      if ((gfl & F_OPT_XDEV) && minor(st.st_dev) != eds->r_minor)
        {
          r = -5;
          goto end;
        }
    }

  while ((dirp = readdir(dp)))
    {
      if ((gfl & F_OPT_KILL_GLOBAL) || (gfl & F_OPT_TERM_ENUM))
        {
          break;
        }

      size_t d_name_l = strlen(dirp->d_name);

      if ((d_name_l == 1 && !strncmp(dirp->d_name, ".", 1))
          || (d_name_l == 2 && !strncmp(dirp->d_name, "..", 2)))
        {
          continue;
        }

      snprintf(buf, PATH_MAX, "%s/%s", dir, dirp->d_name);
      remove_repeating_chars(buf, 0x2F);

      if (dirp->d_type == DT_UNKNOWN)
        {
          _dirp.d_type = get_file_type(buf);
        }
      else
        {
          _dirp.d_type = dirp->d_type;
        }

      if (!(ir = callback_f(buf, _dirp.d_type, arg, eds)))
        {
          if (f & F_ENUMD_ENDFIRSTOK)
            {
              r = ir;
              break;
            }
          else
            {
              r++;
            }
        }
      else
        {
          if (f & F_ENUMD_BREAKONBAD)
            {
              r = ir;
              break;
            }
        }
    }

  end:

  closedir(dp);
  return r;
}

int
dir_exists(char *dir)
{
  int r;

  errno = 0;
  DIR *dd = opendir(dir);

  r = errno;

  if (dd)
    {
      closedir(dd);
    }
  else
    {
      if (!r)
        {
          r++;
        }
    }

  return r;
}

int
reg_match(char *expression, char *match, int flags)
{
  regex_t preg;
  size_t r;
  regmatch_t pmatch[REG_MATCHES_MAX];

  if ((r = regcomp(&preg, expression, (flags | REG_EXTENDED))))
    return r;

  r = regexec(&preg, match, REG_MATCHES_MAX, pmatch, 0);

  regfree(&preg);

  return r;
}

int
split_string(char *line, char dl, pmda output_t)
{
  int i, p, c, llen = strlen(line);

  for (i = 0, p = 0, c = 0; i <= llen; i++)
    {

      while (line[i] == dl && line[i])
        i++;
      p = i;

      while (line[i] != dl && line[i] != 0xA && line[i])
        i++;

      if (i > p)
        {
          char *buffer = md_alloc(output_t, (i - p) + 10);
          if (!buffer)
            return -1;
          memcpy(buffer, &line[p], i - p);
          c++;
        }
    }
  return c;
}

int
split_string_sp_tab(char *line, pmda output_t)
{
  int i, p, c, llen = strlen(line);

  for (i = 0, p = 0, c = 0; i <= llen; i++)
    {

      while ((line[i] == 0x20 && line[i] != 0x9) && line[i])
        i++;
      p = i;

      while ((line[i] != 0x20 && line[i] != 0x9) && line[i] != 0xA && line[i])
        i++;

      if (i > p)
        {
          char *buffer = md_alloc(output_t, (i - p) + 10);
          if (!buffer)
            return -1;
          memcpy(buffer, &line[p], i - p);
          c++;
        }
    }
  return c;
}

int
remove_repeating_chars(char *string, char c)
{
  g_setjmp(0, "remove_repeating_chars", NULL, NULL);
  size_t s_len = strlen(string);
  int i, i_c = -1;

  for (i = 0; i <= s_len; i++, i_c = -1)
    {
      while (string[i + i_c] == c)
        {
          i_c++;
        }
      if (i_c > 0)
        {
          int ct_l = (s_len - i) - i_c;
          if (!memmove(&string[i], &string[i + i_c], ct_l))
            {
              return -1;
            }
          string[i + ct_l] = 0;
          i += i_c;
        }
      else
        {
          i += i_c + 1;
        }
    }

  return 0;
}

int
write_file_text(char *data, char *file)
{
  g_setjmp(0, "write_file_text", NULL, NULL);
  int r;
  FILE *fp;

  if ((fp = fopen(file, "a")) == NULL)
    return 0;

  r = fwrite(data, 1, strlen(data), fp);

  fclose(fp);

  return r;
}

int
delete_file(char *name, unsigned char type, void *arg)
{
  g_setjmp(0, "delete_file", NULL, NULL);
  char *match = (char*) arg;

  if (type != DT_REG)
    {
      return 1;
    }

  if (!reg_match(match, name, 0))
    {
      return remove(name);
    }

  return 2;
}

off_t
read_file(char *file, void *buffer, size_t read_max, off_t offset, FILE *_fp)
{
  g_setjmp(0, "read_file", NULL, NULL);
  size_t read;
  int r;
  FILE *fp = _fp;

  if (!_fp)
    {
      if (!file)
        {
          return 0;
        }

      off_t a_fsz = get_file_size(file);

      if (!a_fsz)
        return 0;

      if (read_max > a_fsz)
        {
          read_max = a_fsz;
        }
      if ((fp = fopen(file, "rb")) == NULL)
        return 0;
    }

  if (offset)
    fseeko(fp, (off_t) offset, SEEK_SET);

  for (read = 0; !feof(fp) && read < read_max;)
    {
      if ((r = fread(&((unsigned char*) buffer)[read], 1, read_max - read, fp))
          < 1)
        break;
      read += r;
    }

  if (!_fp)
    {
      fclose(fp);
    }

  return read;
}

int
file_exists(char *file)
{
  int r = get_file_type(file);

  if (r == DT_REG)
    {
      return 0;
    }

  return 1;
}

ssize_t
file_copy(char *source, char *dest, char *mode, uint32_t flags)
{
  g_setjmp(0, "file_copy", NULL, NULL);

  if (gfl & F_OPT_NOWRITE)
    {
      return 1;
    }

  struct stat st_s, st_d;
  mode_t st_mode = 0;

  if (stat(source, &st_s))
    {
      return -9;
    }

  off_t ssize = st_s.st_size;

  if (ssize < 1)
    {
      return -1;
    }

  FILE *fh_s = fopen(source, "rb");

  if (!fh_s)
    {
      return -2;
    }

  if (!strncmp(mode, "a", 1) && (flags & F_FC_MSET_DEST))
    {
      if (file_exists(dest))
        {
          st_mode = st_s.st_mode;
        }
      else
        {
          if (stat(source, &st_d))
            {
              return -10;
            }
          st_mode = st_d.st_mode;
        }
    }
  else if (flags & F_FC_MSET_SRC)
    {
      st_mode = st_s.st_mode;
    }

  FILE *fh_d = fopen(dest, mode);

  if (!fh_d)
    {
      return -3;
    }

  size_t r = 0, t = 0, w;
  char *buffer = malloc(V_MB);

  while ((r = fread(buffer, 1, V_MB, fh_s)) > 0)
    {
      if ((w = fwrite(buffer, 1, r, fh_d)))
        {
          t += w;
        }
      else
        {
          return -4;
        }
    }

  free(buffer);
  fclose(fh_d);
  fclose(fh_s);

  if (st_mode)
    {
      chmod(dest, st_mode);
    }

  return t;
}

off_t
file_crc32(char *file, uint32_t *crc_out)
{
  g_setjmp(0, "file_crc32", NULL, NULL);
  FILE *fp;

  size_t r;

  *crc_out = 0x0;

  if ((fp = fopen(file, "rb")) == NULL)
    {
      return 0;
    }

  uint8_t *buffer = malloc(CRC_FILE_READ_BUFFER_SIZE), *ptr = buffer;

  uint32_t crc = MAX_uint32_t;

  for (;; ptr = buffer)
    {
      if ((r = fread(buffer, 1, CRC_FILE_READ_BUFFER_SIZE, fp)) < 1)
        {
          break;
        }

      for (; r; r--, ptr++)
        {
          crc = UPDC32(*ptr, crc);
        }
    }

  if (ferror(fp))
    {
      free(buffer);
      fclose(fp);
      return 0;
    }

  *crc_out = ~crc;

  free(buffer);
  fclose(fp);

  return 1;
}

#define F_CFGV_BUILD_FULL_STRING	(a32 << 1)
#define F_CFGV_RETURN_MDA_OBJECT	(a32 << 2)
#define F_CFGV_RETURN_TOKEN_EX		(a32 << 3)
#define F_CFGV_BUILD_DATA_PATH		(a32 << 10)

#define F_CFGV_MODES				(F_CFGV_BUILD_FULL_STRING|F_CFGV_RETURN_MDA_OBJECT|F_CFGV_RETURN_TOKEN_EX)

#define MAX_CFGV_RES_LENGTH			50000

void *
ref_to_val_get_cfgval(char *cfg, char *key, char *defpath, int flags, char *out,
    size_t max)
{

  char buffer[PATH_MAX];

  if (flags & F_CFGV_BUILD_DATA_PATH)
    {
      snprintf(buffer, PATH_MAX, "%s/%s/%s/%s", GLROOT, FTPDATA, defpath, cfg);
    }
  else
    {
      snprintf(buffer, PATH_MAX, "%s", cfg);
    }

  pmda ret;

  if (load_cfg(&cfg_rf, buffer, 0, &ret))
    {
      return NULL;
    }

  mda s_tk =
    { 0 };
  int r;
  size_t c_token = -1;
  char *s_key = key;

  md_init(&s_tk, 4);

  if ((r = split_string(key, 0x40, &s_tk)) == 2)
    {
      p_md_obj s_tk_ptr = s_tk.objects->next;
      flags ^= F_CFGV_BUILD_FULL_STRING;
      flags |= F_CFGV_RETURN_TOKEN_EX;
      c_token = atoi(s_tk_ptr->ptr);
      s_key = s_tk.objects->ptr;

    }

  p_md_obj ptr;
  pmda s_ret = NULL;
  void *p_ret = NULL;

  if ((ptr = get_cfg_opt(s_key, ret, &s_ret)))
    {
      switch (flags & F_CFGV_MODES)
        {
      case F_CFGV_RETURN_MDA_OBJECT:
        p_ret = (void*) ptr;
        break;
      case F_CFGV_BUILD_FULL_STRING:
        ;
        size_t o_w = 0, w;
        while (ptr)
          {
            w = strlen((char*) ptr->ptr);
            if (o_w + w + 1 < max)
              {
                memcpy(&out[o_w], ptr->ptr, w);
                o_w += w;
                if (ptr->next)
                  {
                    memset(&out[o_w], 0x20, 1);
                    o_w++;
                  }
              }
            else
              {
                break;
              }
            ptr = ptr->next;
          }
        out[o_w] = 0x0;
        p_ret = (void*) out;
        break;
      case F_CFGV_RETURN_TOKEN_EX:

        if (c_token < 0 || c_token >= s_ret->count)
          {
            return NULL;
          }

        p_md_obj p_ret_tx = &s_ret->objects[c_token];
        if (p_ret_tx)
          {
            p_ret = (void*) p_ret_tx->ptr;
          }
        break;
      default:
        p_ret = ptr->ptr;
        break;
        }
    }
  md_g_free(&s_tk);
  return p_ret;
}

int
is_char_uppercase(char c)
{
  if (c >= 0x41 && c <= 0x5A)
    {
      return 0;
    }
  return 1;
}

int
g_l_fmode(char *path, size_t max_size, char *output)
{
  struct stat st;
  char buffer[PATH_MAX + 1];
  snprintf(buffer, PATH_MAX, "%s/%s", GLROOT, path);
  remove_repeating_chars(buffer, 0x2F);
  if (lstat(buffer, &st))
    {
      snprintf(output, max_size, "-1");
      return 0;
    }
  snprintf(output, max_size, "%d", IFTODT(st.st_mode));
  return 0;
}

int
g_l_fmode_n(char *path, size_t max_size, char *output)
{
  struct stat st;
  if (lstat(path, &st))
    {
      return 1;
    }
  snprintf(output, max_size, "%d", IFTODT(st.st_mode));
  return 0;
}

int
ref_to_val_macro(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strcmp(match, "m:exe"))
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

int
rtv_q(void *query, char *output, size_t max_size)
{
  mda md_s =
    { 0 };

  md_init(&md_s, 2);
  p_md_obj ptr;

  if (split_string(query, 0x40, &md_s) != 2)
    {
      bzero(output, max_size);
      return 0;
    }

  ptr = md_s.objects;

  char *rtv_l = g_dgetf((char*) ptr->ptr);

  if (!rtv_l)
    {
      bzero(output, max_size);
      return 0;
    }

  ptr = ptr->next;

  int r = 0;
  char *rtv_q = (char*) ptr->ptr;

  if (!strncmp(rtv_q, _MC_GLOB_SIZE, 4))
    {
      snprintf(output, max_size, "%llu", (ulint64_t) get_file_size(rtv_l));
    }
  else if (!strncmp(rtv_q, "count", 5))
    {
      _g_handle hdl =
        { 0 };
      size_t rtv_ll = strlen(rtv_l);

      strncpy(hdl.file, rtv_l, rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
      if (determine_datatype(&hdl))
        {
          goto end;
        }

      snprintf(output, max_size, "%llu",
          (ulint64_t) get_file_size(rtv_l) / (ulint64_t) hdl.block_sz);
    }
  else if (!strncmp(rtv_q, "corrupt", 7))
    {
      _g_handle hdl =
        { 0 };
      size_t rtv_ll = strlen(rtv_l);

      strncpy(hdl.file, rtv_l, rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
      if (determine_datatype(&hdl))
        {
          goto end;
        }

      snprintf(output, max_size, "%llu",
          (ulint64_t) get_file_size(rtv_l) % (ulint64_t) hdl.block_sz);
    }
  else if (!strncmp(rtv_q, "bsize", 5))
    {
      _g_handle hdl =
        { 0 };
      size_t rtv_ll = strlen(rtv_l);

      strncpy(hdl.file, rtv_l, rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
      if (determine_datatype(&hdl))
        {
          goto end;
        }

      snprintf(output, max_size, "%u", (uint32_t) hdl.block_sz);
    }
  else if (!strncmp(rtv_q, _MC_GLOB_MODE, 4))
    {
      return g_l_fmode_n(rtv_l, max_size, output);
    }
  else if (!strncmp(rtv_q, "file", 4))
    {
      snprintf(output, max_size, "%s", rtv_l);
    }
  else if (!strncmp(rtv_q, "read", 4))
    {
      snprintf(output, max_size, "%d",
          !access(rtv_l, R_OK) ? 1 : errno == EACCES ? 0 : -1);
    }
  else if (!strncmp(rtv_q, "write", 5))
    {
      snprintf(output, max_size, "%d",
          !access(rtv_l, W_OK) ? 1 : errno == EACCES ? 0 : -1);
    }
  else
    {
      bzero(output, max_size);
    }

  end:

  md_g_free(&md_s);

  return r;
}

int
ref_to_val_generic(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strncmp(match, "nukestr", 7))
    {
      if (NUKESTR)
        {
          snprintf(output, max_size, NUKESTR, "");
        }
    }
  else if (!strncmp(match, "procid", 6))
    {
      snprintf(output, max_size, "%d", getpid());
    }
  else if (!strncmp(match, "ipc", 3))
    {
      snprintf(output, max_size, "%.8X", (uint32_t) SHM_IPC);
    }
  else if (!strncmp(match, "usroot", 6))
    {
      snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
      DEFPATH_USERS);
      remove_repeating_chars(output, 0x2F);
    }
  else if (!strncmp(match, "logroot", 7))
    {
      snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
      DEFPATH_LOGS);
      remove_repeating_chars(output, 0x2F);
    }
  else if (!strncmp(match, "memlimit", 8))
    {
      snprintf(output, max_size, "%llu", db_max_size);
    }
  else if (!strncmp(match, "curtime", 7))
    {
      snprintf(output, max_size, "%d", (int32_t) time(NULL));
    }
  else if (!strncmp(match, "q:", 2))
    {
      return rtv_q(&match[2], output, max_size);
    }
  else if (!strncmp(match, "exe", 3))
    {
      if (self_get_path(output))
        {
          strcp_s(output, max_size, "UNKNOWN");
        }
    }
  else if (!strncmp(match, "glroot", 6))
    {
      strcp_s(output, max_size, GLROOT);
    }
  else if (!strncmp(match, "siteroot", 8))
    {
      strcp_s(output, max_size, SITEROOT);
    }
  else if (!strncmp(match, "siterootn", 9))
    {
      strcp_s(output, max_size, SITEROOT_N);
    }
  else if (!strncmp(match, "ftpdata", 7))
    {
      strcp_s(output, max_size, FTPDATA);
    }
  else if (!strncmp(match, "logfile", 7))
    {
      strcp_s(output, max_size, LOGFILE);
    }
  else if (!strncmp(match, "imdbfile", 8))
    {
      strcp_s(output, max_size, IMDBLOG);
    }
  else if (!strncmp(match, "gamefile", 8))
    {
      strcp_s(output, max_size, GAMELOG);
    }
  else if (!strncmp(match, "tvragefile", 10))
    {
      strcp_s(output, max_size, TVLOG);
    }
  else if (!strncmp(match, "spec1", 5))
    {
      strcp_s(output, max_size, b_spec1);
    }
  else if (!strncmp(match, "glconf", 6))
    {
      strcp_s(output, max_size, GLCONF_I);
    }
  else
    {
      return 1;
    }
  return 0;
}

char *
dt_rval_generic_nukestr(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (NUKESTR)
    {
      snprintf(output, max_size, NUKESTR, "");
    }
  return output;
}

char *
dt_rval_generic_procid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", getpid());
  return output;
}

char *
dt_rval_generic_ipc(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", getpid());
  return output;
}

char *
dt_rval_generic_usroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
  DEFPATH_USERS);
  remove_repeating_chars(output, 0x2F);
  return output;
}

char *
dt_rval_generic_logroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
  DEFPATH_LOGS);
  remove_repeating_chars(output, 0x2F);
  return output;
}

char *
dt_rval_generic_memlimit(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", db_max_size);
  return output;
}

char *
dt_rval_generic_curtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) time(NULL));
  return output;
}

char *
dt_rval_q(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  if (rtv_q(&match[2], output, max_size))
    {
      output[0] = 0x0;
    }
  return output;
}

char *
dt_rval_generic_exe(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (self_get_path(output))
    {
      strcp_s(output, max_size, "UNKNOWN");
    }
  return output;
}

char *
dt_rval_generic_glroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return GLROOT;
}

char *
dt_rval_generic_siteroot(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return SITEROOT;
}

char *
dt_rval_generic_siterootn(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return SITEROOT_N;
}

char *
dt_rval_generic_ftpdata(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return FTPDATA;
}

char *
dt_rval_generic_imdbfile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return IMDBLOG;
}

char *
dt_rval_generic_tvfile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return TVLOG;
}

char *
dt_rval_generic_gamefile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return GAMELOG;
}

char *
dt_rval_generic_spec1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return b_spec1;
}

char *
dt_rval_generic_glconf(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return GLCONF_I;
}

char *
dt_rval_generic_logfile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return LOGFILE;
}

char *
dt_rval_generic_newline(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return MSG_NL;
}

char *
dt_rval_generic_tab(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return MSG_TAB;
}

#define MSG_GENERIC_NL          ":NL"
#define MSG_GENERIC_TAB         ":TAB"

void *
ref_to_val_lk_generic(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strncmp(match, "nukestr", 7))
    {
      return dt_rval_generic_nukestr;
    }
  else if (!strncmp(match, "procid", 6))
    {
      return dt_rval_generic_procid;
    }
  else if (!strncmp(match, "ipc", 3))
    {
      return dt_rval_generic_ipc;
    }
  else if (!strncmp(match, "usroot", 6))
    {
      return dt_rval_generic_usroot;
    }
  else if (!strncmp(match, "logroot", 7))
    {
      return dt_rval_generic_logroot;
    }
  else if (!strncmp(match, "memlimit", 8))
    {
      return dt_rval_generic_memlimit;
    }
  else if (!strncmp(match, "curtime", 7))
    {
      return dt_rval_generic_curtime;
    }
  else if (!strncmp(match, "q:", 2))
    {
      return dt_rval_q;
    }
  else if (!strncmp(match, MSG_GENERIC_NL, 3))
    {
      return dt_rval_generic_newline;
    }
  else if (!strncmp(match, MSG_GENERIC_TAB, 4))
    {
      return dt_rval_generic_tab;
    }
  else if (!strncmp(match, "exe", 3))
    {
      return dt_rval_generic_exe;
    }
  else if (!strncmp(match, "glroot", 6))
    {
      return dt_rval_generic_glroot;
    }
  else if (!strncmp(match, "siteroot", 8))
    {
      return dt_rval_generic_siteroot;
    }
  else if (!strncmp(match, "siterootn", 9))
    {
      return dt_rval_generic_siterootn;
    }
  else if (!strncmp(match, "ftpdata", 7))
    {
      return dt_rval_generic_ftpdata;
    }
  else if (!strncmp(match, "logfile", 7))
    {
      return dt_rval_generic_logfile;
    }
  else if (!strncmp(match, "imdbfile", 8))
    {
      return dt_rval_generic_imdbfile;
    }
  else if (!strncmp(match, "gamefile", 8))
    {
      return dt_rval_generic_gamefile;
    }
  else if (!strncmp(match, "tvragefile", 10))
    {
      return dt_rval_generic_tvfile;
    }
  else if (!strncmp(match, "spec1", 5))
    {
      return dt_rval_generic_spec1;
    }
  else if (!strncmp(match, "glconf", 6))
    {
      return dt_rval_generic_glconf;
    }

  return NULL;
}

char *
strcp_s(char *dest, size_t max_size, char *source)
{
  size_t s_l = strlen(source);
  s_l >= max_size ? s_l = max_size - 1 : s_l;
  dest[s_l] = 0x0;
  return strncpy(dest, source, s_l);
}

#define	_MC_X_DEVID		"devid"
#define	_MC_X_MINOR		"minor"
#define	_MC_X_MAJOR		"major"
#define	_MC_X_INODE		"inode"
#define	_MC_X_LINKS		"links"
#define	_MC_X_BLKSIZE	        "blksize"
#define	_MC_X_BLOCKS	        "blocks"
#define	_MC_X_ATIME		"atime"
#define	_MC_X_CTIME		"ctime"
#define	_MC_X_MTIME		"mtime"
#define	_MC_X_ISREAD	        "isread"
#define	_MC_X_ISWRITE	        "iswrite"
#define	_MC_X_ISEXEC	        "isexec"
#define	_MC_X_PERM		"perm"
#define _MC_X_OPERM		"operm"
#define _MC_X_GPERM		"gperm"
#define _MC_X_UPERM		"uperm"
#define _MC_X_SPARSE	        "sparse"
#define _MC_X_CRC32		"crc32"
#define _MC_X_DCRC32	        "dec-crc32"
#define _MC_X_BASEPATH	        "basepath"
#define _MC_X_DIRPATH	        "dirpath"
#define _MC_X_PATH		"path"
#define _MC_X_UID		"uid"
#define _MC_X_GID		"gid"

int
ref_to_val_x(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  if (!ref_to_val_generic(NULL, match, output, max_size, mppd))
    {
      return 0;
    }

  __d_xref data = (__d_xref) arg;

  if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      snprintf(output, max_size, "%llu", (ulint64_t) get_file_size(data->name));
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%hu", (unsigned short) IFTODT(st.st_mode));
    }
  else if (!strncmp(match, _MC_X_DEVID, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_dev);
    }
  else if (!strncmp(match, _MC_X_MINOR, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", minor(st.st_dev));
    }
  else if (!strncmp(match, _MC_X_MAJOR, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", major(st.st_dev));
    }
  else if (!strncmp(match, _MC_X_INODE, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_ino);
    }
  else if (!strncmp(match, _MC_X_LINKS, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_nlink);
    }
  else if (!strncmp(match, _MC_X_UID, 3))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_uid);
    }
  else if (!strncmp(match, _MC_X_GID, 3))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_gid);
    }
  else if (!strncmp(match, _MC_X_BLKSIZE, 7))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_blksize);
    }
  else if (!strncmp(match, _MC_X_BLOCKS, 6))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_blocks);
    }
  else if (!strncmp(match, _MC_X_ATIME, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%d", (int32_t) st.st_atime);
    }
  else if (!strncmp(match, _MC_X_CTIME, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%d", (int32_t) get_file_creation_time(&st));
    }
  else if (!strncmp(match, _MC_X_MTIME, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%d", (int32_t) st.st_mtime);
    }
  else if (!strncmp(match, _MC_X_ISREAD, 6))
    {
      snprintf(output, max_size, "%hu", (uint8_t) !(access(data->name, R_OK)));
    }
  else if (!strncmp(match, _MC_X_ISWRITE, 7))
    {
      snprintf(output, max_size, "%hu", (uint8_t) !(access(data->name, W_OK)));
    }
  else if (!strncmp(match, _MC_X_ISEXEC, 6))
    {
      snprintf(output, max_size, "%hu", (uint8_t) !(access(data->name, X_OK)));
    }
  else if (!strncmp(match, _MC_X_UPERM, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          strcp_s(output, max_size, "-1");
        }
      else
        {
          snprintf(output, max_size, "%hu",
              (uint16_t) ((st.st_mode & S_IRWXU) >> 6));
        }
    }
  else if (!strncmp(match, _MC_X_GPERM, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          strcp_s(output, max_size, "-1");
        }
      else
        {
          snprintf(output, max_size, "%hu",
              (uint16_t) ((st.st_mode & S_IRWXG) >> 3));
        }
    }
  else if (!strncmp(match, _MC_X_OPERM, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          strcp_s(output, max_size, "-1");
        }
      else
        {
          snprintf(output, max_size, "%hu",
              (uint16_t) ((st.st_mode & S_IRWXO)));
        }
    }
  else if (!strncmp(match, _MC_X_PERM, 4))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          strcp_s(output, max_size, "-1");
        }
      else
        {
          snprintf(output, max_size, "%hu%hu%hu",
              (uint16_t) ((st.st_mode & S_IRWXU) >> 6),
              (uint16_t) ((st.st_mode & S_IRWXG) >> 3),
              (uint16_t) ((st.st_mode & S_IRWXO)));
        }
    }
  else if (!strncmp(match, _MC_X_SPARSE, 6))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      else
        {
          snprintf(output, max_size, "%f",
              ((float) st.st_blksize * (float) st.st_blocks / (float) st.st_size));
        }
    }
  else if (!strncmp(match, _MC_X_CRC32, 5))
    {
      uint32_t crc32;
      file_crc32(data->name, &crc32);
      snprintf(output, max_size, "%.8X", crc32);
    }
  else if (!strncmp(match, _MC_X_DCRC32, 9))
    {
      uint32_t crc32;
      file_crc32(data->name, &crc32);
      snprintf(output, max_size, "%u", crc32);
    }
  else if (!strncmp(match, "c:", 2))
    {
      g_rtval_ex(data->name, &match[2], max_size, output,
      F_CFGV_BUILD_FULL_STRING);
    }
  else if (!strncmp(match, _MC_X_BASEPATH, 8))
    {
      strcp_s(output, max_size, g_basename(data->name));
    }
  else if (!strncmp(match, _MC_X_DIRPATH, 7))
    {
      strcp_s(output, max_size, data->name);
      g_dirname(output);
    }
  else if (!strncmp(match, _MC_X_PATH, 4))
    {
      strcp_s(output, max_size, data->name);
    }
  else
    {
      return 1;
    }

  return 0;
}

char *
dt_rval_x_path(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_xref) arg)->name;
}

char *
dt_rval_x_basepath(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((__d_xref) arg)->name);
}

char *
dt_rval_x_dirpath(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strcp_s(output, max_size, ((__d_xref) arg)->name);
  g_dirname(output);
  return output;
}

char *
dt_rval_x_c(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  char p_b0[64];
  int ic = 0;

  while (match[ic] != 0x7D && ic < 64)
    {
      p_b0[ic] = match[ic];
      ic++;
    }

  p_b0[ic] = 0x0;

  return g_rtval_ex(((__d_xref) arg)->name, &p_b0[2], max_size, output,
  F_CFGV_BUILD_FULL_STRING);
}

char *
dt_rval_x_size(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{

  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) get_file_size(((__d_xref) arg)->name));
  return output;
}

char *
dt_rval_x_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint16_t) ((__d_xref) arg)->type);
  return output;
}

char *
dt_rval_x_devid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_dev);
  return output;
}

char *
dt_rval_x_minor(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      minor(((__d_xref) arg)->st.st_dev));
  return output;
}

char *
dt_rval_x_major(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      major(((__d_xref) arg)->st.st_dev));
  return output;
}

char *
dt_rval_x_inode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_ino);
  return output;
}

char *
dt_rval_x_links(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_nlink);
  return output;
}

char *
dt_rval_x_uid(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_uid);
  return output;
}

char *
dt_rval_x_gid(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_gid);
  return output;
}

char *
dt_rval_x_blksize(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_blksize);
  return output;
}

char *
dt_rval_x_blocks(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_blocks);
  return output;
}

char *
dt_rval_x_atime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_xref) arg)->st.st_atime);
  return output;
}

char *
dt_rval_x_ctime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_xref) arg)->st.st_ctime);
  return output;
}

char *
dt_rval_x_mtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (int32_t) ((__d_xref) arg)->st.st_mtime);
  return output;
}

char *
dt_rval_x_isread(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->r);
  return output;
}

char *
dt_rval_x_iswrite(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->w);
  return output;
}

char *
dt_rval_x_isexec(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->x);
  return output;
}

char *
dt_rval_x_uperm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint16_t) ((__d_xref) arg)->uperm);
  return output;
}

char *
dt_rval_x_gperm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint16_t) ((__d_xref) arg)->gperm);
  return output;
}

char *
dt_rval_x_operm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint16_t) ((__d_xref) arg)->operm);
  return output;
}

char *
dt_rval_x_perm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hhu%hhu%hhu", (uint16_t) ((__d_xref) arg)->uperm, ((__d_xref) arg)->gperm, ((__d_xref) arg)->operm);
  return output;
}

char *
dt_rval_x_sparse(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((float) ((__d_xref) arg)->st.st_blksize * (float) ((__d_xref) arg)->st.st_blocks
  / (float) ((__d_xref) arg)->st.st_size));
  return output;
}

char *
dt_rval_x_crc32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  uint32_t crc32;
  file_crc32(((__d_xref) arg)->name, &crc32);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, crc32);
  return output;
}

char *
dt_rval_x_deccrc32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  uint32_t crc32;
  file_crc32(((__d_xref) arg)->name, &crc32);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, crc32);
  return output;
}

char *
g_get_stf(char *match)
{
  while (match[0] != 0x7D && match[0] != 0x23 && match[0])
    {
      match++;
    }
  if (match[0] == 0x23)
    {
      match++;
      return match;
    }
  return NULL;
}

void *
as_ref_to_val_lk(char *match, void *c, __d_drt_h mppd, char *defdc)
{
  if (defdc)
    {
      match = g_get_stf(match);

      if (match)
        {
          if (strncmp("%s", defdc, 2))
            {
              size_t i = 0;
              while (match[0] != 0x7D && match[0] && i < sizeof(mppd->direc) - 2)
                {
                  mppd->direc[i] = match[0];
                  i++;
                  match++;
                }

              if (1 < strlen(mppd->direc))
                {
                  mppd->direc[i] = 0x0;
                  goto ct;
                }
            }
        }
      if (mppd->direc[0] == 0x0)
        {
          size_t defdc_l = strlen(defdc);
          strncpy(mppd->direc, defdc, defdc_l);
          mppd->direc[defdc_l] = 0x0;
        }
    }

  ct:

  return c;
}

char *
dt_rval_spec_slen(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match, output, max_size, mppd);

  if (p_o)
    {
      snprintf(output, max_size, "%zu", strlen(p_o));
    }
  else
    {
      output[0] = 0x0;
    }
  return output;
}

char *
dt_rval_spec_gc(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h ) mppd;
  if (_mppd->t_1 > max_size)
    {
      _mppd->t_1 = max_size;
    }

  memset(output, _mppd->c_1, _mppd->t_1);
  output[_mppd->t_1] = 0x0;

  return output;
}

char *
dt_rval_spec_tf_pl(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, localtime(&uts));
  return output;
}

char *
dt_rval_spec_tf_pgm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, gmtime(&uts));
  return output;
}

char *
dt_rval_spec_tf_l(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      localtime(&((__d_drt_h ) mppd)->ts_1));
  return output;
}

char *
dt_rval_spec_tf_gm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      gmtime(&((__d_drt_h ) mppd)->ts_1));
  return output;
}

void *
ref_to_val_af(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  match++;
  char *id = match;
  size_t i = 0;
  while (match[0] != 0x3A && match[0])
    {
      i++;
      match++;
    }
  if (match[0] != 0x3A)
    {
      return NULL;
    }
  match++;
  if (match[0] == 0x0)
    {
      return NULL;
    }

  if (i < 3)
    {
      switch (id[0])
        {
      case 0x6C:

        mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
            mppd);
        if (!mppd->fp_rval1)
          {
            return NULL;
          }
        return as_ref_to_val_lk(match, dt_rval_spec_slen, mppd, NULL);
        break;
      case 0x74:
        ;
        strncpy(mppd->direc, "%d/%h/%Y %H:%M:%S", 18);
        as_ref_to_val_lk(match, NULL, mppd, "%d/%h/%Y %H:%M:%S");
        if (is_ascii_numeric((uint8_t) match[0]) && match[0] != 0x2B
            && match[0] != 0x2D)
          {
            int vb;

            mppd->vp_off1 = (size_t) mppd->hdl->g_proc2(mppd->hdl->_x_ref,
                match, &vb);
            switch (id[1])
              {
            case 0x6C:
              return dt_rval_spec_tf_pl;
              break;
            default:
              return dt_rval_spec_tf_pgm;
              break;
              }
          }
        else
          {
            char b_c[65];
            size_t b_l = 0;

            while ((!is_ascii_numeric((uint8_t) match[0]) || match[0] == 0x2B
                || match[0] == 0x2D) && match[0])
              {
                if (b_l < 64)
                  {
                    b_c[b_l] = match[0];
                  }
                else
                  {
                    break;
                  }
                b_l++;
                match++;
              }

            if (match[0] == 0x0)
              {
                return NULL;
              }

            b_c[b_l] = 0x0;
            if (!b_l || b_l == 64)
              {
                return NULL;
              }

            mppd->ts_1 = (time_t) strtol(b_c, NULL, 10);
            switch (id[1])
              {
            case 0x6C:
              return dt_rval_spec_tf_l;
              break;
            default:
              return dt_rval_spec_tf_gm;
              break;
              }
          }

        break;
      case 0x63:
        ;
        char b_c[65];
        size_t b_l = 0;
        b_c[0] = 0x0;
        while (match[0] != 0x3A && match[0])
          {
            if (b_l < 64)
              {
                b_c[b_l] = match[0];
              }
            b_l++;
            match++;
          }
        b_c[b_l] = 0x0;
        if (match[0] != 0x3A)
          {
            return NULL;
          }
        match++;
        if (match[0] == 0x5C)
          {
            match++;
            switch (match[0])
              {
            case 0x6E:
              mppd->c_1 = 0xA;
              break;
            case 0x72:
              mppd->c_1 = 0xD;
              break;
            case 0x5C:
              mppd->c_1 = 0x5C;
              break;
            case 0x74:
              mppd->c_1 = 0x9;
              break;
            case 0x73:
              mppd->c_1 = 0x20;
              break;
            default:
              return NULL;
              }
          }
        else
          {
            mppd->c_1 = match[0];
          }
        mppd->t_1 = strtoul(b_c, NULL, 10);
        return dt_rval_spec_gc;
        break;
        }
    }
  return NULL;
}

void*
ref_to_val_lk_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_size ,(__d_drt_h)mppd, "%llu");
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_DT_MODE;
        }
      return as_ref_to_val_lk(match, dt_rval_x_mode ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_DEVID, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_devid ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_MINOR, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT |F_XRF_GET_MINOR;
        }
      return as_ref_to_val_lk(match, dt_rval_x_minor ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_MAJOR, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT |F_XRF_GET_MAJOR;
        }
      return as_ref_to_val_lk(match, dt_rval_x_major ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_INODE, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_inode ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_LINKS, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_links ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_UID, 3))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_uid ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_GID, 3))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_gid ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_BLKSIZE, 7))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_blksize ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_BLOCKS, 6))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_blocks ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_ATIME, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_atime ,(__d_drt_h)mppd, "%d");
    }
  else if (!strncmp(match, _MC_X_CTIME, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT | F_XRF_GET_STCTIME;
        }
      return as_ref_to_val_lk(match, dt_rval_x_ctime ,(__d_drt_h)mppd, "%d");
    }
  else if (!strncmp(match, _MC_X_MTIME, 5))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_mtime ,(__d_drt_h)mppd, "%d");
    }
  else if (!strncmp(match, _MC_X_ISREAD, 6))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_READ;
        }
      return as_ref_to_val_lk(match, dt_rval_x_isread ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_ISWRITE, 7))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_WRITE;
        }
      return as_ref_to_val_lk(match, dt_rval_x_iswrite ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_ISEXEC, 6))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_EXEC;
        }
      return as_ref_to_val_lk(match, dt_rval_x_isexec ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_UPERM, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_UPERM | F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_uperm ,(__d_drt_h)mppd, "%hu");
    }
  else if (!strncmp(match, _MC_X_GPERM, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_GPERM | F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_gperm ,(__d_drt_h)mppd, "%hu");
    }
  else if (!strncmp(match, _MC_X_OPERM, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_OPERM | F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_operm ,(__d_drt_h)mppd, "%hu");
    }
  else if (!strncmp(match, _MC_X_PERM, 4))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT | F_XRF_GET_GPERM | F_XRF_GET_UPERM | F_XRF_GET_OPERM;
        }
      return as_ref_to_val_lk(match, dt_rval_x_perm ,(__d_drt_h)mppd, "%hhu%hhu%hhu");
    }
  else if (!strncmp(match, _MC_X_SPARSE, 6))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_sparse ,(__d_drt_h)mppd, "%f");
    }
  else if (!strncmp(match, _MC_X_CRC32, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_x_crc32 ,(__d_drt_h)mppd, "%.8X");
    }
  else if (!strncmp(match, _MC_X_DCRC32, 9))
    {
      return as_ref_to_val_lk(match, dt_rval_x_deccrc32 ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_BASEPATH, 8))
    {
      return as_ref_to_val_lk(match, dt_rval_x_basepath ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, _MC_X_DIRPATH, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_x_dirpath ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, _MC_X_PATH, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_x_path ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, "c:", 2))
    {
      return dt_rval_x_c;
    }
  else if (!strncmp(match, "?", 1))
    {
      return ref_to_val_af(arg, match, output, max_size, (__d_drt_h)mppd);
    }

  return NULL;
}

void *
ref_to_val_ptr_x(void *arg, char *match, int *output)
{
  __d_xref data = (__d_xref) arg;

  if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      *output = sizeof(data->st.st_size);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_size;
    }
  else if (!strncmp(match,_MC_GLOB_MODE, 4))
    {
      *output = sizeof(data->type);
      data->flags |= F_XRF_GET_DT_MODE;
      return &((__d_xref) NULL)->type;
    }
  else if (!strncmp(match, _MC_X_ISREAD, 6))
    {
      *output = ~((int) sizeof(data->r));
      data->flags |= F_XRF_GET_READ;
      return &((__d_xref) NULL)->r;
    }
  else if (!strncmp(match, _MC_X_ISWRITE, 7))
    {
      *output = ~((int) sizeof(data->w));
      data->flags |= F_XRF_GET_WRITE;
      return &((__d_xref) NULL)->w;
    }
  else if (!strncmp(match, _MC_X_ISEXEC, 6))
    {
      *output = ~((int) sizeof(data->x));
      data->flags |= F_XRF_GET_EXEC;
      return &((__d_xref) NULL)->x;
    }
  else if (!strncmp(match, _MC_X_UPERM, 5))
    {
      *output = sizeof(data->uperm);
      data->flags |= F_XRF_GET_UPERM | F_XRF_DO_STAT;
      return &((__d_xref) NULL)->uperm;
    }
  else if (!strncmp(match, _MC_X_GPERM, 5))
    {
      *output = sizeof(data->gperm);
      data->flags |= F_XRF_GET_GPERM | F_XRF_DO_STAT;
      return &((__d_xref) NULL)->gperm;
    }
  else if (!strncmp(match, _MC_X_OPERM, 5))
    {
      *output = sizeof(data->operm);
      data->flags |= F_XRF_GET_OPERM | F_XRF_DO_STAT;
      return &((__d_xref) NULL)->operm;
    }
  else if (!strncmp(match, _MC_X_DEVID, 5))
    {
      *output = sizeof(data->st.st_dev);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_dev;
    }
  else if (!strncmp(match, _MC_X_MINOR, 5))
    {
      *output = sizeof(data->minor);
      data->flags |= F_XRF_GET_MINOR | F_XRF_DO_STAT;
      return &((__d_xref) NULL)->minor;
    }
  else if (!strncmp(match, _MC_X_MAJOR, 5))
    {
      *output = sizeof(data->major);
      data->flags |= F_XRF_GET_MAJOR | F_XRF_DO_STAT;
      return &((__d_xref) NULL)->major;
    }
  else if (!strncmp(match, _MC_X_SPARSE, 6))
    {
      *output = -32;
      data->flags |= F_XRF_GET_SPARSE | F_XRF_DO_STAT;
      return &((__d_xref) NULL)->sparseness;
    }
  else if (!strncmp(match, _MC_X_INODE, 5))
    {
      *output = sizeof(data->st.st_ino);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_ino;
    }
  else if (!strncmp(match, _MC_X_LINKS, 5))
    {
      *output = sizeof(data->st.st_nlink);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_nlink;
    }
  else if (!strncmp(match, _MC_X_UID, 3))
    {
      *output = sizeof(data->st.st_uid);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_uid;
    }
  else if (!strncmp(match, _MC_X_GID, 3))
    {
      *output = sizeof(data->st.st_gid);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_gid;
    }
  else if (!strncmp(match, _MC_X_BLKSIZE, 7))
    {
      *output = sizeof(data->st.st_blksize);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_blksize;
    }
  else if (!strncmp(match, _MC_X_BLOCKS, 6))
    {
      *output = sizeof(data->st.st_blocks);
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_blocks;
    }
  else if (!strncmp(match, _MC_X_ATIME, 5))
    {
      *output = ~((int) sizeof(data->st.st_atime));
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_atime;
    }
  else if (!strncmp(match, _MC_X_CTIME, 5))
    {
      *output = ~((int) sizeof(data->st.st_ctime));
      data->flags |= F_XRF_DO_STAT|F_XRF_GET_STCTIME;
      return &((__d_xref) NULL)->st.st_ctime;
    }
  else if (!strncmp(match, _MC_X_MTIME, 5))
    {
      *output = ~((int) sizeof(data->st.st_mtime));
      data->flags |= F_XRF_DO_STAT;
      return &((__d_xref) NULL)->st.st_mtime;
    }
  else if (!strncmp(match, _MC_X_CRC32, 5))
    {
      *output = sizeof(data->crc32);
      data->flags |= F_XRF_GET_CRC32;
      return &((__d_xref) NULL)->crc32;
    }
  else if (!strncmp(match, "curtime", 7))
    {
      size_t xrf_cto = d_xref_ct_fe(&data->ct[0], GM_MAX);
      if (xrf_cto == -1)
        {
          print_str("ERROR: ct slot limit exceeded!\n");
          gfl = F_OPT_KILL_GLOBAL;
          EXITVAL = 4;
          return NULL;
        }
      data->ct[xrf_cto].active = 1;
      data->ct[xrf_cto].curtime = time(NULL);
      switch (match[7])
        {
          case 0x2D:;
          //data->ct[xrf_cto].ct_off = ~atoi(&match[8]);
          data->ct[xrf_cto].curtime -= atoi(&match[8]);
          break;
          case 0x2B:;
          //data->ct[xrf_cto].ct_off = atoi(&match[8]);
          data->ct[xrf_cto].curtime += atoi(&match[8]);
          break;
        }
      data->flags |= F_XRF_GET_CTIME;
      *output = ~((int) sizeof(data->ct[xrf_cto].curtime));
      return &((__d_xref) NULL)->ct[xrf_cto].curtime;
    }

  return NULL;
}

size_t
d_xref_ct_fe(__d_xref_ct input, size_t sz)
{
  size_t i;

  for (i = 0; i < sz; i++)
    {
      if (!input[i].active)
        {
          return i;
        }
    }
  return -1;
}

char *
g_rtval_ex(char *arg, char *match, size_t max_size, char *output,
    uint32_t flags)
{
  void *ptr = ref_to_val_get_cfgval(arg, match,
  NULL, flags, output, max_size);
  if (!ptr)
    {
      output[0] = 0x0;
      ptr = output;
    }

  return ptr;
}

int
g_is_higher(uint64_t s, uint64_t d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher_s(int64_t s, int64_t d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower(uint64_t s, uint64_t d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower_s(int64_t s, int64_t d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher_2(uint64_t s, uint64_t d)
{
  return (s > d);
}

int
g_is_higher_2_s(int64_t s, int64_t d)
{
  return (s > d);
}

int
g_is_lower_2(uint64_t s, uint64_t d)
{
  return (s < d);
}

int
g_is_lower_2_s(int64_t s, int64_t d)
{
  return (s < d);
}

int
g_is_equal(uint64_t s, uint64_t d)
{
  return (s == d);
}

int
g_is_equal_s(int64_t s, int64_t d)
{
  return (s == d);
}

int
g_is_not_equal(uint64_t s, uint64_t d)
{
  return (s != d);
}

int
g_is_not_equal_s(int64_t s, int64_t d)
{
  return (s != d);
}

int
g_is_higherorequal(uint64_t s, uint64_t d)
{
  return (s >= d);
}

int
g_is_higherorequal_s(int64_t s, int64_t d)
{
  return (s >= d);
}

int
g_is_lowerorequal(uint64_t s, uint64_t d)
{
  return (s <= d);
}

int
g_is_lowerorequal_s(int64_t s, int64_t d)
{
  return (s <= d);
}

int
g_is(uint64_t s, uint64_t d)
{
  return s != 0;
}

int
g_is_s(int64_t s, int64_t d)
{
  return s != 0;
}

int
g_is_not(uint64_t s, uint64_t d)
{
  return s == 0;
}

int
g_is_not_s(int64_t s, int64_t d)
{
  return s == 0;
}

int
g_is_higher_f(float s, float d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher_d(double s, double d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower_f(float s, float d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower_d(double s, double d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher_f_2(float s, float d)
{
  return (s > d);
}

int
g_is_lower_f_2(float s, float d)
{
  return (s < d);
}

int
g_is_equal_f(float s, float d)
{
  return (s == d);
}

int
g_is_equal_d(double s, double d)
{
  return (s == d);
}

int
g_is_not_equal_f(float s, float d)
{
  return (s != d);
}

int
g_is_higherorequal_f(float s, float d)
{
  return (s >= d);
}

int
g_is_lowerorequal_f(float s, float d)
{
  return (s <= d);
}

int
g_is_f(float s, float d)
{
  return s != 0;
}

int
g_is_not_f(float s, float d)
{
  return s == 0;
}

int
g_oper_and(int s, int d)
{
  return (s && d);
}

int
g_oper_or(int s, int d)
{
  return (s || d);
}

uint64_t
g_t8_ptr(void *base, size_t offset)
{
  return (uint64_t) *((uint8_t*) (base + offset));
}

int64_t
g_ts8_ptr(void *base, size_t offset)
{
  return (int64_t) *((int8_t*) (base + offset));
}

uint64_t
g_t16_ptr(void *base, size_t offset)
{
  return (uint64_t) *((uint16_t*) (base + offset));
}

int64_t
g_ts16_ptr(void *base, size_t offset)
{
  return (int64_t) *((int16_t*) (base + offset));
}

uint64_t
g_t32_ptr(void *base, size_t offset)
{
  return (uint64_t) *((uint32_t*) (base + offset));
}

int64_t
g_ts32_ptr(void *base, size_t offset)
{
  return (int64_t) *((int32_t*) (base + offset));
}

uint64_t
g_t64_ptr(void *base, size_t offset)
{
  return *((uint64_t*) (base + offset));
}

int64_t
g_ts64_ptr(void *base, size_t offset)
{
  return *((int64_t*) (base + offset));
}

float
g_tf_ptr(void *base, size_t offset)
{
  return *((float*) (base + offset));
}

float
g_td_ptr(void *base, size_t offset)
{
  return *((double*) (base + offset));
}

#define MAX_SORT_LOOPS				MAX_uint64_t

#define F_INT_GSORT_LOOP_DID_SORT	(a32 << 1)
#define F_INT_GSORT_DID_SORT		(a32 << 2)

int
g_sorti_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(uint64_t s, uint64_t d) = cb1;
  uint64_t
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;

  uint64_t t_b, t_b_n;
  uint32_t ml_f = 0;
  uint64_t ml_i;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if ((flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return 0;
}

int
g_sortis_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(int64_t s, int64_t d) = cb1;
  int64_t
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;

  int64_t t_b, t_b_n;
  uint32_t ml_f = 0;
  uint64_t ml_i;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if ((flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return 0;
}

int
g_sortd_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(double s, double d) = cb1;
  double
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;
  int r = 0;
  uint64_t ml_i;
  uint32_t ml_f = 0;
  double t_b, t_b_n;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if (!r && (flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return r;
}

int
g_sortf_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2)
{
  int
  (*m_op)(float s, float d) = cb1;
  float
  (*g_t_ptr_c)(void *base, size_t offset) = cb2;

  p_md_obj ptr, ptr_n;
  int r = 0;
  uint64_t ml_i;
  uint32_t ml_f = 0;
  float t_b, t_b_n;

  for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++)
    {
      ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
      ptr = md_first(m_ptr);
      while (ptr && ptr->next)
        {
          ptr_n = (p_md_obj) ptr->next;

          t_b = g_t_ptr_c(ptr->ptr, off);
          t_b_n = g_t_ptr_c(ptr_n->ptr, off);

          if (!m_op(t_b, t_b_n))
            {
              ptr = md_swap_s(m_ptr, ptr, ptr_n);
              if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
                {
                  ml_f |= F_INT_GSORT_LOOP_DID_SORT;
                }
            }
          else
            {
              ptr = ptr->next;
            }
        }

      if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT))
        {
          break;
        }

      if (!(ml_f & F_INT_GSORT_DID_SORT))
        {
          ml_f |= F_INT_GSORT_DID_SORT;
        }
    }

  if (!(ml_f & F_INT_GSORT_DID_SORT))
    {
      return -1;
    }

  if (!r && (flags & F_GSORT_RESETPOS))
    {
      m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
    }

  return r;
}

int
g_sort(__g_handle hdl, char *field, uint32_t flags)
{
  g_setjmp(0, "g_sort", NULL, NULL);
  void *m_op = NULL, *g_t_ptr_c = NULL;

  int
  (*g_s_ex)(pmda, size_t, uint32_t, void *, void *) = NULL;
  pmda m_ptr;

  if (!hdl)
    {
      return 1;
    }

  if (!(hdl->flags & F_GH_FFBUFFER))
    {
      m_ptr = &hdl->buffer;
    }
  else
    {
      m_ptr = &hdl->w_buffer;
    }

  if (!hdl->g_proc2)
    {
      return 2;
    }

  void *g_fh_f = NULL, *g_fh_s = NULL;

  switch (flags & F_GSORT_ORDER)
    {
  case F_GSORT_DESC:
    m_op = g_is_lower;
    g_fh_f = g_is_lower_f;
    g_fh_s = g_is_lower_s;
    break;
  case F_GSORT_ASC:
    m_op = g_is_higher;
    g_fh_f = g_is_higher_f;
    g_fh_s = g_is_higher_s;
    break;
    }

  if (!m_op)
    {
      return 3;
    }

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb)
    {
      return 13;
    }

  switch (vb)
    {
  case -32:
    m_op = g_fh_f;
    g_t_ptr_c = g_tf_ptr;
    g_s_ex = g_sortf_exec;
    break;
  case 1:
    g_t_ptr_c = g_t8_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case 2:
    g_t_ptr_c = g_t16_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case 4:
    g_t_ptr_c = g_t32_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case 8:
    g_t_ptr_c = g_t64_ptr;
    g_s_ex = g_sorti_exec;
    break;
  case -2:
    g_t_ptr_c = g_ts8_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  case -3:
    g_t_ptr_c = g_ts16_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  case -5:
    g_t_ptr_c = g_ts32_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  case -9:
    g_t_ptr_c = g_ts64_ptr;
    g_s_ex = g_sortis_exec;
    m_op = g_fh_s;
    break;
  default:
    return 14;
    break;
    }

  return g_s_ex(m_ptr, off, flags, m_op, g_t_ptr_c);

}

int
g_sfc(_d_is_am *cb, char *in, int len)
{
  len--;
  for (; len > -1; len--)
    {
      if (!cb((uint8_t) in[len]))
        {

          return 0;
        }
    }
  return 1;
}

#define F_GLT_LEFT		(a32 << 1)
#define F_GLT_RIGHT		(a32 << 2)

#define F_GLT_DIRECT	(F_GLT_LEFT|F_GLT_RIGHT)

int
g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags)
{
  g_setjmp(0, "g_get_lom_g_t_ptr", NULL, NULL);

  int vb = 0;

  size_t off = (size_t) hdl->g_proc2(hdl->_x_ref, field, &vb);

  if (!vb)
    {
      errno = 0;
      uint32_t t_f = 0;
      int base;

      if (!(lom->flags & F_LOM_TYPES))
        {
          if (field[0] == 0x2D || field[0] == 0x2B)
            {
              lom->flags |= F_LOM_INT_S;
            }
          else if (s_string(field, ".", 0))
            {
              lom->flags |= F_LOM_FLOAT;
            }
          else
            {
              lom->flags |= F_LOM_INT;
            }
        }

      if (!g_sfc(is_ascii_hexadecimal, field, strlen(field)))
        {
          base = 16;
        }
      else
        {
          base = 10;
        }

      switch (flags & F_GLT_DIRECT)
        {
      case F_GLT_LEFT:
        switch (lom->flags & F_LOM_TYPES)
          {
        case F_LOM_INT:
          lom->t_left = (uint64_t) strtoll(field, NULL, base);
          lom->g_lom_vp = g_lom_var_uint;
          break;
        case F_LOM_INT_S:
          lom->ts_left = (int64_t) strtoll(field, NULL, base);
          lom->g_lom_vp = g_lom_var_int;
          break;
        case F_LOM_FLOAT:
          lom->tf_left = (float) strtof(field, NULL);
          lom->g_lom_vp = g_lom_var_float;
          break;
          }
        t_f |= F_LOM_LVAR_KNOWN;
        break;
      case F_GLT_RIGHT:
        switch (lom->flags & F_LOM_TYPES)
          {
        case F_LOM_INT:
          lom->t_right = (uint64_t) strtoll(field, NULL, base);
          lom->g_lom_vp = g_lom_var_uint;
          break;
        case F_LOM_INT_S:
          lom->ts_right = (int64_t) strtoll(field, NULL, base);
          lom->g_lom_vp = g_lom_var_int;
          break;
        case F_LOM_FLOAT:
          lom->tf_right = (float) strtof(field, NULL);
          lom->g_lom_vp = g_lom_var_float;
          break;
          }
        t_f |= F_LOM_RVAR_KNOWN;
        break;
        }

      if (errno == ERANGE)
        {
          return 4;
        }

      lom->flags |= t_f;

      return 0;
    }

  if (off > hdl->block_sz)
    {
      return 601;
    }

  switch (vb)
    {
  case -32:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_tf_ptr_left = g_tf_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_tf_ptr_right = g_tf_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_float;
    lom->flags |= F_LOM_FLOAT;
    break;
  case -2:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts8_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts8_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case -3:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts16_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts16_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case -5:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts32_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts32_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case -9:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_ts_ptr_left = g_ts64_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_ts_ptr_right = g_ts64_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_int;
    lom->flags |= F_LOM_INT_S;
    break;
  case 1:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t8_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t8_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  case 2:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t16_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t16_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  case 4:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t32_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t32_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  case 8:
    switch (flags & F_GLT_DIRECT)
      {
    case F_GLT_LEFT:
      lom->g_t_ptr_left = g_t64_ptr;
      break;
    case F_GLT_RIGHT:
      lom->g_t_ptr_right = g_t64_ptr;
      break;
      }
    lom->g_lom_vp = g_lom_var_uint;
    lom->flags |= F_LOM_INT;
    break;
  default:
    return 608;
    break;
    }

  switch (flags & F_GLT_DIRECT)
    {
  case F_GLT_LEFT:
    lom->t_l_off = off;
    break;
  case F_GLT_RIGHT:
    lom->t_r_off = off;
    break;
    }

  return 0;

}

int
g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
    size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
    uint32_t flags)
{
  g_setjmp(0, "g_build_lom_packet", NULL, NULL);
  int rt = 0;
  md_init(&match->lom, 16);

  __g_lom lom = (__g_lom ) md_alloc(&match->lom, sizeof(_g_lom));

  if (!lom)
    {
      rt = 1;
      goto end;
    }

  int r = 0;

  if ((r = g_get_lom_g_t_ptr(hdl, left, lom, F_GLT_LEFT)))
    {
      rt = r;
      goto end;
    }

  char *r_ptr = right;

  if (!r_ptr)
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        r_ptr = "1.0";
        break;
      case F_LOM_INT:
        r_ptr = "1";
        break;
      case F_LOM_INT_S:
        r_ptr = "1";
        break;
        }
    }

  if ((r = g_get_lom_g_t_ptr(hdl, r_ptr, lom, F_GLT_RIGHT)))
    {
      rt = 4;
      goto end;
    }

  if ((lom->flags & F_LOM_FLOAT)
      && ((lom->flags & F_LOM_INT) | (lom->flags & F_LOM_INT_S)))
    {
      lom->flags ^= (F_LOM_INT | F_LOM_INT_S);
    }
  else if ((lom->flags & F_LOM_INT) && (lom->flags & F_LOM_INT_S))
    {
      lom->flags ^= F_LOM_INT;
    }

  if (!(lom->flags & F_LOM_TYPES))
    {
      rt = 6;
      goto end;
    }

  if (!comp)
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_s;
        break;
        }

    }
  else if (comp_l == 2 && !strncmp(comp, "==", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_equal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_equal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_equal_s;
        break;
        }

    }
  else if (comp_l == 1 && !strncmp(comp, "=", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_equal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_equal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_equal_s;
        break;
        }
    }
  else if (comp_l == 1 && !strncmp(comp, "<", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_lower_f_2;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_lower_2;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_lower_2_s;
        break;
        }
    }
  else if (comp_l == 1 && !strncmp(comp, ">", 1))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_higher_f_2;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_higher_2;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_higher_2_s;
        break;
        }
    }
  else if (comp_l == 2 && !strncmp(comp, "!=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_higherorequal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_not_equal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_not_equal_s;
        break;
        }
    }
  else if (comp_l == 2 && !strncmp(comp, ">=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_higherorequal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_higherorequal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_higherorequal_s;
        break;
        }
    }
  else if (comp_l == 2 && !strncmp(comp, "<=", 2))
    {
      switch (lom->flags & F_LOM_TYPES)
        {
      case F_LOM_FLOAT:
        lom->g_fcomp_ptr = g_is_lowerorequal_f;
        break;
      case F_LOM_INT:
        lom->g_icomp_ptr = g_is_lowerorequal;
        break;
      case F_LOM_INT_S:
        lom->g_iscomp_ptr = g_is_lowerorequal_s;
        break;
        }

    }
  else
    {
      rt = 11;
      goto end;
    }

  if (oper)
    {
      if (oper_l == 2 && !strncmp(oper, "&&", 2))
        {
          lom->g_oper_ptr = g_oper_and;
        }
      else if (oper_l == 2 && !strncmp(oper, "||", 2))
        {
          lom->g_oper_ptr = g_oper_or;
        }
      else
        {
          lom->g_oper_ptr = g_oper_and;
        }
    }

  if (ret)
    {
      *ret = lom;
    }

  end:

  if (rt)
    {
      md_unlink(&match->lom, match->lom.pos);
    }
  else
    {
      match->flags |= F_GM_ISLOM;
      match->flags |= flags;
      if (match->flags & F_GM_IMATCH)
        {
          match->match_i_m = G_NOMATCH;
        }
      else
        {
          match->match_i_m = G_MATCH;
        }
    }

  return rt;
}

__g_match
g_global_register_match(void)
{
  md_init(&_match_rr, 32);

  if (_match_rr.offset >= GM_MAX)
    {
      return NULL;
    }

  return (__g_match ) md_alloc(&_match_rr, sizeof(_g_match));

}

int
is_opr(char in)
{
  if (in == 0x21 || in == 0x26 || in == 0x3D || in == 0x3C || in == 0x3E
      || in == 0x7C)
    {
      return 0;
    }
  return 1;
}

int
get_opr(char *in)
{
  if (!strncmp(in, "&&", 2) || !strncmp(in, "||", 2))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

int
md_copy(pmda source, pmda dest, size_t block_sz)
{
  if (!source || !dest)
    {
      return 1;
    }

  if (dest->count)
    {
      return 2;
    }
  int ret = 0;
  p_md_obj ptr = md_first(source);
  void *d_ptr;

  md_init(dest, source->count);

  while (ptr)
    {
      d_ptr = md_alloc(dest, block_sz);
      if (!d_ptr)
        {
          ret = 10;
          break;
        }
      memcpy(d_ptr, ptr->ptr, block_sz);
      ptr = ptr->next;
    }

  if (ret)
    {
      md_g_free(dest);
    }

  if (source->offset != dest->offset)
    {
      return 3;
    }

  return 0;
}

#define MAX_EXEC_VAR_NAME_LEN   64

int
g_compile_exech(pmda mech, __g_handle hdl, char *instr)
{
  size_t pl, p1, vl1;
  char *in_ptr = instr;

  if (!in_ptr[0])
    {
      return 1;
    }

  md_init(mech, 16);
  __d_exec_ch ptr = md_alloc(mech, sizeof(_d_exec_ch));

  if (!ptr)
    {
      return 4;
    }

  ptr->st_ptr = in_ptr;

  for (p1 = 0, pl = 0; in_ptr[0]; p1++, in_ptr++, pl++)
    {
      if (in_ptr[0] == 0x7B)
        {
          ptr->len = pl;
          pl = 0;
          ptr = md_alloc(mech, sizeof(_d_exec_ch));
          if (!ptr)
            {
              return 9;
            }

          do_gcb: ;

          in_ptr++;
          ptr->st_ptr = in_ptr;
          ptr->dtr.hdl = hdl;
          vl1 = 0;
          ptr->callback = hdl->g_proc1_lookup(hdl->_x_ref, ptr->st_ptr, NULL, 0,
              &ptr->dtr);

          if (!ptr->callback)
            {
              return 10;
            }

          while (in_ptr[0] != 0x7D && in_ptr[0])
            {
              in_ptr++;
              vl1++;
            }
          if (!in_ptr[0])
            {
              return 11;
            }
          /*if (vl1 >= MAX_EXEC_VAR_NAME_LEN)
           {
           return 12;
           }*/

          ptr->len = vl1;
          ptr = md_alloc(mech, sizeof(_d_exec_ch));
          if (!ptr)
            {
              return 13;
            }

          if (in_ptr[1] && in_ptr[1] == 0x7B)
            {
              in_ptr++;
              goto do_gcb;
            }
          else
            {
              in_ptr++;
              ptr->st_ptr = in_ptr;
            }

          if (!in_ptr[0])
            {
              break;
            }
        }
    }

  ptr->len = pl;

  return 0;

}

#define M_EXECH_DCNBT   "ERROR: %s: could not build exec string, output too large\n"

char *
g_exech_build_string(void *d_ptr, pmda mech, __g_handle hdl, char *output,
    size_t maxlen)
{
  p_md_obj ptr = mech->objects;
  __d_exec_ch ch_ptr;

  size_t cw = 0;

  while (ptr)
    {
      ch_ptr = (__d_exec_ch ) ptr->ptr;
      if (!ch_ptr->callback)
        {
          if (!ch_ptr->len)
            {
              goto end_l;
            }
          if (cw + ch_ptr->len >= maxlen)
            {
              print_str(M_EXECH_DCNBT, hdl->file);
              break;
            }
          strncpy(output, ch_ptr->st_ptr, ch_ptr->len);
          output += ch_ptr->len;
          cw += ch_ptr->len;
        }
      else
        {
          char *rs_ptr = ch_ptr->callback(d_ptr, ch_ptr->st_ptr, hdl->mv1_b,
          MAX_VAR_LEN, &ch_ptr->dtr);
          if (rs_ptr)
            {
              size_t rs_len = strlen(rs_ptr);
              if (cw + rs_len >= maxlen)
                {
                  print_str(M_EXECH_DCNBT, hdl->file);
                  break;
                }
              if (!rs_len)
                {
                  goto end_l;
                }
              strncpy(output, rs_ptr, rs_len);
              output += rs_len;
              cw += rs_len;
            }
        }
      end_l:

      ptr = ptr->next;
    }
  output[0] = 0x0;
  return output;
}

int
g_build_argv_c(__g_handle hdl)
{
  md_init(&hdl->exec_args.ac_ref, hdl->exec_args.argc);

  hdl->exec_args.argv_c = calloc(amax / sizeof(char*), sizeof(char**));
  int i, r;
  __d_argv_ch ach;
  char *ptr;
  for (i = 0; i < hdl->exec_args.argc && hdl->exec_args.argv[i]; i++)
    {
      ptr = strchr(hdl->exec_args.argv[i], 0x7B);
      if (ptr)
        {
          hdl->exec_args.argv_c[i] = (char*) calloc(8192, 1);
          size_t t_l = strlen(hdl->exec_args.argv[i]);
          strncpy(hdl->exec_args.argv_c[i], hdl->exec_args.argv[i],
              t_l > 8191 ? 8191 : t_l);
          ach = md_alloc(&hdl->exec_args.ac_ref, sizeof(_d_argv_ch));
          ach->cindex = i;
          if ((r = g_compile_exech(&ach->mech, hdl, hdl->exec_args.argv[i])))
            {
              return r;
            }
        }
      else
        {
          hdl->exec_args.argv_c[i] = hdl->exec_args.argv[i];
        }
    }

  if (!i)
    {
      return -1;
    }

  return 0;
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
              print_str("ERROR: %s: empty -print command\n", hdl->file);
              return 2010;
            }
          if (pp_l > MAX_EXEC_STR)
            {
              print_str("ERROR: %s: -print string too large\n", hdl->file);
              return 2004;
            }
          if ((r = g_compile_exech(&hdl->print_mech, hdl, _print_ptr)))
            {
              print_str("ERROR: %s: [%d]: could not compile print string\n",
                  hdl->file, r);
              return 2009;
            }
        }
      if ((gfl0 & F_OPT_PRINT))
        {
          hdl->g_proc4 = g_omfp_eassemble;
        }
      else
        {
          hdl->g_proc4 = g_omfp_eassemblef;
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
g_load_lom(__g_handle hdl)
{
  g_setjmp(0, "g_load_lom", NULL, NULL);

  if ((hdl->flags & F_GH_HASLOM))
    {
      return 0;
    }

  int rt = 0;

  p_md_obj ptr = md_first(&hdl->_match_rr);
  __g_match _m_ptr;
  int r, ret, c = 0;
  while (ptr)
    {
      _m_ptr = (__g_match) ptr->ptr;
      if ( _m_ptr->flags & F_GM_ISLOM )
        {
          if ((r = g_process_lom_string(hdl, _m_ptr->data, _m_ptr, &ret,
                      _m_ptr->flags)))
            {
              print_str("ERROR: %s: [%d] [%d]: could not load LOM string\n",
                  hdl->file, r, ret);
              rt = 1;
              break;
            }
          c++;
        }
      ptr = ptr->next;
    }

  if (!rt)
    {
      hdl->flags |= F_GH_HASLOM;
      if (gfl & F_OPT_VERBOSE3)
        {
          print_str("NOTICE: %s: loaded %d LOM matches\n", hdl->file, c);
        }
    }
  else
    {
      print_str("ERROR: %s: [%d] LOM specified, but none could be loaded\n",
          hdl->file, rt);
      gfl |= F_OPT_KILL_GLOBAL;
      EXITVAL = 1;

    }

  return rt;
}

int
g_load_strm(__g_handle hdl)
{
  g_setjmp(0, "g_load_strm", NULL, NULL);

  if ((hdl->flags & F_GH_HASSTRM))
    {
      return 0;
    }

  int rt = 0;

  p_md_obj ptr = md_first(&hdl->_match_rr);
  __g_match _m_ptr;
  int c = 0;
  size_t i;

  while (ptr)
    {
      _m_ptr = (__g_match) ptr->ptr;
      if ( (_m_ptr->flags & F_GM_ISREGEX) || (_m_ptr->flags & F_GM_ISMATCH) )
        {
          i = 0;
          char *s_ptr = _m_ptr->data;
          while ((s_ptr[i] != 0x2C || (s_ptr[i] == 0x2C && s_ptr[i - 1] == 0x5C))
              && i < (size_t) 4096)
            {
              i++;
            }

          if (s_ptr[i] == 0x2C && i != (off_t) 4096)
            {
              _m_ptr->dtr.hdl = hdl;
              if (hdl->g_proc1_lookup && (_m_ptr->pmstr_cb = hdl->g_proc1_lookup(hdl->_x_ref, s_ptr, hdl->mv1_b, MAX_VAR_LEN, &_m_ptr->dtr)))
                {
                  _m_ptr->field = s_ptr;
                  s_ptr[i] = 0x0;
                  s_ptr = &s_ptr[i + 1];
                }
              else
                {
                  if ( !hdl->g_proc1_lookup )
                    {
                      print_str("ERROR: %s: MPARSE: lookup failed for '%s', no lookup table exists\n", hdl->file, s_ptr);
                      return 11;
                    }
                  if (gfl & F_OPT_VERBOSE3)
                    {
                      print_str("WARNING: %s: could not lookup field '%s', assuming it wasn't requested..\n", hdl->file, s_ptr);
                    }
                }
            }

          _m_ptr->match = s_ptr;
          if (_m_ptr->flags & F_GM_ISREGEX)
            {
              int re;
              if ((re = regcomp(&_m_ptr->preg, _m_ptr->match,
                          (_m_ptr->regex_flags | g_regex_flags | REG_NOSUB))))
                {
                  print_str("ERROR: %s: regex compilation failed : %d\n", hdl->file, re);
                  return 3;
                }
            }

          c++;
        }
      ptr = ptr->next;
    }

  if (!rt)
    {
      hdl->flags |= F_GH_HASSTRM;
      if (gfl & F_OPT_VERBOSE3)
        {
          print_str("NOTICE: %s: pre-processed %d string match filters\n",
              hdl->file, c);
        }
    }
  else
    {
      print_str(
          "ERROR: %s: [%d] string matches specified, but none could be processed\n",
          hdl->file, rt);

    }

  return rt;
}

#define MAX_LOM_VLEN 	254

int
g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
    uint32_t flags)
{
  g_setjmp(0, "g_process_lom_string", NULL, NULL);
  char *ptr = string, *w_ptr;
  char left[MAX_LOM_VLEN + 1], right[MAX_LOM_VLEN + 1], comp[4], oper[4];
  int i;
  while (ptr[0])
    {
      bzero(left, MAX_LOM_VLEN + 1);
      bzero(right, MAX_LOM_VLEN + 1);
      bzero(comp, 4);
      bzero(oper, 4);
      while (is_ascii_alphanumeric((uint8_t) ptr[0]) && ptr[0] != 0x2D
          && ptr[0] != 0x2B)
        {
          ptr++;
        }

      w_ptr = ptr;
      i = 0;
      while (!is_ascii_alphanumeric((uint8_t) ptr[0]) || ptr[0] == 0x2E
          || ptr[0] == 0x2B || ptr[0] == 0x2D)
        {
          i++;
          ptr++;
        }

      if (i > MAX_LOM_VLEN)
        {
          return 1;
        }

      strncpy(left, w_ptr, i);

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      i = 0;

      w_ptr = ptr;
      while (!is_opr(ptr[0]))
        {
          ptr++;
          i++;
        }

      if (i > 2)
        {
          return 2;
        }

      if (get_opr(w_ptr) == 1)
        {
          strncpy(oper, w_ptr, i);
          goto end_loop;
        }
      else
        {
          strncpy(comp, w_ptr, i);
        }

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      w_ptr = ptr;
      i = 0;
      while (!is_ascii_alphanumeric((uint8_t) ptr[0]) || ptr[0] == 0x2E
          || ptr[0] == 0x2B || ptr[0] == 0x2D)
        {
          i++;
          ptr++;
        }

      if (i > MAX_LOM_VLEN)
        {
          return 3;
        }

      strncpy(right, w_ptr, i);

      while (ptr[0] == 0x20)
        {
          ptr++;
        }

      i = 0;

      w_ptr = ptr;
      while (!is_opr(ptr[0]))
        {
          ptr++;
          i++;
        }

      if (i > 2)
        {
          return 4;
        }

      if (get_opr(w_ptr) == 1)
        {
          strncpy(oper, w_ptr, i);
        }

      end_loop: ;

      if (!strlen(left))
        {
          return 5;
        }

      char *r_ptr = (char*) right, *c_ptr = (char*) comp, *o_ptr = (char*) oper;

      if (!strlen(r_ptr))
        {
          r_ptr = NULL;
        }

      size_t comp_l = strlen(comp), oper_l = strlen(oper);

      if (!strlen(c_ptr))
        {
          c_ptr = NULL;
        }

      if (!strlen(o_ptr))
        {
          o_ptr = NULL;
        }

      *ret = g_build_lom_packet(hdl, left, r_ptr, c_ptr, comp_l, o_ptr, oper_l,
          _gm,
          NULL, flags);

      if (*ret)
        {
          return 6;
        }

    }

  return 0;
}

#define _MC_DIRLOG_FILES	"files"

void *
ref_to_val_ptr_dirlog(void *arg, char *match, int *output)
{
  struct dirlog *data = (struct dirlog *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->uptime));
      return &data->uptime;
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      *output = sizeof(data->uploader);
      return &data->uploader;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      *output = sizeof(data->group);
      return &data->group;
    }
  else if (!strncmp(match, _MC_DIRLOG_FILES, 5))
    {
      *output = sizeof(data->files);
      return &data->files;
    }
  else if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      *output = sizeof(data->bytes);
      return &data->bytes;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      *output = sizeof(data->status);
      return &data->status;
    }
  return NULL;
}

#define _MC_NUKELOG_MULT	"mult"

void *
ref_to_val_ptr_nukelog(void *arg, char *match, int *output)
{

  struct nukelog *data = (struct nukelog *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->nuketime));
      return &data->nuketime;
    }
  else if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      *output = -32;
      return &data->bytes;
    }
  else if (!strncmp(match, _MC_NUKELOG_MULT, 4))
    {
      *output = sizeof(data->mult);
      return &data->mult;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      *output = sizeof(data->status);
      return &data->status;
    }
  return NULL;
}

void *
ref_to_val_ptr_dupefile(void *arg, char *match, int *output)
{

  struct dupefile *data = (struct dupefile *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timeup));
      return &data->timeup;
    }
  return NULL;
}

void *
ref_to_val_ptr_lastonlog(void *arg, char *match, int *output)
{
  struct lastonlog *data = (struct lastonlog *) arg;

  if (!strncmp(match, _MC_GLOB_LOGON, 5))
    {
      *output = ~((int) sizeof(data->logon));
      return &data->logon;
    }
  else if (!strncmp(match, _MC_GLOB_LOGOFF, 6))
    {
      *output = ~((int) sizeof(data->logoff));
      return &data->logoff;
    }
  else if (!strncmp(match, _MC_GLOB_DOWNLOAD, 8))
    {
      *output = sizeof(data->download);
      return &data->download;
    }
  else if (!strncmp(match, _MC_GLOB_UPLOAD, 6))
    {
      *output = sizeof(data->upload);
      return &data->upload;
    }
  return NULL;
}

void *
ref_to_val_ptr_oneliners(void *arg, char *match, int *output)
{

  struct oneliner *data = (struct oneliner *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timestamp));
      return &data->timestamp;
    }
  return NULL;
}

#define _MC_ONLINE_BTXFER 	"btxfer"
#define _MC_ONLINE_BXFER 	"bxfer"
#define _MC_ONLINE_GROUP 	"group"
#define _MC_ONLINE_SSL 		"ssl"
#define _MC_ONLINE_LUPDT	"lupdtime"
#define _MC_ONLINE_LXFRT	"lxfertime"

void *
ref_to_val_ptr_online(void *arg, char *match, int *output)
{

  struct ONLINE *data = (struct ONLINE *) arg;

  if (!strncmp(match, _MC_ONLINE_BTXFER, 6))
    {
      *output = sizeof(data->bytes_txfer);
      return &data->bytes_txfer;
    }
  else if (!strncmp(match, _MC_ONLINE_BXFER, 5))
    {
      *output = sizeof(data->bytes_xfer);
      return &data->bytes_xfer;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      *output = ~((int) sizeof(data->groupid));
      return &data->groupid;
    }
  else if (!strncmp(match, _MC_GLOB_PID, 3))
    {
      *output = ~((int) sizeof(data->procid));
      return &data->procid;
    }
  else if (!strncmp(match, _MC_ONLINE_SSL, 3))
    {
      *output = ~((int) sizeof(data->ssl_flag));
      return &data->ssl_flag;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->login_time));
      return &data->login_time;
    }
  else if (!strncmp(match, _MC_ONLINE_LUPDT, 8))
    {
      *output = ~((int) sizeof(data->tstart.tv_sec));
      return &data->tstart.tv_sec;
    }
  else if (!strncmp(match, _MC_ONLINE_LXFRT, 9))
    {
      *output = ~((int) sizeof(data->txfer.tv_sec));
      return &data->txfer.tv_sec;
    }

  return NULL;
}

#define _MC_IMDB_RELEASED		"released"
#define _MC_IMDB_VOTES			"votes"
#define _MC_IMDB_YEAR			"year"

void *
ref_to_val_ptr_imdb(void *arg, char *match, int *output)
{
  __d_imdb data = (__d_imdb) arg;

  if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      *output = -32;
      return &data->rating;
    }
  else if (!strncmp(match, _MC_IMDB_RELEASED, 8))
    {
      *output = ~((int) sizeof(data->released));
      return &data->released;
    }
  else if (!strncmp(match, _MC_GLOB_RUNTIME, 7))
    {
      *output = sizeof(data->runtime);
      return &data->runtime;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timestamp));
      return &data->timestamp;
    }
  else if (!strncmp(match, _MC_IMDB_VOTES, 5))
    {
      *output = sizeof(data->votes);
      return &data->votes;
    }
  else if (!strncmp(match, _MC_IMDB_YEAR, 4))
    {
      *output = sizeof(data->year);
      return &data->year;
    }

  return NULL;
}

void *
ref_to_val_ptr_game(void *arg, char *match, int *output)
{
  __d_game data = (__d_game) arg;

  if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      *output = -32;
      return &data->rating;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int)sizeof(data->timestamp));
      return &data->timestamp;
    }

  return NULL;
}

#define _MC_TV_STARTED		"started"
#define _MC_TV_ENDED		"ended"
#define _MC_TV_SHOWID		"showid"
#define _MC_TV_SEASONS		"seasons"
#define _MC_TV_SYEAR		"startyear"
#define _MC_TV_EYEAR		"endyear"

void *
ref_to_val_ptr_tv(void *arg, char *match, int *output)
{

  __d_tvrage data = (__d_tvrage) arg;

  if (!strncmp(match, _MC_TV_STARTED, 7))
    {
      *output = ~((int)sizeof(data->started));
      return &data->started;
    }
  else if (!strncmp(match, _MC_GLOB_RUNTIME, 7))
    {
      *output = sizeof(data->runtime);
      return &data->runtime;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->timestamp));
      return &data->timestamp;
    }
  else if (!strncmp(match, _MC_TV_ENDED, 5))
    {
      *output = ~((int) sizeof(data->ended));
      return &data->ended;
    }
  else if (!strncmp(match, _MC_TV_SHOWID, 6))
    {
      *output = sizeof(data->showid);
      return &data->showid;
    }
  else if (!strncmp(match, _MC_TV_SEASONS, 7))
    {
      *output = sizeof(data->seasons);
      return &data->seasons;
    }
  else if (!strncmp(match, _MC_TV_SYEAR, 9))
    {
      *output = sizeof(data->startyear);
      return &data->startyear;
    }
  else if (!strncmp(match, _MC_TV_EYEAR, 7))
    {
      *output = sizeof(data->endyear);
      return &data->endyear;
    }

  return NULL;
}

void *
ref_to_val_ptr_gen1(void *arg, char *match, int *output)
{
  __d_generic_s2044 data = (__d_generic_s2044) arg;

  if (!strcmp(match, "i32"))
    {
      *output = sizeof(data->i32);
      return &data->i32;
    }

  return NULL;
}

#define _MC_GE_I1	"i1"
#define _MC_GE_I2	"i2"
#define _MC_GE_I3	"i3"
#define _MC_GE_I4	"i4"
#define _MC_GE_U1	"u1"
#define _MC_GE_U2	"u2"
#define _MC_GE_U3	"u3"
#define _MC_GE_U4	"u4"
#define _MC_GE_F1	"f1"
#define _MC_GE_F2	"f2"
#define _MC_GE_F3	"f3"
#define _MC_GE_F4	"f4"
#define _MC_GE_UL1	"ul1"
#define _MC_GE_UL2	"ul2"
#define _MC_GE_UL3	"ul3"
#define _MC_GE_UL4	"ul4"
#define _MC_GE_GE1	"ge1"
#define _MC_GE_GE2	"ge2"
#define _MC_GE_GE3	"ge3"
#define _MC_GE_GE4	"ge4"
#define _MC_GE_GE5	"ge5"
#define _MC_GE_GE6	"ge6"
#define _MC_GE_GE7	"ge7"
#define _MC_GE_GE8	"ge8"

void *
ref_to_val_ptr_gen2(void *arg, char *match, int *output)
{
  __d_generic_s1644 data = (__d_generic_s1644) arg;
  if (!strncmp(match, _MC_GE_I1, 2))
    {
      *output = ~((int) sizeof(data->i32_1));
      return &data->i32_1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      *output = ~((int) sizeof(data->i32_2));
      return &data->i32_2;
    }
  else if (!strncmp(match, _MC_GE_I3, 2))
    {
      *output = ~((int) sizeof(data->i32_3));
      return &data->i32_3;
    }
  else if (!strncmp(match, _MC_GE_I4, 2))
    {
      *output = ~((int) sizeof(data->i32_4));
      return &data->i32_4;
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_1;
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_2;
    }
  else if (!strncmp(match, _MC_GE_U3, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_3;
    }
  else if (!strncmp(match, _MC_GE_U4, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_4;
    }
  else if (!strncmp(match, _MC_GE_F1, 2))
    {
      *output = -32;
      return &data->f_1;
    }
  else if (!strncmp(match, _MC_GE_F2, 2))
    {
      *output = -32;
      return &data->f_2;
    }
  else if (!strncmp(match, _MC_GE_F3, 2))
    {
      *output = -32;
      return &data->f_3;
    }
  else if (!strncmp(match, _MC_GE_F4, 2))
    {
      *output = -32;
      return &data->f_4;
    }
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      *output = sizeof(data->ui64_1);
      return &data->ui64_1;
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      *output = sizeof(data->ui64_2);
      return &data->ui64_2;
    }
  else if (!strncmp(match, _MC_GE_UL3, 3))
    {
      *output = sizeof(data->ui64_3);
      return &data->ui64_3;
    }
  else if (!strncmp(match, _MC_GE_UL4, 3))
    {
      *output = sizeof(data->ui64_4);
      return &data->ui64_4;
    }
  return NULL;
}

void *
ref_to_val_ptr_gen3(void *arg, char *match, int *output)
{
  __d_generic_s800 data = (__d_generic_s800) arg;
  if (!strncmp(match, _MC_GE_I1, 2))
    {
      *output = ~((int) sizeof(data->i32_1));
      return &data->i32_1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      *output = ~((int) sizeof(data->i32_2));
      return &data->i32_2;
    }

  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_1;
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      *output = sizeof(data->ui32_1);
      return &data->ui32_2;
    }
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      *output = sizeof(data->ui64_1);
      return &data->ui64_1;
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      *output = sizeof(data->ui64_2);
      return &data->ui64_2;
    }

  return NULL;
}

void *
ref_to_val_ptr_dummy(void *arg, char *match, int *output)
{
  return NULL;
}

char *
g_basename(char *input)
{
  char *b_ptr = strrchr(input, 0x2F);

  if (!b_ptr)
    {
      return input;
    }
  return b_ptr + 1;
}

char *
g_dirname(char *input)
{
  char *b_ptr = strrchr(input, 0x2F);
  if (!b_ptr)
    {
      input[0] = 0x0;
    }
  else
    {
      b_ptr[0] = 0x0;
    }
  return input;
}

char *
dt_rval_dirlog_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((struct dirlog *) arg)->uploader);
  return output;
}

char *
dt_rval_dirlog_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((struct dirlog *) arg)->group);
  return output;
}

char *
dt_rval_dirlog_files(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((struct dirlog *) arg)->files);
  return output;
}

char *
dt_rval_dirlog_size(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu",
      (ulint64_t) ((struct dirlog *) arg)->bytes);
  return output;
}

char *
dt_rval_dirlog_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((struct dirlog *) arg)->status);
  return output;
}

char *
dt_rval_dirlog_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((struct dirlog *) arg)->uptime);
  return output;
}

char *
dt_rval_dirlog_mode_e(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((struct dirlog *) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_dirlog_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct dirlog *) arg)->dirname;
}

char *
dt_rval_dirlog_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((struct dirlog *) arg)->dirname);
}

char *
dt_rval_xg_dirlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT,
      ((struct dirlog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

char *
dt_rval_x_dirlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((struct dirlog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_dirlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return dt_rval_dirlog_user;
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return dt_rval_dirlog_basedir;
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return dt_rval_dirlog_dir;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return dt_rval_dirlog_group;
    }
  else if (!strncmp(match, _MC_DIRLOG_FILES, 5))
    {
      return dt_rval_dirlog_files;
    }
  else if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      return dt_rval_dirlog_size;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return dt_rval_dirlog_status;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_dirlog_time;
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return dt_rval_dirlog_mode_e;
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return dt_rval_x_dirlog;
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return dt_rval_xg_dirlog;
    }
  return NULL;
}

#define _MC_NUKELOG_NUKER		"nuker"
#define _MC_NUKELOG_UNNUKER		"unnuker"
#define _MC_NUKELOG_NUKEE		"nukee"
#define _MC_NUKELOG_REASON		"reason"

char *
dt_rval_nukelog_size(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%f", (float) ((struct nukelog *) arg)->bytes);
  return output;
}

char *
dt_rval_nukelog_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d",
      (int32_t) ((struct nukelog *) arg)->nuketime);
  return output;
}

char *
dt_rval_nukelog_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((struct nukelog *) arg)->status);
  return output;
}

char *
dt_rval_nukelog_mult(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((struct nukelog *) arg)->mult);
  return output;
}

char *
dt_rval_nukelog_mode_e(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((struct nukelog *) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_nukelog_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->dirname;
}

char *
dt_rval_nukelog_basedir_e(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((struct nukelog *) arg)->dirname);
}

char *
dt_rval_nukelog_nuker(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->nuker;
}

char *
dt_rval_nukelog_nukee(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->nukee;
}

char *
dt_rval_nukelog_unnuker(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->unnuker;
}

char *
dt_rval_nukelog_reason(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct nukelog *) arg)->reason;
}

char *
dt_rval_x_nukelog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((struct nukelog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_xg_nukelog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT,
      ((struct nukelog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_nukelog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      return dt_rval_nukelog_size;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_nukelog_time;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return dt_rval_nukelog_status;
    }
  else if (!strncmp(match, _MC_NUKELOG_MULT, 4))
    {
      return dt_rval_nukelog_mult;
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return dt_rval_nukelog_mode_e;
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return dt_rval_nukelog_dir;
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return dt_rval_nukelog_basedir_e;
    }
  else if (!strncmp(match, _MC_NUKELOG_NUKER, 5))
    {
      return dt_rval_nukelog_nuker;
    }
  else if (!strncmp(match, _MC_NUKELOG_NUKEE, 5))
    {
      return dt_rval_nukelog_nukee;
    }
  else if (!strncmp(match, _MC_NUKELOG_UNNUKER, 7))
    {
      return dt_rval_nukelog_unnuker;
    }
  else if (!strncmp(match, _MC_NUKELOG_REASON, 6))
    {
      return dt_rval_nukelog_reason;
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return dt_rval_x_nukelog;
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return dt_rval_xg_nukelog;
    }
  return NULL;
}

#define _MC_GLOB_FILE	"file"

char *
dt_rval_dupefile_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((struct dupefile *) arg)->timeup);
  return output;
}

char *
dt_rval_dupefile_file(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct dupefile *) arg)->filename;
}

char *
dt_rval_dupefile_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct dupefile *) arg)->uploader;
}

void *
ref_to_val_lk_dupefile(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_dupefile_time;
    }
  else if (!strncmp(match, _MC_GLOB_FILE, 4))
    {
      return dt_rval_dupefile_file;
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return dt_rval_dupefile_user;
    }
  return NULL;
}

#define _MC_LASTONLOG_STATS  	"stats"
#define _MC_GLOB_TAG		"tag"

char *
dt_rval_lastonlog_logon(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((struct lastonlog *) arg)->logon);
  return output;
}

char *
dt_rval_lastonlog_logoff(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d",
      (int32_t) ((struct lastonlog *) arg)->logoff);
  return output;
}

char *
dt_rval_lastonlog_upload(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%lu",
      (unsigned long) ((struct lastonlog *) arg)->upload);
  return output;
}

char *
dt_rval_lastonlog_download(void *arg, char *match, char *output,
    size_t max_size, void *mppd)
{
  snprintf(output, max_size, "%lu",
      (unsigned long) ((struct lastonlog *) arg)->download);
  return output;
}

char *
dt_rval_lastonlog_config(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char p_b0[64];
  int ic = 0;

  while (match[ic] != 0x7D && ic < 64)
    {
      p_b0[ic] = match[ic];
      ic++;
    }

  p_b0[ic] = 0x0;

  void *ptr = ref_to_val_get_cfgval(((struct lastonlog *) arg)->uname, p_b0,
  DEFPATH_USERS,
  F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, output, max_size);
  if (ptr)
    {
      return ptr;
    }

  return ref_to_val_get_cfgval(((struct lastonlog *) arg)->gname, p_b0,
  DEFPATH_GROUPS,
  F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, output, max_size);
}

char *
dt_rval_lastonlog_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->uname;
}

char *
dt_rval_lastonlog_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->gname;
}

char *
dt_rval_lastonlog_stats(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->stats;
}

char *
dt_rval_lastonlog_tag(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct lastonlog *) arg)->tagline;
}

void *
ref_to_val_lk_lastonlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_LOGON, 5))
    {
      return dt_rval_lastonlog_logon;
    }
  else if (!strncmp(match, _MC_GLOB_LOGOFF, 6))
    {
      return dt_rval_lastonlog_logoff;
    }
  else if (!strncmp(match, _MC_GLOB_UPLOAD, 6))
    {
      return dt_rval_lastonlog_upload;
    }
  else if (!strncmp(match, _MC_GLOB_DOWNLOAD, 8))
    {
      return dt_rval_lastonlog_download;
    }
  else if (!is_char_uppercase(match[0]))
    {
      return dt_rval_lastonlog_config;
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return dt_rval_lastonlog_user;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return dt_rval_lastonlog_group;
    }
  else if (!strncmp(match, _MC_LASTONLOG_STATS, 5))
    {
      return dt_rval_lastonlog_stats;
    }
  else if (!strncmp(match, _MC_GLOB_TAG, 3))
    {
      return dt_rval_lastonlog_tag;
    }
  return NULL;
}

#define _MC_ONELINERS_MSG	"msg"

char *
dt_rval_oneliners_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d",
      (int32_t) ((struct oneliner *) arg)->timestamp);
  return output;
}

char *
dt_rval_oneliners_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->uname;
}

char *
dt_rval_oneliners_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->gname;
}

char *
dt_rval_oneliners_tag(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->tagline;
}

char *
dt_rval_oneliners_msg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct oneliner *) arg)->message;
}

void *
ref_to_val_lk_oneliners(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_oneliners_time;
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return dt_rval_oneliners_user;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return dt_rval_oneliners_group;
    }
  else if (!strncmp(match, _MC_GLOB_TAG, 3))
    {
      return dt_rval_oneliners_tag;
    }
  else if (!strncmp(match, _MC_ONELINERS_MSG, 3))
    {
      return dt_rval_oneliners_msg;
    }
  return NULL;
}

#define _MC_ONLINE_HOST		"host"

char *
dt_rval_online_ssl(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hd", ((struct ONLINE *) arg)->ssl_flag);
  return output;
}

char *
dt_rval_online_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((struct ONLINE *) arg)->groupid);
  return output;
}

char *
dt_rval_online_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d",
      (int32_t) ((struct ONLINE *) arg)->login_time);
  return output;
}

char *
dt_rval_online_lupdt(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d",
      (int32_t) ((struct ONLINE *) arg)->tstart.tv_sec);
  return output;
}

char *
dt_rval_online_lxfrt(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d",
      (int32_t) ((struct ONLINE *) arg)->txfer.tv_sec);
  return output;
}

char *
dt_rval_online_bxfer(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu",
      (ulint64_t) ((struct ONLINE *) arg)->bytes_xfer);
  return output;
}

char *
dt_rval_online_btxfer(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu",
      (ulint64_t) ((struct ONLINE *) arg)->bytes_txfer);
  return output;
}

char *
dt_rval_online_pid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((struct ONLINE *) arg)->procid);
  return output;
}

char *
dt_rval_online_rate(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  int32_t tdiff = (int32_t) time(NULL) - ((struct ONLINE *) arg)->tstart.tv_sec;
  uint32_t kbps = 0;

  if (tdiff > 0 && ((struct ONLINE *) arg)->bytes_xfer > 0)
    {
      kbps = ((struct ONLINE *) arg)->bytes_xfer / tdiff;
    }
  snprintf(output, max_size, "%u", kbps);
  return output;
}

char *
dt_rval_online_config(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char p_b0[64];
  int ic = 0;

  while (match[ic] != 0x7D && ic < 64)
    {
      p_b0[ic] = match[ic];
      ic++;
    }

  p_b0[ic] = 0x0;

  return ref_to_val_get_cfgval(((struct ONLINE *) arg)->username, p_b0,
  DEFPATH_USERS,
  F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, output, max_size);
}

char *
dt_rval_online_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((struct ONLINE *) arg)->currentdir);
}

char *
dt_rval_online_ndir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strcp_s(output, max_size, ((struct ONLINE *) arg)->currentdir);
  return g_dirname(output);
}

char *
dt_rval_online_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->username;
}

char *
dt_rval_online_tag(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->tagline;
}

char *
dt_rval_online_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->status;
}

char *
dt_rval_online_host(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->host;
}

char *
dt_rval_online_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct ONLINE *) arg)->currentdir;
}

void *
ref_to_val_lk_online(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_ONLINE_SSL, 3))
    {
      return dt_rval_online_ssl;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return dt_rval_online_group;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_online_time;
    }
  else if (!strncmp(match, _MC_ONLINE_LUPDT, 8))
    {
      return dt_rval_online_lupdt;
    }
  else if (!strncmp(match, _MC_ONLINE_LXFRT, 9))
    {
      return dt_rval_online_lxfrt;
    }
  else if (!strncmp(match, _MC_ONLINE_BXFER, 5))
    {
      return dt_rval_online_bxfer;
    }
  else if (!strncmp(match, _MC_ONLINE_BTXFER, 6))
    {
      return dt_rval_online_btxfer;
    }
  else if (!strncmp(match, _MC_GLOB_PID, 3))
    {
      return dt_rval_online_pid;
    }
  else if (!strncmp(match, "rate", 4))
    {
      return dt_rval_online_rate;
    }
  else if (!is_char_uppercase(match[0]))
    {
      return dt_rval_online_config;
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return dt_rval_online_basedir;
    }
  else if (!strncmp(match, _MC_GLOB_DIRNAME, 4))
    {
      return dt_rval_online_ndir;
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return dt_rval_online_user;
    }
  else if (!strncmp(match, _MC_GLOB_TAG, 3))
    {
      return dt_rval_online_tag;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return dt_rval_online_status;
    }
  else if (!strncmp(match, _MC_ONLINE_HOST, 4))
    {
      return dt_rval_online_host;
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return dt_rval_online_dir;
    }
  return NULL;
}

#define _MC_IMDB_ACTORS		"actors"
#define _MC_IMDB_TITLE		"title"
#define _MC_IMDB_IMDBID		"imdbid"
#define _MC_GLOB_GENRE		"genre"
#define _MC_IMDB_RATED		"rated"
#define _MC_IMDB_DIRECT		"director"
#define _MC_IMDB_SYNOPSIS	"plot"

char *
dt_rval_imdb_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((__d_imdb) arg)->timestamp);
  return output;
}

char *
dt_rval_imdb_score(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.1f", ((__d_imdb) arg)->rating);
  return output;
}

char *
dt_rval_imdb_votes(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", (uint32_t) ((__d_imdb) arg)->votes);
  return output;
}

char *
dt_rval_imdb_runtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_imdb) arg)->runtime);
  return output;
}

char *
dt_rval_imdb_released(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((__d_imdb) arg)->released);
  return output;
}

char *
dt_rval_imdb_year(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hu", ((__d_imdb) arg)->year);
  return output;
}

char *
dt_rval_imdb_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((__d_imdb) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_imdb_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((__d_imdb) arg)->dirname);
}

char *
dt_rval_imdb_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->dirname;
}

char *
dt_rval_imdb_imdbid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->imdb_id;
}

char *
dt_rval_imdb_genre(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->genres;
}

char *
dt_rval_imdb_rated(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->rated;
}

char *
dt_rval_imdb_title(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->title;
}

char *
dt_rval_imdb_director(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->director;
}

char *
dt_rval_imdb_actors(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->actors;
}

char *
dt_rval_imdb_synopsis(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_imdb) arg)->synopsis;
}

char *
dt_rval_imdb_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((__d_imdb) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_imdb_xg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT, ((__d_imdb) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_imdb(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_imdb_time;
    }
  else if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      return dt_rval_imdb_score;
    }
  else if (!strncmp(match, _MC_IMDB_VOTES, 5))
    {
      return dt_rval_imdb_votes;
    }
  else if (!strncmp(match, _MC_GLOB_RUNTIME, 7))
    {
      return dt_rval_imdb_runtime;
    }
  else if (!strncmp(match, _MC_IMDB_RELEASED, 8))
    {
      return dt_rval_imdb_released;
    }
  else if (!strncmp(match, _MC_IMDB_YEAR, 4))
    {
      return dt_rval_imdb_year;
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return dt_rval_imdb_mode;
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return dt_rval_imdb_basedir;
    }
  else if (!strncmp(match, _MC_IMDB_IMDBID, 6))
    {
      return dt_rval_imdb_imdbid;
    }
  else if (!strncmp(match, _MC_GLOB_GENRE, 5))
    {
      return dt_rval_imdb_genre;
    }
  else if (!strncmp(match, _MC_IMDB_RATED, 5))
    {
      return dt_rval_imdb_rated;
    }
  else if (!strncmp(match, _MC_IMDB_TITLE, 5))
    {
      return dt_rval_imdb_title;
    }
  else if (!strncmp(match, _MC_IMDB_DIRECT, 8))
    {
      return dt_rval_imdb_director;
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return dt_rval_imdb_dir;
    }
  else if (!strncmp(match, _MC_IMDB_ACTORS, 6))
    {
      return dt_rval_imdb_actors;
    }
  else if (!strncmp(match, _MC_IMDB_SYNOPSIS, 4))
    {
      return dt_rval_imdb_synopsis;
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return dt_rval_imdb_x;
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return dt_rval_imdb_xg;
    }
  return NULL;
}

char *
dt_rval_game_score(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.1f", ((__d_game) arg)->rating);
  return output;
}

char *
dt_rval_game_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((__d_game) arg)->timestamp);
  return output;
}

char *
dt_rval_game_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((__d_game) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_game_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((__d_game) arg)->dirname);
}

char *
dt_rval_game_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_game) arg)->dirname;
}

char *
dt_rval_game_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((__d_game) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_game_xg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT, ((__d_game) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_game(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GLOB_SCORE, 5))
    {
      return dt_rval_game_score;
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_game_time;
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return dt_rval_game_mode;
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return dt_rval_game_basedir;
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return dt_rval_game_dir;
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return dt_rval_game_x;
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return dt_rval_game_xg;
    }
  return NULL;
}

#define _MC_TV_AIRDAY		"airday"
#define _MC_TV_AIRTIME		"airtime"
#define _MC_TV_COUNTRY		"country"
#define _MC_TV_LINK		"link"
#define _MC_TV_NAME		"name"
#define _MC_TV_CLASS		"class"
#define _MC_TV_NETWORK		"network"

char *
dt_rval_tvrage_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->dirname;
}

char *
dt_rval_tvrage_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((__d_tvrage) arg)->dirname);
}

char *
dt_rval_tvrage_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((__d_tvrage) arg)->timestamp);
  return output;
}

char *
dt_rval_tvrage_ended(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((__d_tvrage) arg)->ended);
  return output;
}

char *
dt_rval_tvrage_started(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", (int32_t) ((__d_tvrage) arg)->started);
  return output;
}

char *
dt_rval_tvrage_seasons(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_tvrage) arg)->seasons);
  return output;
}

char *
dt_rval_tvrage_showid(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_tvrage) arg)->showid);
  return output;
}

char *
dt_rval_tvrage_runtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_tvrage) arg)->runtime);
  return output;
}

char *
dt_rval_tvrage_startyear(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_tvrage) arg)->startyear);
  return output;
}

char *
dt_rval_tvrage_endyear(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_tvrage) arg)->endyear);
  return output;
}

char *
dt_rval_tvrage_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((__d_tvrage) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_tvrage_airday(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->airday;
}

char *
dt_rval_tvrage_airtime(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->airtime;
}

char *
dt_rval_tvrage_country(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->country;
}

char *
dt_rval_tvrage_link(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->link;
}

char *
dt_rval_tvrage_name(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->name;
}

char *
dt_rval_tvrage_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->status;
}

char *
dt_rval_tvrage_class(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->class;
}

char *
dt_rval_tvrage_genre(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->genres;
}

char *
dt_rval_tvrage_network(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_tvrage) arg)->network;
}

char *
dt_rval_tvrage_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((__d_tvrage) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

char *
dt_rval_tvrage_xg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT, ((__d_tvrage) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_tvrage(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }
  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return dt_rval_tvrage_time;
    }
  else if (!strncmp(match, _MC_TV_ENDED, 5))
    {
      return dt_rval_tvrage_ended;
    }
  else if (!strncmp(match, _MC_TV_STARTED, 7))
    {
      return dt_rval_tvrage_started;
    }
  else if (!strncmp(match, _MC_TV_SEASONS, 7))
    {
      return dt_rval_tvrage_seasons;
    }
  else if (!strncmp(match, _MC_TV_SHOWID, 6))
    {
      return dt_rval_tvrage_showid;
    }
  else if (!strncmp(match, _MC_GLOB_RUNTIME, 7))
    {
      return dt_rval_tvrage_runtime;
    }
  else if (!strncmp(match, _MC_TV_SYEAR, 9))
    {
      return dt_rval_tvrage_startyear;
    }
  else if (!strncmp(match, _MC_TV_EYEAR, 7))
    {
      return dt_rval_tvrage_endyear;
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return dt_rval_tvrage_mode;
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return dt_rval_tvrage_dir;
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return dt_rval_tvrage_basedir;
    }
  else if (!strncmp(match, _MC_TV_AIRDAY, 6))
    {
      return dt_rval_tvrage_airday;
    }
  else if (!strncmp(match, _MC_TV_AIRTIME, 7))
    {
      return dt_rval_tvrage_airtime;
    }
  else if (!strncmp(match, _MC_TV_COUNTRY, 7))
    {
      return dt_rval_tvrage_country;
    }
  else if (!strncmp(match, _MC_TV_LINK, 4))
    {
      return dt_rval_tvrage_link;
    }
  else if (!strncmp(match, _MC_TV_NAME, 4))
    {
      return dt_rval_tvrage_name;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return dt_rval_tvrage_status;
    }
  else if (!strncmp(match, _MC_TV_CLASS, 5))
    {
      return dt_rval_tvrage_class;
    }
  else if (!strncmp(match, _MC_GLOB_GENRE, 5))
    {
      return dt_rval_tvrage_genre;
    }
  else if (!strncmp(match, _MC_TV_NETWORK, 7))
    {
      return dt_rval_tvrage_network;
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return dt_rval_tvrage_x;
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return dt_rval_tvrage_xg;
    }

  return NULL;
}

char *
dt_rval_gen1_i32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s2044) arg)->i32);
  return output;
}

char *
dt_rval_gen1_ge1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_1;
}

char *
dt_rval_gen1_ge2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_2;
}

char *
dt_rval_gen1_ge3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_3;
}

char *
dt_rval_gen1_ge4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_4;
}

char *
dt_rval_gen1_ge5(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_5;
}

char *
dt_rval_gen1_ge6(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_6;
}

char *
dt_rval_gen1_ge7(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_7;
}

char *
dt_rval_gen1_ge8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s2044) arg)->s_8;
}

void *
ref_to_val_lk_gen1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, "i32", 3))
    {
      return dt_rval_gen1_i32;
    }
  else if (!strncmp(match, _MC_GE_GE1, 3))
    {
      return dt_rval_gen1_ge1;
    }
  else if (!strncmp(match, _MC_GE_GE2, 3))
    {
      return dt_rval_gen1_ge2;
    }
  else if (!strncmp(match, _MC_GE_GE3, 3))
    {
      return dt_rval_gen1_ge3;
    }
  else if (!strncmp(match, _MC_GE_GE4, 3))
    {
      return dt_rval_gen1_ge4;
    }
  else if (!strncmp(match, _MC_GE_GE5, 3))
    {
      return dt_rval_gen1_ge5;
    }
  else if (!strncmp(match, _MC_GE_GE6, 3))
    {
      return dt_rval_gen1_ge6;
    }
  else if (!strncmp(match, _MC_GE_GE7, 3))
    {
      return dt_rval_gen1_ge7;
    }
  else if (!strncmp(match, _MC_GE_GE8, 3))
    {
      return dt_rval_gen1_ge8;
    }

  return NULL;
}

char *
dt_rval_gen2_i1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", ((__d_generic_s1644) arg)->i32_1);
  return output;
}

char *
dt_rval_gen2_i2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", ((__d_generic_s1644) arg)->i32_2);
  return output;
}

char *
dt_rval_gen2_i3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", ((__d_generic_s1644) arg)->i32_3);
  return output;
}

char *
dt_rval_gen2_i4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", ((__d_generic_s1644) arg)->i32_4);
  return output;
}

char *
dt_rval_gen2_ui1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s1644) arg)->ui32_1);
  return output;
}

char *
dt_rval_gen2_ui2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s1644) arg)->ui32_2);
  return output;
}

char *
dt_rval_gen2_ui3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s1644) arg)->ui32_3);
  return output;
}

char *
dt_rval_gen2_ui4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s1644) arg)->ui32_4);
  return output;
}

char *
dt_rval_gen2_uli1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", (ulint64_t) ((__d_generic_s1644) arg)->ui64_1);
  return output;
}

char *
dt_rval_gen2_uli2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", (ulint64_t) ((__d_generic_s1644) arg)->ui64_2);
  return output;
}

char *
dt_rval_gen2_uli3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", (ulint64_t) ((__d_generic_s1644) arg)->ui64_3);
  return output;
}

char *
dt_rval_gen2_uli4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", (ulint64_t) ((__d_generic_s1644) arg)->ui64_4);
  return output;
}

char *
dt_rval_gen2_f1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%f", ((__d_generic_s1644) arg)->f_1);
  return output;
}

char *
dt_rval_gen2_f2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%f", ((__d_generic_s1644) arg)->f_2);
  return output;
}

char *
dt_rval_gen2_f3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%f", ((__d_generic_s1644) arg)->f_3);
  return output;
}

char *
dt_rval_gen2_f4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%f", ((__d_generic_s1644) arg)->f_4);
  return output;
}

char *
dt_rval_gen2_ge1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_1;
}

char *
dt_rval_gen2_ge2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_2;
}

char *
dt_rval_gen2_ge3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_3;
}

char *
dt_rval_gen2_ge4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_4;
}

char *
dt_rval_gen2_ge5(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_5;
}

char *
dt_rval_gen2_ge6(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_6;
}

char *
dt_rval_gen2_ge7(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_7;
}

char *
dt_rval_gen2_ge8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s1644) arg)->s_8;
}

void *
ref_to_val_lk_gen2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GE_I1, 2))
    {
      return dt_rval_gen2_i1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      return dt_rval_gen2_i2;
    }
  else if (!strncmp(match, _MC_GE_I3, 2))
    {
      return dt_rval_gen2_i3;
    }
  else if (!strncmp(match, _MC_GE_I4, 2))
    {
      return dt_rval_gen2_i4;
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      return dt_rval_gen2_ui1;
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      return dt_rval_gen2_ui2;
    }
  else if (!strncmp(match, _MC_GE_U3, 2))
    {
      return dt_rval_gen2_ui3;
    }
  else if (!strncmp(match, _MC_GE_U4, 2))
    {
      return dt_rval_gen2_ui4;
    }
  else if (!strncmp(match, _MC_GE_F1, 2))
    {
      return dt_rval_gen2_f1;
    }
  else if (!strncmp(match, _MC_GE_F2, 2))
    {
      return dt_rval_gen2_f2;
    }
  else if (!strncmp(match, _MC_GE_F3, 2))
    {
      return dt_rval_gen2_f3;
    }
  else if (!strncmp(match, _MC_GE_F4, 2))
    {
      return dt_rval_gen2_f4;
    }
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      return dt_rval_gen2_uli1;
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      return dt_rval_gen2_uli2;
    }
  else if (!strncmp(match, _MC_GE_UL3, 3))
    {
      return dt_rval_gen2_uli3;
    }
  else if (!strncmp(match, _MC_GE_UL4, 3))
    {
      return dt_rval_gen2_uli4;
    }
  else if (!strncmp(match, _MC_GE_GE1, 3))
    {
      return dt_rval_gen2_ge1;
    }
  else if (!strncmp(match, _MC_GE_GE2, 3))
    {
      return dt_rval_gen2_ge2;
    }
  else if (!strncmp(match, _MC_GE_GE3, 3))
    {
      return dt_rval_gen2_ge3;
    }
  else if (!strncmp(match, _MC_GE_GE4, 3))
    {
      return dt_rval_gen2_ge4;
    }
  else if (!strncmp(match, _MC_GE_GE5, 3))
    {
      return dt_rval_gen2_ge5;
    }
  else if (!strncmp(match, _MC_GE_GE6, 3))
    {
      return dt_rval_gen2_ge6;
    }
  else if (!strncmp(match, _MC_GE_GE7, 3))
    {
      return dt_rval_gen2_ge7;
    }
  else if (!strncmp(match, _MC_GE_GE8, 3))
    {
      return dt_rval_gen2_ge8;
    }

  return NULL;
}

char *
dt_rval_gen3_i1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", ((__d_generic_s800) arg)->i32_1);
  return output;
}

char *
dt_rval_gen3_i2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%d", ((__d_generic_s800) arg)->i32_2);
  return output;
}

char *
dt_rval_gen3_ui1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s800) arg)->ui32_1);
  return output;
}

char *
dt_rval_gen3_ui1_hex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.8X", ((__d_generic_s800) arg)->ui32_1);
  return output;
}

char *
dt_rval_gen3_ui1_shex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.8x", ((__d_generic_s800) arg)->ui32_1);
  return output;
}

char *
dt_rval_gen3_ui2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%u", ((__d_generic_s800) arg)->ui32_2);
  return output;
}

char *
dt_rval_gen3_ui2_hex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.8X", ((__d_generic_s800) arg)->ui32_2);
  return output;
}

char *
dt_rval_gen3_ui2_shex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.8x", ((__d_generic_s800) arg)->ui32_2);
  return output;
}

char *
dt_rval_gen3_uli1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", (ulint64_t) ((__d_generic_s800) arg)->ui64_1);
  return output;
}

char *
dt_rval_gen3_uli1_hex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.16llX", (ulint64_t) ((__d_generic_s800) arg)->ui64_1);
  return output;
}

char *
dt_rval_gen3_uli1_shex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.16llx", (ulint64_t) ((__d_generic_s800) arg)->ui64_1);
  return output;
}

char *
dt_rval_gen3_uli2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%llu", (ulint64_t) ((__d_generic_s800) arg)->ui64_2);
  return output;
}

char *
dt_rval_gen3_uli2_hex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.16llX", (ulint64_t) ((__d_generic_s800) arg)->ui64_2);
  return output;
}

char *
dt_rval_gen3_uli2_shex(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%.16llx", (ulint64_t) ((__d_generic_s800) arg)->ui64_2);
  return output;
}

char *
dt_rval_gen3_ge1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s800) arg)->s_1;
}

char *
dt_rval_gen3_ge2(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s800) arg)->s_2;
}

char *
dt_rval_gen3_ge3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s800) arg)->s_3;
}

char *
dt_rval_gen3_ge4(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_generic_s800) arg)->s_4;
}

void *
ref_to_val_lk_gen3(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GE_I1, 2))
    {
      return dt_rval_gen3_i1;
    }
  else if (!strncmp(match, _MC_GE_I2, 2))
    {
      return dt_rval_gen3_i2;
    }
  else if (!strncmp(match, _MC_GE_U1, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen3_ui1, (__d_drt_h) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_U2, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_gen3_ui2 , (__d_drt_h) mppd, "%u");
    }
  else if (!strncmp(match, _MC_GE_UL1, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen3_uli1, (__d_drt_h) mppd, "%llu");
    }
  else if (!strncmp(match, _MC_GE_UL2, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_gen3_uli2, (__d_drt_h) mppd, "%llu");
    }
  else if (!strncmp(match, _MC_GE_GE1, 3))
    {
      return dt_rval_gen3_ge1;
    }
  else if (!strncmp(match, _MC_GE_GE2, 3))
    {
      return dt_rval_gen3_ge2;
    }
  else if (!strncmp(match, _MC_GE_GE3, 3))
    {
      return dt_rval_gen3_ge3;
    }
  else if (!strncmp(match, _MC_GE_GE4, 3))
    {
      return dt_rval_gen3_ge4;
    }

  return NULL;
}

int
process_exec_string(char *input, char *output, size_t max_size, void *callback,
    void *data)
{

  int
  (*call)(void *, char *, char *, size_t) = callback;

  size_t blen = strlen(input);

  if (!blen || blen > MAX_EXEC_STR)
    {
      return 1;
    }

  size_t b_l_1;
  char buffer[255] =
    { 0 }, buffer2[MAX_VAR_LEN] =
    { 0 };
  int i, i2, pi, r = 0, f;

  for (i = 0, pi = 0; i < blen; i++, pi++)
    {
      if (input[i] == 0x7B)
        {
          for (i2 = 0, i++, f = 0, r = 0; i < blen && i2 < 255; i++, i2++)
            {
              if (input[i] == 0x7D)
                {
                  buffer[i2] = 0x0;
                  if (!i2 || strlen(buffer) > MAX_VAR_LEN
                      || (r = call(data, buffer, buffer2, MAX_VAR_LEN)))
                    {
                      if (r)
                        {
                          b_l_1 = strlen(buffer);
                          snprintf(&output[pi],
                          MAX_EXEC_STR - pi - 2, "%c%s%c", 0x7B, buffer, 0x7D);

                          pi += b_l_1 + 2;
                        }
                      f |= 0x1;
                      break;
                    }
                  b_l_1 = strlen(buffer2);
                  memcpy(&output[pi], buffer2, b_l_1);

                  pi += b_l_1;
                  f |= 0x1;
                  break;
                }
              buffer[i2] = input[i];
            }

          if ((f & 0x1))
            {
              pi -= 1;
              continue;
            }
        }
      output[pi] = input[i];
    }

  output[pi] = 0x0;

  return 0;
}

void
child_sig_handler(int signal, siginfo_t * si, void *p)
{
  switch (si->si_code)
    {
  case CLD_KILLED:
    print_str(
        "NOTICE: Child process caught SIGINT (hit CTRL^C again to quit)\n");
    usleep(1000000);
    break;
  case CLD_EXITED:
    break;
  default:
    if (gfl & F_OPT_VERBOSE3)
      {
        print_str("NOTICE: Child caught signal: %d \n", si->si_code);
      }
    break;
    }
}

#define SIG_BREAK_TIMEOUT_NS (useconds_t)1000000.0

void
sig_handler(int signal)
{
  switch (signal)
    {
  case SIGTERM:
    print_str("NOTICE: Caught SIGTERM, terminating gracefully.\n");
    gfl |= F_OPT_KILL_GLOBAL;
    break;
  case SIGINT:
    if (gfl & F_OPT_KILL_GLOBAL)
      {
        print_str("NOTICE: Caught SIGINT twice, terminating..\n");
        exit(0);
      }
    else
      {
        print_str(
            "NOTICE: Caught SIGINT, quitting (hit CTRL^C again to terminate by force)\n");
        gfl |= F_OPT_KILL_GLOBAL;
      }
    usleep(SIG_BREAK_TIMEOUT_NS);
    break;
  default:
    usleep(SIG_BREAK_TIMEOUT_NS);
    print_str("NOTICE: Caught signal %d\n", signal);
    break;
    }
}

int
g_shmap_data(__g_handle hdl, key_t ipc)
{
  g_setjmp(0, "g_shmap_data", NULL, NULL);
  if (hdl->shmid)
    {
      return 1001;
    }

  if ((hdl->shmid = shmget(ipc, 0, 0)) == -1)
    {
      return 1002;
    }

  if ((hdl->data = shmat(hdl->shmid, NULL, SHM_RDONLY)) == (void*) -1)
    {
      hdl->data = NULL;
      return 1003;
    }

  if (shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf) == -1)
    {
      return 1004;
    }

  if (!hdl->ipcbuf.shm_segsz)
    {
      return 1005;
    }

  hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;

  return 0;
}

void *
shmap(key_t ipc, struct shmid_ds *ipcret, size_t size, uint32_t *ret,
    int *shmid)
{
  g_setjmp(0, "shmap", NULL, NULL);

  void *ptr;
  int ir = 0;

  int shmflg = 0;

  int i_shmid = -1;

  if (!shmid)
    {
      shmid = &i_shmid;
    }

  if (*ret & R_SHMAP_ALREADY_EXISTS)
    {
      ir = 1;
    }
  else if ((*shmid = shmget(ipc, size,
  IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) == -1)
    {
      if ( errno == EEXIST)
        {

          if (ret)
            {
              *ret |= R_SHMAP_ALREADY_EXISTS;
            }
          if ((*shmid = shmget(ipc, 0, 0)) == -1)
            {
              if (ret)
                {
                  *ret |= R_SHMAP_FAILED_ATTACH;
                }
              return NULL;
            }
          //shmflg |= SHM_RDONLY;
        }
      else
        {
          return NULL;
        }
    }

  if ((ptr = shmat(*shmid, NULL, shmflg)) == (void*) -1)
    {
      if (ret)
        {
          *ret |= R_SHMAP_FAILED_SHMAT;
        }
      return NULL;
    }

  if (!ipcret)
    {
      return ptr;
    }

  if (ir != 1)
    {
      if (shmctl(*shmid, IPC_STAT, ipcret) == -1)
        {
          return NULL;
        }
    }

  return ptr;
}

pmda
search_cfg_rf(pmda md, char * file)
{
  p_md_obj ptr = md_first(md);
  p_cfg_r ptr_c;
  size_t fn_len = strlen(file);
  while (ptr)
    {
      ptr_c = (p_cfg_r) ptr->ptr;
      if (ptr_c && !strncmp(ptr_c->file, file, fn_len))
        {
          return &ptr_c->cfg;
        }
      ptr = ptr->next;
    }
  return NULL;
}

pmda
register_cfg_rf(pmda md, char *file)
{
  if (!md->count)
    {
      if (md_init(md, 128))
        {
          return NULL;
        }
    }

  pmda pmd = search_cfg_rf(md, file);

  if (pmd)
    {
      return pmd;
    }

  size_t fn_len = strlen(file);

  if (fn_len >= PATH_MAX)
    {
      return NULL;
    }
  g_setjmp(0, "register_cfg_rf-2", NULL, NULL);
  p_cfg_r ptr_c = md_alloc(md, sizeof(cfg_r));

  strncpy(ptr_c->file, file, fn_len);
  md_init(&ptr_c->cfg, 256);

  return &ptr_c->cfg;
}

int
free_cfg_rf(pmda md)
{
  if (!md || !md->count)
    {
      return 0;
    }

  p_md_obj ptr = md_first(md);
  p_cfg_r ptr_c;
  while (ptr)
    {
      ptr_c = (p_cfg_r) ptr->ptr;
      free_cfg(&ptr_c->cfg);
      ptr = ptr->next;
    }

  return md_g_free(md);
}

off_t
s_string(char *input, char *m, off_t offset)
{
  off_t i, m_l = strlen(m), i_l = strlen(input);
  for (i = offset; i <= i_l - m_l; i++)
    {
      if (!strncmp(&input[i], m, m_l))
        {
          return i;
        }
    }
  return offset;
}

off_t
s_string_r(char *input, char *m)
{
  off_t i, m_l = strlen(m), i_l = strlen(input);

  for (i = i_l - 1 - m_l; i >= 0; i--)
    {
      if (!strncmp(&input[i], m, m_l))
        {
          return i;
        }
    }
  return (off_t) -1;
}

#define MAX_SDENTRY_LEN		20000
#define MAX_SENTRY_LEN		1024

int
m_load_input_n(__g_handle hdl, FILE *input)
{
  g_setjmp(0, "m_load_input_n", NULL, NULL);

  if (!hdl->w_buffer.objects)
    {
      md_init(&hdl->w_buffer, 256);
    }

  if (!hdl->g_proc0)
    {
      return -1;
    }

  if (!hdl->d_memb)
    {
      return -2;
    }

  char *buffer = malloc(MAX_SDENTRY_LEN + 1);
  buffer[0] = 0x0;

  int i, rf = -9;

  char *l_ptr;
  void *st_buffer = NULL;
  uint32_t rw = 0, c = 0;

  while ((l_ptr = fgets(buffer, MAX_SDENTRY_LEN, input)) != NULL)
    {
      rf = 1;

      if (buffer[0] == 0xA)
        {
          if (!rw)
            {
              if (c)
                {
                  rf = 0;
                }
              break;
            }
          if (rw == hdl->d_memb)
            {
              rw = 0;
              rf = 0;
              c++;
            }
          else
            {
              print_str(
                  "ERROR: DATA IMPORT: [%d/%d] parameter count mismatch\n", rw,
                  hdl->d_memb);
              break;
            }

          continue;
        }

      if (!rw || !st_buffer)
        {
          st_buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);
        }

      i = 0;
      while (l_ptr[i] && l_ptr[i] != 0x20)
        {
          i++;
        }

      memset(&l_ptr[i], 0x0, 1);

      while (l_ptr[i] == 0x20)
        {
          i++;
        }

      char *s_p1 = &l_ptr[i + 1];

      size_t s_p1_l = strlen(s_p1);
      if (s_p1[s_p1_l - 1] == 0xA)
        {
          s_p1[s_p1_l - 1] = 0x0;
          s_p1_l--;
        }

      if (!s_p1_l)
        {
          print_str("WARNING: DATA IMPORT: null value '%s'\n", l_ptr);
        }

      int bd = hdl->g_proc0(st_buffer, l_ptr, s_p1);
      if (!bd)
        {
          print_str("ERROR: DATA IMPORT: failed extracting '%s'\n", l_ptr);
          rf = 3;
        }
      rw += 1;
    }

  free(buffer);

  return rf;
}

int
gcb_dirlog(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  errno = 0;

  struct dirlog * ptr = (struct dirlog *) buffer;
  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      uint16_t v_i = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->uploader = v_i;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GROUP, 5))
    {
      uint16_t v_i = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->group = v_i;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_DIRLOG_FILES, 5))
    {
      uint16_t k_ui = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->files = k_ui;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_SIZE, 4))
    {
      ulint64_t k_uli = (ulint64_t) strtoll(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->bytes = k_uli;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t k_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->uptime = k_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_STATUS, 6))
    {
      uint16_t k_us = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->status = k_us;
      return 1;
    }
  return 0;
}

int
gcb_nukelog(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  errno = 0;

  struct nukelog* ptr = (struct nukelog*) buffer;
  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_NUKELOG_REASON, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->reason, val, v_l > 59 ? 59 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_SIZE, 4))
    {
      float v_f = strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->bytes = v_f;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_NUKELOG_MULT, 4))
    {
      uint16_t v_ui = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->mult = v_ui;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_NUKELOG_NUKEE, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->nukee, val, v_l > 12 ? 12 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_NUKELOG_NUKER, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->nuker, val, v_l > 12 ? 12 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_NUKELOG_UNNUKER, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 1;
        }
      memcpy(ptr->unnuker, val, v_l > 12 ? 12 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t k_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->nuketime = k_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_STATUS, 6))
    {
      uint16_t k_us = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->status = k_us;
      return 1;
    }
  return 0;
}

int
gcb_oneliner(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  struct oneliner* ptr = (struct oneliner*) buffer;
  errno = 0;

  if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->uname, val, v_l > 23 ? 23 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GROUP, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->gname, val, v_l > 23 ? 23 : v_l);
      return 1;

    }
  else if (k_l == 3 && !strncmp(key, _MC_GLOB_TAG, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->tagline, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timestamp = v_i;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_ONELINERS_MSG, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->message, val, v_l > 99 ? 99 : v_l);
      return 1;
    }
  return 0;
}

int
gcb_dupefile(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  struct dupefile* ptr = (struct dupefile*) buffer;
  errno = 0;

  if (k_l == 4 && !strncmp(key, _MC_GLOB_FILE, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->filename, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->uploader, val, v_l > 23 ? 23 : v_l);
      return 1;

    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timeup = v_i;
      return 1;
    }
  return 0;
}

int
gcb_lastonlog(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  struct lastonlog* ptr = (struct lastonlog*) buffer;
  errno = 0;

  if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->uname, val, v_l > 23 ? 23 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GROUP, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->gname, val, v_l > 23 ? 23 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GLOB_TAG, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->tagline, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_LOGON, 5))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->logon = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_LOGOFF, 6))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->logoff = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_UPLOAD, 6))
    {
      unsigned long v_ul = (unsigned long) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->upload = v_ul;
      return 1;
    }
  else if (k_l == 8 && !strncmp(key, _MC_GLOB_DOWNLOAD, 8))
    {
      unsigned long v_ul = (unsigned long) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->download = v_ul;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_LASTONLOG_STATS, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->stats, val, v_l > 6 ? 6 : v_l);
      return 1;
    }
  return 0;
}

int
gcb_game(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_game ptr = (__d_game) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timestamp = v_i;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_SCORE, 5))
    {
      float v_f = strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->rating = v_f;
      return 1;
    }
  return 0;
}

int
gcb_tv(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_tvrage ptr = (__d_tvrage) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timestamp = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_TV_AIRDAY, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->airday, val, v_l > 31 ? 31 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_AIRTIME, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->airtime, val, v_l > 5 ? 5 : v_l);
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_COUNTRY, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->country, val, v_l > 23 ? 23 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_TV_LINK, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->link, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_TV_NAME, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->name, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_TV_ENDED, 5))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ended = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_STARTED, 7))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->started = v_i;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_GLOB_RUNTIME, 7))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->runtime = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_SEASONS, 7))
    {
      uint16_t v_ui = (uint16_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->seasons = v_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_TV_SHOWID, 6))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->showid = v_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_STATUS, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->status, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_TV_CLASS, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->class, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GENRE, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->genres, val, v_l > 255 ? 255 : v_l);
      return 1;
    }
  else if (k_l == 9 && !strncmp(key, _MC_TV_SYEAR, 9))
    {
      uint16_t v_ui = (uint16_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->startyear = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_EYEAR, 7))
    {
      uint16_t v_ui = (uint16_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->endyear = v_ui;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_TV_NETWORK, 7))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->network, val, v_l > 255 ? 255 : v_l);
      return 1;
    }
  return 0;
}

int
gcb_imdbh(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_imdb ptr = (__d_imdb) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->timestamp = v_i;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_IMDB_IMDBID, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->imdb_id, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_SCORE, 5))
    {
      float v_f = strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->rating = v_f;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_IMDB_VOTES, 5))
    {
      uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->votes = v_ui;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_IMDB_YEAR, 4))
    {
      uint16_t v_ui = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->year = v_ui;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GENRE, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->genres, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_IMDB_TITLE, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->title, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 8 && !strncmp(key, _MC_IMDB_RELEASED, 8))
    {
      int32_t v_i = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->released = v_i;
      return 1;
    }
  else if (k_l == 7 && !strncmp(key, _MC_GLOB_RUNTIME, 7))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->runtime = v_ui;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_IMDB_RATED, 5))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->rated, val, v_l > 7 ? 7 : v_l);
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_IMDB_ACTORS, 6))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->actors, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 8 && !strncmp(key, _MC_IMDB_DIRECT, 8))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->director, val, v_l > 63 ? 63 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_IMDB_SYNOPSIS, 4))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->synopsis, val, v_l > 197 ? 197 : v_l);
      return 1;
    }

  return 0;
}

int
gcb_gen1(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_generic_s2044 ptr = (__d_generic_s2044) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GE_GE1, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_1, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE2, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_2, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE3, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_3, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE4, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_4, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE5, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_5, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE6, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_6, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE7, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_7, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE8, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_8, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, "i32", 3))
    {
      uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32 = v_ui;
      return 1;
    }

  return 0;
}

int
gcb_gen2(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_generic_s1644 ptr = (__d_generic_s1644) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GE_GE1, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_1, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE2, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_2, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE3, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_3, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE4, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_4, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE5, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_5, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE6, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_6, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE7, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_7, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE8, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_8, val, v_l > 127 ? 127 : v_l);
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I1, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_1 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I2, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_2 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I3, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_3 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I4, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_4 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U1, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_1 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U2, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_2 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U3, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_3 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U4, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_4 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F1, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_1 = v_f;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F2, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_2 = v_f;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F3, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_3 = v_f;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_F4, 2))
    {
      float v_f = (float) strtof(val, NULL);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->f_4 = v_f;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL1, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_1 = v_ul;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL2, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_2 = v_ul;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL3, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_3 = v_ul;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL4, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_4 = v_ul;
      return 1;
    }
  return 0;
}

int
gcb_gen3(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  __d_generic_s800 ptr = (__d_generic_s800) buffer;
  errno = 0;

  if (k_l == 3 && !strncmp(key, _MC_GE_GE1, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_1, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE2, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_2, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE3, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_3, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_GE4, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->s_4, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I1, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_1 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_I2, 2))
    {
      int32_t v_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->i32_2 = v_ui;
      return 1;
    }

  else if (k_l == 2 && !strncmp(key, _MC_GE_U1, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_1 = v_ui;
      return 1;
    }
  else if (k_l == 2 && !strncmp(key, _MC_GE_U2, 2))
    {
      uint32_t v_ui = (uint32_t) strtoul(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui32_2 = v_ui;
      return 1;
    }

  else if (k_l == 3 && !strncmp(key, _MC_GE_UL1, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_1 = v_ul;
      return 1;
    }
  else if (k_l == 3 && !strncmp(key, _MC_GE_UL2, 3))
    {
      uint64_t v_ul = (uint64_t) strtoull(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->ui64_2 = v_ul;
      return 1;
    }

  return 0;
}

#define LCFG_MAX_LOADED 0x200
#define LCFG_MAX_LOAD_LINES 10000
#define LCFG_MAX_LINE_SIZE	16384

int
load_cfg(pmda pmd, char *file, uint32_t flags, pmda *res)
{
  g_setjmp(0, "load_cfg", NULL, NULL);
  int r = 0;
  FILE *fh;
  pmda md;

  if (flags & F_LCONF_NORF)
    {
      md_init(pmd, 256);
      md = pmd;
    }
  else
    {
      if (pmd->offset > LCFG_MAX_LOADED)
        {
          free_cfg_rf(pmd);
        }
      md = register_cfg_rf(pmd, file);
    }

  if (!md)
    {
      return 1;
    }

  off_t f_sz = get_file_size(file);

  if (!f_sz)
    {
      return 2;
    }

  if (!(fh = fopen(file, "r")))
    {
      return 3;
    }

  char *buffer = malloc(LCFG_MAX_LINE_SIZE + 2);
  p_cfg_h pce;
  int rd, i, c = 0;

  while (fgets(buffer, LCFG_MAX_LINE_SIZE + 1, fh) && c < LCFG_MAX_LOAD_LINES
      && !ferror(fh) && !feof(fh))
    {
      if (strlen(buffer) < 3)
        {
          continue;
        }

      for (i = 0; buffer[i] == 0x20 || buffer[i] == 0x9; i++)
        {
        }

      if (buffer[i] == 0x23)
        {
          continue;
        }

      pce = (p_cfg_h) md_alloc(md, sizeof(cfg_h));
      md_init(&pce->data, 32);
      if ((rd = split_string_sp_tab(buffer, &pce->data)) < 1)
        {
          md_g_free(&pce->data);
          md_unlink(md, md->pos);
          continue;
        }
      c++;
      pce->key = pce->data.objects->ptr;
    }

  fclose(fh);
  free(buffer);

  if (res)
    {
      *res = md;
    }

  return r;
}

void
free_cfg(pmda md)
{
  g_setjmp(0, "free_cfg", NULL, NULL);

  if (!md || !md->objects)
    {
      return;
    }

  p_md_obj ptr = md_first(md);
  p_cfg_h pce;

  while (ptr)
    {
      pce = (p_cfg_h) ptr->ptr;
      if (pce && &pce->data)
        {
          md_g_free(&pce->data);
        }
      ptr = ptr->next;
    }

  md_g_free(md);
}

p_md_obj
get_cfg_opt(char *key, pmda md, pmda *ret)
{
  if (!md->count)
    {
      return NULL;
    }

  p_md_obj ptr = md_first(md);
  size_t pce_key_sz, key_sz = strlen(key);
  p_cfg_h pce;

  while (ptr)
    {
      pce = (p_cfg_h) ptr->ptr;
      pce_key_sz = strlen(pce->key);
      if (pce_key_sz == key_sz && !strncmp(pce->key, key, pce_key_sz))
        {
          p_md_obj r_ptr = md_first(&pce->data);
          if (r_ptr)
            {
              if (ret)
                {
                  *ret = &pce->data;
                }
              return (p_md_obj) r_ptr->next;
            }
          else
            {
              return NULL;
            }
        }
      ptr = ptr->next;
    }

  return NULL;
}

int
self_get_path(char *out)
{
  g_setjmp(0, "self_get_path", NULL, NULL);

  char path[PATH_MAX];
  int r;

  snprintf(path, PATH_MAX, "/proc/%d/exe", getpid());

  if (access(path, R_OK))
    {
      snprintf(path, PATH_MAX, "/compat/linux/proc/%d/exe", getpid());
    }

  if (access(path, R_OK))
    {
      snprintf(path, PATH_MAX, "/proc/%d/file", getpid());
    }

  if ((r = readlink(path, out, PATH_MAX)) == -1)
    {
      return 2;
    }
  out[r] = 0x0;
  return 0;
}

int
is_ascii_text(uint8_t in_c)
{
  if ((in_c >= 0x0 && in_c <= 0x7F))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_lowercase_text(uint8_t in_c)
{
  if ((in_c >= 0x61 && in_c <= 0x7A))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_alphanumeric(uint8_t in_c)
{
  if ((in_c >= 0x61 && in_c <= 0x7A) || (in_c >= 0x41 && in_c <= 0x5A)
      || (in_c >= 0x30 && in_c <= 0x39))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_numeric(uint8_t in_c)
{
  if ((in_c >= 0x30 && in_c <= 0x39))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_hexadecimal(uint8_t in_c)
{
  if ((in_c >= 0x41 && in_c <= 0x46) || (in_c >= 0x61 && in_c <= 0x66))
    {
      return 0;
    }
  return 1;
}

int
is_ascii_uppercase_text(uint8_t in_c)
{
  if ((in_c >= 0x41 && in_c <= 0x5A))
    {
      return 0;
    }

  return 1;
}

char *
replace_char(char w, char r, char *string)
{
  int s_len = strlen(string), i;
  for (i = 0; i < s_len + 100 && string[i] != 0; i++)
    {
      if (string[i] == w)
        {
          string[i] = r;
        }
    }

  return string;
}

#define SSD_MAX_LINE_SIZE 	32768
#define SSD_MAX_LINE_PROC 	15000
//#define SSD_MAX_FILE_SIZE	(V_MB*32)

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
get_file_type(char *file)
{
  struct stat sb;

  if (stat(file, &sb) == -1)
    return errno;

  return IFTODT(sb.st_mode);
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
