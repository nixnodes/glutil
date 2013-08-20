/*
 * ============================================================================
 * Name        : glutil
 * Authors     : nymfo, siska
 * Version     : 1.3-10
 * Description : glFTPd binary log utility
 * ============================================================================
 */

#define _BSD_SOURCE
#define _GNU_SOURCE

#define _LARGEFILE64_SOURCE 1
#define _LARGEFILE_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdio.h>

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

/* gl's ipc key */
#ifndef shm_ipc
#define shm_ipc 0x0000DEAD
#endif

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

/* see README file about this */
#ifndef du_fld
#define du_fld "/glftpd/bin/glutil.folders"
#endif

/* folders to skip during fs rebuild (-r) */
#ifndef PREG_DIR_SKIP
#define PREG_DIR_SKIP "\\/(Sample|Covers|Subs|Cover|Proof)$"
#endif

#ifndef PREG_SFV_SKIP
#define PREG_SFV_SKIP PREG_DIR_SKIP
#endif

/* file extensions to skip generating crc32 (SFV mode)*/
#ifndef PREG_SFV_SKIP_EXT
#define PREG_SFV_SKIP_EXT "\\.(nfo|sfv)$"
#endif

/* -------------------------- */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef WEXITSTATUS
#define	WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#endif

#define VER_MAJOR 1
#define VER_MINOR 3
#define VER_REVISION 10
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
#define AAINT uint64_t
#define ENV_64
char ARCH = 1;
#else
#define AAINT uint32_t
#define ENV_32
char ARCH = 0;
#endif

#define MAX_uint64_t 		0xFFFFFFFFFFFFFFFF
#define MAX_ULONG 			0xFFFFFFFF

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct e_arg {
	int depth;
	uint32_t flags;
	char buffer[1024];
	struct dirlog *dirlog;
	struct nukelog *nukelog;
	struct dupefile *dupefile;
	struct lastonlog *lastonlog;
	struct oneliner *oneliner;
	struct ONLINE *online;
	time_t t_stor;
} ear;

typedef struct option_reference_array {
	char *option;
	void *function, *arg_cnt;
}*p_ora;

struct d_stats {
	uint64_t bw, br, rw;
};

typedef struct mda_object {
	void *ptr, *next, *prev;
//	unsigned char flags;
}*p_md_obj, md_obj;

#define F_MDA_REFPTR		0x1
#define F_MDA_FREE			0x2
#define F_MDA_REUSE			0x4
#define F_MDA_WAS_REUSED	0x8
#define F_MDA_EOF			0x10
#define F_MDA_FIRST_REUSED  0x20

typedef struct mda_header {
	p_md_obj objects, pos, r_pos, c_pos;
	off_t offset, r_offset, count;
	uint32_t flags;
	void *lref_ptr;
} mda, *pmda;

typedef struct config_header {
	char *key, *value;
	mda data;
} cfg_h, *p_cfg_h;

struct g_handle {
	FILE *fh;
	off_t offset, bw, br, total_sz;
	off_t rw;
	uint32_t block_sz, flags;
	mda buffer, w_buffer;
	void *data;
	size_t buffer_count;
	void *last;
	char s_buffer[4096], file[4096], mode[32];
	mode_t st_mode;
	struct ONLINE *ol;
	int shmid;
	struct shmid_ds ipcbuf;
};

typedef struct g_cfg_ref {
	mda cfg;
	char file[PATH_MAX];
} cfg_r, *p_cfg_r;

typedef struct ___si_argv0 {
	int ret;
	uint32_t flags;
	char p_buf_1[4096];
	char p_buf_2[PATH_MAX];
	char s_ret[262144];
} _si_argv0, *__si_argv0;

typedef struct sig_jmp_buf {
	sigjmp_buf env, p_env;
	uint32_t flags, pflags;
	int id, pid;
	unsigned char ci, pci;
	char type[32];
	void *callback, *arg;
} sigjmp, *p_sigjmp;

/*
 * CRC-32 polynomial 0x04C11DB7 (0xEDB88320)
 * see http://en.wikipedia.org/wiki/Cyclic_redundancy_check#Commonly_used_and_standardized_CRCs
 */

static uint32_t crc_32_tab[] = { 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
		0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4,
		0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
		0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB,
		0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
		0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E,
		0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75,
		0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
		0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808,
		0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
		0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
		0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
		0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162,
		0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49,
		0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC,
		0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
		0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3,
		0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
		0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6,
		0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D,
		0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
		0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
		0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767,
		0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
		0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A,
		0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31,
		0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
		0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14,
		0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
		0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B,
		0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE,
		0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
		0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
		0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
		0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ ((uint8_t)octet)) & 0xff] ^ ((crc) >> 8))

uint32_t crc32(uint32_t crc32, uint8_t *buf, size_t len) {
	register uint32_t oldcrc32;

	if (crc32) {
		oldcrc32 = crc32;
	} else {
		oldcrc32 = MAX_ULONG;
	}

	for (; len; len--, buf++) {
		oldcrc32 = UPDC32(*buf, oldcrc32);
	}

	return ~oldcrc32;
}

#define PTRSZ 				(sizeof(void*))

#define REG_MATCHES_MAX	 	4

#define UPD_MODE_RECURSIVE 	0x1
#define UPD_MODE_SINGLE		0x2
#define UPD_MODE_CHECK		0x3
#define UPD_MODE_DUMP		0x4
#define UPD_MODE_DUMP_NUKE	0x5
#define UPD_MODE_DUPE_CHK	0x6
#define UPD_MODE_REBUILD	0x7
#define UPD_MODE_DUMP_DUPEF 0x8
#define UPD_MODE_DUMP_LON	0x9
#define UPD_MODE_DUMP_ONEL	0xA
#define UPD_MODE_DUMP_ONL	0xB
#define UPD_MODE_NOOP		0xC
#define UPD_MODE_MACRO		0xD
#define UPD_MODE_FORK		0xE
#define UPD_MODE_BACKUP		0xF
#define UPD_MODE_DUMP_USERS	0x10
#define UPD_MODE_DUMP_GRPS	0x11
#define UPD_MODE_DUMP_GEN	0x12

#define PRIO_UPD_MODE_MACRO 0x1001

#define F_OPT_FORCE 		0x1
#define F_OPT_VERBOSE 		0x2
#define F_OPT_VERBOSE2 		0x4
#define F_OPT_VERBOSE3 		0x8
#define F_OPT_SFV	 		0x10
#define F_OPT_NOWRITE		0x20
#define F_OPT_NOBUFFER		0x40
#define F_OPT_UPDATE		0x80
#define F_OPT_FIX			0x100
#define F_OPT_FOLLOW_LINKS	0x200
#define F_OPT_FORMAT_BATCH	0x400
#define F_OPT_KILL_GLOBAL	0x800
#define F_OPT_MODE_RAWDUMP  0x1000
#define F_OPT_HAS_G_REGEX	0x2000
#define F_OPT_VERBOSE4 		0x4000
#define F_OPT_WBUFFER		0x8000
#define F_OPT_FORCEWSFV		0x10000
#define F_OPT_FORMAT_COMP   0x20000
#define F_OPT_DAEMONIZE		0x40000
#define F_OPT_LOOP			0x80000
#define F_OPT_LOOPEXEC		0x100000
#define F_OPT_PS_SILENT		0x200000
#define F_OPT_PS_TIME		0x400000
#define F_OPT_PS_LOGGING	0x800000
#define F_OPT_TERM_ENUM		0x1000000
#define F_OPT_HAS_G_MATCH	0x2000000
#define F_OPT_HAS_M_ARG1	0x4000000
#define F_OPT_HAS_M_ARG2	0x8000000
#define F_OPT_HAS_M_ARG3	0x10000000
#define F_OPT_PREEXEC		0x20000000
#define F_OPT_POSTEXEC		0x40000000
#define F_OPT_NOBACKUP		0x80000000
#define F_OPT_C_GHOSTONLY	0x100000000
#define F_OPT_XDEV			0x200000000
#define F_OPT_XBLK			0x400000000

#define F_MD_NOREAD			0x1

#define F_DL_FOPEN_BUFFER	0x1
#define F_DL_FOPEN_FILE		0x2
#define F_DL_FOPEN_REWIND	0x4
#define F_DL_FOPEN_SHM		0x8

#define F_EARG_SFV 			0x1
#define F_EAR_NOVERB		0x2

#define F_FC_MSET_SRC		0x1
#define F_FC_MSET_DEST		0x2

#define F_GH_NOMEM  		0x1
#define F_GH_ISDIRLOG		0x2
#define F_GH_EXEC			0x4
#define F_GH_ISNUKELOG		0x8
#define F_GH_FFBUFFER		0x10
#define F_GH_WAPPEND		0x20
#define F_GH_DFWASWIPED		0x40
#define F_GH_DFNOWIPE		0x80
#define F_GH_ISDUPEFILE		0x100
#define F_GH_ISLASTONLOG	0x200
#define F_GH_ISONELINERS	0x400
#define F_GH_SHM			0x800
#define F_GH_ISONLINE		0x1000

#define F_OVRR_IPC			0x1
#define F_OVRR_GLROOT		0x2
#define F_OVRR_SITEROOT		0x4
#define F_OVRR_DUPEFILE		0x8
#define F_OVRR_LASTONLOG	0x10
#define F_OVRR_ONELINERS	0x20
#define F_OVRR_DIRLOG		0x40
#define F_OVRR_NUKELOG		0x80
#define F_OVRR_NUKESTR		0x100

#define F_AV_RETURN_STRING 	0x1

#define F_PD_RECURSIVE 		0x1
#define F_PD_MATCHDIR		0x2
#define F_PD_MATCHREG		0x4

#define F_PD_MATCHTYPES		(F_PD_MATCHDIR|F_PD_MATCHREG)

/* these bits determine file type */
#define F_GH_ISTYPE			(F_GH_ISNUKELOG|F_GH_ISDIRLOG|F_GH_ISDUPEFILE|F_GH_ISLASTONLOG|F_GH_ISONELINERS|F_GH_ISONLINE)

#define V_MB				0x100000

#define DL_SZ sizeof(struct dirlog)
#define NL_SZ sizeof(struct nukelog)
#define DF_SZ sizeof(struct dupefile)
#define LO_SZ sizeof(struct lastonlog)
#define OL_SZ sizeof(struct oneliner)
#define ON_SZ sizeof(struct ONLINE)

#define CRC_FILE_READ_BUFFER_SIZE 26214400
#define	DB_MAX_SIZE 	536870912   /* max file size allowed to load into memory */
#define MAX_EXEC_STR 	262144

#define	PIPE_READ_MAX	0x2000

#define MSG_GEN_NODFILE "ERROR: %s: could not open data file: %s\n"
#define MSG_GEN_DFWRITE "ERROR: %s: [%d] [%llu] writing record to dirlog failed! (mode: %s)\n"
#define MSG_GEN_DFCORRU "ERROR: %s: corrupt data file detected! (data file size [%llu] is not a multiple of block size [%d])\n"
#define MSG_GEN_DFRFAIL "ERROR: %s: rebuilding data file failed!\n"

#define DEFPATH_LOGS 	"/logs"
#define DEFPATH_USERS 	"/users"
#define DEFPATH_GROUPS 	"/groups"

#define DEFF_DIRLOG 	"dirlog"
#define DEFF_NUKELOG 	"nukelog"
#define DEFF_LASTONLOG  "laston.log"
#define DEFF_DUPEFILE 	"dupefile"
#define DEFF_ONELINERS 	"oneliners.log"
#define DEFF_DULOG	 	"glutil.log"

#define F_SIGERR_CONTINUE 	0x1  /* continue after exception */

#define ID_SIGERR_UNSPEC 	0x0
#define ID_SIGERR_MEMCPY 	0x1
#define ID_SIGERR_STRCPY 	0x2
#define ID_SIGERR_FREE 		0x3
#define ID_SIGERR_FREAD 	0x4
#define ID_SIGERR_FWRITE 	0x5
#define ID_SIGERR_FOPEN 	0x6
#define ID_SIGERR_FCLOSE 	0x7
#define ID_SIGERR_MEMMOVE 	0x8

sigjmp g_sigjmp = { { { { 0 } } } };
uint64_t gfl = F_OPT_WBUFFER;
uint32_t ofl = 0;
FILE *fd_log = NULL;
char LOGFILE[PATH_MAX] = { log_file };

void e_pop(p_sigjmp psm) {
	memcpy(psm->p_env, psm->env, sizeof(sigjmp_buf));
	psm->pid = psm->id;
	psm->pflags = psm->flags;
	psm->pci = psm->ci;
}

void e_push(p_sigjmp psm) {
	memcpy(psm->env, psm->p_env, sizeof(sigjmp_buf));
	psm->id = psm->pid;
	psm->flags = psm->pflags;
	psm->ci = psm->pci;
}

void g_setjmp(uint32_t flags, char *type, void *callback, void *arg) {
	if (flags) {
		g_sigjmp.flags = flags;
	}
	g_sigjmp.id = ID_SIGERR_UNSPEC;

	bzero(g_sigjmp.type, 32);
	memcpy(g_sigjmp.type, type, strlen(type));

	return;
}

void *g_memcpy(void *dest, const void *src, size_t n) {
	void *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags = 0;
	g_sigjmp.id = ID_SIGERR_MEMCPY;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = memcpy(dest, src, n);
	}
	e_push(&g_sigjmp);
	return ret;
}

void *g_memmove(void *dest, const void *src, size_t n) {
	void *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags = 0;
	g_sigjmp.id = ID_SIGERR_MEMMOVE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = memmove(dest, src, n);
	}
	e_push(&g_sigjmp);
	return ret;
}

char *g_strncpy(char *dest, const char *src, size_t n) {
	char *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags = 0;
	g_sigjmp.id = ID_SIGERR_STRCPY;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = strncpy(dest, src, n);
	}
	e_push(&g_sigjmp);
	return ret;
}

void g_free(void *ptr) {
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FREE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		free(ptr);
	}
	e_push(&g_sigjmp);
}

size_t g_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = 0;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FREAD;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fread(ptr, size, nmemb, stream);
	}
	e_push(&g_sigjmp);
	return ret;
}

size_t g_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = 0;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FWRITE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fwrite(ptr, size, nmemb, stream);
	}
	e_push(&g_sigjmp);
	return ret;
}

FILE *gg_fopen(const char *path, const char *mode) {
	FILE *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FOPEN;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fopen(path, mode);
	}
	e_push(&g_sigjmp);
	return ret;
}

int g_fclose(FILE *fp) {
	int ret = 0;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FCLOSE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fclose(fp);
	}
	e_push(&g_sigjmp);
	return ret;
}

#define MSG_DEF_UNKN1 	"(unknown)"

