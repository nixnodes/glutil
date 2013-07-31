/*
 * ============================================================================
 * Name        : dirupdate
 * Authors     : nymfo, siska
 * Version     : 0.9-9 RC1
 * Description : glftpd directory log manipulation tool
 * ============================================================================
 *
 * Credits     : Bsugar for the original idea, design and beta testing
 *
 */

#include "glconf.h"

/* gl root  */
#ifndef glroot
#define glroot "/glftpd"
#endif

/* site root, relative to gl root */
#ifndef siteroot
#define siteroot "/site"
#endif

/* dirlog and nukelog locations */
#ifndef dir_log
#define dir_log "/glftpd/ftp-data/logs/dirlog"
#endif

#ifndef nuke_log
#define nuke_log "/glftpd/ftp-data/logs/nukelog"
#endif

/* see README file about this */
#ifndef du_fld
#define du_fld "/glftpd/bin/dirupdate.folders"
#endif

#ifndef PREG_DIR_SKIP
#define PREG_DIR_SKIP "\\/(Sample|Covers|Subs|Cover|Proof)$"
#endif
#ifndef PREG_SFV_SKIP
#define PREG_SFV_SKIP PREG_DIR_SKIP
#endif
#ifndef PREG_SFV_SKIP_EXT
#define PREG_SFV_SKIP_EXT "\\.(nfo|sfv)$"
#endif

/* -------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>
#include <setjmp.h>

#define VER_MAJOR 0
#define VER_MINOR 9
#define VER_REVISION 9
#define VER_STR "_RC1"

#define ULLONG unsigned long long int

#if __x86_64__ || __ppc64__
#define AAINT ULLONG
#define ENV_64
char ARCH = 1;
#else
#define AAINT unsigned int
#define ENV_32
char ARCH = 0;
#endif

#if _FILE_OFFSET_BITS != 64
#define _FILE_OFFSET_BITS 64
#endif

typedef struct e_arg {
	int depth;
	unsigned int flags;
	char buffer[1024];
	struct dirlog *dirlog;
	struct nukelog *nukelog;
	time_t t_stor;
} ear;

typedef struct option_reference_array {
	char *option;
	void *function, *arg_cnt;
}*p_ora;

struct d_stats {
	ULLONG bw, br, rw;
};

typedef struct mda_object {
	void *ptr, *next, *prev;
	unsigned char flags;
}*p_md_obj, md_obj;

#define F_MDA_REFPTR		0x1
#define F_MDA_UNLINK_FREE	0x2

typedef struct mda_header {
	p_md_obj objects, pos, r_pos;
	off_t offset, r_offset, count;
	unsigned int flags;
	void *lref_ptr;
} mda, *pmda;

struct g_handle {
	FILE *fh;
	off_t offset, bw, br, total_sz;
	off_t rw;
	unsigned int block_sz, flags;
	mda buffer, w_buffer;
	void *data;
	size_t buffer_count;
	void *last;
	char s_buffer[4096], file[4096], mode[32];
};

typedef struct sig_jmp_buf {
	sigjmp_buf env, p_env;
	unsigned int flags, pflags;
	int id, pid;
	unsigned char ci, pci;
	char type[32];
	void *callback, *arg;
} sigjmp, *p_sigjmp;

#define BYTE unsigned char
#define uLong unsigned int

#define UPDC32(octet,crc) (crc_32_tab[((crc)\
     ^ ((BYTE)octet)) & 0xff] ^ ((crc) >> 8))


static uLong crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uLong crc32buf(uLong crc32, BYTE *buf, size_t len)
{
    register uLong oldcrc32;

    if ( crc32 ) {
    	oldcrc32 = crc32;
    }
    else {
    	oldcrc32 = 0xFFFFFFFF;
    }

    for ( ; len; len--, buf++)
    {
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
#define F_OPT_BUFFER		0x8000

#define F_MD_NOREAD			0x1

#define F_DL_FOPEN_BUFFER	0x1
#define F_DL_FOPEN_FILE		0x2
#define F_DL_FOPEN_REWIND	0x4

#define F_EAR_NOVERB		0x1

#define F_GH_NOMEM  		0x1
#define F_GH_ISDIRLOG		0x2
#define F_GH_EXEC			0x4
#define F_GH_ISNUKELOG		0x8
#define F_GH_FFBUFFER		0x10

#define F_EARG_SFV 			0x1

#define V_MB				0x100000

#define DL_SZ (int)sizeof(struct dirlog)
#define NL_SZ (int)sizeof(struct nukelog)

#define CRC_FILE_READ_BUFFER_SIZE 26214400
#define	DB_MAX_SIZE 536870912   /* max file size allowed to load into memory */

#define MAX_ULLONG 0xFFFFFFFFFFFFFFFF
#define MAX_ULONG 0xFFFFFFFF
#define	PIPE_READ_MAX	0x2000

#define MSG_GEN_NODFILE "ERROR: could not open dirlog [%s]\n"
#define MSG_GEN_DFWRITE "ERROR: %s: [%d] [%llu] writing record to dirlog failed! (mode: %s)\n"
#define MSG_GEN_DFCORRU "ERROR: %s: corrupt data file detected! (data file size [%llu] is not a multiple of block size [%d])\n"
#define MSG_GEN_DFRFAIL "ERROR: [%s] rebuilding data file failed!\n"

#define F_SIGERR_CONTINUE 0x1  /* continue after exception */

#define ID_SIGERR_UNSPEC 0x0
#define ID_SIGERR_MEMCPY 0x1
#define ID_SIGERR_STRCPY 0x2
#define ID_SIGERR_FREE 0x3
#define ID_SIGERR_FREAD 0x4
#define ID_SIGERR_FWRITE 0x5
#define ID_SIGERR_FOPEN 0x6
#define ID_SIGERR_FCLOSE 0x7
#define ID_SIGERR_MEMMOVE 0x8

sigjmp g_sigjmp = { { { { 0 } } } };

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

void g_setjmp(unsigned int flags, char *type, void *callback, void *arg) {
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

void sighdl_error(int sig, siginfo_t* siginfo, void* context) {

	char *s_ptr1 = "EXCEPTION", *s_ptr2 = "(unknown)", *s_ptr3 = "";
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
			(ULLONG) (AAINT) siginfo->si_addr);

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

	printf("%s: [%s] [%s]%s%s\n", s_ptr1, g_sigjmp.type, s_ptr2, buffer1,
			s_ptr3);

	usleep(250000);

	g_sigjmp.ci++;

	if (g_sigjmp.flags & F_SIGERR_CONTINUE) {
		siglongjmp(g_sigjmp.env, 0);
	}

	g_sigjmp.ci = 0;
	g_sigjmp.flags = 0;

	exit(0);
}

/* ---------------------------------------------------------------------------------- */

struct g_handle actdl = { 0 };
struct g_handle actnl = { 0 };
struct d_stats dl_stats = { 0 };
struct d_stats nl_stats = { 0 };

mda dirlog_buffer = { 0 };
mda nukelog_buffer = { 0 };

unsigned int gfl = 0x0;
int updmode = 0;
char **argv_off = NULL;
char GLROOT[255] = { glroot };
char SITEROOT_N[255] = { siteroot };
char SITEROOT[255] = { 0 };
char DIRLOG[255] = { dir_log };
char NUKELOG[255] = { nuke_log };
char DU_FLD[255] = { du_fld };
long long int db_max_size = DB_MAX_SIZE;
int glob_regex_flags = 0;
char GLOB_REGEX[4096] = { 0 };

#define MAX_EXEC_STR 0x200000

char *exec_str = NULL;

char b_glob[MAX_EXEC_STR] = { 0 };

int glob_reg_i_m = 0;

char *hpd_up =
		"glFTPd dirlog tool, version %d.%d-%d%s-%s\n"
				"\n"
				"Main:\n"
				"  -s <folders>          Import specific directories. Use quotation marks with multiple arguments\n"
				"                           <folders> are passed relative to SITEROOT, separated by space\n"
				"                           use -f to overwrite existing entries\n"
				"  -r [-u]               Rebuild dirlog for an entire section, reads sections from '.folders' file\n"
				"                           -u only imports new records and does not truncate existing dirlog\n"
				"                           use -f to ignore .folders and do a full recursive scan instead\n"
				"  -d, --dump [--raw]    Print contents of entire dirlog in readable form to stdout (-vv prints dir nuke status)\n"
				"  -n, --nukedump        Print contents of entire nukelog in readable form to stdout\n"
				"  -c, --check [--fix]   Compare dirlog and filesystem records and warn on differences\n"
				"                           --fix attempts to correct dirlog\n"
				"                           folder creation dates are ignored unless -f is given\n"
				"  -p, --dupechk         Look for duplicate records within dirlog and print to stdout\n"
				"  -e <dirlog|nukelog>   Rebuilds existing dirlog/nukelog, based on filtering rules (see --exec\n"
				"                           and --regex)\n"
				"\n"
				"Options:\n"
				"  -f                    Force operation where it applies\n"
				"  -v                    Increase verbosity level (use -vv or more for greater effect)\n"
				"  -k, --nowrite         Perform a dry run, executing normally except no writing is done\n"
				"  -b, --nobuffer        Disable dirlog/nukelog memory buffering\n"
				"  --wbuffer             Explicit memory pre-caching during filesystem based rebuild\n"
				"  --memlimit=<bytes>    Maximum file size that can be pre-buffered into memory\n"
				"  --sfv                 Generate a new SFV files inside folders (works with both -s and -r)\n"
				"  --exec <..command..{[base]dir}..{user}..{group..{size}..{files}..{created}..>\n"
				"                         While parsing dirlog/filesystem, execute command for each record\n"
				"                            can be used used with -d, -r, -e, and -c,\n"
				"                            operators {..} are overwritten with dirlog values\n"
				"  -y, --followlinks     Follows symbolic links (default is to skip)\n"
				"  --regex <match>       Regex match filter string, used during various operations.\n"
				"                           can be used with -r, -e -p, -d and -n\n"
				"  --regexi <match>      Case insensitive variant of --regex\n"
				"  --iregex <match>      Same as --regex with inverted match\n"
				"  --iregexi <match>     Same as --regexi with inverted match\n"
				"  --batch               Prints dirlog data non-formatted\n"
				"  --version             Print version and exit\n"
				"\n"
				"Directory and file:\n"
				"  --glroot=PATH         Override glFTPd root path\n"
				"  --siteroot=PATH       Override site root path (relative to glFTPd root)\n"
				"  --dirlog=FILE         Override path to dirlog file\n"
				"  --nukelog=FILE        Override path to nukelog file\n"
				"  --folders=FILE        Override path to folders file (contains sections and depths,\n"
				"                           used on recursive imports)\n"
				"\n";

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