void sighdl_error(int sig, siginfo_t* siginfo, void* context) {

	char *s_ptr1 = MSG_DEF_UNKN1, *s_ptr2 = MSG_DEF_UNKN1, *s_ptr3 = "";
	char buffer1[4096] = { 0 };

	switch (sig) {
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

	sprintf(buffer1, ", fault address: 0x%.16llX",
			(ulint64_t) (AAINT) siginfo->si_addr);

	switch (g_sigjmp.id) {
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

	if (g_sigjmp.flags & F_SIGERR_CONTINUE) {
		s_ptr3 = ", resuming execution..";
	}

	printf("EXCEPTION: %s: [%s] [%s] [%d]%s%s\n", s_ptr1, g_sigjmp.type, s_ptr2,
			siginfo->si_errno, buffer1, s_ptr3);

	usleep(250000);

	g_sigjmp.ci++;

	if (g_sigjmp.flags & F_SIGERR_CONTINUE) {
		siglongjmp(g_sigjmp.env, 0);
	}

	g_sigjmp.ci = 0;
	g_sigjmp.flags = 0;

	exit(siginfo->si_errno);
}

struct tm *get_localtime(void) {
	time_t t = time(NULL);
	return localtime(&t);
}

#define F_MSG_TYPE_ANY		 	0xFFFFFFFF
#define F_MSG_TYPE_EXCEPTION 	0x1
#define F_MSG_TYPE_ERROR 		0x2
#define F_MSG_TYPE_WARNING 		0x4
#define F_MSG_TYPE_NOTICE		0x8
#define F_MSG_TYPE_STATS		0x10
#define F_MSG_TYPE_NORMAL		0x20

#define F_MSG_TYPE_EEW 			(F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_ERROR|F_MSG_TYPE_WARNING)

uint32_t LOGLVL = F_MSG_TYPE_EEW;

uint32_t get_msg_type(char *msg) {
	if (!strncmp(msg, "INIT:", 5)) {
		return F_MSG_TYPE_ANY;
	}
	if (!strncmp(msg, "EXCEPTION:", 10)) {
		return F_MSG_TYPE_EXCEPTION;
	}
	if (!strncmp(msg, "ERROR:", 6)) {
		return F_MSG_TYPE_ERROR;
	}
	if (!strncmp(msg, "WARNING:", 8)) {
		return F_MSG_TYPE_WARNING;
	}
	if (!strncmp(msg, "NOTICE:", 7)) {
		return F_MSG_TYPE_NOTICE;
	}
	if (!strncmp(msg, "STATS:", 6)) {
		return F_MSG_TYPE_STATS;
	}

	return F_MSG_TYPE_NORMAL;
}

int w_log(char *w, char *ow) {

	if (ow && !(get_msg_type(ow) & LOGLVL)) {
		return 1;
	}

	size_t wc, wll;

	wll = strlen(w);

	if ((wc = fwrite(w, 1, wll, fd_log)) != wll) {
		printf("ERROR: %s: writing log failed [%d/%d]\n", LOGFILE, (int) wc,
				(int) wll);
	}

	fflush(fd_log);

	return 0;
}

#define PSTR_MAX	(V_MB/4)

int print_str(const char * volatile buf, ...) {
	g_setjmp(0, "print_str", NULL, NULL);

	char d_buffer_2[PSTR_MAX];
	va_list al;
	va_start(al, buf);

	if ((gfl & F_OPT_PS_LOGGING) || (gfl & F_OPT_PS_TIME)) {
		struct tm tm = *get_localtime();
		sprintf(d_buffer_2, "[%.2u-%.2u-%.2u %.2u:%.2u:%.2u] %s",
				(tm.tm_year + 1900) % 100, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec, buf);
	}

	if ((gfl & F_OPT_PS_LOGGING) && fd_log) {
		char wl_buffer[PSTR_MAX];
		vsprintf(wl_buffer, d_buffer_2, al);
		w_log(wl_buffer, (char*) buf);
	}

	if (gfl & F_OPT_PS_SILENT) {
		return 0;
	}

	va_end(al);
	va_start(al, buf);

	if (gfl & F_OPT_PS_TIME) {
		vprintf(d_buffer_2, al);
	} else {
		vprintf(buf, al);

	}

	fflush(stdout);

	return 0;
}

/* ---------------------------------------------------------------------------------- */

struct d_stats dl_stats = { 0 };
struct d_stats nl_stats = { 0 };

struct g_handle g_act_1 = { 0 };
struct g_handle g_act_2 = { 0 };

mda dirlog_buffer = { 0 };
mda nukelog_buffer = { 0 };

int updmode = 0;
char *argv_off = NULL;
char GLROOT[255] = { glroot };
char SITEROOT_N[255] = { siteroot };
char SITEROOT[255] = { 0 };
char DIRLOG[255] = { dir_log };
char NUKELOG[255] = { nuke_log };
char DU_FLD[255] = { du_fld };
char DUPEFILE[255] = { dupe_file };
char LASTONLOG[255] = { last_on_log };
char ONELINERS[255] = { oneliner_file };
char FTPDATA[255] = { ftp_data };
char *LOOPEXEC = NULL;
long long int db_max_size = DB_MAX_SIZE;
key_t SHM_IPC = (key_t) shm_ipc;
int glob_regex_flags = 0;
char GLOB_REGEX[4096] = { 0 };
char GLOB_MATCH[4096] = { 0 };
int EXITVAL = 0;

int loop_interval = 0;
int loop_max = 0;
char *NUKESTR = NULL;

char *exec_str = NULL;

int glob_reg_i_m = 0;
int glob_match_i_m = 0;
mda glconf = { 0 };

char b_spec1[PATH_MAX];

void *_p_macro_argv = NULL;
int _p_macro_argc = 0;

uint32_t g_sleep = 0;
uint32_t g_usleep = 0;

void *p_argv_off = NULL;
void *prio_argv_off = NULL;

mda cfg_rf = { 0 };

uint32_t flags_udcfg = 0;

char *hpd_up =
		"glFTPd binary log tool, version %d.%d-%d%s-%s\n"
				"\n"
				"Main options:\n"
				"  -s <folders>          Import specific directories. Use quotation marks with multiple arguments\n"
				"                           <folders> are passed relative to SITEROOT, separated by space\n"
				"                           Use -f to overwrite existing entries\n"
				"  -r [-u]               Rebuild dirlog based on filesystem data\n"
				"                           .folders file (see README) defines a list of dirs in SITEROOT to scan\n"
				"                           -u only imports new records and does not truncate existing dirlog\n"
				"                           -f ignores .folders file and does a full recursive scan\n"
				"  -d, [--raw]           Print directory log to stdout in readable format (-vv prints dir nuke status)\n"
				"  -n, [--raw]           Print nuke log to stdout\n"
				"  -i, [--raw]           Print dupe file to stdout\n"
				"  -l, [--raw]           Print last-on log to stdout\n"
				"  -o, [--raw]           Print oneliners to stdout\n"
				"  -w  [--raw]|[--comp]  Print online users data from shared memory to stdout\n"
				"  -t                    Print all user files inside /ftp-data/users\n"
				"  -g                    Print all group files inside /ftp-data/groups\n"
				"  -x <root dir> [--recursive] [--dir] [--file]\n"
				"                         Parses filesystem and processes each item found with internal filters/hooks\n"
				"                         --dir scans directories only\n"
				"                         --file scans files only (default is both dirs and files)\n"
				"  -c, --check [--fix] [--ghost]\n"
				"                         Compare dirlog and filesystem records and warn on differences\n"
				"                           --fix attempts to correct dirlog\n"
				"                           --ghost only looks for dirlog records with missing directories on filesystem\n"
				"                           Folder creation dates are ignored unless -f is given\n"
				"  -p, --dupechk         Look for duplicate records within dirlog and print to stdout\n"
				"  -e <dirlog|nukelog|dupefile|lastonlog>\n"
				"                         Rebuilds existing data file, based on filtering rules (see --exec,\n"
				"                           --[i]regex[i] and --[i]match\n"
				"  -m <macro>            Searches subdirs for script that has the given macro defined, and executes\n"
				"\n"
				"Options:\n"
				"  -f                    Force operation where it applies\n"
				"  -v                    Increase verbosity level (use -vv or more for greater effect)\n"
				"  -k, --nowrite         Perform a dry run, executing normally except no writing is done\n"
				"  -b, --nobuffer        Disable data file memory buffering\n"
				"  -y, --followlinks     Follows symbolic links (default is skip)\n"
				"  --nowbuffer           Disable write pre-caching (faster but less safe), applies to -r\n"
				"  --memlimit=<bytes>    Maximum file size that can be pre-buffered into memory\n"
				"  --sfv                 Generate new SFV files inside target folders, works with -r [-u] and -s\n"
				"                           Used by itself, triggers -r (fs rebuild) dry run (does not modify dirlog)\n"
				"                           Avoid using this if doing a full recursive rebuild\n"
				"  --exec <command {[base]dir}|{user}|{group}|{size}|{files}|{time}|{nuker}|{tag}|{msg}..\n"
				"          ..|{unnuker}|{nukee}|{reason}|{logon}|{logoff}|{upload}|{download}|{file}|{host}..\n"
				"          ..|{ssl}|{lupdtime}|{lxfertime}|{bxfer}|{btxfer}|{pid}|{rate}|{glroot}|{siteroot}..\n"
				"          ..|{exe}|{glroot}|{logfile}|{siteroot}|{usroot}|{logroot}|{ftpdata}|{PID}|{IPC}>\n"
				"                         While parsing data structure/filesystem, execute command for each record\n"
				"                            Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x and -n\n"
				"                            Operators {..} are overwritten with dirlog values\n"
				"  --preexec <command {exe}|{glroot}|{logfile}|{siteroot}|{usroot}|{logroot}|{ftpdata}|{PID}|{IPC}>\n"
				"                         Execute shell <command> before starting main procedure\n"
				"  --postexec <command {exe}|{glroot}|{logfile}|{siteroot}|{usroot}|{logroot}|{ftpdata}|{PID}|{IPC}>\n"
				"                         Execute shell <command> after main procedure finishes\n"
				"  --match <match>       Regular filter string (exact matches)\n"
				"                           Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x and -n\n"
				"  --imatch <match>      Inverted --match\n"
				"  --regex <match>       Regex filter string, used during various operations\n"
				"                           Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x and -n\n"
				"  --regexi <match>      Case insensitive variant of --regex\n"
				"  --iregex <match>      Same as --regex with inverted match\n"
				"  --iregexi <match>     Same as --regexi with inverted match\n"
				"  --xdev                Ignores files/dirs on other filesystems\n"
				"                           Applies to -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x and -n (can apply to other modes)\n"
				"  --xblk                Ignores files/dirs on non-block devices\n"
				"                           Applies to -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x and -n (can apply to other modes)\n"
				"  --batch               Prints with simple formatting\n"
				"  --ipc <key>           Override gl's shared memory segment key setting\n"
				"  --daemon              Fork process into background\n"
				"  --loop <interval>     Loops the given 'Main option' operation\n"
				"                           Use caution, some operations might fail when looped\n"
				"                           This is usefull when running yourown scripts (--exec)\n"
				"  --loopexec <command {exe}|{glroot}|{logfile}|{siteroot}|{usroot}|{logroot}|{ftpdata}|{PID}|{IPC}>\n"
				"                         Execute command each loop\n"
				"  --loglevel <0-6>      Log verbosity level (1: exception only..6: everything)\n"
				"                           Level 0 turns logging off\n"
				"  --silent              Silent mode\n"
				"  --ftime               Prepend formatted timestamps to output\n"
				"  --log                 Force logging enabled\n"
				"  --fork <command>      Fork process into background and execute <command>\n"
				"  --arg[1-3] <argument> Set values that fill {m:arg[1-3]} variables\n"
				"                           Used only with when running macros (-m)\n"
				"  --[u]sleep <timeout>  Wait for <timeout> before running\n"
				"  --version             Print version and exit\n"
				"\n"
				"Directory and file:\n"
				"  --glroot=PATH         Override default glFTPd root path\n"
				"  --siteroot=PATH       Override default site root path (relative to glFTPd root)\n"
				"  --dirlog=FILE         Override default path to directory log\n"
				"  --nukelog=FILE        Override default path to nuke log\n"
				"  --dupefile=FILE       Override default path to dupe file\n"
				"  --lastonlog=FILE      Override default path to last-on log\n"
				"  --oneliners=FILE      Override default path to oneliners file\n"
				"  --folders=FILE        Override default path to folders file (contains sections and depths,\n"
				"                           used on recursive imports)\n"
				"  --logfile=FILE        Override default log file path\n"
				"\n";

int g_cpg(void *arg, void *out, int m, size_t sz) {
	char *buffer;
	if (m == 2) {
		buffer = (char *) arg;
	} else {
		buffer = ((char **) arg)[0];
	}
	if (!buffer)
		return 1;
	bzero(out, sz);
	g_strncpy(out, buffer, strlen(buffer));

	return 0;
}
void *g_pg(void *arg, int m) {
	if (m == 2) {
		return (char *) arg;
	}
	return ((char **) arg)[0];
}

char *g_pd(void *arg, int m, size_t l) {
	char *buffer = (char*) g_pg(arg, m);
	char *ptr = NULL;
	size_t a_len = strlen(buffer);
	if (!a_len || a_len >= l) {
		return NULL;
	}
	if (a_len) {
		ptr = (char*) calloc(a_len + 10, 1);
		g_strncpy(ptr, buffer, a_len);
	}
	return ptr;
}

int prio_opt_g_macro(void *arg, int m) {
	prio_argv_off = g_pg(arg, m);
	updmode = PRIO_UPD_MODE_MACRO;
	return 0;
}

int opt_g_loglvl(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	int lvl = atoi(buffer), i;
	uint32_t t_LOGLVL = 0;

	for (i = 0; i < lvl; i++) {
		t_LOGLVL <<= 1;
		t_LOGLVL |= 0x1;
	}

	LOGLVL = t_LOGLVL;
	gfl |= F_OPT_PS_LOGGING;

	return 0;
}

int opt_g_verbose(void *arg, int m) {
	gfl |= F_OPT_VERBOSE;
	return 0;
}

int opt_g_verbose2(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2;
	return 0;
}

int opt_g_verbose3(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3;
	return 0;
}

int opt_g_verbose4(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4;
	return 0;
}

int opt_g_force(void *arg, int m) {
	gfl |= F_OPT_FORCE;
	return 0;
}

int opt_g_update(void *arg, int m) {
	gfl |= F_OPT_UPDATE;
	return 0;
}

int opt_g_loop(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	loop_interval = (int) strtol(buffer, NULL, 10);
	gfl |= F_OPT_LOOP;
	return 0;
}

int opt_loop_max(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	loop_max = (int) strtol(buffer, NULL, 10);
	return 0;
}

int opt_g_udc(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_DUMP_GEN;
	return 0;
}

int opt_g_recursive(void *arg, int m) {
	flags_udcfg |= F_PD_RECURSIVE;
	return 0;
}

int opt_g_udc_dir(void *arg, int m) {
	flags_udcfg |= F_PD_MATCHDIR;
	return 0;
}

int opt_g_udc_f(void *arg, int m) {
	flags_udcfg |= F_PD_MATCHREG;
	return 0;
}

int opt_g_loopexec(void *arg, int m) {
	LOOPEXEC = g_pd(arg, m, MAX_EXEC_STR);
	if (LOOPEXEC) {
		gfl |= F_OPT_LOOPEXEC;
	}
	return 0;
}

int opt_g_daemonize(void *arg, int m) {
	gfl |= F_OPT_DAEMONIZE;
	return 0;
}

int opt_g_fix(void *arg, int m) {
	gfl |= F_OPT_FIX;
	return 0;
}

int opt_g_sfv(void *arg, int m) {
	gfl |= F_OPT_SFV;
	return 0;
}

int opt_batch_output_formatting(void *arg, int m) {
	gfl |= F_OPT_FORMAT_BATCH;
	return 0;
}

int opt_compact_output_formatting(void *arg, int m) {
	gfl |= F_OPT_FORMAT_COMP;
	return 0;
}

int opt_g_nowrite(void *arg, int m) {
	gfl |= F_OPT_NOWRITE;
	return 0;
}

int opt_g_nobuffering(void *arg, int m) {
	gfl |= F_OPT_NOBUFFER;
	return 0;
}

int opt_g_buffering(void *arg, int m) {
	gfl ^= F_OPT_WBUFFER;
	return 0;
}

int opt_g_followlinks(void *arg, int m) {
	gfl |= F_OPT_FOLLOW_LINKS;
	return 0;
}

int opt_g_ftime(void *arg, int m) {
	gfl |= F_OPT_PS_TIME;
	return 0;
}

int opt_update_single_record(void *arg, int m) {
	argv_off = g_pg(arg, m);
	updmode = UPD_MODE_SINGLE;
	return 0;
}

int opt_recursive_update_records(void *arg, int m) {
	updmode = UPD_MODE_RECURSIVE;
	return 0;
}

int opt_raw_dump(void *arg, int m) {
	gfl |= F_OPT_MODE_RAWDUMP;
	return 0;
}

int opt_silent(void *arg, int m) {
	gfl |= F_OPT_PS_SILENT;
	return 0;
}

int opt_logging(void *arg, int m) {
	gfl |= F_OPT_PS_LOGGING;
	return 0;
}

int opt_nobackup(void *arg, int m) {
	gfl |= F_OPT_NOBACKUP;
	return 0;
}

int opt_backup(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_BACKUP;
	return 0;
}

int opt_exec(void *arg, int m) {
	exec_str = g_pd(arg, m, MAX_EXEC_STR);
	return 0;
}

int opt_shmipc(void *arg, int m) {
	char *buffer = g_pg(arg, m);

	if (!strlen(buffer)) {
		return 1;
	}

	SHM_IPC = (key_t) strtoul(buffer, NULL, 16);

	if (!SHM_IPC) {
		return 2;
	}

	ofl |= F_OVRR_IPC;

	return 0;
}

int opt_log_file(void *arg, int m) {
	g_cpg(arg, LOGFILE, m, PATH_MAX);
	gfl |= F_OPT_PS_LOGGING;
	return 0;
}

char MACRO_ARG1[255] = { 0 };
char MACRO_ARG2[255] = { 0 };
char MACRO_ARG3[255] = { 0 };

int opt_g_arg1(void *arg, int m) {
	g_cpg(arg, MACRO_ARG1, m, 255);
	gfl |= F_OPT_HAS_M_ARG1;
	return 0;
}

int opt_g_arg2(void *arg, int m) {
	g_cpg(arg, MACRO_ARG2, m, 255);
	gfl |= F_OPT_HAS_M_ARG2;
	return 0;
}

int opt_g_arg3(void *arg, int m) {
	g_cpg(arg, MACRO_ARG3, m, 255);
	gfl |= F_OPT_HAS_M_ARG3;
	return 0;
}

char *GLOBAL_PREEXEC = NULL;
char *GLOBAL_POSTEXEC = NULL;

int opt_g_preexec(void *arg, int m) {
	GLOBAL_PREEXEC = g_pd(arg, m, MAX_EXEC_STR);
	if (GLOBAL_PREEXEC) {
		gfl |= F_OPT_PREEXEC;
	}
	return 0;
}

int opt_g_postexec(void *arg, int m) {
	GLOBAL_POSTEXEC = g_pd(arg, m, MAX_EXEC_STR);
	if (GLOBAL_POSTEXEC) {
		gfl |= F_OPT_POSTEXEC;
	}
	return 0;
}

int opt_glroot(void *arg, int m) {
	g_cpg(arg, GLROOT, m, 255);
	ofl |= F_OVRR_GLROOT;
	return 0;
}

int opt_siteroot(void *arg, int m) {
	g_cpg(arg, SITEROOT_N, m, 255);
	ofl |= F_OVRR_SITEROOT;
	return 0;
}

int opt_dupefile(void *arg, int m) {
	g_cpg(arg, DUPEFILE, m, 255);
	ofl |= F_OVRR_DUPEFILE;
	return 0;
}

int opt_lastonlog(void *arg, int m) {
	g_cpg(arg, LASTONLOG, m, 255);
	ofl |= F_OVRR_LASTONLOG;
	return 0;
}

int opt_oneliner(void *arg, int m) {
	g_cpg(arg, ONELINERS, m, 255);
	ofl |= F_OVRR_ONELINERS;
	return 0;
}

int opt_rebuild(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_REBUILD;
	return 0;
}

int opt_dirlog_file(void *arg, int m) {
	g_cpg(arg, DIRLOG, m, 255);
	ofl |= F_OVRR_DIRLOG;
	return 0;
}

int opt_g_regexi(void *arg, int m) {
	g_cpg(arg, GLOB_REGEX, m, 4096);
	glob_regex_flags |= REG_ICASE;
	return 0;
}

int opt_g_sleep(void *arg, int m) {
	g_sleep = atoi(g_pg(arg, m));
	return 0;
}

int opt_g_usleep(void *arg, int m) {
	g_usleep = atoi(g_pg(arg, m));
	return 0;
}

int opt_g_match(void *arg, int m) {
	g_cpg(arg, GLOB_MATCH, m, 4096);
	glob_match_i_m = 0;
	return 0;
}

int opt_g_imatch(void *arg, int m) {
	g_cpg(arg, GLOB_MATCH, m, 4096);
	glob_match_i_m = 1;
	return 0;
}

int opt_g_regex(void *arg, int m) {
	g_cpg(arg, GLOB_REGEX, m, 4096);
	return 0;
}

int opt_g_iregexi(void *arg, int m) {
	g_cpg(arg, GLOB_REGEX, m, 4096);
	glob_regex_flags |= REG_ICASE;
	glob_reg_i_m = REG_NOMATCH;
	return 0;
}

int opt_g_iregex(void *arg, int m) {
	g_cpg(arg, GLOB_REGEX, m, 4096);
	glob_reg_i_m = REG_NOMATCH;
	return 0;
}

int opt_nukelog_file(void *arg, int m) {
	g_cpg(arg, NUKELOG, m, 255);
	ofl |= F_OVRR_NUKELOG;
	return 0;
}

int opt_dirlog_sections_file(void *arg, int m) {
	g_cpg(arg, DU_FLD, m, 255);
	return 0;
}

int print_version(void *arg, int m) {
	print_str("glutil-%d.%d-%d%s-%s\n", VER_MAJOR, VER_MINOR,
	VER_REVISION, VER_STR, ARCH ? "x86_64" : "i686");
	updmode = UPD_MODE_NOOP;
	return 0;
}

int opt_dirlog_check(void *arg, int m) {
	updmode = UPD_MODE_CHECK;
	return 0;
}

int opt_check_ghost(void *arg, int m) {
	gfl |= F_OPT_C_GHOSTONLY;
	return 0;
}

int opt_g_xdev(void *arg, int m) {
	gfl |= F_OPT_XDEV;
	return 0;
}

int opt_g_xblk(void *arg, int m) {
	gfl |= F_OPT_XBLK;
	return 0;
}

int opt_dirlog_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP;
	return 0;
}

int opt_dupefile_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_DUPEF;
	return 0;
}

int opt_online_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_ONL;
	return 0;
}

int opt_lastonlog_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_LON;
	return 0;
}

int opt_dump_users(void *arg, int m) {
	updmode = UPD_MODE_DUMP_USERS;
	return 0;
}

int opt_dump_grps(void *arg, int m) {
	updmode = UPD_MODE_DUMP_GRPS;
	return 0;
}

int opt_dirlog_dump_nukelog(void *arg, int m) {
	updmode = UPD_MODE_DUMP_NUKE;
	return 0;
}

int opt_oneliner_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_ONEL;
	return 0;
}

int print_help(void *arg, int m) {
	print_str(hpd_up, VER_MAJOR, VER_MINOR, VER_REVISION,
	VER_STR, ARCH ? "x86_64" : "i686");
	if (m != -1) {
		updmode = UPD_MODE_NOOP;
	}
	return 0;
}

int opt_g_ex_fork(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_FORK;
	gfl |= F_OPT_DAEMONIZE;
	return 0;
}

int opt_dirlog_chk_dupe(void *arg, int m) {
	updmode = UPD_MODE_DUPE_CHK;
	return 0;
}

int opt_membuffer_limit(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	if (!buffer) {
		return 1;
	}
	long long int l_buffer = atoll(buffer);
	if (l_buffer > 1024) {
		db_max_size = l_buffer;
		print_str("NOTICE: max memory buffer limit set to %lld bytes\n",
				l_buffer);
	} else {
		print_str(
				"NOTICE: invalid memory buffer limit, using default (%lld bytes)\n",
				db_max_size);
	}
	return 0;
}

/* generic types */
typedef int _d_ag_handle_i(struct g_handle *);
typedef int _d_achar_i(char *);
typedef int _d_avoid_i(void);

/* specific types */
typedef int __d_enum_cb(char *, unsigned char, void *);
typedef int __d_ref_to_val(void *, char *, char *, size_t);
typedef int __d_format_block(char *, ear *, char *);
typedef uint64_t __d_dlfind(char *, int, uint32_t, void *);
typedef pmda __d_cfg(pmda md, char * file);

_d_ag_handle_i g_cleanup, gh_rewind, determine_datatype, g_close;
_d_avoid_i dirlog_check_dupe, dirlog_print_stats, rebuild_dirlog,
		dirlog_check_records;
_d_achar_i self_get_path, file_exists, get_file_type, dir_exists,
		dirlog_update_record, g_dump_ug, g_dump_gen;

__d_enum_cb proc_section, proc_release, ssd_4macro, g_process_directory;

__d_ref_to_val ref_to_val_dirlog, ref_to_val_nukelog, ref_to_val_dupefile,
		ref_to_val_lastonlog, ref_to_val_oneliners, ref_to_val_online,
		ref_to_val_generic, ref_to_val_macro;
__d_format_block lastonlog_format_block, dupefile_format_block,
		oneliner_format_block, online_format_block, nukelog_format_block,
		dirlog_format_block;
__d_dlfind dirlog_find, dirlog_find_old, dirlog_find_simple;
__d_cfg search_cfg_rf, register_cfg_rf;

uint64_t file_crc32(char *, uint32_t *);

int data_backup_records(char*);
ssize_t file_copy(char *, char *, char *, uint32_t);

int split_string(char *, char, pmda);
int release_generate_block(char *, ear *);
off_t get_file_size(char *);
char **process_macro(void *, char **);
char *generate_chars(size_t, char, char*);
time_t get_file_creation_time(struct stat *);
int dirlog_write_record(struct dirlog *, off_t, int);

char *string_replace(char *, char *, char *, char *, size_t);
int enum_dir(char *, void *, void *, int);
int update_records(char *, int);
off_t read_file(char *, void *, size_t, off_t);
int option_crc32(void *, int);

int load_cfg(pmda md, char * file, uint32_t flags, pmda *res);
int free_cfg_rf(pmda md);

int reg_match(char *, char *, int);
int delete_file(char *, unsigned char, void *);

int write_file_text(char *, char *);

size_t str_match(char *, char *);
size_t exec_and_wait_for_output(char*, char*);

int get_file_type_no(struct stat *sb);

p_md_obj get_cfg_opt(char *, pmda, pmda*);

uint64_t nukelog_find(char *, int, struct nukelog *);
int parse_args(int argc, char **argv, void*fref_t[]);

int process_opt(char *, void *, void *, int);

int g_fopen(char *, char *, uint32_t, struct g_handle *);
void *g_read(void *buffer, struct g_handle *, size_t);
int process_exec_string(char *, char *, void *, void*);

int g_do_exec(void *, void *, char*);
int is_char_uppercase(char);

void sig_handler(int);
void child_sig_handler(int, siginfo_t*, void*);
int flush_data_md(struct g_handle *, char *);
int rebuild(void *);
int rebuild_data_file(char *, struct g_handle *);

int g_bmatch(void *, struct g_handle *);
int do_match(char *, void *, void *);

size_t g_load_data_md(void *, size_t, char *);
int g_load_record(struct g_handle *, const void *);
int remove_repeating_chars(char *string, char c);
p_md_obj md_first(pmda md);
int g_buffer_into_memory(char *, struct g_handle *);
int g_print_stats(char *, uint32_t, size_t);

int shmap(struct g_handle *, key_t);
int g_map_shm(struct g_handle *, key_t);

char *build_data_path(char *, char *, char *);
void *ref_to_val_get_cfgval(char *, char *, char *, int, char *, size_t);

int get_relative_path(char *, char *, char *);

void free_cfg(pmda);

int is_ascii_text(uint8_t in);

int g_init(int argc, char **argv);

char **build_argv(char *args, size_t max, int *c);

char *g_dgetf(char *str);

void *prio_f_ref[] = { "--silent", opt_silent, (void*) 0, "-arg1", opt_g_arg1,
		(void*) 1, "--arg1", opt_g_arg1, (void*) 1, "-arg2", opt_g_arg2,
		(void*) 1, "--arg2", opt_g_arg2, (void*) 1, "-arg3", opt_g_arg3,
		(void*) 1, "--arg3", opt_g_arg3, (void*) 1, "-vvvv", opt_g_verbose4,
		(void*) 0, "-vvv", opt_g_verbose3, (void*) 0, "-vv", opt_g_verbose2,
		(void*) 0, "-v", opt_g_verbose, (void*) 0, "-m", prio_opt_g_macro,
		(void*) 1, NULL, NULL,
		NULL };

void *f_ref[] = { "-xdev", opt_g_xdev, (void*) 0, "--xdev", opt_g_xdev,
		(void*) 0, "-xblk", opt_g_xblk, (void*) 0, "--xblk", opt_g_xblk,
		(void*) 0, "-file", opt_g_udc_f, (void*) 0, "--file", opt_g_udc_f,
		(void*) 0, "-dir", opt_g_udc_dir, (void*) 0, "--dir", opt_g_udc_dir,
		(void*) 0, "--loopmax", opt_loop_max, (void*) 1, "--ghost",
		opt_check_ghost, (void*) 0, "-x", opt_g_udc, (void*) 1, "-recursive",
		opt_g_recursive, (void*) 0, "--recursive", opt_g_recursive, (void*) 0,
		"-g", opt_dump_grps, (void*) 0, "-t", opt_dump_users, (void*) 0,
		"--backup", opt_backup, (void*) 1, "--postexec", opt_g_postexec,
		(void*) 1, "--preexec", opt_g_preexec, (void*) 1, "--usleep",
		opt_g_usleep, (void*) 1, "--sleep", opt_g_sleep, (void*) 1, "-arg1",
		NULL, (void*) 1, "--arg1", NULL, (void*) 1, "-arg2",
		NULL, (void*) 1, "--arg2", NULL, (void*) 1, "-arg3", NULL, (void*) 1,
		"--arg3", NULL, (void*) 1, "-m",
		NULL, (void*) 1, "--imatch", opt_g_imatch, (void*) 1, "--match",
		opt_g_match, (void*) 1, "--fork", opt_g_ex_fork, (void*) 1, "-vvvv",
		opt_g_verbose4, (void*) 0, "-vvv", opt_g_verbose3, (void*) 0, "-vv",
		opt_g_verbose2, (void*) 0, "-v", opt_g_verbose, (void*) 0, "--loglevel",
		opt_g_loglvl, (void*) 1, "--ftime", opt_g_ftime, (void*) 0, "--logfile",
		opt_log_file, (void*) 0, "--log", opt_logging, (void*) 0, "--silent",
		opt_silent, (void*) 0, "--loopexec", opt_g_loopexec, (void*) 1,
		"--loop", opt_g_loop, (void*) 1, "--daemon", opt_g_daemonize, (void*) 0,
		"-w", opt_online_dump, (void*) 0, "--ipc", opt_shmipc, (void*) 1, "-l",
		opt_lastonlog_dump, (void*) 0, "--oneliners", opt_oneliner, (void*) 1,
		"-o", opt_oneliner_dump, (void*) 0, "--lastonlog", opt_lastonlog,
		(void*) 1, "-i", opt_dupefile_dump, (void*) 0, "--dupefile",
		opt_dupefile, (void*) 1, "--nowbuffer", opt_g_buffering, (void*) 0,
		"--raw", opt_raw_dump, (void*) 0, "--iregexi", opt_g_iregexi, (void*) 1,
		"--iregex", opt_g_iregex, (void*) 1, "--regexi", opt_g_regexi,
		(void*) 1, "--regex", opt_g_regex, (void*) 1, "-e", opt_rebuild,
		(void*) 1, "--comp", opt_compact_output_formatting, (void*) 0,
		"--batch", opt_batch_output_formatting, (void*) 0, "-y",
		opt_g_followlinks, (void*) 0, "--allowsymbolic", opt_g_followlinks,
		(void*) 0, "--followlinks", opt_g_followlinks, (void*) 0,
		"--allowlinks", opt_g_followlinks, (void*) 0, "-exec", opt_exec,
		(void*) 1, "--exec", opt_exec, (void*) 1, "--fix", opt_g_fix, (void*) 0,
		"-u", opt_g_update, (void*) 0, "--memlimit", opt_membuffer_limit,
		(void*) 1, "-p", opt_dirlog_chk_dupe, (void*) 0, "--dupechk",
		opt_dirlog_chk_dupe, (void*) 0, "-b", opt_g_nobuffering, (void*) 0,
		"--nobuffer", opt_g_nobuffering, (void*) 0, "--nukedump",
		opt_dirlog_dump_nukelog, (void*) 0, "-n", opt_dirlog_dump_nukelog,
		(void*) 0, "--help", print_help, (void*) 0, "--version", print_version,
		(void*) 0, "--folders", opt_dirlog_sections_file, (void*) 1, "--dirlog",
		opt_dirlog_file, (void*) 1, "--nukelog", opt_nukelog_file, (void*) 1,
		"--siteroot", opt_siteroot, (void*) 1, "--glroot", opt_glroot,
		(void*) 1, "-k", opt_g_nowrite, (void*) 0, "--nowrite", opt_g_nowrite,
		(void*) 0, "--sfv", opt_g_sfv, (void*) 0, "--crc32", option_crc32,
		(void*) 1, "--nobackup", opt_nobackup, (void*) 0, "-c",
		opt_dirlog_check, (void*) 0, "--check", opt_dirlog_check, (void*) 0,
		"--dump", opt_dirlog_dump, (void*) 0, "-d", opt_dirlog_dump, (void*) 0,
		"-f", opt_g_force, (void*) 0, "-s", opt_update_single_record, (void*) 1,
		"-r", opt_recursive_update_records, (void*) 0, NULL, NULL, NULL };

int md_init(pmda md, int nm) {
	if (md->objects) {
		return 1;
	}
	bzero(md, sizeof(mda));
	md->objects = calloc(nm, sizeof(md_obj));
	md->count = nm;
	md->pos = md->objects;
	md->r_pos = md->objects;
	return 0;
}

int md_g_free(pmda md) {
	if (!md || !md->objects)
		return 1;

	if (!(md->flags & F_MDA_REFPTR)) {
		p_md_obj ptr = md_first(md), ptr_s;
		while (ptr) {
			ptr_s = ptr->next;
			if (ptr->ptr) {
				g_free(ptr->ptr);
				ptr->ptr = NULL;
			}
			ptr = ptr_s;
		}
	}

	g_free(md->objects);
	bzero(md, sizeof(mda));

	return 0;
}

AAINT md_relink(pmda md) {
	off_t off, l = 1;

	p_md_obj last = NULL, cur = md->objects;

	for (off = 0; off < md->count; off++) {
		if (cur->ptr) {
			if (last) {
				last->next = cur;
				cur->prev = last;
				l++;
			}
			last = cur;
		}
		cur++;
	}
	return l;
}

p_md_obj md_first(pmda md) {
	off_t off = 0;
	p_md_obj ptr = md->objects;

	for (off = 0; off < md->count; off++, ptr++) {
		if (ptr->ptr) {
			return ptr;
		}
	}

	return NULL;
}

#define MDA_MDALLOC_RE	0x1

void *md_alloc(pmda md, int b) {
	int flags = 0;

	if (md->offset >= md->count) {
		if (gfl & F_OPT_VERBOSE3) {
			print_str(
					"NOTICE: re-allocating memory segment to increase size; current address: 0x%.16llX, current size: %llu\n",
					(ulint64_t) (AAINT) md->objects, (ulint64_t) md->count);
		}
		md->objects = realloc(md->objects, (md->count * sizeof(md_obj)) * 2);
		md->pos = md->objects;
		md->pos += md->count;
		bzero(md->pos, md->count * sizeof(md_obj));

		md->count *= 2;
		AAINT rlc = md_relink(md);
		flags |= MDA_MDALLOC_RE;
		if (gfl & F_OPT_VERBOSE3) {
			print_str(
					"NOTICE: re-allocation done; new address: 0x%.16llX, new size: %llu, re-linked %llu records\n",
					(ulint64_t) (AAINT) md->objects, (ulint64_t) md->count,
					(ulint64_t) rlc);
		}
	}

	p_md_obj prev = md->pos;
	AAINT pcntr = 0;
	while (md->pos->ptr
			&& (pcntr = ((md->pos - md->objects) / sizeof(md_obj))) < md->count) {
		md->pos++;
	}

	if (pcntr >= md->count) {
		return NULL;
	}

	if (md->pos > md->objects && !(md->pos - 1)->ptr) {
		flags |= MDA_MDALLOC_RE;
	}

	if (md->pos->ptr)
		return NULL;

	if (md->flags & F_MDA_REFPTR) {
		md->pos->ptr = md->lref_ptr;
	} else {
		md->pos->ptr = calloc(1, b);
	}

	if (prev != md->pos) {
		prev->next = md->pos;
		md->pos->prev = prev;
	}

	md->offset++;

	if (flags & MDA_MDALLOC_RE) {
		md_relink(md);
	}

	return md->pos->ptr;
}

void *md_unlink(pmda md, p_md_obj md_o) {
	if (!md_o) {
		return NULL;
	}

	p_md_obj c_ptr = NULL;

	if (md_o->prev) {
		((p_md_obj) md_o->prev)->next = (p_md_obj) md_o->next;
		c_ptr = md_o->prev;
	}

	if (md_o->next) {
		((p_md_obj) md_o->next)->prev = (p_md_obj) md_o->prev;
		c_ptr = md_o->next;
	}

	md->offset--;
	if (md->pos == md_o && c_ptr) {
		md->pos = c_ptr;
	}
	if (!(md->flags & F_MDA_REFPTR) && md_o->ptr) {
		g_free(md_o->ptr);
	}
	md_o->ptr = NULL;

	return (void*) c_ptr;
}

int setup_sighandlers(void) {
	struct sigaction sa = { { 0 } }, sa_c = { { 0 } }, sa_e = { { 0 } };
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

int g_shutdown(void *arg) {
	g_cleanup(&g_act_1);
	g_cleanup(&g_act_2);
	free_cfg_rf(&cfg_rf);
	if (NUKESTR) {
		g_free(NUKESTR);
	}

	if ((gfl & F_OPT_PS_LOGGING) && fd_log) {
		g_fclose(fd_log);
	}

	if (_p_macro_argv) {
		g_free(_p_macro_argv);
	}

	if (GLOBAL_PREEXEC) {
		g_free(GLOBAL_PREEXEC);
	}

	if (GLOBAL_POSTEXEC) {
		g_free(GLOBAL_POSTEXEC);
	}

	if (LOOPEXEC) {
		g_free(LOOPEXEC);
	}

	if (exec_str) {
		g_free(exec_str);
	}

	_p_macro_argc = 0;

	exit(EXITVAL);
}

int g_cleanup(struct g_handle *hdl) {
	int r = 0;

	r += md_g_free(&hdl->buffer);
	r += md_g_free(&hdl->w_buffer);
	if (!(hdl->flags & F_GH_SHM) && hdl->data) {
		g_free(hdl->data);
	} else if ((hdl->flags & F_GH_SHM) && hdl->shmid && hdl->data) {
		shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf);
		if (hdl->ipcbuf.shm_nattch <= 1) {
			shmctl(hdl->shmid, IPC_RMID, 0);
		}
		shmdt(hdl->data);
	}
	bzero(hdl, sizeof(struct g_handle));
	return r;
}

char *build_data_path(char *file, char *path, char *sd) {
	remove_repeating_chars(path, 0x2F);

	if (strlen(path) && !file_exists(path)) {
		return path;
	}

	char buffer[4096] = { 0 };

	sprintf(buffer, "%s/%s/%s/%s", GLROOT, FTPDATA, sd, file);

	remove_repeating_chars(buffer, 0x2F);

	size_t blen = strlen(buffer), plen = strlen(path);

	if (blen > 255) {
		return path;
	}

	if (plen == blen && !strncmp(buffer, path, plen)) {
		return path;
	}

	if (gfl & F_OPT_VERBOSE3) {
		print_str("NOTICE: %s: was not found, setting default data path: %s\n",
				path, buffer);
	}

	bzero(path, 255);
	g_memcpy(path, buffer, strlen(buffer));
	return path;
}

#define F_LCONF_NORF 	0x1

int g_init(int argc, char **argv) {
	g_setjmp(0, "g_init", NULL, NULL);
	int r;

	if (strlen(LOGFILE)) {
		gfl |= F_OPT_PS_LOGGING;
	}

	if (setup_sighandlers()) {
		print_str(
				"WARNING: UNABLE TO SETUP SIGNAL HANDLERS! (this is weird, please report it!)\n");
		sleep(5);
	}

	r = parse_args(argc, argv, f_ref);
	if (r == -2 || r == -1) {
		print_help(NULL, -1);
		return 4;
	}

	if (gfl & F_OPT_PS_LOGGING) {
		build_data_path(DEFF_DULOG, LOGFILE, DEFPATH_LOGS);
		if (!(fd_log = gg_fopen(LOGFILE, "a"))) {
			gfl ^= F_OPT_PS_LOGGING;
			if (!(gfl & F_OPT_MODE_RAWDUMP)) {
				print_str(
						"ERROR: %s: [%d]: could not open file for writing, logging disabled\n",
						LOGFILE, errno);
			}
		}
	}

	if (updmode && updmode != UPD_MODE_NOOP && !(gfl & F_OPT_MODE_RAWDUMP)
			&& !(gfl & F_OPT_FORMAT_COMP)) {
		print_str("INIT: glutil %d.%d-%d%s-%s starting [PID: %d]\n",
		VER_MAJOR,
		VER_MINOR,
		VER_REVISION, VER_STR, ARCH ? "x86_64" : "i686", getpid());
	}

#ifdef GLCONF

	if ((r = load_cfg(&glconf, GLCONF, F_LCONF_NORF, NULL))) {
		print_str("WARNING: %s: could not load GLCONF file [%d]\n", GLCONF, r);
	}

	if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_MODE_RAWDUMP) && glconf.offset) {
		print_str("NOTICE: %s: loaded %d config lines into memory\n", GLCONF,
				(int) glconf.offset);
	}

	p_md_obj ptr = get_cfg_opt("ipc_key", &glconf, NULL);

	if (ptr && !(ofl & F_OVRR_IPC)) {
		SHM_IPC = (key_t) strtol(ptr->ptr, NULL, 16);
	}

	ptr = get_cfg_opt("rootpath", &glconf, NULL);

	if (ptr && !(ofl & F_OVRR_GLROOT)) {
		bzero(GLROOT, 255);
		g_memcpy(GLROOT, ptr->ptr, strlen((char*) ptr->ptr));
		if ((gfl & F_OPT_VERBOSE2) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str("GLCONF: using 'rootpath': %s\n", GLROOT);
		}
	}

	ptr = get_cfg_opt("min_homedir", &glconf, NULL);

	if (ptr && !(ofl & F_OVRR_SITEROOT)) {
		bzero(SITEROOT_N, 255);
		g_memcpy(SITEROOT_N, ptr->ptr, strlen((char*) ptr->ptr));
		if ((gfl & F_OPT_VERBOSE2) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str("GLCONF: using 'min_homedir': %s\n", SITEROOT_N);
		}
	}

	ptr = get_cfg_opt("ftp-data", &glconf, NULL);

	if (ptr) {
		bzero(FTPDATA, 255);
		g_memcpy(FTPDATA, ptr->ptr, strlen((char*) ptr->ptr));
		if ((gfl & F_OPT_VERBOSE2) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str("GLCONF: using 'ftp-data': %s\n", FTPDATA);
		}
	}

	ptr = get_cfg_opt("nukedir_style", &glconf, NULL);

	if (ptr) {
		NUKESTR = calloc(255, 1);
		NUKESTR = string_replace(ptr->ptr, "%N", "%s", NUKESTR, 255);
		if ((gfl & F_OPT_VERBOSE2) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str("GLCONF: using 'nukedir_style': %s\n", NUKESTR);
		}
		ofl |= F_OVRR_NUKESTR;

	}

	remove_repeating_chars(FTPDATA, 0x2F);
#else
	if ( && !(gfl & F_OPT_MODE_RAWDUMP) ) {
		print_str("WARNING: GLCONF not defined in glconf.h");
	}
#endif

	remove_repeating_chars(GLROOT, 0x2F);
	remove_repeating_chars(SITEROOT_N, 0x2F);

	if (!strlen(GLROOT)) {
		print_str("ERROR: glftpd root directory not specified!\n");
		return 2;
	}

	if (!strlen(SITEROOT_N)) {
		print_str("ERROR: glftpd site root directory not specified!\n");
		return 2;
	}

	build_data_path(DEFF_DIRLOG, DIRLOG, DEFPATH_LOGS);
	build_data_path(DEFF_NUKELOG, NUKELOG, DEFPATH_LOGS);
	build_data_path(DEFF_LASTONLOG, LASTONLOG, DEFPATH_LOGS);
	build_data_path(DEFF_DUPEFILE, DUPEFILE, DEFPATH_LOGS);
	build_data_path(DEFF_ONELINERS, ONELINERS, DEFPATH_LOGS);

	bzero(SITEROOT, 255);
	sprintf(SITEROOT, "%s%s", GLROOT, SITEROOT_N);
	remove_repeating_chars(SITEROOT, 0x2F);

	if (strlen(GLOB_REGEX)) {
		gfl |= F_OPT_HAS_G_REGEX;
	}

	if (strlen(GLOB_MATCH)) {
		gfl |= F_OPT_HAS_G_MATCH;
	}

	if (!updmode && (gfl & F_OPT_SFV)) {
		updmode = UPD_MODE_RECURSIVE;
		if (!(gfl & F_OPT_NOWRITE)) {
			gfl |= F_OPT_FORCEWSFV | F_OPT_NOWRITE;
		}

		if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str(
					"NOTICE: switching to non-destructive filesystem rebuild mode\n");
		}
	}

	if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_MODE_RAWDUMP)) {
		if (gfl & F_OPT_NOBUFFER) {
			print_str("NOTICE: disabling memory buffering\n");
		}
		if (SHM_IPC && SHM_IPC != shm_ipc) {
			print_str("NOTICE: IPC key set to '0x%.8X'\n", SHM_IPC);
		}
		if (ofl & F_OVRR_GLROOT) {
			print_str("NOTICE: GLROOT path set to '%s'\n", GLROOT);
		}
		if (ofl & F_OVRR_SITEROOT) {
			print_str("NOTICE: SITEROOT path set to '%s'\n", SITEROOT);
		}
		if (ofl & F_OVRR_DIRLOG) {
			print_str("NOTICE: DIRLOG path set to '%s'\n", DIRLOG);
		}
		if (ofl & F_OVRR_NUKELOG) {
			print_str("NOTICE: NUKELOG path set to '%s'\n", NUKELOG);
		}
		if (ofl & F_OVRR_DUPEFILE) {
			print_str("NOTICE: DUPEFILE path set to '%s'\n", DUPEFILE);
		}
		if (ofl & F_OVRR_LASTONLOG) {
			print_str("NOTICE: LASTONLOG path set to '%s'\n", LASTONLOG);
		}
		if (ofl & F_OVRR_ONELINERS) {
			print_str("NOTICE: ONELINERS path set to '%s'\n", ONELINERS);
		}
		if ((gfl & F_OPT_VERBOSE2) && (gfl & F_OPT_PS_LOGGING)) {
			print_str("NOTICE: Logging enabled: %s\n", LOGFILE);
		}
	}

	if ((gfl & F_OPT_VERBOSE) && (gfl & F_OPT_NOWRITE)
			&& !(gfl & F_OPT_MODE_RAWDUMP)) {
		print_str("NOTICE: performing dry run, no writing will be done\n");
	}

	if (updmode && (gfl & F_OPT_PREEXEC)) {
		print_str("PREEXEC: running: '%s'\n", GLOBAL_PREEXEC);
		if (g_do_exec(NULL, ref_to_val_generic, GLOBAL_PREEXEC) == -1) {
			print_str("ERROR: POSTEXEC failed: '%s'\n", GLOBAL_PREEXEC);
		}
	}

	if (gfl & F_OPT_DAEMONIZE) {
		if (!(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str("NOTICE: forking into background.. [PID: %d]\n",
					getpid());
		}
		daemon(0, 0);
	}

	if (g_usleep) {
		usleep(g_usleep);
	} else if (g_sleep) {
		sleep(g_sleep);
	}

	uint64_t mloop_c = 0;
	char m_b1[128];
	int m_f = 0x1;

	g_setjmp(0, "main(start)", NULL, NULL);

	enter:

	if ((m_f & 0x1)) {
		sprintf(m_b1, "main(loop) [c:%llu]", (long long unsigned int) mloop_c);
		g_setjmp(0, m_b1, NULL, NULL);
		m_f ^= 0x1;
	}

	switch (updmode) {
	case UPD_MODE_RECURSIVE:
		rebuild_dirlog();
		break;
	case UPD_MODE_SINGLE:
		dirlog_update_record(argv_off);
		break;
	case UPD_MODE_CHECK:
		dirlog_check_records();
		break;
	case UPD_MODE_DUMP:
		dirlog_print_stats();
		break;
	case UPD_MODE_DUMP_NUKE:
		g_print_stats(NUKELOG, 0, 0);
		break;
	case UPD_MODE_DUMP_DUPEF:
		g_print_stats(DUPEFILE, 0, 0);
		break;
	case UPD_MODE_DUMP_LON:
		g_print_stats(LASTONLOG, 0, 0);
		break;
	case UPD_MODE_DUMP_ONEL:
		g_print_stats(ONELINERS, 0, 0);
		break;
	case UPD_MODE_DUPE_CHK:
		dirlog_check_dupe();
		break;
	case UPD_MODE_REBUILD:
		rebuild(p_argv_off);
		break;
	case UPD_MODE_DUMP_ONL:
		g_print_stats("ONLINE USERS", F_DL_FOPEN_SHM, ON_SZ);
		break;
	case UPD_MODE_FORK:
		if (p_argv_off) {
			system(p_argv_off);
		}
		break;
	case UPD_MODE_BACKUP:
		data_backup_records(g_dgetf(p_argv_off));
		break;
	case UPD_MODE_DUMP_USERS:
		g_dump_ug(DEFPATH_USERS);
		break;
	case UPD_MODE_DUMP_GRPS:
		g_dump_ug(DEFPATH_GROUPS);
		break;
	case UPD_MODE_DUMP_GEN:
		g_dump_gen(p_argv_off);
		break;
	case UPD_MODE_NOOP:
		break;
	default:
		print_str("ERROR: no mode specified\n");
		print_help(NULL, -1);
		break;
	}

	if ((gfl & F_OPT_LOOP) && !(gfl & F_OPT_KILL_GLOBAL)
			&& (!loop_max || mloop_c < loop_max - 1)) {
		g_cleanup(&g_act_1);
		g_cleanup(&g_act_2);
		free_cfg_rf(&cfg_rf);
		sleep(loop_interval);
		if (gfl & F_OPT_LOOPEXEC) {
			g_do_exec(NULL, ref_to_val_generic, LOOPEXEC);
		}
		mloop_c++;
		goto enter;
	}

	if (updmode && (gfl & F_OPT_POSTEXEC)) {
		print_str("POSTEXEC: running: '%s'\n", GLOBAL_POSTEXEC);
		if (g_do_exec(NULL, ref_to_val_generic, GLOBAL_POSTEXEC) == -1) {
			print_str("ERROR: POSTEXEC failed: '%s'\n", GLOBAL_POSTEXEC);
		}
	}

	g_shutdown(NULL);

	return EXITVAL;
}

int main(int argc, char *argv[]) {
	char **p_argv = (char**) argv;

	_p_macro_argc = argc;

	parse_args(argc, argv, prio_f_ref);

	switch (updmode) {
	case PRIO_UPD_MODE_MACRO:
		;
		char **ptr;
		ptr = process_macro(prio_argv_off, NULL);
		if (ptr) {
			_p_macro_argv = p_argv = ptr;
			gfl = F_OPT_WBUFFER;
		} else {
			g_shutdown(NULL);
		}
		break;
	}

	updmode = 0;

	g_init(_p_macro_argc, p_argv);
	g_shutdown(NULL);

	return EXITVAL;
}

char **process_macro(void * arg, char **out) {
	g_setjmp(0, "process_macro", NULL, NULL);
	if (!arg) {
		print_str("ERROR: missing data type argument (-m <macro name>)\n");
		return NULL;
	}

	char *a_ptr = (char*) arg;

	char buffer[PATH_MAX] = { 0 };

	if (self_get_path(buffer)) {
		print_str("ERROR: could not get own path\n");
		return NULL;
	}

	char *dirn = dirname(buffer);

	_si_argv0 av = { 0 };

	av.ret = -1;

	g_strncpy(av.p_buf_1, a_ptr, strlen(a_ptr));

	if (gfl & F_OPT_VERBOSE) {
		print_str("MACRO: '%s': searching for macro inside '%s/' (recursive)\n",
				av.p_buf_1, dirn);
	}

	if (enum_dir(dirn, ssd_4macro, &av, 0) < 0) {
		print_str("ERROR: %s: recursion failed (macro not found)\n",
				av.p_buf_1);
		return NULL;
	}

	if (av.ret == -1) {
		print_str("ERROR: %s: could not find macro\n", av.p_buf_1);
		return NULL;
	}

	g_strncpy(b_spec1, av.p_buf_2, strlen(av.p_buf_2));

	char *s_buffer = (char*) calloc(262144, 1), **s_ptr = NULL;
	int r;

	if ((r = process_exec_string(av.s_ret, s_buffer, ref_to_val_macro,
	NULL))) {
		print_str("ERROR: [%d]: could not process exec string: '%s'\n", r,
				av.s_ret);
		goto end;
	}

	int c = 0;
	s_ptr = build_argv(s_buffer, 4096, &c);

	if (!c) {
		print_str("ERROR: %s: macro was declared, but no arguments found\n",
				av.p_buf_1);
	}

	_p_macro_argc = c;

	if (gfl & F_OPT_VERBOSE2) {
		print_str("MACRO: '%s': built argument string array with %d elements\n",
				av.p_buf_1, c);
	}

	print_str("MACRO: '%s': EXECUTING: '%s'\n", av.p_buf_1, s_buffer);

	end:

	g_free(s_buffer);

	return s_ptr;
}