int opt_g_nowrite(void *arg, int m) {
	gfl |= F_OPT_NOWRITE;
	return 0;
}

int opt_g_nobuffering(void *arg, int m) {
	gfl |= F_OPT_NOBUFFER;
	return 0;
}

int opt_g_buffering(void *arg, int m) {
	gfl |= F_OPT_BUFFER;
	return 0;
}

int opt_g_followlinks(void *arg, int m) {
	gfl |= F_OPT_FOLLOW_LINKS;
	return 0;
}

int opt_update_single_record(void *arg, int m) {
	argv_off = (char**) arg;
	updmode = UPD_MODE_SINGLE;
	return 0;
}

int opt_recursive_update_records(void *arg, int m) {
	updmode = UPD_MODE_RECURSIVE;
	return 0;
}

int opt_raw_dump(void *arg, int m) {
	gfl = F_OPT_MODE_RAWDUMP;
	return 0;
}

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

int opt_exec(void *arg, int m) {
	char *buffer;
	if (m == 2) {
		buffer = (char *) arg;
	} else {
		buffer = ((char **) arg)[0];
	}
	size_t blen = strlen(buffer);

	if (!blen || blen + 1 > MAX_EXEC_STR) {
		return 1;
	}

	blen++;
	exec_str = calloc(blen, 1);
	g_strncpy(exec_str, buffer, blen);
	return 0;
}

int opt_glroot(void *arg, int m) {
	g_cpg(arg, GLROOT, m, 255);
	if (gfl & F_OPT_VERBOSE) {
		printf("NOTE: GLROOT path set to '%s'\n", GLROOT);
	}
	return 0;
}

int opt_siteroot(void *arg, int m) {
	g_cpg(arg, SITEROOT_N, m, 255);
	if (gfl & F_OPT_VERBOSE) {
		printf("NOTE: SITEROOT path set to '%s'\n", SITEROOT_N);
	}
	return 0;
}

void *p_argv_off = NULL;

int opt_rebuild(void *arg, int m) {
	if (m == 2) {
		p_argv_off = arg;
	} else {
		p_argv_off = (void*) ((char **) arg)[0];
	}
	updmode = UPD_MODE_REBUILD;
	return 0;
}

int opt_dirlog_file(void *arg, int m) {
	g_cpg(arg, DIRLOG, m, 255);
	if (gfl & F_OPT_VERBOSE) {
		printf("NOTE: dirlog file set to '%s'\n", DIRLOG);
	}
	return 0;
}

int opt_g_regexi(void *arg, int m) {
	g_cpg(arg, GLOB_REGEX, m, 4096);
	glob_regex_flags |= REG_ICASE;
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
	if (gfl & F_OPT_VERBOSE) {
		printf("NOTE: nukelog file set to '%s'\n", NUKELOG);
	}
	return 0;
}

int opt_dirlog_sections_file(void *arg, int m) {
	g_cpg(arg, DU_FLD, m, 255);
	if (gfl & F_OPT_VERBOSE) {
		printf("NOTE: sections file set to '%s'\n", DU_FLD);
	}
	return 0;
}

int print_version(void *arg, int m) {
	printf("dirupdate-%d.%d-%d%s-%s\n", VER_MAJOR, VER_MINOR,
	VER_REVISION, VER_STR, ARCH ? "x86_64" : "i686");
	return 0;
}

int opt_dirlog_check(void *arg, int m) {
	updmode = UPD_MODE_CHECK;
	return 0;
}

int opt_dirlog_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP;
	return 0;
}

int opt_dirlog_dump_nukelog(void *arg, int m) {
	updmode = UPD_MODE_DUMP_NUKE;
	return 0;
}

int print_help(void *arg, int m) {
	printf(hpd_up, VER_MAJOR, VER_MINOR, VER_REVISION,
	VER_STR, ARCH ? "x86_64" : "i686");
	return 0;
}

int opt_dirlog_chk_dupe(void *arg, int m) {
	updmode = UPD_MODE_DUPE_CHK;
	return 0;
}

int opt_membuffer_limit(void *arg, int m) {
	char *buffer;
	if (m == 2) {
		buffer = (char *) arg;
	} else {
		buffer = ((char **) arg)[0];
	}
	if (!buffer)
		return 1;
	long long int l_buffer = atoll(buffer);
	if (l_buffer > 1024) {
		db_max_size = l_buffer;
		printf("NOTE: max memory buffer limit set to %lld bytes\n", l_buffer);
	} else {
		printf(
				"NOTE: invalid memory buffer limit, using default (%lld bytes)\n",
				db_max_size);
	}
	return 0;
}

ULLONG file_crc32(char *file, uLong *crc_out);
int rebuild_dirlog(void);
int data_backup_records(char*);
ssize_t file_copy(char *source, char *dest, char *mode);
int dirlog_check_records(void);
int dirlog_print_stats(void);
int dirlog_format_block(char *name, ear *iarg, char *output);
int proc_section(char *name, unsigned char type, void *arg);
int proc_release(char *name, unsigned char type, void *arg);
int split_string(char *line, char dl, pmda output_t);
int release_generate_block(char *name, ear *iarg);
long get_file_size(char *file);
time_t get_file_creation_time(struct stat *st);
int dirlog_write_record(struct dirlog *buffer, off_t offset, int whence);
ULLONG dirlog_find(char *dirname, int mode, unsigned int flags, void *callback);
int join_string(char *input[], char *output, int size, int count, char del);
int enum_dir(char *dir, void *cb, void *arg, int f);
int file_exists(char *file);
int update_records(char *dirname, int depth);
off_t read_file(char *file, unsigned char *buffer, size_t read_max,
		off_t offset);
int option_crc32(void *arg);
int write_file_text(char *data, char *file);
int reg_match(char *expression, char *match, int flags);
int delete_file(char *name, unsigned char type, void *arg);
int get_file_type(char *file);
int nukelog_format_block(char *name, ear *iarg, char *output);
ULLONG nukelog_find(char *dirname, int mode, struct nukelog *output1);
int parse_args(int argc, char *argv[]);
int get_relative_path(char *subject, char *root, char *output);
int process_opt(char *opt, void *arg, void *reference_array, int m);
int nukelog_print_stats(void);
int dir_exists(char *dir);
int dirlog_update_record(char **argv);
int dirlog_check_dupe(void);
int gh_rewind(struct g_handle *hdl);
int g_fopen(char *file, char *mode, unsigned int flags, struct g_handle *hdl,
		size_t size);
int g_close(struct g_handle *hdl);
void *g_read(void *buffer, struct g_handle *hdl, size_t size);
int process_exec_string(char *, char *, struct dirlog *);
int g_do_exec(void *, struct g_handle *);
size_t exec_and_wait_for_output(char*, char*);
void sig_handler(int);
void child_sig_handler(int, siginfo_t*, void*);
int flush_data_md(struct g_handle *hdl, char *outfile);
int rebuild(void *arg);
int rebuild_data_file(char *file, struct g_handle *hdl);
int g_bmatch(void *d_ptr, struct g_handle *hdl);
size_t g_load_data(FILE *fh, void *output, size_t max);
int g_load_record(struct g_handle *hdl, const void *data);
int remove_repeating_chars(char *string, char c);