char **build_argv(char *args, size_t max, int *c) {
	char **ptr = (char **) calloc(max, sizeof(char **));

	size_t args_l = strlen(args);
	int i_0, l_p = 0, b_c = 0;
	char sp_1 = 0x20, sp_2 = 0x22;

	*c = 0;

	for (i_0 = 0; i_0 < args_l && b_c < max; i_0++) {
		if (i_0 == 0) {
			while (args[i_0] == sp_1) {
				i_0++;
			}
			if (args[i_0] == sp_2) {
				while (args[i_0] == sp_2) {
					i_0++;
				}
				sp_1 = sp_2;
				l_p = i_0;
			}
		}
		if (((args[i_0] == sp_1 || args[i_0] == sp_2) || i_0 == args_l - 1)
				&& i_0 > l_p) {

			if (i_0 == args_l - 1) {
				if (!(args[i_0] == sp_1 || args[i_0] == sp_2)) {
					i_0++;
				}

			}

			size_t ptr_b_l = i_0 - l_p;
			ptr[b_c] = (char*) calloc(ptr_b_l + 1, 1);
			g_strncpy(ptr[b_c], &args[l_p], ptr_b_l);
			b_c++;
			*c += 1;

			int ii_l = 1;
			while (args[i_0] == sp_1 || args[i_0] == sp_2) {
				if (sp_1 != sp_2) {
					if (args[i_0] == sp_2) {
						i_0++;
						break;
					}
				}
				i_0++;
			}
			l_p = i_0;
			if (sp_1 == sp_2) {
				sp_1 = 0x20;
				sp_2 = 0x22;
				while (args[i_0] == sp_1 || args[i_0] == sp_2) {
					i_0++;
				}
				l_p = i_0;
			} else {
				if (args[i_0 - ii_l] == 0x22) {
					sp_1 = 0x22;
					sp_2 = 0x22;
					while (args[i_0] == sp_1 || args[i_0] == sp_2) {
						i_0++;
					}
					l_p = i_0;
				}

			}

		}

	}

	return ptr;
}

char *g_dgetf(char *str) {
	if (!str) {
		return NULL;
	}
	if (!strncmp(str, "dirlog", 6)) {
		return DIRLOG;
	} else if (!strncmp(str, "nukelog", 7)) {
		return NUKELOG;
	} else if (!strncmp(str, "dupefile", 8)) {
		return DUPEFILE;
	} else if (!strncmp(str, "lastonlog", 9)) {
		return LASTONLOG;
	} else if (!strncmp(str, "oneliners", 9)) {
		return ONELINERS;
	}
	return NULL;
}

int rebuild(void *arg) {
	g_setjmp(0, "rebuild", NULL, NULL);
	if (!arg) {
		print_str("ERROR: missing data type argument (-e <dirlog|nukelog>)\n");
		return 1;
	}

	char *a_ptr = (char*) arg;
	char *datafile = g_dgetf(a_ptr);
	struct g_handle hdl = { 0 };

	if (datafile) {
		if (g_fopen(datafile, "r", F_DL_FOPEN_BUFFER, &hdl)) {
			return 3;
		}

		if (!hdl.buffer_count) {
			print_str(
					"ERROR: data log rebuilding requires buffering, increase mem limit (or dump with --raw --nobuffer for huge files)\n");
			return 4;
		}

		if (rebuild_data_file(datafile, &hdl)) {
			print_str(MSG_GEN_DFRFAIL, datafile);
			return 5;
		}

		print_str("STATS: %s: wrote %llu bytes in %llu records\n", datafile,
				(ulint64_t) hdl.bw, (ulint64_t) hdl.rw);
	} else {
		print_str("ERROR: [%s] unrecognized data type\n", a_ptr);
		return 2;
	}

	return 0;
}

typedef struct ___std_rh_0 {
	uint32_t flags;
	uint64_t st_1, st_2;
} _std_rh, *__std_rh;

int g_dump_ug(char *ug) {
	g_setjmp(0, "g_dump_ug", NULL, NULL);
	_std_rh ret = { 0 };
	char buffer[PATH_MAX] = { 0 };

	ret.flags = flags_udcfg | F_PD_MATCHREG;

	sprintf(buffer, "%s/%s/%s", GLROOT, FTPDATA, ug);

	remove_repeating_chars(buffer, 0x2F);

	return enum_dir(buffer, g_process_directory, &ret, 0);
}

int g_dump_gen(char *root) {
	g_setjmp(0, "g_dump_gen", NULL, NULL);
	if (!root) {
		return 1;
	}

	_std_rh ret = { 0 };

	ret.flags = flags_udcfg;

	if (!(ret.flags & F_PD_MATCHTYPES)) {
		ret.flags |= F_PD_MATCHTYPES;
	}

	if (!file_exists(root)) {
		ret.flags ^= F_PD_MATCHTYPES;
		ret.flags |= F_PD_MATCHREG;
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: %s is a file\n", root);
		}
		g_process_directory(root, DT_REG, &ret);
		return 0;
	}

	char buffer[PATH_MAX] = { 0 };
	sprintf(buffer, "%s", root);
	remove_repeating_chars(buffer, 0x2F);

	int r = enum_dir(buffer, g_process_directory, &ret, 0);

	print_str("STATS: %s: OK: %llu/%llu\n", root,
			(unsigned long long int) ret.st_1,
			(unsigned long long int) ret.st_1 + ret.st_2);

	return r;
}

int g_process_directory(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "g_process_directory", NULL, NULL);
	__std_rh aa_rh = (__std_rh) arg;

	switch (type) {
	case DT_REG:
		if (aa_rh->flags & F_PD_MATCHREG) {
			print_str("FILE: %s\n", name);
			if (do_match(name, name, ref_to_val_generic)) {
				aa_rh->st_2++;
				break;
			}
			aa_rh->st_1++;
		}
		break;
	case DT_DIR:
		if (aa_rh->flags & F_PD_MATCHDIR) {
			print_str("DIR: %s\n", name);
			if (do_match(name, name, ref_to_val_generic)) {
				aa_rh->st_2++;
				break;
			}
			aa_rh->st_1++;
		}
		if (aa_rh->flags & F_PD_RECURSIVE) {
			enum_dir(name, g_process_directory, arg, 0);
		}
		break;
	}

	return 0;
}

/*
 typedef struct ___ch_dp {
 int nres;
 char **res;
 } _ch_dp, *__ch_dp;

 int dirlog_check_dupe2(void) {
 if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &g_act_1)) {
 return 1;
 }

 if (!g_act_1.buffer.count) {
 return 2;
 }

 __ch_dp res_d = calloc(g_act_1.buffer.count, sizeof(_ch_dp));

 int res_d_i = 0;
 p_md_obj ptr = NULL;
 struct dirlog *d_ptr = NULL;

 while (ptr) {
 d_ptr = (struct dirlog *) ptr->ptr;

 ptr = ptr->next;
 }

 return 0;
 }
 */

void g_progress_stats(time_t s_t, time_t e_t, off_t total, off_t done) {
	float diff = (float) (e_t - s_t);
	float rate = ((float) done / diff);

	fprintf(stderr,
			"PROCESSING: %llu/%llu [ %.2f%s ] | %.2f r/s | ETA: %.1f s\r",
			(long long unsigned int) done, (long long unsigned int) total,
			((float) done / ((float) total / 100.0)), "%", rate,
			(float) total / rate);

}

int dirlog_check_dupe(void) {
	g_setjmp(0, "dirlog_check_dupe", NULL, NULL);
	struct dirlog buffer, buffer2;
	struct dirlog *d_ptr = NULL, *dd_ptr = NULL;
	char *s_buffer, *ss_buffer, *s_pb, *ss_pb;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &g_act_1)) {
		return 2;
	}
	off_t st1, st2 = 0, st3 = 0;
	p_md_obj pmd_st1 = NULL, pmd_st2 = NULL;
	g_setjmp(0, "dirlog_check_dupe(loop)", NULL, NULL);
	time_t s_t = time(NULL), e_t = time(NULL), d_t = time(NULL);

	off_t nrec = g_act_1.total_sz / g_act_1.block_sz;

	if (g_act_1.buffer_count) {
		nrec = g_act_1.buffer_count;
	}
	g_progress_stats(s_t, e_t, nrec, st3);
	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		//if (!sigsetjmp(g_sigjmp.env, 1)) {
		st3++;
		if (gfl & F_OPT_KILL_GLOBAL) {
			break;
		}
		/*if (g_act_1.buffer.pos && (g_act_1.buffer.pos->flags & F_MD_NOREAD)) {
		 continue;
		 }*/
		if (g_bmatch(d_ptr, &g_act_1)) {
			continue;
		}

		g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_dupe(loop)(2)", NULL, NULL);

		s_buffer = strdup(d_ptr->dirname);
		s_pb = basename(s_buffer);
		size_t s_pb_l = strlen(s_pb);
		if (s_pb_l < 4) {
			goto end_loop1;
		}

		if (gfl & F_OPT_VERBOSE) {
			e_t = time(NULL);

			if (e_t - d_t) {
				d_t = time(NULL);
				g_progress_stats(s_t, e_t, nrec, st3);
			}
		}

		st1 = g_act_1.offset;
		//g_act_1.offset = 0;
		if (g_act_1.buffer_count) {
			//st2 = g_act_1.buffer_count;
			pmd_st1 = g_act_1.buffer.r_pos;
			pmd_st2 = g_act_1.buffer.pos;
		} else {
			st2 = (off_t) ftello(g_act_1.fh);
		}
		//gh_rewind(&g_act_1);
		g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_dupe(loop)(3)",
		NULL,
		NULL);
		int ch = 0;
		while ((dd_ptr = (struct dirlog *) g_read(&buffer2, &g_act_1, DL_SZ))) {
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			ss_buffer = strdup(dd_ptr->dirname);
			ss_pb = basename(ss_buffer);
			size_t ss_pb_l = strlen(ss_pb);

			if (ss_pb_l == s_pb_l && !strncmp(s_pb, ss_pb, s_pb_l)
					&& strncmp(d_ptr->dirname, dd_ptr->dirname,
							strlen(d_ptr->dirname))) {
				/*if (g_act_1.buffer_count && g_act_1.buffer.pos) {
				 g_act_1.buffer.pos->flags |= F_MD_NOREAD;
				 }*/

				if (!ch) {
					print_str("\rDUPE %s\n", d_ptr->dirname);
				}
				print_str("\rDUPE %s\n", dd_ptr->dirname);
				ch++;
			}
			g_free(ss_buffer);
		}

		g_act_1.offset = st1;
		if (g_act_1.buffer_count) {
			//g_act_1.buffer.offset = st2;
			g_act_1.buffer.r_pos = pmd_st1;
			g_act_1.buffer.pos = pmd_st2;
		} else {
			fseeko(g_act_1.fh, (off_t) st2, SEEK_SET);
		}
		end_loop1:

		g_free(s_buffer);
	}
	if (gfl & F_OPT_VERBOSE) {
		g_progress_stats(s_t, e_t, nrec, st3);
		print_str("\nSTATS: processed %llu/%llu records\n", st3, nrec);
	}

	//}
	return 0;
}

int gh_rewind(struct g_handle *hdl) {
	g_setjmp(0, "gh_rewind", NULL, NULL);
	if (hdl->buffer_count) {
		hdl->buffer.r_pos = hdl->buffer.objects;
		hdl->buffer.pos = hdl->buffer.r_pos;
		hdl->buffer.offset = 0;
		hdl->offset = 0;
	} else {
		if (hdl->fh) {
			rewind(hdl->fh);
			hdl->offset = 0;
		}
	}
	return 0;
}

int dirlog_update_record(char *argv) {
	g_setjmp(0, "dirlog_update_record", NULL, NULL);

	int r, seek = SEEK_END, ret = 0, dr;
	off_t offset = 0;
	uint64_t rl = MAX_uint64_t;
	struct dirlog dl = { 0 };
	ear arg = { 0 };
	arg.dirlog = &dl;

	if (gfl & F_OPT_SFV) {
		gfl |= F_OPT_NOWRITE | F_OPT_FORCE | F_OPT_FORCEWSFV;
	}

	mda dirchain = { 0 };
	p_md_obj ptr;

	md_init(&dirchain, 1024);

	if ((r = split_string(argv, 0x20, &dirchain)) < 1) {
		print_str("ERROR: [dirlog_update_record]: missing arguments\n");
		ret = 1;
		goto r_end;
	}

	data_backup_records(DIRLOG);

	char s_buffer[2048];
	ptr = dirchain.objects;
	while (ptr) {
		sprintf(s_buffer, "%s/%s", SITEROOT, (char*) ptr->ptr);
		remove_repeating_chars(s_buffer, 0x2F);
		size_t s_buf_len = strlen(s_buffer);
		if (s_buffer[s_buf_len - 1] == 0x2F) {
			s_buffer[s_buf_len - 1] = 0x0;
		}

		rl = dirlog_find(s_buffer, 0, 0, NULL);

		char *mode = "a";

		if (!(gfl & F_OPT_FORCE) && rl < MAX_uint64_t) {
			print_str(
					"WARNING: %s: [%llu] already exists in dirlog (use -f to overwrite)\n",
					(char*) ptr->ptr, rl);
			ret = 4;
			goto end;
		} else if (rl < MAX_uint64_t) {
			if (gfl & F_OPT_VERBOSE) {
				print_str(
						"WARNING: %s: [%llu] overwriting existing dirlog record\n",
						(char*) ptr->ptr, rl);
			}
			offset = rl;
			seek = SEEK_SET;
			mode = "r+";
		}

		if (g_fopen(DIRLOG, mode, 0, &g_act_1)) {
			goto r_end;
		}

		if ((r = release_generate_block(s_buffer, &arg))) {
			if (r < 5) {
				print_str(
						"ERROR: %s: [%d] generating dirlog data chunk failed\n",
						(char*) ptr->ptr, r);
			}
			ret = 3;
			goto end;
		}

		if ((dr = dirlog_write_record(arg.dirlog, offset, seek))) {
			print_str(
			MSG_GEN_DFWRITE, (char*) ptr->ptr, dr, (ulint64_t) offset, mode);
			ret = 6;
			goto end;
		}

		char buffer[2048] = { 0 };

		if (dirlog_format_block((char*) ptr->ptr, &arg, buffer) > 0)
			print_str(buffer);

		end:

		g_close(&g_act_1);
		ptr = ptr->next;
	}
	r_end: md_g_free(&dirchain);

	print_str("STATS: wrote %llu bytes in %llu records\n", dl_stats.bw,
			dl_stats.rw);

	return ret;
}

int option_crc32(void *arg, int m) {
	g_setjmp(0, "option_crc32", NULL, NULL);
	char *buffer;
	if (m == 2) {
		buffer = (char *) arg;
	} else {
		buffer = ((char **) arg)[0];
	}

	updmode = UPD_MODE_NOOP;

	if (!buffer)
		return 1;

	uint32_t crc32;

	uint64_t read = file_crc32(buffer, &crc32);

	if (read)
		print_str("%.8X\n", (uint32_t) crc32);
	else {
		print_str("ERROR: %s: [%d] could not get CRC32\n", buffer,
		errno);
		EXITVAL = 1;
	}

	return 0;
}

int data_backup_records(char *file) {
	g_setjmp(0, "data_backup_records", NULL, NULL);
	int r;
	off_t r_sz;

	if (!file) {
		print_str("ERROR: null argument passed (this is likely a bug)\n");
		return -1;
	}

	if ((gfl & F_OPT_NOWRITE) || (gfl & F_OPT_NOBACKUP)) {
		return 0;
	}

	if (file_exists(file)) {
		print_str("WARNING: %s: data file doesn't exist\n", file);
		return 0;
	}

	if (!(r_sz = get_file_size(file)) && (gfl & F_OPT_VERBOSE)) {
		print_str("WARNING: %s: refusing to backup 0-byte data file\n", file);
		return 0;
	}

	char buffer[255];

	sprintf(buffer, "%s.bk", file);

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: creating data backup: %s ..\n", file, buffer);
	}

	if ((r = (int) file_copy(file, buffer, "wb", F_FC_MSET_SRC)) < 1) {
		print_str("ERROR: %s: [%d] failed to create backup %s\n", file, r,
				buffer);
		return r;
	}
	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: %s: created data backup: %s\n", file, buffer);
	}
	return 0;
}

int dirlog_check_records(void) {
	g_setjmp(0, "dirlog_check_records", NULL, NULL);
	struct dirlog buffer, buffer4;
	ear buffer3 = { 0 };
	char s_buffer[255], s_buffer2[255], s_buffer3[255] = { 0 };
	buffer3.dirlog = &buffer4;
	int r = 0, r2;
	char *mode = "r";
	uint32_t flags = 0;
	off_t dsz;

	if ((dsz = get_file_size(DIRLOG)) % DL_SZ) {
		print_str(MSG_GEN_DFCORRU, DIRLOG, (ulint64_t) dsz, (int) DL_SZ);
		print_str("NOTICE: use -r to rebuild (see --help)\n");
		return -1;
	}

	/*if (gfl & F_OPT_FIX) {
	 data_backup_records(DIRLOG);
	 }*/

	if (g_fopen(DIRLOG, mode, F_DL_FOPEN_BUFFER | flags, &g_act_1)) {
		return 2;
	}

	if (!g_act_1.buffer_count && (gfl & F_OPT_FIX)) {
		print_str(
				"ERROR: internal buffering must be enabled when fixing, increase limit with --memlimit (see --help)\n");
	}

	struct dirlog *d_ptr = NULL;
	int ir;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_records(loop)",
			NULL,
			NULL);

			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			sprintf(s_buffer, "%s%s", GLROOT, d_ptr->dirname);

			if (d_ptr->status == 1) {
				char *c_nb, *base, *c_nd, *dir;
				c_nb = strdup(d_ptr->dirname);
				base = basename(c_nb);
				c_nd = strdup(d_ptr->dirname);
				dir = dirname(c_nd);

				sprintf(s_buffer2, NUKESTR, base);
				sprintf(s_buffer3, "%s/%s/%s", GLROOT, dir, s_buffer2);
				remove_repeating_chars(s_buffer3, 0x2F);
				g_free(c_nb);
				g_free(c_nd);
			}

			if ((d_ptr->status == 0 && dir_exists(s_buffer))
					|| (d_ptr->status == 1 && dir_exists(s_buffer3))) {
				print_str(
						"WARNING: %s: listed in dirlog but does not exist in filesystem\n",
						s_buffer);
				if (gfl & F_OPT_FIX) {
					if (!md_unlink(&g_act_1.buffer, g_act_1.buffer.pos)) {
						print_str("ERROR: %s: unlinking ghost record failed\n",
								s_buffer);
					}
					r++;
				}
				continue;
			}

			if (gfl & F_OPT_C_GHOSTONLY) {
				continue;
			}

			//g_do_exec(d_ptr, &g_act_1);
			struct nukelog n_buffer;
			ir = r;
			if (d_ptr->status == 1 || d_ptr->status == 2) {
				if (nukelog_find(d_ptr->dirname, 2, &n_buffer) == MAX_uint64_t) {
					print_str(
							"WARNING: %s: was marked as '%sNUKED' in dirlog but not found in nukelog\n",
							s_buffer, d_ptr->status == 2 ? "UN" : "");
				} else {
					if ((d_ptr->status == 1 && n_buffer.status != 0)
							|| (d_ptr->status == 2 && n_buffer.status != 1)
							|| (d_ptr->status == 0)) {
						print_str(
								"WARNING: %s: MISMATCH: was marked as '%sNUKED' in dirlog, but nukelog reads '%sNUKED'\n",
								s_buffer, d_ptr->status == 2 ? "UN" : "",
								n_buffer.status == 1 ? "UN" : "");
					}
				}
				continue;
			}
			buffer3.flags |= F_EAR_NOVERB;

			if ((r2 = release_generate_block(s_buffer, &buffer3))) {
				if (r2 == 5) {
					if (gfl & F_OPT_FIX) {
						if (remove(s_buffer)) {
							print_str(
									"WARNING: %s: failed removing empty directory\n",
									s_buffer);

						} else {
							if (gfl & F_OPT_VERBOSE) {
								print_str("FIX: %s: removed empty directory\n",
										s_buffer);
							}
						}
					}
				} else {
					print_str(
							"WARNING: [%s] - could not get directory information from the filesystem\n",
							s_buffer);
				}
				r++;
				continue;
			}
			if (d_ptr->files != buffer4.files) {
				print_str(
						"WARNING: [%s] file counts in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->files, buffer4.files);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->files = buffer4.files;
				}
			}

			if (d_ptr->bytes != buffer4.bytes) {
				print_str(
						"WARNING: [%s] directory sizes in dirlog and on disk do not match ( dirlog: %llu , filesystem: %llu )\n",
						d_ptr->dirname, (ulint64_t) d_ptr->bytes,
						(ulint64_t) buffer4.bytes);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->bytes = buffer4.bytes;
				}
			}

			if (d_ptr->group != buffer4.group) {
				print_str(
						"WARNING: [%s] group ids in dirlog and on disk do not match (dirlog:%hu filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->group, buffer4.group);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->group = buffer4.group;
				}
			}

			if (d_ptr->uploader != buffer4.uploader) {
				print_str(
						"WARNING: [%s] user ids in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->uploader, buffer4.uploader);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->uploader = buffer4.uploader;
				}
			}

			if ((gfl & F_OPT_FORCE) && d_ptr->uptime != buffer4.uptime) {
				print_str(
						"WARNING: [%s] folder creation dates in dirlog and on disk do not match (dirlog:%u, filesystem:%u)\n",
						d_ptr->dirname, d_ptr->uptime, buffer4.uptime);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->uptime = buffer4.uptime;
				}
			}
			if (r == ir) {
				if (gfl & F_OPT_VERBOSE2) {
					print_str("OK: %s\n", d_ptr->dirname);
				}
			} else {
				if (gfl & F_OPT_VERBOSE2) {
					print_str("BAD: %s\n", d_ptr->dirname);
				}

			}
		}

	}

	if (!(gfl & F_OPT_KILL_GLOBAL) && (gfl & F_OPT_FIX) && r) {
		if (rebuild_data_file(DIRLOG, &g_act_1)) {
			print_str(MSG_GEN_DFRFAIL, DIRLOG);
		} else {
			print_str("STATS: %s: wrote %llu bytes in %llu records\n", DIRLOG,
					(ulint64_t) g_act_1.bw, (ulint64_t) g_act_1.rw);
		}
	}

	g_close(&g_act_1);

	return r;
}

int do_match(char *mstr, void *d_ptr, void *callback) {
	if ((gfl & F_OPT_HAS_G_MATCH) && mstr) {
		size_t mstr_l = strlen(mstr);

		int irl = strlen(GLOB_MATCH) != mstr_l, ir = strncmp(mstr, GLOB_MATCH,
				mstr_l);

		if ((glob_match_i_m && (ir || irl))
				|| (!glob_match_i_m && (!ir && !irl))) {
			if ((gfl & F_OPT_VERBOSE3) && !(gfl & F_OPT_MODE_RAWDUMP)) {
				print_str("WARNING: %s: match positive, ignoring this record\n",
						mstr);
			}
			return 2;
		}

	}

	if ((gfl & F_OPT_HAS_G_REGEX) && mstr
			&& reg_match(GLOB_REGEX, mstr, glob_regex_flags) == glob_reg_i_m) {
		if ((gfl & F_OPT_VERBOSE3) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str(
					"WARNING: %s: REGEX match positive, ignoring this record\n",
					mstr);
		}
		return 3;
	}

	int r_e = g_do_exec(d_ptr, callback, NULL);

	if (exec_str && WEXITSTATUS(r_e)) {
		if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_MODE_RAWDUMP)) {
			print_str(
					"WARNING: [%d] external call returned non-zero, ignoring this record\n",
					WEXITSTATUS(r_e));
		}
		return 1;
	}

	return 0;
}

int g_bmatch(void *d_ptr, struct g_handle *hdl) {
	g_setjmp(0, "g_bmatch", NULL, NULL);

	char *mstr = NULL;
	void *callback = NULL;

	switch (hdl->flags & F_GH_ISTYPE) {
	case F_GH_ISDIRLOG:
		mstr = (char*) ((struct dirlog*) d_ptr)->dirname;
		callback = ref_to_val_dirlog;
		break;
	case F_GH_ISNUKELOG:
		mstr = (char*) ((struct nukelog*) d_ptr)->dirname;
		callback = ref_to_val_nukelog;
		break;
	case F_GH_ISDUPEFILE:
		mstr = (char*) ((struct dupefile*) d_ptr)->filename;
		callback = ref_to_val_dupefile;
		break;
	case F_GH_ISLASTONLOG:
		mstr = (char*) ((struct lastonlog*) d_ptr)->uname;
		callback = ref_to_val_lastonlog;
		break;
	case F_GH_ISONELINERS:
		mstr = (char*) ((struct oneliner*) d_ptr)->uname;
		callback = ref_to_val_oneliners;
		break;
	case F_GH_ISONLINE:
		mstr = (char*) ((struct ONLINE*) d_ptr)->username;
		if (!strlen(mstr)) {
			return -1;
		}
		callback = ref_to_val_online;
		break;
	}

	return do_match(mstr, d_ptr, callback);
}

int g_print_stats(char *file, uint32_t flags, size_t block_sz) {
	g_setjmp(0, "g_print_stats", NULL, NULL);

	if (block_sz) {
		g_act_1.block_sz = block_sz;
	}

	if (g_fopen(file, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1)) {
		return 2;
	}

	void *ptr;
	void *buffer = calloc(1, g_act_1.block_sz);
	char sbuffer[4096], *ns_ptr;
	off_t c = 0;
	ear e;
	int re_c, r;

	while ((ptr = g_read(buffer, &g_act_1, g_act_1.block_sz))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "g_print_stats(loop)", NULL,
			NULL);

			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			if ((r = g_bmatch(ptr, &g_act_1))) {
				if (r == -1) {
					break;
				}
				continue;
			}
			c++;
			if (gfl & F_OPT_MODE_RAWDUMP) {
				g_fwrite((void*) ptr, g_act_1.block_sz, 1, stdout);
			} else {
				re_c = 0;
				ns_ptr = "UNKNOWN";
				switch (g_act_1.flags & F_GH_ISTYPE) {
				case F_GH_ISDIRLOG:
					e.dirlog = (struct dirlog*) ptr;
					ns_ptr = e.dirlog->dirname;
					re_c += dirlog_format_block(ns_ptr, &e, sbuffer);
					break;
				case F_GH_ISNUKELOG:
					e.nukelog = (struct nukelog*) ptr;
					ns_ptr = e.dirlog->dirname;
					re_c += nukelog_format_block(ns_ptr, &e, sbuffer);
					break;
				case F_GH_ISDUPEFILE:
					e.dupefile = (struct dupefile*) ptr;
					ns_ptr = e.dupefile->filename;
					re_c += dupefile_format_block(ns_ptr, &e, sbuffer);
					break;
				case F_GH_ISLASTONLOG:
					e.lastonlog = (struct lastonlog*) ptr;
					ns_ptr = e.lastonlog->uname;
					re_c += lastonlog_format_block(ns_ptr, &e, sbuffer);
					break;
				case F_GH_ISONELINERS:
					e.oneliner = (struct oneliner*) ptr;
					ns_ptr = e.oneliner->uname;
					re_c += oneliner_format_block(ns_ptr, &e, sbuffer);
					break;
				case F_GH_ISONLINE:
					e.online = (struct ONLINE*) ptr;
					ns_ptr = e.online->username;
					if (!(re_c = online_format_block(ns_ptr, &e, sbuffer))) {
						goto end;
					}
					if (c == 1 && (gfl & F_OPT_FORMAT_COMP)) {
						print_str(
								"+-------------------------------------------------------------------------------------------------------------------------------------------\n"
										"|                      USER/HOST                          |    TIME ONLINE     |    TRANSFER RATE      |        STATUS       \n"
										"|---------------------------------------------------------|--------------------|-----------------------|------------------------------------\n");
					}
					break;
				}
				if (re_c) {
					print_str(sbuffer);
				} else {
					print_str("ERROR: %s: zero-length formatted result\n",
							ns_ptr);
					continue;
				}
			}

		}
	}

	end:

	if (gfl & F_OPT_MODE_RAWDUMP) {
		fflush(stdout);
	}

	g_setjmp(0, "dirlog_print_stats(2)", NULL, NULL);

	if (!(gfl & F_OPT_FORMAT_BATCH) && !(gfl & F_OPT_MODE_RAWDUMP)
			&& !(gfl & F_OPT_FORMAT_COMP) && !(g_act_1.flags & F_GH_ISONLINE)) {
		print_str("STATS: %s: read %llu records\n", file,
				(unsigned long long int) c);
	}

	g_free(buffer);
	g_close(&g_act_1);

	return 0;
}

int dirlog_print_stats(void) {
	g_setjmp(0, "dirlog_print_stats", NULL, NULL);
	struct dirlog buffer;
	char sbuffer[2048];

	int c = 0;
	ear e;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &g_act_1))
		return 2;

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_print_stats(loop)",
			NULL,
			NULL);

			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			if (g_bmatch(d_ptr, &g_act_1)) {
				continue;
			}

			c++;
			e.dirlog = d_ptr;
			if (gfl & F_OPT_MODE_RAWDUMP) {
				g_fwrite((void*) d_ptr, DL_SZ, 1, stdout);
			} else {
				if (dirlog_format_block(d_ptr->dirname, &e, sbuffer) > 0) {
					print_str(sbuffer);
				}
			}

			if ((gfl & F_OPT_VERBOSE2) && !(gfl & F_OPT_MODE_RAWDUMP)) {
				struct nukelog n_buffer = { 0 };
				if (nukelog_find(d_ptr->dirname, 2, &n_buffer) < MAX_uint64_t) {
					e.nukelog = &n_buffer;
					bzero(sbuffer, 2048);
					if (nukelog_format_block(d_ptr->dirname, &e, sbuffer) > 0) {
						print_str(sbuffer);
					}
				}
			}

		}
	}

	g_setjmp(0, "dirlog_print_stats(2)", NULL, NULL);

	if (!(gfl & F_OPT_FORMAT_BATCH) && !(gfl & F_OPT_MODE_RAWDUMP)) {
		print_str("STATS: %s: read %d records\n", DIRLOG, c);
	}

	g_close(&g_act_1);

	return 0;
}

#define 	ACT_WRITE_BUFFER_MEMBERS	10000

int rebuild_dirlog(void) {
	g_setjmp(0, "rebuild_dirlog", NULL, NULL);
	char mode[255] = { 0 };
	uint32_t flags = 0;

	if (!(ofl & F_OVRR_NUKESTR)) {
		print_str(
				"WARNING: failed extracting nuke string from glftpd.conf, nuked dirs might not get detected properly\n");
	}

	if (gfl & F_OPT_NOWRITE) {
		g_strncpy(mode, "r", 1);
		flags |= F_DL_FOPEN_BUFFER;
	} else if (gfl & F_OPT_UPDATE) {
		g_strncpy(mode, "a+", 2);
		flags |= F_DL_FOPEN_BUFFER | F_DL_FOPEN_FILE;
	} else {
		g_strncpy(mode, "w+", 2);
	}

	if (gfl & F_OPT_WBUFFER) {
		g_strncpy(g_act_1.mode, "r", 1);
		md_init(&g_act_1.w_buffer, ACT_WRITE_BUFFER_MEMBERS);
		g_act_1.block_sz = DL_SZ;
		g_act_1.flags |= F_GH_FFBUFFER | F_GH_WAPPEND
				| ((gfl & F_OPT_UPDATE) ? F_GH_DFNOWIPE : 0);
		g_act_1.w_buffer.flags |= F_MDA_REUSE;
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: %s: explicit write pre-caching enabled\n",
					DIRLOG);
		}
	} else {
		g_strncpy(g_act_1.mode, mode, strlen(mode));
		data_backup_records(DIRLOG);
	}
	g_act_1.block_sz = DL_SZ;

	int dfex = file_exists(DIRLOG);

	if ((gfl & F_OPT_UPDATE) && dfex) {
		print_str(
				"WARNING: %s: requested update, but no dirlog exists - removing update flag..\n",
				DIRLOG);
		gfl ^= F_OPT_UPDATE;
		flags ^= F_DL_FOPEN_BUFFER;
	}

	if (!strncmp(g_act_1.mode, "r", 1) && dfex) {
		if (gfl & F_OPT_VERBOSE) {
			print_str(
					"WARNING: %s: requested read mode access but file not there\n",
					DIRLOG);
		}
	} else if (g_fopen(DIRLOG, g_act_1.mode, flags, &g_act_1)) {
		print_str("ERROR: could not open dirlog, mode '%s', flags %u\n",
				g_act_1.mode, flags);
		return errno;
	}

	if (gfl & F_OPT_FORCE) {
		print_str("SCANNING: '%s'\n", SITEROOT);
		update_records(SITEROOT, 0);
		goto end;
	}

	char buffer[V_MB + 1] = { 0 };
	mda dirchain = { 0 }, buffer2 = { 0 };

	if (read_file(DU_FLD, buffer, V_MB, 0) < 1) {
		print_str(
				"WARNING: unable to read folders file, doing full siteroot recursion in '%s'..\n",
				SITEROOT);
		gfl |= F_OPT_FORCE;
		update_records(SITEROOT, 0);
		goto end;
	}

	int r, r2;

	md_init(&dirchain, 1024);

	if ((r = split_string(buffer, 0x13, &dirchain)) < 1) {
		print_str("ERROR: [%d] could not parse input from %s\n", r, DU_FLD);
		goto r_end;
	}

	if (gfl & F_OPT_VERBOSE3) {
		print_str("NOTICE: %s: allocating %u B for references (overhead)\n",
				DIRLOG, (uint32_t) (ACT_WRITE_BUFFER_MEMBERS * sizeof(md_obj)));
	}

	int i = 0, ib;
	char s_buffer[2048] = { 0 };
	p_md_obj ptr = dirchain.objects;

	while (ptr) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "rebuild_dirlog(loop)", NULL,
			NULL);
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			md_init(&buffer2, 6);
			i++;
			if ((r2 = split_string((char*) ptr->ptr, 0x20, &buffer2)) != 2) {
				print_str("ERROR: [%d] could not parse line %d from %s\n", r2,
						i, DU_FLD);
				goto lend;
			}
			bzero(s_buffer, 2048);
			sprintf(s_buffer, "%s/%s", SITEROOT, (char*) buffer2.objects->ptr);
			remove_repeating_chars(s_buffer, 0x2F);

			size_t s_buf_len = strlen(s_buffer);
			if (s_buffer[s_buf_len - 1] == 0x2F) {
				s_buffer[s_buf_len - 1] = 0x0;
			}

			ib = strtol((char*) ((p_md_obj) buffer2.objects->next)->ptr,
			NULL, 10);

			if (errno == ERANGE) {
				print_str("ERROR: could not get depth from line %d\n", i);
				goto lend;
			}
			if (dir_exists(s_buffer)) {
				print_str("ERROR: %s: directory doesn't exist (line %d)\n",
						s_buffer, i);
				goto lend;
			}
			char *ndup = strdup(s_buffer);
			char *nbase = basename(ndup);

			print_str("SCANNING: '%s', depth: %d\n", nbase, ib);
			if (update_records(s_buffer, ib) < 1) {
				print_str("WARNING: %s: nothing was processed\n", nbase);
			}

			g_free(ndup);
			lend:

			md_g_free(&buffer2);
		}
		ptr = ptr->next;
	}

	if (g_act_1.flags & F_GH_FFBUFFER) {
		rebuild_data_file(DIRLOG, &g_act_1);
	}

	r_end:

	md_g_free(&dirchain);

	end:

	g_close(&g_act_1);

	print_str("STATS: wrote %llu bytes in %llu records\n", dl_stats.bw,
			dl_stats.rw);

	return 0;
}

int parse_args(int argc, char **argv, void*fref_t[]) {
	g_setjmp(0, "parse_args", NULL, NULL);
	int i, oi, vi, ret, r, c = 0;

	char *c_arg;
	mda cmd_lt = { 0 };

	p_ora ora = (p_ora) fref_t;

	for (i = 1, ret = 0; i < argc; i++, r = 0) {
		c_arg = argv[i];
		bzero(&cmd_lt, sizeof(mda));
		md_init(&cmd_lt, 256);

		if ((r = split_string(c_arg, 0x3D, &cmd_lt)) == 2) {
			c_arg = (char*) cmd_lt.objects->ptr;
		}

		if ((vi = process_opt(c_arg, NULL, fref_t, 1)) < 0) {
			if (fref_t != prio_f_ref) {
				print_str("CMDLINE: [%d] invalid argument '%s'\n", vi, c_arg);
				ret = -2;
				goto end;
			}

		}

		if (r == 2) {
			ret += process_opt(c_arg, ((p_md_obj) cmd_lt.objects->next)->ptr,
					fref_t, 2);

			c++;
		} else {
			oi = i;
			AAINT vp;
			void *buffer = NULL;

			if ((vp = (AAINT) ora[vi].arg_cnt)) {
				if (i + vp > argc - 1) {
					if (fref_t != prio_f_ref) {
						print_str(
								"CMDLINE: '%s' missing argument parameters [%llu]\n",
								argv[i], (ulint64_t) ((i + vp) - (argc - 1)));
						c = 0;
						goto end;
					}

				}
				buffer = &argv[i + 1];
				i += vp;
			}
			ret += process_opt(argv[oi], buffer, fref_t, 0);

			c++;
		}
		md_g_free(&cmd_lt);

	}
	end:

	if (!c) {
		return -1;
	}

	return ret;
}

int process_opt(char *opt, void *arg, void *reference_array, int m) {
	g_setjmp(0, "process_opt", NULL, NULL);
	int (*proc_opt_generic)(void *arg, int m);
	int i = 0;
	p_ora ora = (p_ora) reference_array;

	while (ora->option) {
		if (strlen(ora->option) == strlen(opt)
				&& !strncmp(ora->option, opt, strlen(ora->option))) {
			if (m == 1)
				return i;
			else {
				if (ora->function) {
					proc_opt_generic = ora->function;
					if (proc_opt_generic) {
						return proc_opt_generic(arg, m);
					}
				} else {
					return -4;
				}
			}
		}

		ora++;
		i++;
	}
	return -2;
}

int update_records(char *dirname, int depth) {
	g_setjmp(0, "update_records", NULL, NULL);
	struct dirlog buffer = { 0 };
	ear arg = { 0 };

	if (dir_exists(dirname))
		return 2;

	arg.depth = depth;
	arg.dirlog = &buffer;

	return enum_dir(dirname, proc_section, &arg, 0);

}

int proc_release(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "proc_release", NULL, NULL);
	ear *iarg = (ear*) arg;
	uint32_t crc32 = 0;
	char buffer[255] = { 0 };
	char *fn, *fn2, *fn3, *base;

	if (!reg_match("\\/[.]{1,2}$", name, 0))
		return 1;

	if (!reg_match("\\/[.].*$", name, REG_NEWLINE))
		return 1;

	switch (type) {
	case DT_REG:
		fn2 = strdup(name);
		fn3 = strdup(name);
		base = basename(fn3);
		if ((gfl & F_OPT_SFV)
				&& (updmode == UPD_MODE_RECURSIVE || updmode == UPD_MODE_SINGLE)
				&& reg_match(PREG_SFV_SKIP, name,
				REG_ICASE | REG_NEWLINE) && reg_match(PREG_SFV_SKIP_EXT, fn2,
				REG_ICASE | REG_NEWLINE) && file_crc32(name, &crc32)) {
			fn = strdup(name);
			char *dn = basename(dirname(fn));
			g_free(fn2);
			fn2 = strdup(name);
			sprintf(buffer, "%s/%s.sfv", dirname(fn2), dn);
			char buffer2[1024];
			sprintf(buffer2, "%s %.8X\n", base, (uint32_t) crc32);
			if (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV)) {
				if (!write_file_text(buffer2, buffer)) {
					print_str("ERROR: %s: failed writing to SFV file: '%s'\n",
							name, buffer);
				}
			}
			iarg->flags |= F_EARG_SFV;
			g_strncpy(iarg->buffer, buffer, strlen(buffer));
			sprintf(buffer, "  %.8X", (uint32_t) crc32);
			g_free(fn);
		}
		off_t fs = get_file_size(name);
		iarg->dirlog->bytes += fs;
		iarg->dirlog->files++;
		if (gfl & F_OPT_VERBOSE4) {
			print_str("     %s  %.2fMB%s\n", base,
					(double) fs / 1024.0 / 1024.0, buffer);
		}
		g_free(fn3);
		g_free(fn2);
		break;
	case DT_DIR:
		if ((gfl & F_OPT_SFV)
				&& (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV))) {
			enum_dir(name, delete_file, (void*) "\\.sfv$", 0);
		}

		if (!reg_match(PREG_DIR_SKIP, name, REG_NEWLINE | REG_ICASE)) {
			return 2;
		}
		enum_dir(name, proc_release, iarg, 0);
		break;
	}

	return 0;
}

int proc_section(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "proc_section", NULL, NULL);
	ear *iarg = (ear*) arg;
	int r;
	uint64_t rl;

	if (!reg_match("\\/[.]{1,2}", name, 0)) {
		return 1;
	}

	if (!reg_match("\\/[.].*$", name, REG_NEWLINE)) {
		return 1;
	}

	if (!reg_match(PREG_DIR_SKIP, name, REG_NEWLINE | REG_ICASE)) {
		return 1;
	}

	switch (type) {
	case DT_DIR:
		iarg->depth--;
		if (!iarg->depth || (gfl & F_OPT_FORCE)) {
			if (gfl & F_OPT_UPDATE) {
				if (((rl = dirlog_find(name, 1, F_DL_FOPEN_REWIND, NULL))
						< MAX_uint64_t)) {
					if (gfl & F_OPT_VERBOSE2) {
						print_str(
								"WARNING: %s: [%llu] record already exists, not importing\n",
								name, rl);
					}
					goto end;
				}
			}
			bzero(iarg->buffer, 1024);
			iarg->flags = 0;
			if ((r = release_generate_block(name, iarg))) {
				if (r < 5)
					print_str(
							"ERROR: %s: [%d] generating dirlog data chunk failed\n",
							name, r);
				goto end;
			}

			if (g_bmatch(iarg->dirlog, &g_act_1)) {
				goto end;
			}

			if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV)) {
				iarg->dirlog->bytes += (uint64_t) get_file_size(iarg->buffer);
				iarg->dirlog->files++;
				print_str("SFV: succesfully generated '%s'\n",
				basename(iarg->buffer));
			}

			if (g_act_1.flags & F_GH_FFBUFFER) {
				if ((r = g_load_record(&g_act_1, (const void*) iarg->dirlog))) {
					print_str(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
							(ulint64_t) g_act_1.w_buffer.offset, "wbuffer");
				}
			} else {
				if ((r = dirlog_write_record(iarg->dirlog, 0, SEEK_END))) {
					print_str(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
							(ulint64_t) g_act_1.offset - 1, "w");
					goto end;
				}
			}

			char buffer[2048] = { 0 };

			if ((gfl & F_OPT_VERBOSE)
					&& dirlog_format_block(name, iarg, buffer) > 0) {
				print_str(buffer);
			}

			if (gfl & F_OPT_FORCE) {
				enum_dir(name, proc_section, iarg, 0);
			}
		} else {
			enum_dir(name, proc_section, iarg, 0);
		}
		end: iarg->depth++;
		break;
	}
	return 0;
}

int release_generate_block(char *name, ear *iarg) {
	g_setjmp(0, "release_generate_block", NULL, NULL);
	bzero(iarg->dirlog, sizeof(struct dirlog));

	int r, ret = 0;
	struct stat st = { 0 }, st2 = { 0 };

	if (gfl & F_OPT_FOLLOW_LINKS) {
		if (stat(name, &st)) {
			return 1;
		}
	} else {
		if (lstat(name, &st)) {
			return 1;
		}

		if ((st.st_mode & S_IFMT) == S_IFLNK) {
			if (gfl & F_OPT_VERBOSE2) {
				print_str("WARNING: %s - is symbolic link, skipping..\n", name);
			}
			return 6;
		}
	}

	time_t orig_ctime = get_file_creation_time(&st);

	if ((gfl & F_OPT_SFV)
			&& (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV))) {
		enum_dir(name, delete_file, (void*) "\\.sfv$", 0);
	}

	if ((gfl & F_OPT_VERBOSE2) && !(iarg->flags & F_EAR_NOVERB))
		print_str("ENTERING: %s\n", name);

	if ((r = enum_dir(name, proc_release, iarg, 0)) < 1
			|| !iarg->dirlog->files) {
		if (gfl & F_OPT_VERBOSE) {
			print_str("WARNING: %s: [%d] - empty directory\n", name, r);
		}
		ret = 5;
	}
	g_setjmp(0, "release_generate_block(2)", NULL, NULL);

	if ((gfl & F_OPT_VERBOSE2) && !(iarg->flags & F_EAR_NOVERB))
		print_str("EXITING: %s\n", name);

	if ((gfl & F_OPT_SFV) && !(gfl & F_OPT_NOWRITE)) {
		if (gfl & F_OPT_FOLLOW_LINKS) {
			if (stat(name, &st2)) {
				ret = 1;
			}
		} else {
			if (lstat(name, &st2)) {
				ret = 1;
			}
		}
		time_t c_ctime = get_file_creation_time(&st2);
		if (c_ctime != orig_ctime) {
			if (gfl & F_OPT_VERBOSE4) {
				print_str(
						"NOTICE: %s: restoring original folder modification date\n",
						name);
			}
			struct utimbuf utb;
			utb.actime = 0;
			utb.modtime = orig_ctime;
			if (utime(name, &utb)) {
				print_str(
						"WARNING: %s: SFVGEN failed to restore original modification date\n",
						name);
			}
		}

	}

	if (ret) {
		goto end;
	}

	char *namedup = strdup(name);
	char *bn = basename(namedup);

	iarg->dirlog->uptime = orig_ctime;
	iarg->dirlog->uploader = (uint16_t) st.st_uid;
	iarg->dirlog->group = (uint16_t) st.st_gid;
	char buffer[255] = { 0 };

	if ((r = get_relative_path(name, GLROOT, buffer))) {
		print_str("ERROR: [%s] could not get relative to root directory name\n",
				bn);
		ret = 2;
		goto r_end;
	}

	struct nukelog n_buffer = { 0 };
	if (nukelog_find(buffer, 2, &n_buffer) < MAX_uint64_t) {
		iarg->dirlog->status = n_buffer.status + 1;
		g_strncpy(iarg->dirlog->dirname, n_buffer.dirname,
				strlen(n_buffer.dirname));
	} else {
		g_strncpy(iarg->dirlog->dirname, buffer, strlen(buffer));
	}

	r_end:

	g_free(namedup);

	end:

	return ret;
}