void *f_ref[] = { "--wbuffer", opt_g_buffering, (void*) 0, "--raw",
		opt_raw_dump, (void*) 0, "--iregexi", opt_g_iregexi, (void*) 1,
		"--iregex", opt_g_iregex, (void*) 1, "--regexi", opt_g_regexi,
		(void*) 1, "--regex", opt_g_regex, (void*) 1, "-e", opt_rebuild,
		(void*) 1, "--batch", opt_batch_output_formatting, (void*) 0, "-y",
		opt_g_followlinks, (void*) 0, "--allowsymbolic", opt_g_followlinks,
		(void*) 0, "--followlinks", opt_g_followlinks, (void*) 0,
		"--allowlinks", opt_g_followlinks, (void*) 0, "--exec", opt_exec,
		(void*) 1, "--fix", opt_g_fix, (void*) 0, "-u", opt_g_update, (void*) 0,
		"--memlimit", opt_membuffer_limit, (void*) 1, "-p", opt_dirlog_chk_dupe,
		(void*) 0, "--dupechk", opt_dirlog_chk_dupe, (void*) 0, "-b",
		opt_g_nobuffering, (void*) 0, "--nobuffer", opt_g_nobuffering,
		(void*) 0, "--nukedump", opt_dirlog_dump_nukelog, (void*) 0, "-n",
		opt_dirlog_dump_nukelog, (void*) 0, "--help", print_help, (void*) 0,
		"--version", print_version, (void*) 0, "--folders",
		opt_dirlog_sections_file, (void*) 1, "--dirlog", opt_dirlog_file,
		(void*) 1, "--nukelog", opt_nukelog_file, (void*) 1, "--siteroot",
		opt_siteroot, (void*) 1, "--glroot", opt_glroot, (void*) 1, "-k",
		opt_g_nowrite, (void*) 0, "--nowrite", opt_g_nowrite, (void*) 0,
		"--sfv", opt_g_sfv, (void*) 0, "--crc32", option_crc32, (void*) 1,
		"--backup", NULL, (void*) 1, "-c", opt_dirlog_check, (void*) 0,
		"--check", opt_dirlog_check, (void*) 0, "--dump", opt_dirlog_dump,
		(void*) 0, "-d", opt_dirlog_dump, (void*) 0, "-vvvv", opt_g_verbose4,
		(void*) 0, "-vvv", opt_g_verbose3, (void*) 0, "-vv", opt_g_verbose2,
		(void*) 0, "-v", opt_g_verbose, (void*) 0, "-f", opt_g_force, (void*) 0,
		"-s", opt_update_single_record, (void*) 1, "-r",
		opt_recursive_update_records, (void*) 0, NULL };

int md_init(pmda md, int nm) {
	if (md->objects)
		return 1;
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
		p_md_obj ptr = md->objects, ptr_s;
		while (ptr) {
			ptr_s = ptr->next;
			g_free(ptr->ptr);
			ptr = ptr_s;
		}
	}

	g_free(md->objects);
	bzero(md, sizeof(mda));

	return 0;
}

AAINT md_relink(pmda md) {
	AAINT off, l = 0;

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

#define MDA_MDALLOC_RE	0x1

void *md_alloc(pmda md, int b) {
	int flags = 0;

	if (md->offset >= md->count) {
		if (gfl & F_OPT_VERBOSE3) {
			printf(
					"NOTE: re-allocating memory segment to increase size; current address: 0x%.16llX, current size: %llu\n",
					(ULLONG) (AAINT) md->objects, (ULLONG) md->count);
		}
		md->objects = realloc(md->objects, (md->count * sizeof(md_obj)) * 2);
		md->pos = md->objects;
		md->pos += md->count;
		bzero(md->pos, md->count * sizeof(md_obj));

		md->count *= 2;
		AAINT rlc = md_relink(md);
		flags |= MDA_MDALLOC_RE;
		if (gfl & F_OPT_VERBOSE3) {
			printf(
					"NOTE: re-allocation done; new address: 0x%.16llX, new size: %llu, re-linked %llu records\n",
					(ULLONG) (AAINT) md->objects, (ULLONG) md->count,
					(ULLONG) rlc);
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
	p_md_obj c_ptr = NULL;

	if (md_o->prev) {
		((p_md_obj) md_o->prev)->next = (p_md_obj) md_o->next;
		c_ptr = md_o->prev;
	}

	if (md_o->next) {
		((p_md_obj) md_o->next)->prev = (p_md_obj) md_o->prev;
		c_ptr = md_o->next;
	}

	if (c_ptr) {
		md->offset--;
		md->pos = c_ptr;
		if (!(md->flags & F_MDA_REFPTR) && md_o->ptr) {
			g_free(md_o->ptr);
			md_o->ptr = NULL;
		}
	}

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
	md_g_free(&actdl.buffer);
	md_g_free(&actdl.w_buffer);
	md_g_free(&actnl.buffer);
	md_g_free(&actnl.w_buffer);
	exit(0);
}

int main(int argc, char *argv[]) {

	if (setup_sighandlers()) {
		printf(
				"WARNING: UNABLE TO SETUP SIGNAL HANDLERS! (this is weird, please report it!)\n");
		sleep(5);
	}

	g_setjmp(0, "main", NULL, NULL);

	//sigsetjmp(g_sigjmp.env, 1);

	int r = parse_args(argc, argv);
	if (r == -2 || r == -1) {
		print_help(NULL, 0);
		return 4;
	}

	bzero(SITEROOT, 255);
	sprintf(SITEROOT, "%s%s", GLROOT, SITEROOT_N);

	if (!strlen(GLROOT)) {
		printf("ERROR: glftpd root directory not specified!\n");
		return 2;
	}

	if (!strlen(SITEROOT_N)) {
		printf("ERROR: glftpd site root directory not specified!\n");
		return 2;
	}

	if (strlen(GLOB_REGEX)) {
		gfl |= F_OPT_HAS_G_REGEX;
	}

	if (gfl & F_OPT_VERBOSE && !(gfl & F_OPT_MODE_RAWDUMP)) {
		if (gfl & F_OPT_NOBUFFER)
			printf("NOTE: disabling memory buffering\n");
	}

	actdl.flags |= F_GH_ISDIRLOG;
	actnl.flags |= F_GH_ISNUKELOG;

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
		nukelog_print_stats();
		break;
	case UPD_MODE_DUPE_CHK:
		dirlog_check_dupe();
		break;
	case UPD_MODE_REBUILD:
		rebuild(p_argv_off);
		break;
	}

	g_shutdown(NULL);

	exit(0);
}

int rebuild(void *arg) {
	if (!arg) {
		printf("ERROR: missing data type argument (-e <dirlog|nukelog>)\n");
		return 1;
	}

	char *a_ptr = (char*) arg;

	if (!strncmp(a_ptr, "dirlog", 6)) {
		if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &actdl, DL_SZ)) {
			printf(MSG_GEN_NODFILE, DIRLOG);
			return 3;
		}

		if (rebuild_data_file(DIRLOG, &actdl)) {
			printf(MSG_GEN_DFRFAIL, DIRLOG);
			return 3;
		}

		printf("STATS: %s: wrote %llu bytes in %llu records\n", DIRLOG,
				(ULLONG) actdl.bw, (ULLONG) actdl.rw);
	} else {
		printf("ERROR: [%s] unrecognized data type\n", a_ptr);
		return 2;
	}
	return 0;
}

int dirlog_check_dupe(void) {
	g_setjmp(0, "dirlog_check_dupe", NULL, NULL);
	struct dirlog buffer, buffer2;
	struct dirlog *d_ptr = NULL, *dd_ptr = NULL;
	char *s_buffer, *ss_buffer, *s_pb, *ss_pb;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &actdl, DL_SZ)) {
		printf(MSG_GEN_NODFILE, DIRLOG);
		return 2;
	}
	off_t st1, st2;
	p_md_obj pmd_st1 = NULL, pmd_st2 = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &actdl, DL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_dupe(loop)", NULL, NULL);
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			if (actdl.buffer.pos && actdl.buffer.pos->flags & F_MD_NOREAD) {
				continue;
			}
			if (g_bmatch(d_ptr, &actdl)) {
				continue;
			}
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_dupe(loop)(2)", NULL,
			NULL);
			s_buffer = strdup(d_ptr->dirname);
			s_pb = basename(s_buffer);
			size_t s_pb_l = strlen(s_pb);
			if (s_pb_l < 4) {
				goto end_loop1;
			}
			st1 = actdl.offset;
			actdl.offset = 0;
			if (actdl.buffer_count) {
				st2 = actdl.buffer_count;
				pmd_st1 = actdl.buffer.r_pos;
				pmd_st2 = actdl.buffer.pos;
			} else {
				st2 = (off_t) ftello(actdl.fh);
			}
			gh_rewind(&actdl);
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_dupe(loop)(3)", NULL,
			NULL);
			int ch = 0;
			while ((dd_ptr = (struct dirlog *) g_read(&buffer2, &actdl, DL_SZ))) {
				if (gfl & F_OPT_KILL_GLOBAL) {
					break;
				}
				ss_buffer = strdup(dd_ptr->dirname);
				ss_pb = basename(ss_buffer);
				size_t ss_pb_l = strlen(ss_pb);

				if (ss_pb_l == s_pb_l && !strncmp(s_pb, ss_pb, s_pb_l)
						&& strncmp(d_ptr->dirname, dd_ptr->dirname,
								strlen(d_ptr->dirname))) {
					if (actdl.buffer_count && actdl.buffer.pos) {
						actdl.buffer.pos->flags |= F_MD_NOREAD;
					}
					if (!ch) {
						printf("DUPE %s\n", d_ptr->dirname);
					}
					printf("DUPE %s\n", dd_ptr->dirname);
					ch++;
				}
				g_free(ss_buffer);
			}

			actdl.offset = st1;
			if (actdl.buffer_count) {
				actdl.buffer.offset = st2;
				actdl.buffer.r_pos = pmd_st1;
				actdl.buffer.pos = pmd_st2;
			} else {
				fseeko(actdl.fh, (off_t) st2, SEEK_SET);
			}
			end_loop1:

			g_free(s_buffer);
		}

	}
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