int get_relative_path(char *subject, char *root, char *output) {
	g_setjmp(0, "get_relative_path", NULL, NULL);
	char *root_dir = root;

	if (!root_dir)
		return 11;

	int i, root_dir_len = strlen(root_dir);

	for (i = 0; i < root_dir_len; i++) {
		if (subject[i] != root_dir[i])
			break;
	}

	while (subject[i] != 0x2F && i > 0)
		i--;

	g_memcpy(output, &subject[i], strlen(subject) - i);
	return 0;
}

#define STD_FMT_TIME_STR  	"%d %b %Y %T"

int dirlog_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "dirlog_format_block", NULL, NULL);
	char buffer2[255];
	char *ndup = strdup(iarg->dirlog->dirname), *base = NULL;

	if (gfl & F_OPT_VERBOSE)
		base = iarg->dirlog->dirname;
	else
		base = basename(ndup);

	time_t t_t = (time_t) iarg->dirlog->uptime;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	int c;

	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(output, "DIRLOG;%s;%llu;%hu;%u;%hu;%hu;%hu\n", base,
				(ulint64_t) iarg->dirlog->bytes, iarg->dirlog->files,
				iarg->dirlog->uptime, iarg->dirlog->uploader,
				iarg->dirlog->group, iarg->dirlog->status);
	} else {
		c =
				sprintf(output,
						"DIRLOG: %s - %llu Mbytes in %hu files - created %s by %hu.%hu [%hu]\n",
						base, (ulint64_t) (iarg->dirlog->bytes / 1024 / 1024),
						iarg->dirlog->files, buffer2, iarg->dirlog->uploader,
						iarg->dirlog->group, iarg->dirlog->status);
	}

	g_free(ndup);

	return c;
}

int nukelog_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "nukelog_format_block", NULL, NULL);
	char buffer2[255] = { 0 };
	char *ndup = strdup(iarg->nukelog->dirname), *base = NULL;

	if (gfl & F_OPT_VERBOSE)
		base = iarg->nukelog->dirname;
	else
		base = basename(ndup);

	time_t t_t = (time_t) iarg->nukelog->nuketime;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));
	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(output, "NUKELOG;%s;%s;%hu;%.2f;%s;%s;%u\n", base,
				iarg->nukelog->reason, iarg->nukelog->mult,
				iarg->nukelog->bytes,
				!iarg->nukelog->status ?
						iarg->nukelog->nuker : iarg->nukelog->unnuker,
				iarg->nukelog->nukee, iarg->nukelog->nuketime);
	} else {
		c =
				sprintf(output,
						"NUKELOG: %s - %s, reason: '%s' [%.2f MB] - factor: %hu, %s: %s, creator: %s - %s\n",
						base, !iarg->nukelog->status ? "NUKED" : "UNNUKED",
						iarg->nukelog->reason, iarg->nukelog->bytes,
						iarg->nukelog->mult,
						!iarg->nukelog->status ? "nuker" : "unnuker",
						!iarg->nukelog->status ?
								iarg->nukelog->nuker : iarg->nukelog->unnuker,
						iarg->nukelog->nukee, buffer2);
	}
	g_free(ndup);

	return c;
}

int dupefile_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "dupefile_format_block", NULL, NULL);
	char buffer2[255] = { 0 };

	time_t t_t = (time_t) iarg->dupefile->timeup;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));
	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(output, "DUPEFILE;%s;%s;%u\n", iarg->dupefile->filename,
				iarg->dupefile->uploader, iarg->dupefile->timeup);
	} else {
		c = sprintf(output, "DUPEFILE: %s - uploader: %s, time: %s\n",
				iarg->dupefile->filename, iarg->dupefile->uploader, buffer2);
	}

	return c;
}

int lastonlog_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "lastonlog_format_block", NULL, NULL);
	char buffer2[255] = { 0 }, buffer3[255] = { 0 }, buffer4[12] = { 0 };

	time_t t_t_ln = (time_t) iarg->lastonlog->logon;
	time_t t_t_lf = (time_t) iarg->lastonlog->logoff;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t_ln));
	strftime(buffer3, 255, STD_FMT_TIME_STR, localtime(&t_t_lf));

	g_memcpy(buffer4, iarg->lastonlog->stats, sizeof(iarg->lastonlog->stats));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(output, "LASTONLOG;%s;%s;%s;%u;%u;%u;%u;%s\n",
				iarg->lastonlog->uname, iarg->lastonlog->gname,
				iarg->lastonlog->tagline, (uint32_t) iarg->lastonlog->logon,
				(uint32_t) iarg->lastonlog->logoff,
				(uint32_t) iarg->lastonlog->upload,
				(uint32_t) iarg->lastonlog->download, buffer4);
	} else {
		c =
				sprintf(output,
						"LASTONLOG: user: %s/%s [%s] - logon: %s, logoff: %s - up/down: %u/%u B, changes: %s\n",
						iarg->lastonlog->uname, iarg->lastonlog->gname,
						iarg->lastonlog->tagline, buffer2, buffer3,
						(uint32_t) iarg->lastonlog->upload,
						(uint32_t) iarg->lastonlog->download, buffer4);
	}

	return c;
}

int oneliner_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "oneliner_format_block", NULL, NULL);
	char buffer2[255] = { 0 };

	time_t t_t = (time_t) iarg->oneliner->timestamp;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(output, "ONELINER;%s;%s;%s;%u;%s\n", iarg->oneliner->uname,
				iarg->oneliner->gname, iarg->oneliner->tagline,
				(uint32_t) iarg->oneliner->timestamp, iarg->oneliner->message);
	} else {
		c = sprintf(output,
				"ONELINER: user: %s/%s [%s] - time: %s, message: %s\n",
				iarg->oneliner->uname, iarg->oneliner->gname,
				iarg->oneliner->tagline, buffer2, iarg->oneliner->message);
	}

	return c;
}

#define FMT_SP_OFF 	30

int online_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "online_format_block", NULL, NULL);
	char buffer2[255] = { 0 };

	time_t t_t = (time_t) iarg->online->login_time;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	if (!strlen(iarg->online->username)) {
		return 0;
	}

	int32_t tdiff = (int32_t) time(NULL) - iarg->online->tstart.tv_sec;
	float kb = (iarg->online->bytes_xfer / 1024), kbps = 0.0;

	if (tdiff > 0 && kb > 0) {
		kbps = kb / (float) tdiff;
	}

	time_t ltime = time(NULL) - (time_t) iarg->online->login_time;

	if (ltime < 0) {
		ltime = 0;
	}

	int c = 0;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(output,
				"ONLINE;%s;%s;%u;%u;%s;%u;%u;%llu;%llu;%llu;%s;%s\n",
				iarg->online->username, iarg->online->host,
				(uint32_t) iarg->online->groupid,
				(uint32_t) iarg->online->login_time, iarg->online->tagline,
				(uint32_t) iarg->online->ssl_flag,
				(uint32_t) iarg->online->procid,
				(ulint64_t) iarg->online->bytes_xfer,
				(ulint64_t) iarg->online->bytes_txfer, (ulint64_t) kbps,
				iarg->online->status, iarg->online->currentdir);
	} else {
		if (gfl & F_OPT_FORMAT_COMP) {
			char sp_buffer[255], sp_buffer2[255], sp_buffer3[255];
			char d_buffer[255] = { 0 };
			sprintf(d_buffer, "%u", (uint32_t) ltime);
			size_t d_len1 = strlen(d_buffer);
			sprintf(d_buffer, "%.2f", kbps);
			size_t d_len2 = strlen(d_buffer);
			generate_chars(
					54
							- (strlen(iarg->online->username)
									+ strlen(iarg->online->host)), 0x20,
					sp_buffer);
			generate_chars(10 - d_len1, 0x20, sp_buffer2);
			generate_chars(13 - d_len2, 0x20, sp_buffer3);
			c = sprintf(output,
					"| %s!%s%s |        %us%s |     %.2fKB/s%s |  %s\n",
					iarg->online->username, iarg->online->host, sp_buffer,
					(uint32_t) ltime, sp_buffer2, kbps, sp_buffer3,
					iarg->online->status);
		} else {
			c = sprintf(output, "[ONLINE]\n"
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
					"    CWD:             %s\n\n", iarg->online->username,
					iarg->online->host, (uint32_t) iarg->online->groupid,
					buffer2, (uint32_t) ltime, iarg->online->tagline,
					(!iarg->online->ssl_flag ?
							"NO" :
							(iarg->online->ssl_flag == 1 ?
									"YES" :
									(iarg->online->ssl_flag == 2 ?
											"YES (DATA)" : "UNKNOWN"))),
					(uint32_t) iarg->online->procid,
					(ulint64_t) iarg->online->bytes_xfer, kbps,
					iarg->online->status, iarg->online->currentdir);
		}
	}

	return c;
}

char *generate_chars(size_t num, char chr, char*buffer) {
	g_setjmp(0, "generate_chars", NULL, NULL);
	bzero(buffer, 255);
	if (num < 1 || num > 254) {
		return buffer;
	}
	memset(buffer, (int) chr, num);

	return buffer;
}

off_t get_file_size(char *file) {
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

time_t get_file_creation_time(struct stat *st) {
	return (time_t) birthtime(st);
}

uint64_t dirlog_find(char *dirn, int mode, uint32_t flags, void *callback) {
	g_setjmp(0, "dirlog_find", NULL, NULL);

	if (!(ofl & F_OVRR_NUKESTR)) {
		return dirlog_find_old(dirn, mode, flags, callback);
	}

	struct dirlog buffer;
	int (*callback_f)(struct dirlog *) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
		return MAX_uint64_t;

	int r;
	uint64_t ur = MAX_uint64_t;

	char buffer_s[255] = { 0 }, buffer_s2[255], buffer_s3[255];
	char *dup, *dup2, *base, *dir;

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	size_t d_l = strlen(buffer_s);

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {

		if (!strncmp(buffer_s, d_ptr->dirname, d_l)) {
			goto match;
		}
		dup = strdup(d_ptr->dirname);
		base = basename(dup);
		dup2 = strdup(d_ptr->dirname);
		dir = dirname(dup2);
		sprintf(buffer_s2, NUKESTR, base);
		sprintf(buffer_s3, "%s/%s", dir, buffer_s2);
		remove_repeating_chars(buffer_s3, 0x2F);
		g_free(dup);
		g_free(dup2);
		if (!strncmp(buffer_s3, buffer_s, d_l)) {
			match: ur = g_act_1.offset - 1;
			if (mode == 2 && callback) {
				if (callback_f(&buffer)) {
					break;
				}

			} else {
				break;
			}
		}
	}

	if (mode != 1)
		g_close(&g_act_1);

	return ur;
}

uint64_t dirlog_find_old(char *dirn, int mode, uint32_t flags, void *callback) {
	g_setjmp(0, "dirlog_find_old", NULL, NULL);
	struct dirlog buffer;
	int (*callback_f)(struct dirlog *data) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
		return MAX_uint64_t;

	int r;
	uint64_t ur = MAX_uint64_t;

	char buffer_s[255] = { 0 };
	char *dup, *dup2, *base, *dir;
	int gi1, gi2;

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	gi2 = strlen(buffer_s);

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		dup = strdup(d_ptr->dirname);
		base = basename(dup);
		gi1 = strlen(base);
		dup2 = strdup(d_ptr->dirname);
		dir = dirname(dup2);
		if (!strncmp(&buffer_s[gi2 - gi1], base, gi1)
				&& !strncmp(buffer_s, d_ptr->dirname, strlen(dir))) {

			ur = g_act_1.offset - 1;
			if (mode == 2 && callback) {
				if (callback_f(&buffer)) {
					g_free(dup);
					g_free(dup2);
					break;
				}

			} else {
				g_free(dup);
				g_free(dup2);
				break;
			}
		}
		g_free(dup);
		g_free(dup2);
	}

	if (mode != 1)
		g_close(&g_act_1);

	return ur;
}

size_t str_match(char *input, char *match) {
	size_t i_l = strlen(input), m_l = strlen(match);

	size_t i;

	for (i = 0; i < i_l - m_l + 1; i++) {
		if (!strncmp(&input[i], match, m_l)) {
			return i;
		}
	}

	return -1;
}

char *string_replace(char *input, char *match, char *with, char *output,
		size_t max_out) {
	size_t i_l = strlen(input), w_l = strlen(with), m_l = strlen(match);

	size_t m_off = str_match(input, match);

	if ((int) m_off < 0) {
		return output;
	}

	bzero(output, max_out);

	g_strncpy(output, input, m_off);
	g_strncpy(&output[m_off], with, w_l);
	g_strncpy(&output[m_off + w_l], &input[m_off + m_l], i_l - m_off - m_l);

	return output;
}

uint64_t dirlog_find_simple(char *dirn, int mode, uint32_t flags,
		void *callback) {
	g_setjmp(0, "dirlog_find_simple", NULL, NULL);
	struct dirlog buffer;
	int (*callback_f)(struct dirlog *data) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1)) {
		return MAX_uint64_t;
	}

	int r;
	uint64_t ur = MAX_uint64_t;

	char buffer_s[255] = { 0 };

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	size_t s_blen = strlen(buffer_s), d_ptr_blen;

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		d_ptr_blen = strlen(d_ptr->dirname);
		if (d_ptr_blen == s_blen
				&& !strncmp(buffer_s, d_ptr->dirname, d_ptr_blen)) {
			ur = g_act_1.offset - 1;
			if (mode == 2 && callback) {
				if (callback_f(&buffer)) {
					break;
				}
			} else {

				break;
			}
		}
	}

	if (mode != 1) {
		g_close(&g_act_1);
	}

	return ur;
}

uint64_t nukelog_find(char *dirn, int mode, struct nukelog *output) {
	g_setjmp(0, "nukelog_find", NULL, NULL);
	struct nukelog buffer = { 0 };

	uint64_t r = MAX_uint64_t;
	char *dup, *dup2, *base, *dir;

	if (g_fopen(NUKELOG, "r", F_DL_FOPEN_BUFFER, &g_act_2)) {
		goto r_end;
	}

	int gi1, gi2;
	gi2 = strlen(dirn);

	struct nukelog *n_ptr = NULL;

	while ((n_ptr = (struct nukelog *) g_read(&buffer, &g_act_2, NL_SZ))) {
		dup = strdup(n_ptr->dirname);
		base = basename(dup);
		gi1 = strlen(base);
		dup2 = strdup(n_ptr->dirname);
		dir = dirname(dup2);

		if (gi1 >= gi2 || gi1 < 2) {
			goto l_end;
		}

		if (!strncmp(&dirn[gi2 - gi1], base, gi1)
				&& !strncmp(dirn, n_ptr->dirname, strlen(dir))) {
			if (output) {
				g_memcpy(output, n_ptr, NL_SZ);
			}
			r = g_act_2.offset - 1;
			if (mode != 2) {
				g_free(dup);
				g_free(dup2);
				break;
			}
		}
		l_end: g_free(dup2);
		g_free(dup);
	}

	if (mode != 1) {
		g_close(&g_act_2);
	}

	r_end:

	return r;
}

int rebuild_data_file(char *file, struct g_handle *hdl) {
	g_setjmp(0, "rebuild_data_file", NULL, NULL);
	int ret = 0, r;
	off_t sz_r;
	struct stat st;
	char buffer[4096] = { 0 };

	if (strlen(file) + 4 > 4096) {
		return 1;
	}

	if (gfl & F_OPT_NOWRITE) {
		return 0;
	}

	bzero(hdl->s_buffer, 4096);
	sprintf(hdl->s_buffer, "%s.%d.dtm", file, getpid());
	sprintf(buffer, "%s.bk", file);

	if (hdl->buffer_count
			&& (exec_str || (gfl & F_OPT_HAS_G_REGEX)
					|| (gfl & F_OPT_HAS_G_MATCH))
			&& updmode != UPD_MODE_RECURSIVE) {
		g_setjmp(0, "rebuild_data_file(2)", NULL, NULL);
		if (gfl & F_OPT_VERBOSE2) {
			print_str("NOTICE: %s: filtering data..\n", file);
		}

		p_md_obj ptr = md_first(&hdl->buffer);

		while (ptr) {
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			if (g_bmatch(ptr->ptr, hdl)) {
				if (!(ptr = md_unlink(&hdl->buffer, ptr))) {
					if (!hdl->buffer.offset) {
						if (!(gfl & F_OPT_FORCE)) {
							print_str(
									"WARNING: %s: everything got filtered, refusing to write 0-byte data file\n",
									file);
							return 11;
						}
					} else {
						print_str(
								"ERROR: %s: failed unlinking record entry after match!\n",
								file);
						return 12;
					}
				}
				hdl->buffer_count--;
				continue;
			}
			ptr = ptr->next;
		}
	}

	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: %s: flushing data to disk..\n", hdl->s_buffer);
	}

	hdl->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if (!file_exists(file)) {
		if (stat(file, &st)) {
			print_str(
					"WARNING: %s: could not get stats from data file! (chmod manually)\n",
					file);
		} else {
			hdl->st_mode = 0;
			hdl->st_mode |= st.st_mode;
		}
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: using mode %o\n", hdl->s_buffer, hdl->st_mode);
	}

	off_t rw_o = hdl->rw, bw_o = hdl->bw;

	if ((r = flush_data_md(hdl, hdl->s_buffer))) {
		if (r == 1) {
			if (gfl & F_OPT_VERBOSE) {
				print_str("WARNING: %s: empty buffer (nothing to flush)\n",
						hdl->s_buffer);
			}
			ret = 0;
		} else {
			print_str("ERROR: %s: [%d] flushing data failed!\n", hdl->s_buffer,
					r);
			ret = 2;
		}

		goto end;
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: flushed %llu records, %llu bytes\n",
				hdl->s_buffer, (ulint64_t) (hdl->rw - rw_o),
				(ulint64_t) (hdl->bw - bw_o));
	}

	g_setjmp(0, "rebuild_data_file(3)", NULL, NULL);

	if (gfl & F_OPT_KILL_GLOBAL) {
		print_str(
				"WARNING: %s: aborting rebuild (will not be writing what was done up to here)\n",
				file);
		if (!(gfl & F_OPT_NOWRITE)) {
			remove(hdl->s_buffer);
		}
		return 0;
	}

	g_setjmp(0, "rebuild_data_file(4)", NULL, NULL);

	if (!(gfl & F_OPT_FORCE) && !(gfl & F_OPT_NOWRITE)
			&& (sz_r = get_file_size(hdl->s_buffer)) < hdl->block_sz) {
		print_str(
				"ERROR: %s: [%u/%u] generated data file is smaller than a single record!\n",
				hdl->s_buffer, (uint32_t) sz_r, (uint32_t) hdl->block_sz);
		ret = 7;
		if (!(gfl & F_OPT_NOWRITE)) {
			remove(hdl->s_buffer);
		}
		goto end;
	}

	g_setjmp(0, "rebuild_data_file(5)", NULL, NULL);

	if (!(gfl & F_OPT_NOWRITE)
			&& !((hdl->flags & F_GH_WAPPEND) && (hdl->flags & F_GH_DFWASWIPED))) {
		if (data_backup_records(file)) {
			ret = 3;
			if (!(gfl & F_OPT_NOWRITE)) {
				remove(hdl->s_buffer);
			}
			goto end;
		}
	}

	g_setjmp(0, "rebuild_data_file(6)", NULL, NULL);

	if (!(gfl & F_OPT_NOWRITE)) {

		if (hdl->fh) {
			g_fclose(hdl->fh);
			hdl->fh = NULL;
		}

		if (!file_exists(file) && !(hdl->flags & F_GH_DFNOWIPE)
				&& (hdl->flags & F_GH_WAPPEND)
				&& !(hdl->flags & F_GH_DFWASWIPED)) {
			if (remove(file)) {
				print_str("ERROR: %s: could not clean old data file\n", file);
				ret = 9;
				goto end;
			}
			hdl->flags |= F_GH_DFWASWIPED;
		}

		g_setjmp(0, "rebuild_data_file(7)", NULL, NULL);

		if (!strncmp(hdl->mode, "a", 1) || (hdl->flags & F_GH_WAPPEND)) {
			if ((r = (int) file_copy(hdl->s_buffer, file, "ab",
			F_FC_MSET_SRC)) < 1) {
				print_str("ERROR: %s: [%d] merging temp file failed!\n",
						hdl->s_buffer, r);
				ret = 4;
			}

		} else {
			if ((r = rename(hdl->s_buffer, file))) {
				print_str("ERROR: %s: [%d] renaming temporary file failed!\n",
						hdl->s_buffer,
						errno);
				ret = 4;
			}
		}
	}
	end:

	return ret;
}

#define MAX_WBUFFER_HOLD	100000

int g_load_record(struct g_handle *hdl, const void *data) {
	g_setjmp(0, "g_load_record", NULL, NULL);
	void *buffer = NULL;

	if (hdl->w_buffer.offset == MAX_WBUFFER_HOLD) {
		hdl->w_buffer.flags |= F_MDA_FREE;
		rebuild_data_file(hdl->file, hdl);
		p_md_obj ptr = hdl->w_buffer.objects, ptr_s;
		if (gfl & F_OPT_VERBOSE3) {
			print_str("NOTICE: scrubbing write cache..\n");
		}
		while (ptr) {
			ptr_s = ptr->next;
			g_free(ptr->ptr);
			bzero(ptr, sizeof(md_obj));
			ptr = ptr_s;
		}
		hdl->w_buffer.pos = hdl->w_buffer.objects;
		hdl->w_buffer.offset = 0;
	}

	buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);

	if (!buffer) {
		return 2;
	}

	memcpy(buffer, data, hdl->block_sz);

	return 0;
}

int flush_data_md(struct g_handle *hdl, char *outfile) {
	g_setjmp(0, "flush_data_md", NULL, NULL);

	if (gfl & F_OPT_NOWRITE) {
		return 0;
	}

	FILE *fh = NULL;
	size_t bw = 0;
	unsigned char *buffer = NULL;
	char *mode = "w";

	int ret = 0;

	if (!(gfl & F_OPT_FORCE)) {
		if (hdl->flags & F_GH_FFBUFFER) {
			if (!hdl->w_buffer.offset) {
				return 1;
			}
		} else {
			if (!hdl->buffer_count) {
				return 1;
			}
		}
	}

	if ((fh = gg_fopen(outfile, mode)) == NULL) {
		return 2;
	}

	size_t v = (V_MB * 8) / hdl->block_sz;

	buffer = calloc(v, hdl->block_sz);

	p_md_obj ptr;

	if (hdl->flags & F_GH_FFBUFFER) {
		ptr = md_first(&hdl->w_buffer);
	} else {
		ptr = md_first(&hdl->buffer);
	}

	g_setjmp(0, "flush_data_md(loop)", NULL, NULL);

	while (ptr) {
		if ((bw = g_fwrite(ptr->ptr, hdl->block_sz, 1, fh)) != 1) {
			ret = 3;
			break;
		}
		hdl->bw += hdl->block_sz;
		hdl->rw++;
		ptr = ptr->next;
	}

	if (!hdl->bw && !(gfl & F_OPT_FORCE)) {
		ret = 5;
	}

	g_setjmp(0, "flush_data_md(2)", NULL, NULL);

	g_free(buffer);
	g_fclose(fh);

	if (hdl->st_mode) {
		chmod(outfile, hdl->st_mode);
	}

	return ret;
}