int dirlog_update_record(char **argv) {
	g_setjmp(0, "dirlog_update_record", NULL, NULL);
	if (!argv || !argv[0])
		return 1;

	int r, seek = SEEK_END, ret = 0, dr;
	off_t offset = 0;
	ULLONG rl = MAX_ULLONG;
	struct dirlog dl = { 0 };
	ear arg = { 0 };
	arg.dirlog = &dl;

	mda dirchain = { 0 };
	p_md_obj ptr;

	md_init(&dirchain, 1024);

	if ((r = split_string(argv[0], 0x20, &dirchain)) < 1) {
		printf("ERROR: [dirlog_update_record]: missing arguments\n");
		ret = 1;
		goto r_end;
	}

	data_backup_records(DIRLOG);

	char s_buffer[2048];
	ptr = dirchain.objects;
	while (ptr) {
		sprintf(s_buffer, "%s/%s", SITEROOT, (char*) ptr->ptr);
		remove_repeating_chars(s_buffer, 0x2F);

		rl = dirlog_find(s_buffer, 0, 0, NULL);

		char *mode = "a";

		if (!(gfl & F_OPT_FORCE) && rl < MAX_ULLONG) {
			printf(
					"WARNING: %s: [%llu] already exists in dirlog (use -f to overwrite)\n",
					(char*) ptr->ptr, rl);
			ret = 4;
			goto end;
		} else if (rl < MAX_ULLONG) {
			if (gfl & F_OPT_VERBOSE) {
				printf(
						"WARNING: %s: [%llu] overwriting existing dirlog record\n",
						(char*) ptr->ptr, rl);
			}
			offset = rl;
			seek = SEEK_SET;
			mode = "r+";
		}

		if (g_fopen(DIRLOG, mode, 0, &actdl, DL_SZ)) {
			printf(MSG_GEN_NODFILE, mode);
			goto end;
		}

		if ((r = release_generate_block(s_buffer, &arg))) {
			if (r < 5) {
				printf("ERROR: %s: [%d] generating dirlog data chunk failed\n",
						(char*) ptr->ptr, r);
			}
			ret = 3;
			goto end;
		}

		if ((dr = dirlog_write_record(arg.dirlog, offset, seek))) {
			printf(
			MSG_GEN_DFWRITE, (char*) ptr->ptr, dr, (ULLONG) offset, mode);
			ret = 6;
			goto end;
		}

		char buffer[2048] = { 0 };

		if (dirlog_format_block((char*) ptr->ptr, &arg, buffer) > 0)
			printf(buffer);

		end:

		g_close(&actdl);
		ptr = ptr->next;
	}
	r_end: md_g_free(&dirchain);

	printf("DIRLOG: wrote %llu bytes in %llu records\n", dl_stats.bw,
			dl_stats.rw);

	return ret;
}

int option_crc32(void *arg) {
	g_setjmp(0, "option_crc32", NULL, NULL);
	char **argv = (char**) arg;
	uLong crc32;

	if (!argv[0])
		return 1;

	ULLONG read = file_crc32(argv[0], &crc32);

	if (read)
		printf("%s: CRC32: %.8X | length: %llu bytes\n", argv[0],
				(unsigned int) crc32, read);
	else
		printf("ERROR: %s: [%d] could not get CRC32\n", argv[0], errno);

	return 0;
}

int data_backup_records(char *file) {
	g_setjmp(0, "data_backup_records", NULL, NULL);
	int r;

	if (gfl & F_OPT_NOWRITE)
		return 0;

	char buffer[255];

	sprintf(buffer, "%s.bk", file);

	if (gfl & F_OPT_VERBOSE2) {
		printf("NOTE: %s: creating data backup: %s ..\n", file, buffer);
	}

	if ((r = file_copy(file, buffer, "w")) < 1) {
		printf("ERROR: %s: [%d] failed to create backup %s\n", file, r, buffer);
		return r;
	}
	if (gfl & F_OPT_VERBOSE) {
		printf("NOTE: %s: created data backup: %s\n", file, buffer);
	}
	return 0;
}

int dirlog_check_records(void) {
	g_setjmp(0, "dirlog_check_records", NULL, NULL);
	struct dirlog buffer, buffer4;
	ear buffer3 = { 0 };
	char s_buffer[255];
	buffer3.dirlog = &buffer4;
	int r = 0, r2;
	char *mode = "r";
	unsigned int flags = 0;
	off_t dsz;

	if ((dsz = get_file_size(DIRLOG)) % DL_SZ) {
		printf(MSG_GEN_DFCORRU, DIRLOG, (ULLONG) dsz, DL_SZ);
		printf("NOTE: use -r to rebuild (see --help)\n");
		return -1;
	}

	if (gfl & F_OPT_FIX) {
		/*mode = "r+";
		 flags = F_DL_FOPEN_FILE;*/
		data_backup_records(DIRLOG);
	}

	if (g_fopen(DIRLOG, mode, F_DL_FOPEN_BUFFER | flags, &actdl, DL_SZ)) {
		return 2;
	}

	if (!actdl.buffer_count && gfl & F_OPT_FIX) {
		printf(
				"ERROR: internal buffering must be enabled when fixing, increase limit with --memlimit (see --help)\n");
	}

	struct dirlog *d_ptr = NULL;
	int ir;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &actdl, DL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_records(loop)", NULL,
			NULL);
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			sprintf(s_buffer, "%s%s", GLROOT, d_ptr->dirname);
			g_do_exec(d_ptr, &actdl);
			struct nukelog n_buffer;
			ir = r;
			if (d_ptr->status == 1 || d_ptr->status == 2) {
				if (nukelog_find(d_ptr->dirname, 2, &n_buffer) == MAX_ULLONG) {
					printf(
							"WARNING: %s: was marked as '%sNUKED' in dirlog but not found in nukelog\n",
							s_buffer, d_ptr->status == 2 ? "UN" : "");
				} else {
					if ((d_ptr->status == 1 && n_buffer.status != 0)
							|| (d_ptr->status == 2 && n_buffer.status != 1)
							|| (d_ptr->status == 0)) {
						printf(
								"WARNING: %s: MISMATCH: was marked as '%sNUKED' in dirlog, but nukelog reads '%sNUKED'\n",
								s_buffer, d_ptr->status == 2 ? "UN" : "",
								n_buffer.status == 1 ? "UN" : "");
					}
				}
				continue;
			}
			buffer3.flags |= F_EAR_NOVERB;

			if (dir_exists(s_buffer)) {
				printf(
						"WARNING: %s: listed in dirlog but does not exist in filesystem\n",
						s_buffer);
				if (gfl & F_OPT_FIX) {
					if (!md_unlink(&actdl.buffer, actdl.buffer.pos)) {
						printf("ERROR: %s: unlinking ghost record failed\n",
								s_buffer);
					}
					r++;
				}
				continue;
			}

			if ((r2 = release_generate_block(s_buffer, &buffer3))) {
				if (r2 == 5) {
					if (gfl & F_OPT_FIX) {
						if (remove(s_buffer)) {
							printf(
									"WARNING: %s: failed removing empty directory\n",
									s_buffer);

						} else {
							if (gfl & F_OPT_VERBOSE) {
								printf("FIX: %s: removed empty directory\n",
										s_buffer);
							}
						}
					}
				} else {
					printf(
							"WARNING: [%s] - could not get directory information from the filesystem\n",
							s_buffer);
				}
				r++;
				continue;
			}
			if (d_ptr->files != buffer4.files) {
				printf(
						"WARNING: [%s] file counts in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->files, buffer4.files);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->files = buffer4.files;
				}
			}

			if (d_ptr->bytes != buffer4.bytes) {
				printf(
						"WARNING: [%s] directory sizes in dirlog and on disk do not match ( dirlog: %llu , filesystem: %llu )\n",
						d_ptr->dirname, (ULLONG) d_ptr->bytes,
						(ULLONG) buffer4.bytes);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->bytes = buffer4.bytes;
				}
			}

			if (d_ptr->group != buffer4.group) {
				printf(
						"WARNING: [%s] group ids in dirlog and on disk do not match (dirlog:%hu filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->group, buffer4.group);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->group = buffer4.group;
				}
			}

			if (d_ptr->uploader != buffer4.uploader) {
				printf(
						"WARNING: [%s] user ids in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->uploader, buffer4.uploader);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->uploader = buffer4.uploader;
				}
			}

			if (gfl & F_OPT_FORCE && d_ptr->uptime != buffer4.uptime) {
				printf(
						"WARNING: [%s] folder creation dates in dirlog and on disk do not match (dirlog:%u, filesystem:%u)\n",
						d_ptr->dirname, d_ptr->uptime, buffer4.uptime);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->uptime = buffer4.uptime;
				}
			}
			if (r == ir) {
				if (gfl & F_OPT_VERBOSE2) {
					printf("OK: %s\n", d_ptr->dirname);
				}
			} else {
				if (gfl & F_OPT_VERBOSE2) {
					printf("BAD: %s\n", d_ptr->dirname);
				}

			}
		}

	}

	if (!(gfl & F_OPT_KILL_GLOBAL) && gfl & F_OPT_FIX && r) {
		if (rebuild_data_file(DIRLOG, &actdl)) {
			printf(MSG_GEN_DFRFAIL, DIRLOG);
		}
	}

	g_close(&actdl);

	return r;
}

int g_bmatch(void *d_ptr, struct g_handle *hdl) {
	g_setjmp(0, "g_bmatch", NULL, NULL);
	int r_e = g_do_exec(d_ptr, hdl);

	if (exec_str && WEXITSTATUS(r_e)) {
		if (gfl & F_OPT_VERBOSE && !(gfl & F_OPT_MODE_RAWDUMP)) {
			printf(
					"WARNING: [%d] external call returned non-zero, ignoring this record\n",
					WEXITSTATUS(r_e));
		}
		return 1;
	}

	if (gfl & F_OPT_HAS_G_REGEX && hdl->flags & F_GH_ISDIRLOG
			&& reg_match(GLOB_REGEX, ((struct dirlog*) d_ptr)->dirname,
					glob_regex_flags) == glob_reg_i_m) {
		if (gfl & F_OPT_VERBOSE && !(gfl & F_OPT_MODE_RAWDUMP)) {

			printf("WARNING: %s: regex match positive, ignoring this record\n",
					((struct dirlog*) d_ptr)->dirname);
		}
		return 2;
	}
	return 0;
}

int dirlog_print_stats(void) {
	g_setjmp(0, "dirlog_print_stats", NULL, NULL);
	struct dirlog buffer;
	char sbuffer[2048];
	ear e = { 0 };
	int c = 0;

	if (g_fopen(DIRLOG, "r", 0, &actdl, DL_SZ))
		return 2;

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &actdl, DL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_print_stats(loop)", NULL, NULL);

			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			if (g_bmatch(d_ptr, &actdl)) {
				continue;
			}

			c++;
			e.dirlog = d_ptr;
			if (gfl & F_OPT_MODE_RAWDUMP) {
				g_fwrite((void*) d_ptr, DL_SZ, 1, stdout);
			} else {
				if (dirlog_format_block(d_ptr->dirname, &e, sbuffer) > 0) {
					printf(sbuffer);
				}
			}

			if (gfl & F_OPT_VERBOSE2 && !(gfl & F_OPT_MODE_RAWDUMP)) {
				struct nukelog n_buffer = { 0 };
				if (nukelog_find(d_ptr->dirname, 2, &n_buffer) < MAX_ULLONG) {
					e.nukelog = &n_buffer;
					bzero(sbuffer, 2048);
					if (nukelog_format_block(d_ptr->dirname, &e, sbuffer) > 0) {
						printf(sbuffer);
					}
				}
			}

		}
	}

	g_setjmp(0, "dirlog_print_stats(2)", NULL, NULL);

	if (!(gfl & F_OPT_FORMAT_BATCH) && !(gfl & F_OPT_MODE_RAWDUMP)) {
		printf("STATS: %s: read %d records\n", DIRLOG, c);
	}

	g_close(&actdl);

	return 0;
}

int nukelog_print_stats(void) {
	struct nukelog buffer;
	char sbuffer[2048];
	ear e = { 0 };
	int c = 0;

	g_setjmp(0, "nukelog_print_stats", NULL, NULL);

	if (g_fopen(NUKELOG, "r", 0, &actnl, NL_SZ))
		return 2;

	struct nukelog *n_ptr = NULL;

	while ((n_ptr = (struct nukelog *) g_read(&buffer, &actnl, NL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "nukelog_print_stats(loop)", NULL,
			NULL);
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			if (g_bmatch(n_ptr, &actnl)) {
				continue;
			}

			c++;
			e.nukelog = n_ptr;
			if (gfl & F_OPT_MODE_RAWDUMP) {
				g_fwrite((void*) n_ptr, NL_SZ, 1, stdout);
			} else {
				if (nukelog_format_block(n_ptr->dirname, &e, sbuffer) > 0) {
					printf(sbuffer);
				}
			}
		}
	}

	g_setjmp(0, "nukelog_print_stats(2)", NULL, NULL);

	if (!(gfl & F_OPT_FORMAT_BATCH) && !(gfl & F_OPT_MODE_RAWDUMP)) {
		printf("STATS: %s: read %d records\n", NUKELOG, c);
	}
	g_close(&actnl);

	return 0;
}

#define 	ACT_WRITE_BUFFER_MEMBERS	10000

int rebuild_dirlog(void) {
	g_setjmp(0, "rebuild_dirlog", NULL, NULL);
	char mode[255] = { 0 };
	unsigned int flags = 0;

	if (gfl & F_OPT_NOWRITE) {
		g_strncpy(mode, "r", 1);
		flags |= F_DL_FOPEN_BUFFER;
	} else if (gfl & F_OPT_UPDATE) {
		g_strncpy(mode, "a+", 2);
		flags |= F_DL_FOPEN_BUFFER | F_DL_FOPEN_FILE;
	} else {
		g_strncpy(mode, "w+", 2);
	}

	g_strncpy(actdl.mode, mode, strlen(mode));

	if (gfl & F_OPT_BUFFER) {
		bzero(mode, 255);
		g_strncpy(mode, "r", 2);
	}

	if (gfl & F_OPT_BUFFER) {
		md_init(&actdl.w_buffer, ACT_WRITE_BUFFER_MEMBERS);
		actdl.block_sz = DL_SZ;
		actdl.flags |= F_GH_FFBUFFER;
		if (gfl & F_OPT_VERBOSE) {
			printf("NOTE: %s: explicit write pre-caching enabled\n", DIRLOG);
		}
	} else {
		data_backup_records(DIRLOG);
	}

	if (g_fopen(DIRLOG, mode, flags, &actdl, DL_SZ)) {
		printf("ERROR: could not open dirlog, mode '%s', flags %u\n", mode,
				flags);
		return errno;
	}

	if (gfl & F_OPT_FORCE) {
		printf("SCANNING: '%s'\n", SITEROOT);
		update_records(SITEROOT, 0);
		goto end;
	}

	char buffer[1024 * 1024] = { 0 };
	mda dirchain = { 0 }, buffer2 = { 0 };

	if (read_file(DU_FLD, (unsigned char*) buffer, 1024 * 1024, 0) < 1) {
		printf(
				"WARNING: unable to read folders file, doing full siteroot recursion in '%s'..\n",
				SITEROOT);
		gfl |= F_OPT_FORCE;
		update_records(SITEROOT, 0);
		goto end;
	}

	int r, r2;

	md_init(&dirchain, 1024);

	if ((r = split_string(buffer, 0x13, &dirchain)) < 1) {
		printf("ERROR: [%d] could not parse input from %s\n", r, DU_FLD);
		goto r_end;
	}

	if (gfl & F_OPT_VERBOSE3) {
		printf("NOTE: %s: allocating %u KBytes for references (overhead)\n",
				DIRLOG,
				(unsigned int) (ACT_WRITE_BUFFER_MEMBERS * sizeof(md_obj))
						/ 1024);
	}

	int i = 0, ib;
	char s_buffer[2048] = { 0 };
	p_md_obj ptr = dirchain.objects;

	while (ptr) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "rebuild_dirlog(loop)", NULL, NULL);
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			md_init(&buffer2, 6);
			i++;
			if ((r2 = split_string((char*) ptr->ptr, 0x20, &buffer2)) != 2) {
				printf("ERROR: [%d] could not parse line %d from %s\n", r2, i,
						DU_FLD);
				goto lend;
			}
			bzero(s_buffer, 2048);
			sprintf(s_buffer, "%s/%s", SITEROOT, (char*) buffer2.objects->ptr);
			remove_repeating_chars(s_buffer, 0x2F);

			ib = strtol((char*) ((p_md_obj) buffer2.objects->next)->ptr, NULL,
					10);

			if (errno == ERANGE) {
				printf("ERROR: could not get depth from line %d\n", i);
				goto lend;
			}
			if (file_exists(s_buffer)) {
				printf("ERROR: %s: directory doesn't exist (line %d)\n",
						s_buffer, i);
				goto lend;
			}
			char *ndup = strdup(s_buffer);
			char *nbase = basename(ndup);
			printf("SCANNING: '%s', depth: %d\n", nbase, ib);
			if (update_records(s_buffer, ib) < 1) {
				printf("WARNING: %s: nothing was processed\n", nbase);
			}
			g_free(ndup);
			lend: md_g_free(&buffer2);
		}
		ptr = ptr->next;
	}

	if (actdl.flags & F_GH_FFBUFFER) {
		rebuild_data_file(DIRLOG, &actdl);
	}

	r_end:

	md_g_free(&actdl.w_buffer);
	md_g_free(&dirchain);

	end:

	g_close(&actdl);

	printf("DIRLOG: wrote %llu bytes in %llu records\n", dl_stats.bw,
			dl_stats.rw);

	return 0;
}

int parse_args(int argc, char *argv[]) {
	g_setjmp(0, "parse_args", NULL, NULL);
	int i, oi, vi, ret, r, c = 0;
	void *buffer = NULL;
	char *c_arg;
	mda cmd_lt = { 0 };

	p_ora ora = (p_ora) f_ref;

	for (i = 1, ret = 0; i < argc; i++, r = 0) {
		c_arg = argv[i];
		bzero(&cmd_lt, sizeof(mda));
		md_init(&cmd_lt, 256);

		if ((r = split_string(c_arg, 0x3D, &cmd_lt)) == 2) {
			c_arg = (char*) cmd_lt.objects->ptr;
		}

		if ((vi = process_opt(c_arg, NULL, f_ref, 1)) < 0) {
			printf("CMDLINE: [%d] invalid argument '%s'\n", vi, c_arg);
			ret = -2;
			goto end;
		}

		if (r == 2) {
			ret += process_opt(c_arg, ((p_md_obj) cmd_lt.objects->next)->ptr,
					f_ref, 2);

			c++;
		} else {
			oi = i;
			AAINT vp;

			if ((vp = (AAINT) ora[vi].arg_cnt)) {
				if (i + vp > argc - 1) {
					printf("CMDLINE: '%s' missing argument parameters [%llu]\n",
							argv[i], (ULLONG) ((i + vp) - (argc - 1)));
					c = 0;
					goto end;
				}
				buffer = &argv[i + 1];
				i += vp;
			}
			ret += process_opt(argv[oi], buffer, f_ref, 0);

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
					if (proc_opt_generic)
						return proc_opt_generic(arg, m);
				} else
					return -4;
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

	if (gfl & F_OPT_VERBOSE && gfl & F_OPT_NOWRITE)
		printf("NOTE: performing dry run, no writing will be done\n");

	return enum_dir(dirname, proc_section, &arg, 0);

}

int proc_release(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "proc_release", NULL, NULL);
	ear *iarg = (ear*) arg;
	uLong crc32 = 0;
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
		if (gfl & F_OPT_SFV
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
			sprintf(buffer2, "%s %.8X\n", base, (unsigned int) crc32);
			if (!(gfl & F_OPT_NOWRITE) && !write_file_text(buffer2, buffer))
				printf("ERROR: failed writing to SFV file: '%s'\n", buffer);
			iarg->flags |= F_EARG_SFV;
			g_strncpy(iarg->buffer, buffer, strlen(buffer));
			sprintf(buffer, "  %.8X", (unsigned int) crc32);
			g_free(fn);
		}
		off_t fs = get_file_size(name);
		iarg->dirlog->bytes += fs;
		iarg->dirlog->files++;
		if (gfl & F_OPT_VERBOSE4) {
			printf("     %s  %.2fMB%s\n", base, (double) fs / 1024.0 / 1024.0,
					buffer);
		}
		g_free(fn3);
		g_free(fn2);
		break;
	case DT_DIR:
		enum_dir(name, proc_release, iarg, 0);
		break;
	}

	return 0;
}