size_t g_load_data_md(void *output, size_t max, char *file) {
	g_setjmp(0, "g_load_data_md", NULL, NULL);
	size_t fr, c_fr = 0;
	FILE *fh;

	if (!(fh = gg_fopen(file, "rb"))) {
		return 0;
	}

	unsigned char *b_output = (unsigned char*) output;
	while ((fr = g_fread(&b_output[c_fr], 1, max - c_fr, fh))) {
		c_fr += fr;
	}

	g_fclose(fh);
	return c_fr;
}

int load_data_md(pmda md, char *file, struct g_handle *hdl) {
	g_setjmp(0, "load_data_md", NULL, NULL);

	int r = 0;
	size_t count = 0;

	if (!hdl->block_sz) {
		return -2;
	}

	if (hdl->flags & F_GH_SHM) {
		if ((r = shmap(hdl, SHM_IPC))) {
			md_g_free(md);
			return r;
		}
		count = hdl->total_sz / hdl->block_sz;
	} else {
		count = hdl->total_sz / hdl->block_sz;
		hdl->data = calloc(count, hdl->block_sz);
	}

	if (md_init(md, count)) {
		return -4;
	}

	size_t i, b_read = 0;

	hdl->buffer_count = 0;
	md->flags |= F_MDA_REFPTR;

	if (!(hdl->flags & F_GH_SHM)) {
		if ((b_read = g_load_data_md(hdl->data, hdl->total_sz, file))
				!= hdl->total_sz) {
			md_g_free(md);
			return -9;
		}
	}

	unsigned char *w_ptr = (unsigned char*) hdl->data;

	for (i = 0; i < count; i++) {
		md->lref_ptr = (void*) w_ptr;
		w_ptr += hdl->block_sz;
		if (!md_alloc(md, hdl->block_sz)) {
			md_g_free(md);
			return -5;
		}

		hdl->buffer_count++;
	}

	g_setjmp(0, "load_data_md", NULL, NULL);

	r = hdl->buffer_count;

	return r;
}

#define MSG_DEF_SHM "SHARED MEMORY"

int g_map_shm(struct g_handle *hdl, key_t ipc) {
	hdl->flags |= F_GH_SHM;

	if (hdl->buffer.count) {
		return 0;
	}

	if (!SHM_IPC) {
		print_str(
				"ERROR: %s: could not get IPC key, set manually (--ipc <key>)\n",
				MSG_DEF_SHM);
		return 1;
	}

	int r = load_data_md(&hdl->buffer, NULL, hdl);

	if (!hdl->buffer_count) {
		if (((gfl & F_OPT_VERBOSE) && r != 1002) || (gfl & F_OPT_VERBOSE4)) {
			print_str(
					"ERROR: %s: [%u/%u] [%u] [%u] could not map shared memory segment! [%d]\n",
					MSG_DEF_SHM, (uint32_t) hdl->buffer_count,
					(uint32_t) (hdl->total_sz / hdl->block_sz),
					(uint32_t) hdl->total_sz, hdl->block_sz, r);
		}
		return 9;
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: mapped %u records\n",
		MSG_DEF_SHM, (uint32_t) hdl->buffer_count);
	}

	hdl->flags |= F_GH_ISONLINE;

	return 0;
}

int g_buffer_into_memory(char *file, struct g_handle *hdl) {
	g_setjmp(0, "g_buffer_into_memory", NULL, NULL);

	if (hdl->buffer.count) {
		return 0;
	}

	struct stat st;

	if (stat(file, &st) == -1) {
		print_str("ERROR: %s: [%d] unable to get information from file!\n",
				file,
				errno);
		return 2;
	}

	if (!st.st_size) {
		print_str("ERROR: %s: 0-byte data file detected!!\n", file);
		return 3;
	}

	if (st.st_size > db_max_size) {
		print_str(
				"WARNING: %s: disabling memory buffering, file too big (max %lld MB)\n",
				file, db_max_size / 1024 / 1024);
		hdl->flags |= F_GH_NOMEM;
		return 5;
	}

	hdl->total_sz = st.st_size;

	bzero(hdl->file, 4096);
	g_strncpy(hdl->file, file, strlen(file));

	if (determine_datatype(hdl)) {
		print_str("ERROR: %s: could not determine data type\n", file);
		return 6;
	}

	if (st.st_size % hdl->block_sz) {
		print_str(MSG_GEN_DFCORRU, file, (ulint64_t) st.st_size, hdl->block_sz);
		return 12;
	}

	if (gfl & F_OPT_VERBOSE2)
		print_str(
				"NOTICE: %s: loading data file into memory [%u records] [%llu bytes]\n",
				file, (uint32_t) (hdl->total_sz / hdl->block_sz),
				(ulint64_t) hdl->total_sz);

	int r;
	if ((r = load_data_md(&hdl->buffer, file, hdl))
			!= (hdl->total_sz / hdl->block_sz)) {
		print_str(
				"ERROR: %s: [%d/%u] [%u] [%u] could not load data into memory!\n",
				file, r, (uint32_t) (hdl->total_sz / hdl->block_sz),
				(uint32_t) hdl->total_sz, hdl->block_sz);
		return 4;
	} else {
		if (gfl & F_OPT_VERBOSE2) {
			print_str("NOTICE: %s: loaded %u records into memory\n", file, r);
		}
	}
	return 0;
}

int determine_datatype(struct g_handle *hdl) {
	if (!strncmp(hdl->file, DIRLOG, strlen(DIRLOG))) {
		hdl->flags |= F_GH_ISDIRLOG;
		hdl->block_sz = DL_SZ;
	} else if (!strncmp(hdl->file, NUKELOG, strlen(NUKELOG))) {
		hdl->flags |= F_GH_ISNUKELOG;
		hdl->block_sz = NL_SZ;
	} else if (!strncmp(hdl->file, DUPEFILE, strlen(DUPEFILE))) {
		hdl->flags |= F_GH_ISDUPEFILE;
		hdl->block_sz = DF_SZ;
	} else if (!strncmp(hdl->file, LASTONLOG, strlen(LASTONLOG))) {
		hdl->flags |= F_GH_ISLASTONLOG;
		hdl->block_sz = LO_SZ;
	} else if (!strncmp(hdl->file, ONELINERS, strlen(ONELINERS))) {
		hdl->flags |= F_GH_ISONELINERS;
		hdl->block_sz = OL_SZ;
	} else {
		return 1;
	}
	return 0;
}

int g_fopen(char *file, char *mode, uint32_t flags, struct g_handle *hdl) {
	g_setjmp(0, "g_fopen", NULL, NULL);

	if (flags & F_DL_FOPEN_SHM) {
		if (g_map_shm(hdl, SHM_IPC)) {
			return 12;
		}
		return 0;
	}

	if (flags & F_DL_FOPEN_REWIND) {
		gh_rewind(hdl);
	}

	if (!(gfl & F_OPT_NOBUFFER) && (flags & F_DL_FOPEN_BUFFER)
			&& !(hdl->flags & F_GH_NOMEM)) {
		if (!g_buffer_into_memory(file, hdl)) {
			if (!(flags & F_DL_FOPEN_FILE)) {
				return 0;
			}
		} else {
			if (!(hdl->flags & F_GH_NOMEM)) {
				return 11;
			}
		}
	}

	if (hdl->fh) {
		return 0;
	}

	if (strlen(file) > 4096) {
		print_str(MSG_GEN_NODFILE, file, "file path too large");
		return 2;
	}

	hdl->total_sz = get_file_size(file);

	if (!hdl->total_sz) {
		print_str(MSG_GEN_NODFILE, file, "zero-byte data file");
	}

	bzero(hdl->file, 4096);
	g_strncpy(hdl->file, file, strlen(file));

	if (determine_datatype(hdl)) {
		print_str(MSG_GEN_NODFILE, file, "could not determine data-type");
		return 3;
	}

	if (hdl->total_sz % hdl->block_sz) {
		print_str(MSG_GEN_DFCORRU, file, (ulint64_t) hdl->total_sz,
				hdl->block_sz);
		return 4;
	}

	FILE *fd;

	if (!(fd = gg_fopen(file, mode))) {
		print_str(MSG_GEN_NODFILE, file, "not available");
		return 1;
	}

	hdl->fh = fd;

	return 0;
}

int g_close(struct g_handle *hdl) {
	g_setjmp(0, "g_close", NULL, NULL);
	bzero(&dl_stats, sizeof(struct d_stats));
	dl_stats.br += hdl->br;
	dl_stats.bw += hdl->bw;
	dl_stats.rw += hdl->rw;

	if (hdl->fh) {
		g_fclose(hdl->fh);
		hdl->fh = NULL;
	}

	if ((hdl->flags & F_GH_SHM)) {
		shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf);
		if (hdl->ipcbuf.shm_nattch <= 1) {
			shmctl(hdl->shmid, IPC_RMID, 0);
		}
		shmdt(hdl->data);
		hdl->data = NULL;
		hdl->shmid = 0;
	}

	if (hdl->buffer_count) {
		hdl->offset = 0;
		hdl->buffer.r_pos = hdl->buffer.objects;
		if ((hdl->flags & F_GH_SHM)) {
			md_g_free(&hdl->buffer);
			md_g_free(&hdl->w_buffer);
		}
		/*if (hdl->data && !(hdl->flags & F_GH_SHM)) {
		 g_free(hdl->data);
		 hdl->data = NULL;
		 }*/
	}

	hdl->br = 0;
	hdl->bw = 0;
	hdl->rw = 0;

	return 0;
}

int g_do_exec(void *buffer, void *callback, char *ex_str) {
	g_setjmp(0, "g_do_exec", NULL, NULL);

	if (callback) {
		char b_glob[MAX_EXEC_STR] = { 0 };
		char *e_str;
		if (ex_str) {
			e_str = ex_str;
		} else {
			if (!exec_str) {
				return -1;
			}
			e_str = exec_str;
		}
		bzero(b_glob, MAX_EXEC_STR);

		if (process_exec_string(e_str, b_glob, callback, buffer)) {
			return 2;
		}

		return system(b_glob);
	} else if (ex_str) {
		if (strlen(ex_str) > MAX_EXEC_STR) {
			return -1;
		}
		return system(ex_str);
	} else {
		return -1;
	}
}

void *g_read(void *buffer, struct g_handle *hdl, size_t size) {
	g_setjmp(0, "g_read", NULL, NULL);
	if (hdl->buffer_count) {
		hdl->buffer.pos = hdl->buffer.r_pos;
		if (!hdl->buffer.pos) {
			return NULL;
		}
		hdl->buffer.r_pos = hdl->buffer.r_pos->next;
		hdl->buffer.offset++;
		hdl->offset++;
		hdl->br += hdl->block_sz;
		return (struct dirlog *) hdl->buffer.pos->ptr;
	}

	if (!buffer) {
		print_str("IO ERROR: no buffer to write to\n");
		return NULL;
	}

	if (!hdl->fh) {
		print_str("IO ERROR: data file handle not open\n");
		return NULL;
	}

	if (feof(hdl->fh)) {
		return NULL;
	}

	size_t fr;

	if ((fr = g_fread(buffer, 1, size, hdl->fh)) != size) {
		if (fr == 0) {
			return NULL;
		}
		return NULL;
	}

	hdl->br += fr;
	hdl->offset++;

	return buffer;
}

size_t read_from_pipe(char *buffer, FILE *pipe) {
	size_t read = 0, r;

	while (!feof(pipe)) {
		if ((r = g_fread(&buffer[read], 1, PIPE_READ_MAX - read, pipe)) <= 0) {
			break;
		}
		read += r;
	}

	return read;
}

size_t exec_and_wait_for_output(char *command, char *output) {
	char buf[PIPE_READ_MAX] = { 0 };
	size_t r = 0;
	FILE *pipe = NULL;

	if (!(pipe = popen(command, "r"))) {
		return 0;
	}

	r = read_from_pipe(buf, pipe);

	pclose(pipe);

	if (output && r) {

		g_strncpy(output, buf, strlen(buf));
	}

	return r;
}

int dirlog_write_record(struct dirlog *buffer, off_t offset, int whence) {
	g_setjmp(0, "dirlog_write_record", NULL, NULL);
	if (gfl & F_OPT_NOWRITE) {
		return 0;
	}

	if (!buffer) {
		return 2;
	}

	if (!g_act_1.fh) {
		print_str("ERROR: dirlog handle is not open\n");
		return 1;
	}

	if (whence == SEEK_SET
			&& fseeko(g_act_1.fh, offset * DL_SZ, SEEK_SET) < 0) {
		print_str("ERROR: seeking dirlog failed!\n");
		return 1;
	}

	int fw;

	if ((fw = g_fwrite(buffer, 1, DL_SZ, g_act_1.fh)) < DL_SZ) {
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

#define F_ENUMD_ENDFIRSTOK		0x1
#define F_ENUMD_BREAKONBAD		0x2

int enum_dir(char *dir, void *cb, void *arg, int f) {
	g_setjmp(0, "enum_dir", NULL, NULL);
	int (*callback_f)(char *, unsigned char, void *) = NULL;
	struct dirent *dirp;
	int r = 0, ir;

	callback_f = cb;

	if (!callback_f) {
		return -1;
	}

	DIR *dp = opendir(dir);

	if (!dp) {
		return -2;
	}

	char buf[1024];
	int d_type;
	struct stat st;

	stat(dir, &st);

	unsigned int i_minor = minor(st.st_dev);

	while ((dirp = readdir(dp))) {
		if ((gfl & F_OPT_KILL_GLOBAL) || (gfl & F_OPT_TERM_ENUM)) {
			break;
		}

		size_t d_name_l = strlen(dirp->d_name);

		if ((d_name_l == 1 && !strncmp(dirp->d_name, ".", 1))
				|| (d_name_l == 2 && !strncmp(dirp->d_name, "..", 2))) {
			continue;
		}

		sprintf(buf, "%s/%s", dir, dirp->d_name);
		remove_repeating_chars(buf, 0x2F);

		stat(buf, &st);

		if ((gfl & F_OPT_XBLK) && major(st.st_dev) != 8) {
			continue;
		}

		d_type = get_file_type_no(&st);

		if ((gfl & F_OPT_XDEV) && d_type == DT_DIR
				&& minor(st.st_dev) != i_minor) {
			continue;
		}

		if (!(ir = callback_f(buf, d_type, arg))) {
			if (f & F_ENUMD_ENDFIRSTOK) {
				r = ir;
				break;
			} else {
				r++;
			}
		} else {
			if (f & F_ENUMD_BREAKONBAD) {
				r = ir;
				break;
			}
		}
	}

	closedir(dp);
	return r;
}

int dir_exists(char *dir) {
	g_setjmp(0, "dir_exists", NULL, NULL);
	int r;

	errno = 0;
	DIR *dd = opendir(dir);

	r = errno;

	if (dd) {
		closedir(dd);
	}
	else {
		if ( !r ) {
			r++;
		}
	}

	return r;
}

int reg_match(char *expression, char *match, int flags) {
	g_setjmp(0, "reg_match", NULL, NULL);
	regex_t preg;
	size_t r;
	regmatch_t pmatch[REG_MATCHES_MAX];

	if ((r = regcomp(&preg, expression, (flags | REG_EXTENDED))))
		return r;

	r = regexec(&preg, match, REG_MATCHES_MAX, pmatch, 0);

	regfree(&preg);

	return r;
}

int split_string(char *line, char dl, pmda output_t) {
	g_setjmp(0, "split_string", NULL, NULL);
	int i, p, c, llen = strlen(line);

	for (i = 0, p = 0, c = 0; i <= llen; i++) {

		while (line[i] == dl && line[i])
			i++;
		p = i;

		while (line[i] != dl && line[i] != 0xA && line[i])
			i++;

		if (i > p) {
			char *buffer = md_alloc(output_t, (i - p) + 10);
			if (!buffer)
				return -1;
			g_memcpy(buffer, &line[p], i - p);
			c++;
		}
	}
	return c;
}

int split_string_sp_tab(char *line, pmda output_t) {
	g_setjmp(0, "split_string_sp_tab", NULL, NULL);
	int i, p, c, llen = strlen(line);

	for (i = 0, p = 0, c = 0; i <= llen; i++) {

		while ((line[i] == 0x20 && line[i] != 0x9) && line[i])
			i++;
		p = i;

		while ((line[i] != 0x20 && line[i] != 0x9) && line[i] != 0xA && line[i])
			i++;

		if (i > p) {
			char *buffer = md_alloc(output_t, (i - p) + 10);
			if (!buffer)
				return -1;
			g_memcpy(buffer, &line[p], i - p);
			c++;
		}
	}
	return c;
}

int remove_repeating_chars(char *string, char c) {
	g_setjmp(0, "remove_repeating_chars", NULL, NULL);
	size_t s_len = strlen(string);
	int i, i_c = -1;

	for (i = 0; i <= s_len; i++, i_c = -1) {
		while (string[i + i_c] == c) {
			i_c++;
		}
		if (i_c > 0) {
			int ct_l = (s_len - i) - i_c;
			if (!g_memmove(&string[i], &string[i + i_c], ct_l)) {
				return -1;
			}
			string[i + ct_l] = 0;
			i += i_c;
		} else {
			i += i_c + 1;
		}
	}

	return 0;
}

int write_file_text(char *data, char *file) {
	g_setjmp(0, "write_file_text", NULL, NULL);
	int r;
	FILE *fp;

	if ((fp = gg_fopen(file, "a")) == NULL)
		return 0;

	r = g_fwrite(data, 1, strlen(data), fp);

	g_fclose(fp);

	return r;
}

int delete_file(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "delete_file", NULL, NULL);
	char *match = (char*) arg;

	if (type != DT_REG) {
		return 1;
	}

	if (!reg_match(match, name, 0)) {
		return remove(name);
	}

	return 2;
}

off_t read_file(char *file, void *buffer, size_t read_max, off_t offset) {
	g_setjmp(0, "read_file", NULL, NULL);
	size_t read;
	int r;
	FILE *fp;

	off_t a_fsz = get_file_size(file);

	if (!a_fsz)
		return 0;

	if (read_max > a_fsz) {
		read_max = a_fsz;
	}

	if ((fp = gg_fopen(file, "rb")) == NULL)
		return 0;

	if (offset)
		fseeko(fp, (off_t) offset, SEEK_SET);

	for (read = 0; !feof(fp) && read < read_max;) {
		if ((r = g_fread(&((unsigned char*) buffer)[read], 1, read_max - read,
				fp)) < 1)
			break;
		read += r;
	}

	g_fclose(fp);

	return read;
}

int file_exists(char *file) {
	g_setjmp(0, "file_exists", NULL, NULL);

	int r = get_file_type(file);

	if (r == DT_REG) {
		return 0;
	}

	return 1;
}

ssize_t file_copy(char *source, char *dest, char *mode, uint32_t flags) {
	g_setjmp(0, "file_copy", NULL, NULL);

	if (gfl & F_OPT_NOWRITE) {
		return 1;
	}

	struct stat st_s, st_d;
	mode_t st_mode = 0;

	if (stat(source, &st_s)) {
		return -9;
	}

	off_t ssize = st_s.st_size;

	if (ssize < 1) {
		return -1;
	}

	FILE *fh_s = gg_fopen(source, "rb");

	if (!fh_s) {
		return -2;
	}

	if (!strncmp(mode, "a", 1) && (flags & F_FC_MSET_DEST)) {
		if (file_exists(dest)) {
			st_mode = st_s.st_mode;
		} else {
			if (stat(source, &st_d)) {
				return -10;
			}
			st_mode = st_d.st_mode;
		}
	} else if (flags & F_FC_MSET_SRC) {
		st_mode = st_s.st_mode;
	}

	FILE *fh_d = gg_fopen(dest, mode);

	if (!fh_d) {
		return -3;
	}

	size_t r = 0, t = 0, w;
	char *buffer = calloc(V_MB, 1);

	while ((r = g_fread(buffer, 1, V_MB, fh_s)) > 0) {
		if ((w = g_fwrite(buffer, 1, r, fh_d))) {
			t += w;
		} else {
			return -4;
		}
	}

	g_free(buffer);
	g_fclose(fh_d);
	g_fclose(fh_s);

	if (st_mode) {
		chmod(dest, st_mode);
	}

	return t;
}

uint64_t file_crc32(char *file, uint32_t *crc_out) {
	g_setjmp(0, "file_crc32", NULL, NULL);
	FILE *fp;
	int read;
	size_t r;
	unsigned char *buffer = calloc(CRC_FILE_READ_BUFFER_SIZE, 1);

	*crc_out = 0x0;

	if ((fp = gg_fopen((char*) &file[0], "rb")) == NULL) {
		g_free(buffer);
		return 0;
	}

	uint32_t crc = MAX_ULONG;

	for (read = 0, r = 0; !feof(fp);) {
		if ((r = g_fread(buffer, 1, CRC_FILE_READ_BUFFER_SIZE, fp)) < 1)
			break;
		crc = crc32(crc, buffer, r);
		bzero(buffer, CRC_FILE_READ_BUFFER_SIZE);
		read += r;
	}

	if (read)
		*crc_out = crc;

	g_free(buffer);
	g_fclose(fp);

	return read;
}

#define F_CFGV_BUILD_FULL_STRING	0x1
#define F_CFGV_RETURN_MDA_OBJECT	0x2
#define F_CFGV_RETURN_TOKEN_EX		0x4

#define F_CFGV_BUILD_DATA_PATH		0x1000

#define F_CFGV_MODES				(F_CFGV_BUILD_FULL_STRING|F_CFGV_RETURN_MDA_OBJECT|F_CFGV_RETURN_TOKEN_EX)

#define MAX_CFGV_RES_LENGTH			50000

void *ref_to_val_get_cfgval(char *cfg, char *key, char *defpath, int flags,
		char *out, size_t max) {
	g_setjmp(0, "ref_to_val_get_cfgval", NULL, NULL);
	char buffer[PATH_MAX];

	if (flags & F_CFGV_BUILD_DATA_PATH) {
		sprintf(buffer, "%s/%s/%s/%s", GLROOT, FTPDATA, defpath, cfg);
	} else {
		sprintf(buffer, "%s", cfg);
	}

	pmda ret;

	if (load_cfg(&cfg_rf, buffer, 0, &ret)) {
		return NULL;
	}

	mda s_tk = { 0 };
	int r;
	size_t c_token = -1;
	char *s_key = key;

	md_init(&s_tk, 4);

	if ((r = split_string(key, 0x3A, &s_tk)) == 2) {
		p_md_obj s_tk_ptr = s_tk.objects->next;
		flags ^= F_CFGV_BUILD_FULL_STRING;
		flags |= F_CFGV_RETURN_TOKEN_EX;
		c_token = atoi(s_tk_ptr->ptr);
		s_key = s_tk.objects->ptr;

	}

	p_md_obj ptr;
	pmda s_ret = NULL;
	void *p_ret = NULL;

	if ((ptr = get_cfg_opt(s_key, ret, &s_ret))) {
		switch (flags & F_CFGV_MODES) {
		case F_CFGV_RETURN_MDA_OBJECT:
			p_ret = (void*) ptr;
			break;
		case F_CFGV_BUILD_FULL_STRING:
			;
			size_t o_w = 0, w;
			while (ptr) {
				w = strlen((char*) ptr->ptr);
				if (o_w + w + 1 < max) {
					g_memcpy(&out[o_w], ptr->ptr, w);
					o_w += w;
					if (ptr->next) {
						memset(&out[o_w], 0x20, 1);
						o_w++;
					}
				} else {
					break;
				}
				ptr = ptr->next;
			}
			p_ret = (void*) out;
			break;

		case F_CFGV_RETURN_TOKEN_EX:
			if (c_token < 0 || c_token >= s_ret->count) {
				return NULL;
			}
			p_md_obj p_ret_tx = &s_ret->objects[c_token];
			if (p_ret_tx) {
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

int is_char_uppercase(char c) {
	if (c >= 0x41 && c <= 0x5A) {
		return 0;
	}
	return 1;
}

int ref_to_val_macro(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_macro", NULL, NULL);

	if (!output) {
		return 2;
	}

	if (!strcmp(match, "m:exe")) {

		if (self_get_path(output)) {
			sprintf(output, "UNKNOWN");
		}
	} else if (!strcmp(match, "m:glroot")) {
		sprintf(output, GLROOT);
	} else if (!strcmp(match, "m:siteroot")) {
		sprintf(output, SITEROOT);
	} else if (!strcmp(match, "m:ftpdata")) {
		sprintf(output, FTPDATA);
	} else if ((gfl & F_OPT_PS_LOGGING) && !strcmp(match, "m:logfile")) {
		sprintf(output, LOGFILE);
	} else if (!strcmp(match, "m:PID")) {
		sprintf(output, "%d", getpid());
	} else if (!strcmp(match, "m:IPC")) {
		sprintf(output, "%.8X", (uint32_t) SHM_IPC);
	} else if (!strcmp(match, "m:spec1")) {
		sprintf(output, "%s", b_spec1);
	} else if (!strcmp(match, "m:arg1")) {
		sprintf(output, "%s", MACRO_ARG1);
	} else if (!strcmp(match, "m:arg2")) {
		sprintf(output, "%s", MACRO_ARG2);
	} else if (!strcmp(match, "m:arg3")) {
		sprintf(output, "%s", MACRO_ARG3);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_generic(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_generic", NULL, NULL);

	if (!output) {
		return 2;
	}

	if (!strcmp(match, "exe")) {
		if (self_get_path(output)) {
			sprintf(output, "UNKNOWN");
		}
	} else if (!strcmp(match, "glroot")) {
		sprintf(output, GLROOT);
	} else if (!strcmp(match, "siteroot")) {
		sprintf(output, SITEROOT);
	} else if (!strcmp(match, "ftpdata")) {
		sprintf(output, FTPDATA);
	} else if (!strcmp(match, "logfile")) {
		sprintf(output, LOGFILE);
	} else if (!strcmp(match, "PID")) {
		sprintf(output, "%d", getpid());
	} else if (!strcmp(match, "IPC")) {
		sprintf(output, "%.8X", (uint32_t) SHM_IPC);
	} else if (!strcmp(match, "spec1")) {
		sprintf(output, "%s", b_spec1);
	} else if (!strcmp(match, "usroot")) {
		sprintf(output, "%s/%s/%s", GLROOT, FTPDATA, DEFPATH_USERS);
		remove_repeating_chars(output, 0x2F);
	} else if (!strcmp(match, "logroot")) {
		sprintf(output, "%s/%s/%s", GLROOT, FTPDATA, DEFPATH_LOGS);
		remove_repeating_chars(output, 0x2F);
	} else if (arg && !strcmp(match, "arg")) {
		sprintf(output, (char*) arg);
	} else if (arg && !strncmp(match, "c:", 2)) {
		char *buffer = calloc(max_size + 1, 1);
		void *ptr = ref_to_val_get_cfgval((char*) arg, &match[2],
		DEFPATH_USERS,
		F_CFGV_BUILD_FULL_STRING, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			sprintf(output, (char*) ptr);
			g_free(buffer);
			return 0;
		}

		ptr = ref_to_val_get_cfgval((char*) arg, &match[2], DEFPATH_GROUPS,
		F_CFGV_BUILD_FULL_STRING, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			sprintf(output, (char*) ptr);
			g_free(buffer);
			return 0;
		}
		g_free(buffer);
		return 1;
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_dirlog(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_dirlog", NULL, NULL);
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(arg, match, output, max_size)) {
		return 0;
	}

	struct dirlog *data = (struct dirlog *) arg;

	if (!strcmp(match, "dir")) {
		sprintf(output, data->dirname);
	} else if (!strcmp(match, "basedir")) {
		char *s_buffer = strdup(data->dirname), *base = basename(s_buffer);
		sprintf(output, base);
		g_free(s_buffer);
	} else if (!strcmp(match, "user")) {
		sprintf(output, "%d", data->uploader);
	} else if (!strcmp(match, "group")) {
		sprintf(output, "%d", data->group);
	} else if (!strcmp(match, "files")) {
		sprintf(output, "%u", (uint32_t) data->files);
	} else if (!strcmp(match, "size")) {
		sprintf(output, "%llu", (ulint64_t) data->bytes);
	} else if (!strcmp(match, "status")) {
		sprintf(output, "%d", (int) data->status);
	} else if (!strcmp(match, "time")) {
		sprintf(output, "%u", (uint32_t) data->uptime);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_nukelog(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_nukelog", NULL, NULL);
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(arg, match, output, max_size)) {
		return 0;
	}

	struct nukelog *data = (struct nukelog *) arg;

	if (!strcmp(match, "dir")) {
		sprintf(output, data->dirname);
	} else if (!strcmp(match, "basedir")) {
		char *s_buffer = strdup(data->dirname), *base = basename(s_buffer);
		g_strncpy(output, base, strlen(base));
		g_free(s_buffer);
	} else if (!strcmp(match, "nuker")) {
		sprintf(output, data->nuker);
	} else if (!strcmp(match, "nukee")) {
		sprintf(output, data->nukee);
	} else if (!strcmp(match, "unnuker")) {
		sprintf(output, data->unnuker);
	} else if (!strcmp(match, "reason")) {
		sprintf(output, data->reason);
	} else if (!strcmp(match, "size")) {
		sprintf(output, "%llu", (ulint64_t) data->bytes);
	} else if (!strcmp(match, "time")) {
		sprintf(output, "%u", (uint32_t) data->nuketime);
	} else if (!strcmp(match, "status")) {
		sprintf(output, "%d", (int) data->status);
	} else if (!strcmp(match, "mult")) {
		sprintf(output, "%d", (int) data->mult);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_dupefile(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_dupefile", NULL, NULL);
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(arg, match, output, max_size)) {
		return 0;
	}

	struct dupefile *data = (struct dupefile *) arg;

	if (!strcmp(match, "file")) {
		g_strncpy(output, data->filename, sizeof(data->filename));
	} else if (!strcmp(match, "user")) {
		sprintf(output, "%s", data->uploader);
	} else if (!strcmp(match, "time")) {
		sprintf(output, "%u", (uint32_t) data->timeup);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_lastonlog(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_lastonlog", NULL, NULL);
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(arg, match, output, max_size)) {
		return 0;
	}

	struct lastonlog *data = (struct lastonlog *) arg;

	if (!strcmp(match, "user")) {
		sprintf(output, data->uname);
	} else if (!strcmp(match, "group")) {
		sprintf(output, data->gname);
	} else if (!strcmp(match, "stats")) {
		sprintf(output, data->stats);
	} else if (!strcmp(match, "tag")) {
		sprintf(output, data->tagline);
	} else if (!strcmp(match, "logon")) {
		sprintf(output, "%u", (uint32_t) data->logon);
	} else if (!strcmp(match, "logoff")) {
		sprintf(output, "%u", (uint32_t) data->logoff);
	} else if (!strcmp(match, "upload")) {
		sprintf(output, "%u", (uint32_t) data->upload);
	} else if (!strcmp(match, "download")) {
		sprintf(output, "%u", (uint32_t) data->download);
	} else if (!is_char_uppercase(match[0])) {
		char *buffer = calloc(max_size + 1, 1);
		void *ptr = ref_to_val_get_cfgval(data->uname, match, DEFPATH_USERS,
		F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			sprintf(output, (char*) ptr);
			g_free(buffer);
			return 0;
		}

		ptr = ref_to_val_get_cfgval(data->gname, match, DEFPATH_GROUPS,
		F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			sprintf(output, (char*) ptr);
			g_free(buffer);
			return 0;
		}
		g_free(buffer);
		return 1;
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_oneliners(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_oneliners", NULL, NULL);
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(arg, match, output, max_size)) {
		return 0;
	}

	struct oneliner *data = (struct oneliner *) arg;

	if (!strcmp(match, "user")) {
		sprintf(output, data->uname);
	} else if (!strcmp(match, "group")) {
		sprintf(output, data->gname);
	} else if (!strcmp(match, "tag")) {
		sprintf(output, data->tagline);
	} else if (!strcmp(match, "msg")) {
		sprintf(output, data->message);
	} else if (!strcmp(match, "time")) {
		sprintf(output, "%u", (uint32_t) data->timestamp);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_online(void *arg, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val_online", NULL, NULL);
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(arg, match, output, max_size)) {
		return 0;
	}

	struct ONLINE *data = (struct ONLINE *) arg;

	if (!strcmp(match, "user")) {
		sprintf(output, data->username);
	} else if (!strcmp(match, "tag")) {
		sprintf(output, data->tagline);
	} else if (!strcmp(match, "status")) {
		sprintf(output, data->status);
	} else if (!strcmp(match, "host")) {
		sprintf(output, data->host);
	} else if (!strcmp(match, "dir")) {
		sprintf(output, data->currentdir);
	} else if (!strcmp(match, "ssl")) {
		sprintf(output, "%u", (uint32_t) data->ssl_flag);
	} else if (!strcmp(match, "group")) {
		sprintf(output, "%u", (uint32_t) data->groupid);
	} else if (!strcmp(match, "time")) {
		sprintf(output, "%u", (uint32_t) data->login_time);
	} else if (!strcmp(match, "lupdtime")) {
		sprintf(output, "%u", (uint32_t) data->tstart.tv_sec);
	} else if (!strcmp(match, "lxfertime")) {
		sprintf(output, "%u", (uint32_t) data->txfer.tv_sec);
	} else if (!strcmp(match, "bxfer")) {
		sprintf(output, "%llu", (ulint64_t) data->bytes_xfer);
	} else if (!strcmp(match, "btxfer")) {
		sprintf(output, "%llu", (ulint64_t) data->bytes_txfer);
	} else if (!strcmp(match, "pid")) {
		sprintf(output, "%u", (uint32_t) data->procid);
	} else if (!strcmp(match, "rate")) {
		int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
		uint32_t kbps = 0;

		if (tdiff > 0 && data->bytes_xfer > 0) {
			kbps = data->bytes_xfer / tdiff;
		}
		sprintf(output, "%u", kbps);
	} else if (!is_char_uppercase(match[0])) {
		char *buffer = calloc(max_size + 1, 1);
		void *ptr = ref_to_val_get_cfgval(data->username, match,
		DEFPATH_USERS, F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH,
				buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			sprintf(output, ptr);
		}
		g_free(buffer);
		return 0;
	} else {
		return 1;
	}
	return 0;
}

int process_exec_string(char *input, char *output, void *callback, void *data) {
	g_setjmp(0, "process_exec_string", NULL, NULL);

	if (!callback) {
		return 1;
	}

	int (*call)(void *, char *, char *, size_t) = callback;

	if (!output) {
		return 3;
	}

	size_t blen = strlen(input), blen2 = strlen(input), blenmax = 0;

	if (!blen || blen > MAX_EXEC_STR) {
		return 4;
	}

	blenmax = blen * 2;

	if (blenmax > MAX_EXEC_STR) {
		return 5;
	}
	size_t b_l_1;
	char buffer[8192] = { 0 }, buffer2[8192] = { 0 }, *buffer_o =
			(char*) calloc(
			MAX_EXEC_STR, 1);
	int i, i2, pi, r;

	for (i = 0, pi = 0; i < blen2; i++, pi++) {
		if (input[i] == 0x7B) {
			bzero(buffer, 255);
			for (i2 = 0, i++, r = 0; i < blen2 && i2 < 255; i++, i2++) {
				if (input[i] == 0x7D) {
					if (!i2 || strlen(buffer) > 255
							|| (r = call(data, buffer, buffer2, 255))) {
						if (r) {
							b_l_1 = strlen(buffer);
							sprintf(&buffer_o[pi], "%c%s%c", 0x7B, buffer,
									0x7D);

							pi += b_l_1 + 2;
						}
						i++;
						break;
					}
					b_l_1 = strlen(buffer2);
					g_memcpy(&buffer_o[pi], buffer2, b_l_1);

					pi += b_l_1;
					i++;
					break;
				}
				buffer[i2] = input[i];
			}
		}
		buffer_o[pi] = input[i];

	}

	buffer_o[pi] = 0x0;
	size_t l = strlen(buffer_o);

	if (l <= MAX_EXEC_STR) {
		g_memcpy(output, buffer_o, l);
	}

	g_free(buffer_o);
	return 0;
}

void child_sig_handler(int signal, siginfo_t * si, void *p) {
	switch (si->si_code) {
	case CLD_KILLED:
		print_str(
				"NOTICE: Child process caught SIGINT (hit CTRL^C again to quit)\n");
		usleep(1000000);
		break;
	case CLD_EXITED:
		break;
	default:
		if (gfl & F_OPT_VERBOSE3) {
			print_str("NOTICE: Child caught signal: %d \n", si->si_code);
		}
		break;
	}
}

#define SIG_BREAK_TIMEOUT_NS (double)1000000.0

void sig_handler(int signal) {
	switch (signal) {
	case SIGTERM:
		print_str("NOTICE: Caught SIGTERM, terminating gracefully.\n");
		gfl |= F_OPT_KILL_GLOBAL;
		break;
	case SIGINT:
		if (gfl & F_OPT_KILL_GLOBAL) {
			print_str(
					"NOTICE: Caught SIGINT twice in %.2f seconds, terminating..\n",
					SIG_BREAK_TIMEOUT_NS / (1000000.0));
			exit(0);
		} else {
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

int shmap(struct g_handle *hdl, key_t ipc) {
	g_setjmp(0, "shmap", NULL, NULL);
	if (hdl->shmid) {
		return 1001;
	}

	if ((hdl->shmid = shmget(ipc, 0, 0)) == -1) {
		return 1002;
	}

	if ((hdl->data = shmat(hdl->shmid, NULL, SHM_RDONLY)) == (void*) -1) {
		hdl->data = NULL;
		return 1003;
	}

	if (shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf) == -1) {
		return 1004;
	}

	if (!hdl->ipcbuf.shm_segsz) {
		return 1005;
	}

	hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;

	return 0;
}

pmda search_cfg_rf(pmda md, char * file) {
	g_setjmp(0, "search_cfg_rf", NULL, NULL);
	p_md_obj ptr = md_first(md);
	p_cfg_r ptr_c;
	size_t fn_len = strlen(file);
	while (ptr) {
		ptr_c = (p_cfg_r) ptr->ptr;
		if (ptr_c && !strncmp(ptr_c->file, file, fn_len)) {
			return &ptr_c->cfg;
		}
		ptr = ptr->next;
	}
	return NULL;
}

pmda register_cfg_rf(pmda md, char *file) {
	g_setjmp(0, "register_cfg_rf", NULL, NULL);
	if (!md->count) {
		if (md_init(md, 128)) {
			return NULL;
		}
	}

	pmda pmd = search_cfg_rf(md, file);

	if (pmd) {
		return pmd;
	}

	size_t fn_len = strlen(file);

	if (fn_len >= PATH_MAX) {
		return NULL;
	}

	p_cfg_r ptr_c = md_alloc(md, sizeof(cfg_r));

	g_strncpy(ptr_c->file, file, fn_len);
	md_init(&ptr_c->cfg, 256);

	return &ptr_c->cfg;
}

int free_cfg_rf(pmda md) {
	g_setjmp(0, "free_cfg_rf", NULL, NULL);
	p_md_obj ptr = md_first(md);
	p_cfg_r ptr_c;
	while (ptr) {
		ptr_c = (p_cfg_r) ptr->ptr;
		free_cfg(&ptr_c->cfg);
		ptr = ptr->next;
	}

	return md_g_free(md);
}

int load_cfg(pmda pmd, char *file, uint32_t flags, pmda *res) {
	g_setjmp(0, "load_cfg", NULL, NULL);
	int r = 0;
	FILE *fh;
	pmda md;

	if (flags & F_LCONF_NORF) {
		md_init(pmd, 256);
		md = pmd;
	} else {
		md = register_cfg_rf(pmd, file);
	}

	if (!md) {
		return 1;
	}

	off_t f_sz = get_file_size(file);

	if (!f_sz) {
		return 2;
	}

	if (!(fh = gg_fopen(file, "r"))) {
		return 3;
	}

	char *buffer = calloc(V_MB, 1);
	p_cfg_h pce;
	int rd, i;

	while (fgets(buffer, V_MB, fh)) {
		if (strlen(buffer) < 3) {
			continue;
		}

		for (i = 0; buffer[i] == 0x20 || buffer[i] == 0x9; i++) {
		}

		if (buffer[i] == 0x23) {
			continue;
		}

		pce = (p_cfg_h) md_alloc(md, sizeof(cfg_h));
		md_init(&pce->data, 32);
		if ((rd = split_string_sp_tab(buffer, &pce->data)) < 1) {
			md_g_free(&pce->data);
			md_unlink(md, md->pos);
			continue;
		}

		pce->key = pce->data.objects->ptr;
	}

	g_fclose(fh);
	g_free(buffer);

	if (res) {
		*res = md;
	}

	return r;
}

void free_cfg(pmda md) {
	g_setjmp(0, "free_cfg", NULL, NULL);

	if (!md->objects) {
		return;
	}

	p_md_obj ptr = md_first(md);
	p_cfg_h pce;

	while (ptr) {
		pce = (p_cfg_h) ptr->ptr;
		if (pce) {
			md_g_free(&pce->data);
		}
		ptr = ptr->next;
	}

	md_g_free(md);
}

p_md_obj get_cfg_opt(char *key, pmda md, pmda *ret) {
	g_setjmp(0, "get_cfg_opt", NULL, NULL);
	if (!md->count) {
		return NULL;
	}

	p_md_obj ptr = md_first(md);
	size_t pce_key_sz, key_sz = strlen(key);
	p_cfg_h pce;

	while (ptr) {
		pce = (p_cfg_h) ptr->ptr;
		pce_key_sz = strlen(pce->key);
		if (pce_key_sz == key_sz && !strncmp(pce->key, key, pce_key_sz)) {
			p_md_obj r_ptr = md_first(&pce->data);
			if (r_ptr) {
				if (ret) {
					*ret = &pce->data;
				}
				return (p_md_obj) r_ptr->next;
			} else {
				return NULL;
			}
		}
		ptr = ptr->next;
	}

	return NULL;
}

int self_get_path(char *out) {
	g_setjmp(0, "self_get_path", NULL, NULL);

	char path[PATH_MAX];
	int r;

	sprintf(path, "/proc/%d/exe", getpid());
	if ((r = readlink(path, out, PATH_MAX)) == -1) {
		return 2;
	}
	out[r] = 0x0;
	return 0;
}

int is_ascii_text(uint8_t in_c) {
	if (in_c >= 0x0 && in_c <= 0x7F) {
		return 0;
	}

	return 1;
}

char *replace_char(char w, char r, char *string) {
	int s_len = strlen(string), i;
	for (i = 0; i < s_len + 100 && string[i] != 0; i++) {
		if (string[i] == w) {
			string[i] = r;
		}
	}

	return string;
}

#define SSD_MAX_LINE_SIZE 	262144
#define SSD_MAX_LINE_PROC 	30000

int ssd_4macro(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "ssd_4macro", NULL, NULL);

	off_t name_sz;
	switch (type) {
	case DT_REG:
		name_sz = get_file_size(name);
		if (!name_sz) {
			break;
		}

		FILE *fh = gg_fopen(name, "r");

		if (!fh) {
			break;
		}

		char *buffer = calloc(1, SSD_MAX_LINE_SIZE);

		size_t b_len, lc = 0;
		int hit = 0, i;

		while (fgets(buffer, SSD_MAX_LINE_SIZE, fh) && lc < SSD_MAX_LINE_PROC) {
			lc++;
			b_len = strlen(buffer);
			if (b_len < 8) {
				continue;
			}

			for (i = 0; i < b_len && i < SSD_MAX_LINE_SIZE; i++) {
				if (is_ascii_text((unsigned char) buffer[i])) {
					break;
				}
			}

			if (strncmp(buffer, "#@MACRO:", 8)) {
				continue;
			}

			__si_argv0 ptr = (__si_argv0 ) arg;

			char buffer2[4096] = { 0 };
			sprintf(buffer2, "%s:", ptr->p_buf_1);

			size_t pb_l = strlen(buffer2);

			if (!strncmp(buffer2, &buffer[8], pb_l)) {
				buffer = replace_char(0xA, 0x0, buffer);
				buffer = replace_char(0xD, 0x0, buffer);
				b_len = strlen(buffer);
				size_t d_len = b_len - 8 - pb_l;
				if (d_len > sizeof(ptr->s_ret)) {
					d_len = sizeof(ptr->s_ret);
				}
				bzero(ptr->s_ret, sizeof(ptr->s_ret));
				g_strncpy(ptr->s_ret, &buffer[8 + pb_l], d_len);
				g_strncpy(ptr->p_buf_2, name, strlen(name));
				ptr->ret = d_len;
				gfl |= F_OPT_TERM_ENUM;
				break;
			}
			hit++;
		}

		g_fclose(fh);
		g_free(buffer);
		break;
	case DT_DIR:
		enum_dir(name, ssd_4macro, arg, 0);
		break;
	}

	return 0;
}

int get_file_type(char *file) {
	struct stat sb;

	if (stat(file, &sb) == -1)
		return errno;

	switch (sb.st_mode & S_IFMT) {
	case S_IFBLK:
		return DT_BLK;
	case S_IFCHR:
		return DT_CHR;
	case S_IFDIR:
		return DT_DIR;
	case S_IFIFO:
		return DT_FIFO;
	case S_IFLNK:
		return DT_LNK;
	case S_IFREG:
		return DT_REG;
	case S_IFSOCK:
		return DT_SOCK;
	default:
		return 0;
	}
}

int get_file_type_no(struct stat *sb) {
	switch (sb->st_mode & S_IFMT) {
	case S_IFBLK:
		return DT_BLK;
	case S_IFCHR:
		return DT_CHR;
	case S_IFDIR:
		return DT_DIR;
	case S_IFIFO:
		return DT_FIFO;
	case S_IFLNK:
		return DT_LNK;
	case S_IFREG:
		return DT_REG;
	case S_IFSOCK:
		return DT_SOCK;
	default:
		return 0;
	}
}