int proc_section(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "proc_section", NULL, NULL);
	ear *iarg = (ear*) arg;
	int r;
	ULLONG rl;

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
		if (!iarg->depth || gfl & F_OPT_FORCE) {
			if (gfl & F_OPT_UPDATE) {
				if (((rl = dirlog_find(name, 1, F_DL_FOPEN_REWIND, NULL))
						< MAX_ULLONG)) {
					if (gfl & F_OPT_VERBOSE2) {
						printf(
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
					printf(
							"ERROR: %s: [%d] generating dirlog data chunk failed\n",
							name, r);
				goto end;
			}

			/*if (gfl & F_OPT_UPDATE && iarg->dirlog->status == 1) {
			 printf(
			 "WARNING: %s: refusing to import nuked directories while doing update\n",
			 name);
			 goto end;
			 }*/

			if (g_bmatch(iarg->dirlog, &actdl)) {
				goto end;
			}

			if (gfl & F_OPT_SFV && iarg->flags & F_EARG_SFV) {
				iarg->dirlog->bytes += get_file_size(iarg->buffer);
				iarg->dirlog->files++;
				printf("SFV: succesfully generated '%s'\n",
				basename(iarg->buffer));
			}

			if (actdl.flags & F_GH_FFBUFFER) {
				if (g_load_record(&actdl, (const void*) iarg->dirlog)) {
					printf(MSG_GEN_DFWRITE, iarg->dirlog->dirname, 0,
							(ULLONG) actdl.w_buffer.offset, "wbuffer");
				}
			} else {
				if ((r = dirlog_write_record(iarg->dirlog, 0, SEEK_END))) {
					printf(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
							(ULLONG) actdl.offset - 1, "w");
					goto end;
				}
			}

			char buffer[2048] = { 0 };

			if (gfl & F_OPT_VERBOSE
					&& dirlog_format_block(name, iarg, buffer) > 0) {
				printf(buffer);
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
				printf("WARNING: %s - is symbolic link, skipping..\n", name);
			}
			return 6;
		}
	}

	time_t orig_ctime = get_file_creation_time(&st);

	if (gfl & F_OPT_SFV && !(gfl & F_OPT_NOWRITE)) {
		enum_dir(name, delete_file, (void*) "\\.sfv$", 0);
	}

	if (gfl & F_OPT_VERBOSE2 && !(iarg->flags & F_EAR_NOVERB))
		printf("ENTERING: %s\n", name);

	if ((r = enum_dir(name, proc_release, iarg, 0)) < 1
			|| !iarg->dirlog->files) {
		if (gfl & F_OPT_VERBOSE) {
			printf("WARNING: %s: [%d] - empty directory\n", name, r);
		}
		ret = 5;
	}
	g_setjmp(0, "release_generate_block(2)", NULL, NULL);

	if (gfl & F_OPT_VERBOSE2 && !(iarg->flags & F_EAR_NOVERB))
		printf("EXITING: %s\n", name);

	if (gfl & F_OPT_SFV && !(gfl & F_OPT_NOWRITE)) {
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
				printf(
						"NOTE: %s: restoring original folder modification date\n",
						name);
			}
			struct utimbuf utb;
			utb.actime = 0;
			utb.modtime = orig_ctime;
			if (utime(name, &utb)) {
				printf(
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
		printf("ERROR: [%s] could not get relative to root directory name\n",
				bn);
		ret = 2;
		goto r_end;
	}

	struct nukelog n_buffer = { 0 };
	if (nukelog_find(buffer, 2, &n_buffer) < MAX_ULLONG) {
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

int dirlog_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "dirlog_format_block", NULL, NULL);
	char buffer[2048], buffer2[255];
	char *ndup = strdup(iarg->dirlog->dirname), *base = NULL;

	if (gfl & F_OPT_VERBOSE)
		base = iarg->dirlog->dirname;
	else
		base = basename(ndup);

	time_t t_t = (time_t) iarg->dirlog->uptime;

	strftime(buffer2, 255, "%a, %d %b %Y %T %z", localtime(&t_t));

	int c;

	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(buffer, "DIRLOG %s %llu %hu %u %hu %hu %hu\n", base,
				(ULLONG) iarg->dirlog->bytes, iarg->dirlog->files,
				iarg->dirlog->uptime, iarg->dirlog->uploader,
				iarg->dirlog->group, iarg->dirlog->status);
	} else {
		c =
				sprintf(buffer,
						"DIRLOG: %s - %llu Mbytes in %hu files - created %s by %hu.%hu [%hu]\n",
						base, (ULLONG) (iarg->dirlog->bytes / 1024 / 1024),
						iarg->dirlog->files, buffer2, iarg->dirlog->uploader,
						iarg->dirlog->group, iarg->dirlog->status);
	}

	g_free(ndup);

	g_memcpy(output, buffer, 2048);

	return c;
}

int nukelog_format_block(char *name, ear *iarg, char *output) {
	g_setjmp(0, "nukelog_format_block", NULL, NULL);
	char buffer[2048] = { 0 }, buffer2[255] = { 0 };
	char *ndup = strdup(iarg->nukelog->dirname), *base = NULL;

	if (gfl & F_OPT_VERBOSE)
		base = iarg->nukelog->dirname;
	else
		base = basename(ndup);

	time_t t_t = (time_t) iarg->nukelog->nuketime;

	strftime(buffer2, 255, "%a, %d %b %Y %T %z", localtime(&t_t));
	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = sprintf(buffer, "NUKELOG %s %s %hu %.2f %s %s %u\n", base,
				iarg->nukelog->reason, iarg->nukelog->mult,
				iarg->nukelog->bytes,
				!iarg->nukelog->status ?
						iarg->nukelog->nuker : iarg->nukelog->unnuker,
				iarg->nukelog->nukee, iarg->nukelog->nuketime);
	} else {
		c =
				sprintf(buffer,
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

	g_memcpy(output, buffer, 2048);

	return c;
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

ULLONG dirlog_find(char *dirn, int mode, unsigned int flags, void *callback) {
	g_setjmp(0, "dirlog_find", NULL, NULL);
	struct dirlog buffer;
	int (*callback_f)(struct dirlog *data) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &actdl, DL_SZ))
		return MAX_ULLONG;

	int r;
	ULLONG ur = MAX_ULLONG;

	char buffer_s[255] = { 0 };
	char *dup, *dup2, *base, *dir;
	int gi1, gi2;

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	gi2 = strlen(buffer_s);

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &actdl, DL_SZ))) {
		dup = strdup(d_ptr->dirname);
		base = basename(dup);
		gi1 = strlen(base);
		dup2 = strdup(d_ptr->dirname);
		dir = dirname(dup2);

		if (!strncmp(&buffer_s[gi2 - gi1], base, gi1)
				&& !strncmp(buffer_s, d_ptr->dirname, strlen(dir))) {
			ur = actdl.offset - 1;
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
		g_close(&actdl);

	return ur;
}

ULLONG nukelog_find(char *dirn, int mode, struct nukelog *output) {
	g_setjmp(0, "nukelog_find", NULL, NULL);
	struct nukelog buffer = { 0 };

	ULLONG r = MAX_ULLONG;
	char *dup, *dup2, *base, *dir;

	if (g_fopen(NUKELOG, "r", F_DL_FOPEN_BUFFER, &actnl, NL_SZ)) {
		goto r_end;
	}

	int gi1, gi2;
	gi2 = strlen(dirn);

	struct nukelog *n_ptr = NULL;

	while ((n_ptr = (struct nukelog *) g_read(&buffer, &actnl, NL_SZ))) {
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
			r = actnl.offset - 1;
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
		g_close(&actnl);
	}

	r_end:

	return r;
}

int rebuild_data_file(char *file, struct g_handle *hdl) {
	g_setjmp(0, "rebuild_data_file", NULL, NULL);
	int ret = 0, r;
	off_t sz_r;
	char buffer[4096] = { 0 };

	if (strlen(file) + 4 > 4096) {
		return 1;
	}

	bzero(hdl->s_buffer, 4096);
	sprintf(hdl->s_buffer, "%s.dtm", file);
	sprintf(buffer, "%s.bk", file);

	if ((exec_str || gfl & F_OPT_HAS_G_REGEX) && updmode != UPD_MODE_RECURSIVE) {
		g_setjmp(0, "rebuild_data_file(2)", NULL, NULL);
		if (gfl & F_OPT_VERBOSE2) {
			printf("NOTE: %s: filtering data..\n", file);
		}

		p_md_obj ptr = hdl->buffer.objects;

		while (ptr) {
			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			if (g_bmatch(ptr->ptr, hdl)) {
				if (!(ptr = md_unlink(&hdl->buffer, ptr))) {
					if (!hdl->buffer.offset) {
						printf(
								"WARNING: %s: everything got filtered, refusing to write 0-byte data file\n",
								file);
						return 11;
					} else {
						printf(
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
		printf("NOTE: %s: flushing data to disk..\n", file);
	}

	if ((r = flush_data_md(hdl, hdl->s_buffer))) {
		if (r == 1) {
			if (gfl & F_OPT_VERBOSE) {
				printf("WARNING: %s: empty buffer (nothing to flush)\n",
						hdl->s_buffer);
			}
			ret = 0;
		} else {
			printf("ERROR: %s: [%d] flushing data failed!\n", hdl->s_buffer, r);
			ret = 2;
		}

		goto end;
	}

	g_setjmp(0, "rebuild_data_file(3)", NULL, NULL);

	if (gfl & F_OPT_KILL_GLOBAL) {
		printf(
				"WARNING: %s: aborting rebuild (will not be writing what was done up to here)\n",
				file);
		if (!(gfl & F_OPT_NOWRITE)) {
			remove(hdl->s_buffer);
		}
		return 0;
	}

	if (!(gfl & F_OPT_NOWRITE)
			&& (sz_r = get_file_size(hdl->s_buffer)) < hdl->block_sz) {
		printf(
				"ERROR: %s: [%u/%u] generated data file is smaller than a single record!\n",
				hdl->s_buffer, (unsigned int) sz_r,
				(unsigned int) hdl->block_sz);
		ret = 7;
		if (!(gfl & F_OPT_NOWRITE)) {
			remove(hdl->s_buffer);
		}
		goto end;
	}

	if (!(gfl & F_OPT_NOWRITE)) {
		if (data_backup_records(DIRLOG)) {
			ret = 3;
			if (!(gfl & F_OPT_NOWRITE)) {
				remove(hdl->s_buffer);
			}
			goto end;
		}
	}

	g_setjmp(0, "rebuild_data_file(3)", NULL, NULL);

	if (!(gfl & F_OPT_NOWRITE)) {

		if (hdl->fh) {
			g_fclose(hdl->fh);
			hdl->fh = NULL;
		}

		if (!strncmp(hdl->mode, "a", 1)) {
			if ((r = (int) file_copy(hdl->s_buffer, file, "a")) == -1) {
				printf("ERROR: %s: [%d] merging temp file failed!\n",
						hdl->s_buffer,
						errno);
				ret = 4;
			}
//remove(hdl->s_buffer);
		} else {
			if ((r = rename(hdl->s_buffer, file))) {
				printf("ERROR: %s: [%d] renaming temporary file failed!\n",
						hdl->s_buffer,
						errno);
				ret = 4;
			}
		}
	}
	end:

	return ret;
}

int g_load_record(struct g_handle *hdl, const void *data) {
	void *buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);

	if (!buffer) {
		return 1;
	}

	g_memcpy(buffer, data, hdl->block_sz);

	return 0;
}

int flush_data_md(struct g_handle *hdl, char *outfile) {
	g_setjmp(0, "flush_data_md", NULL, NULL);
	FILE *fh = NULL;
	size_t bw = 0;
	unsigned char *buffer = NULL;
	char *mode = "w";

	int ret = 0;

	if (hdl->flags & F_GH_FFBUFFER) {
		if (!hdl->w_buffer.offset) {
			return 1;
		}
	} else {
		if (!hdl->buffer_count) {
			return 1;
		}
	}

	/*if (strlen(hdl->mode)) {
	 printf("-%s\n", hdl->mode);
	 mode = hdl->mode;
	 }*/

	if ((fh = gg_fopen(outfile, mode)) == NULL) {
		return 2;
	}

	size_t v = (V_MB * 32) / hdl->block_sz;

	buffer = calloc(v, hdl->block_sz);

	p_md_obj ptr;

	if (hdl->flags & F_GH_FFBUFFER) {
		ptr = hdl->w_buffer.objects;
	} else {
		ptr = hdl->buffer.objects;
	}

	size_t rw = 0;

	g_setjmp(0, "flush_data_md(loop)", NULL, NULL);

	while (ptr) {
		if (gfl & F_OPT_KILL_GLOBAL) {
			break;
		}
		g_memcpy(&buffer[rw * hdl->block_sz], ptr->ptr, hdl->block_sz);
		rw++;
		//printf("%d %d %d\n", rw, v, hdl->buffer_count);
		if (!(gfl & F_OPT_NOWRITE) && (rw == v || !ptr->next)) {
			if ((bw = g_fwrite(buffer, hdl->block_sz, rw, fh)) != rw) {
				ret = 3;
				break;
			}
			hdl->bw += bw * hdl->block_sz;
			hdl->rw += rw;
			bzero(buffer, v * hdl->block_sz);
			rw = 0;
		}
		ptr = ptr->next;
	}

	g_setjmp(0, "flush_data_md(2)", NULL, NULL);

	g_free(buffer);
	g_fclose(fh);

	return ret;
}

size_t g_load_data(FILE *fh, void *output, size_t max) {
	size_t fr, c_fr = 0;
	unsigned char *b_output = (unsigned char*) output;
	while ((fr = g_fread(&b_output[c_fr], 1, max - c_fr, fh))) {
		c_fr += fr;
	}
	return c_fr;
}

int load_data_md(pmda md, char *file, struct g_handle *hdl) {
	g_setjmp(0, "load_data_md", NULL, NULL);
	FILE *fh;
	int r = 0;

	if (!hdl->block_sz) {
		return -2;
	}

//void *buffer = calloc(1, hdl->block_sz);

	if (!(fh = gg_fopen(file, "r"))) {
		return -3;
	}

	size_t count = hdl->total_sz / hdl->block_sz;
	hdl->data = calloc(count, hdl->block_sz);

	if (md_init(md, count)) {
		r = -4;
		goto end;
	}

	size_t i, b_read = 0;

	hdl->buffer_count = 0;
	md->flags |= F_MDA_REFPTR;

	if ((b_read = g_load_data(fh, hdl->data, hdl->total_sz)) != hdl->total_sz) {
		r = -9;
		md_g_free(md);
		goto end;
	}

	unsigned char *w_ptr = (unsigned char*) hdl->data;

	for (i = 0; i < count; i++) {
		md->lref_ptr = (void*) w_ptr;
		w_ptr += hdl->block_sz;
		if (!md_alloc(md, hdl->block_sz)) {
			r = -5;
			md_g_free(md);
			goto end;
		}

		//g_memcpy(ptr, buffer, hdl->block_sz);
		hdl->buffer_count++;

		//b_read++;
	}

	g_setjmp(0, "load_data_md", NULL, NULL);

	r = hdl->buffer_count;

	end: g_fclose(fh);
//r_end: g_free(buffer);

	return r;
}

int g_buffer_into_memory(char *file, struct g_handle *hdl,
		unsigned int block_sz) {
	g_setjmp(0, "g_buffer_into_memory", NULL, NULL);

	if (hdl->buffer.count) {
		return 0;
	}

	struct stat st;

	if (stat(file, &st) == -1) {
		printf("ERROR: %s: [%d] unable to get information from file!\n", file,
		errno);
		return 2;
	}

	if (!st.st_size) {
		printf("ERROR: %s: 0-byte data file detected!!\n", file);
		return 3;
	}

	if (st.st_size % block_sz) {
		printf(MSG_GEN_DFCORRU, file, (ULLONG) st.st_size, block_sz);
		return 12;
	}

	if (st.st_size > db_max_size) {
		printf(
				"WARNING: %s: disabling memory buffering, file too big (max %lld MB)\n",
				file, db_max_size / 1024 / 1024);
		hdl->flags |= F_GH_NOMEM;
		return 5;
	}

	hdl->total_sz = st.st_size;
	hdl->block_sz = block_sz;

	if (gfl & F_OPT_VERBOSE2)
		printf(
				"NOTE: %s: loading data file into memory [%u records] [%llu bytes]\n",
				file, (unsigned int) (hdl->total_sz / hdl->block_sz),
				(ULLONG) hdl->total_sz);

	int r;
	if ((r = load_data_md(&hdl->buffer, file, hdl))
			!= (hdl->total_sz / hdl->block_sz)) {
		printf(
				"ERROR: %s: [%d/%u] [%u] [%u] could not load data file into memory!\n",
				file, r, (unsigned int) (hdl->total_sz / hdl->block_sz),
				(unsigned int) hdl->total_sz, hdl->block_sz);
		return 4;
	} else {
		if (gfl & F_OPT_VERBOSE2)
			printf("NOTE: %s: loaded %u records into memory\n", file, r);
	}
	return 0;
}

int g_fopen(char *file, char *mode, unsigned int flags, struct g_handle *hdl,
		size_t size) {
	g_setjmp(0, "g_fopen", NULL, NULL);

	if (flags & F_DL_FOPEN_REWIND) {
		gh_rewind(hdl);
	}

	if (!(gfl & F_OPT_NOBUFFER) && flags & F_DL_FOPEN_BUFFER
			&& !(hdl->flags & F_GH_NOMEM)) {
		if (!g_buffer_into_memory(file, hdl, size)) {
			if (!(flags & F_DL_FOPEN_FILE)) {
				return 0;
			}
		}
	}

	if (hdl->fh) {
		return 0;
	}

	if (strlen(file) > 4096) {
		return 2;
	}

	FILE *fd;

	if (!(fd = gg_fopen(file, mode))) {
		return 1;
	}

	g_strncpy(hdl->file, file, strlen(file));
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

	if (hdl->buffer_count) {
		hdl->offset = 0;
		hdl->buffer.r_pos = hdl->buffer.objects;
	}

	hdl->br = 0;
	hdl->bw = 0;
	hdl->rw = 0;

	return 0;
}

int g_do_exec(void *buffer, struct g_handle *hdl) {

	g_setjmp(0, "g_do_exec", NULL, NULL);

	if (!(hdl->flags & F_GH_ISDIRLOG) || !exec_str) {
		return 1;
	}

	bzero(b_glob, MAX_EXEC_STR);
	if (process_exec_string(exec_str, b_glob, (struct dirlog*) buffer)) {
		return 1;
	}

//printf(buffer2);

	return system(b_glob);
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
		hdl->br += DL_SZ;
		return (struct dirlog *) hdl->buffer.pos->ptr;
	}

	if (!buffer) {
		return NULL;
	}

	if (!hdl->fh) {
		printf("IO ERROR: dirlog handle not open\n");
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
	size_t read = 0;

	while (!feof(pipe)) {
		if ((read += g_fread(&buffer[read], 1, PIPE_READ_MAX - read, pipe))
				<= 0) {
			break;
		}
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
	if (gfl & F_OPT_NOWRITE)
		return 0;

	if (!buffer)
		return 2;

	if (!actdl.fh) {
		printf("ERROR: dirlog handle is not open\n");
		return 1;
	}

	if (whence == SEEK_SET && fseeko(actdl.fh, offset * DL_SZ, SEEK_SET) < 0) {
		printf("ERROR: seeking dirlog failed!\n");
		return 1;
	}

	int fw;

	if ((fw = g_fwrite(buffer, 1, DL_SZ, actdl.fh)) < DL_SZ) {
		printf("ERROR: could not write dirlog record! %d/%d\n", fw,
				(int) DL_SZ);
		return 1;
	}

	actdl.bw += (off_t) fw;
	actdl.rw++;

	if (whence == SEEK_SET)
		actdl.offset = offset;
	else
		actdl.offset++;

	return 0;
}

int enum_dir(char *dir, void *cb, void *arg, int f) {
	g_setjmp(0, "enum_dir", NULL, NULL);
	int (*callback_f)(char *data, unsigned char type, void *arg) = NULL;
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

	char buf[1024] = { 0 };
	int d_type;

	while ((dirp = readdir(dp))) {
		if (gfl & F_OPT_KILL_GLOBAL) {
			break;
		}

		sprintf(buf, "%s/%s", dir, dirp->d_name);

		d_type = get_file_type(buf);
		if (!(ir = callback_f(buf, d_type, arg))) {
			if (f == 1) {
				closedir(dp);
				return ir;
			} else {
				r++;
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

	return r;
}

int reg_match(char *expression, char *match, int flags) {
	g_setjmp(0, "reg_match", NULL, NULL);
	regex_t preg;
	size_t r;
	regmatch_t pmatch[REG_MATCHES_MAX];

	if ((r = regcomp(&preg, expression, flags | REG_EXTENDED)))
		return r;

	r = regexec(&preg, match, REG_MATCHES_MAX, pmatch, REG_EXTENDED);

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
			char *buffer = md_alloc(output_t, 2048);
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

	if (gfl & F_OPT_NOWRITE)
		return 0;

	if (type != DT_REG)
		return 1;

	if (!reg_match(match, name, 0))
		return remove(name);

	return 2;
}

off_t read_file(char *file, unsigned char *buffer, size_t read_max,
		off_t offset) {
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

	if ((fp = gg_fopen(file, "r")) == NULL)
		return 0;

	if (offset)
		fseeko(fp, (off_t) offset, SEEK_SET);

	for (read = 0; !feof(fp) && read < read_max;) {
		if ((r = g_fread(&buffer[read], 1, read_max - read, fp)) < 1)
			break;
		read += r;
	}

	g_fclose(fp);

	return read;
}

int file_exists(char *file) {
	g_setjmp(0, "file_exists", NULL, NULL);
	int r;

	errno = 0;
	FILE *fd = gg_fopen(file, "r");

	r = errno;

	if (fd) {
		g_fclose(fd);
	}

	return r;
}

ssize_t file_copy(char *source, char *dest, char *mode) {
	g_setjmp(0, "file_copy", NULL, NULL);

	off_t ssize = get_file_size(source);

	if (ssize < 1) {
		return -1;
	}

	FILE *fh_s = gg_fopen(source, "r");

	if (!fh_s) {
		return -2;
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

	return t;
}

ULLONG file_crc32(char *file, uLong *crc_out) {
	g_setjmp(0, "file_crc32", NULL, NULL);
	FILE *fp;
	int read;
	size_t r;
	unsigned char *buffer = calloc(CRC_FILE_READ_BUFFER_SIZE, 1);

	*crc_out = 0x0;

	if ((fp = gg_fopen((char*) &file[0], "r")) == NULL) {
		g_free(buffer);
		return 0;
	}

	uLong crc = MAX_ULONG;

	for (read = 0, r = 0; !feof(fp);) {
		if ((r = g_fread(buffer, 1, CRC_FILE_READ_BUFFER_SIZE, fp)) < 1)
			break;
		crc = crc32buf(crc, buffer, r);
		bzero(buffer, CRC_FILE_READ_BUFFER_SIZE);
		read += r;
	}

	if (read)
		*crc_out = crc;

	g_free(buffer);
	g_fclose(fp);

	return read;
}

int ref_to_val(struct dirlog *data, char *match, char *output, size_t max_size) {
	g_setjmp(0, "ref_to_val", NULL, NULL);
	if (output) {
		bzero(output, max_size);
	}

	if (!strcmp(match, "dir")) {
		g_strncpy(output, data->dirname, 255);
	} else if (!strcmp(match, "basedir")) {
		char *s_buffer = strdup(data->dirname);
		g_strncpy(output, basename(s_buffer), strlen(s_buffer));
		g_free(s_buffer);
	} else if (!strcmp(match, "user")) {
		sprintf(output, "%d", data->uploader);
	} else if (!strcmp(match, "group")) {
		sprintf(output, "%d", data->group);
	} else if (!strcmp(match, "files")) {
		sprintf(output, "%u", (unsigned int) data->files);
	} else if (!strcmp(match, "size")) {
		sprintf(output, "%llu", (ULLONG) data->bytes);
	} else if (!strcmp(match, "created")) {
		sprintf(output, "%u", (unsigned int) data->uptime);
	} else {
		return 1;
	}
	return 0;
}

int process_exec_string(char *input, char *output, struct dirlog *data) {
	g_setjmp(0, "process_exec_string", NULL, NULL);
	if (!exec_str) {
		return 1;
	}

	if (!output) {
		return 2;
	}

	size_t blen = strlen(exec_str), blen2 = strlen(input), blenmax = 0;

	if (!blen || blen > MAX_EXEC_STR) {
		return 1;
	}

	blenmax = blen * 2;

	if (blenmax > MAX_EXEC_STR) {
		return 1;
	}

	char buffer[1024] = { 0 }, buffer2[1024] = { 0 }, *buffer_o = calloc(
	MAX_EXEC_STR, 1);
	int i, i2, pi;

	for (i = 0, pi = 0; i < blen2; i++, pi++) {
		if (input[i] == 0x7B) {
			bzero(buffer, 255);
			for (i2 = 0, i++; i < blen2 && i2 < 255; i++, i2++) {
				if (input[i] == 0x7D) {
					if (!i2 || strlen(buffer) > 255
							|| ref_to_val(data, buffer, buffer2, 255)) {
						i++;
						break;
					}

					g_memcpy(&buffer_o[pi], buffer2, strlen(buffer2));

					pi += strlen(buffer2);
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
		printf(
				"NOTE: Child process caught SIGINT (hit CTRL^C again to quit)\n");
		usleep(1000000);
		break;
	case CLD_EXITED:
		break;
	default:
		if (gfl & F_OPT_VERBOSE3) {
			printf("NOTE: Child caught signal: %d \n", si->si_code);
		}
		break;
	}
}

#define SIG_BREAK_TIMEOUT_NS (double)1000000.0

void sig_handler(int signal) {
	switch (signal) {
	case SIGTERM:
		printf("NOTE: Caught SIGTERM, terminating gracefully.\n");
		gfl |= F_OPT_KILL_GLOBAL;
		break;
	case SIGINT:
		if (gfl & F_OPT_KILL_GLOBAL) {
			printf("NOTE: Caught SIGINT twice in %.2f seconds, terminating..\n",
			SIG_BREAK_TIMEOUT_NS / (1000000.0));
			exit(0);
		} else {
			printf(
					"NOTE: Caught SIGINT, quitting (hit CTRL^C again to terminate by force)\n");
			gfl |= F_OPT_KILL_GLOBAL;
		}
		usleep(SIG_BREAK_TIMEOUT_NS);
		break;
	default:
		usleep(SIG_BREAK_TIMEOUT_NS);
		printf("NOTE: Caught signal %d\n", signal);
		break;
	}
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

