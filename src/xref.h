/*
 * xref.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef XREF_H_
#define XREF_H_

#define _GNU_SOURCE

#include <glutil.h>
#include <t_glob.h>

#include <im_hdr.h>

#define _MC_X_DEVID             "devid"
#define _MC_X_MINOR             "minor"
#define _MC_X_MAJOR             "major"
#define _MC_X_INODE             "inode"
#define _MC_X_LINKS             "links"
#define _MC_X_BLKSIZE           "blksize"
#define _MC_X_BLOCKS            "blocks"
#define _MC_X_ATIME             "atime"
#define _MC_X_CTIME             "ctime"
#define _MC_X_MTIME             "mtime"
#define _MC_X_ISREAD            "isread"
#define _MC_X_ISWRITE           "iswrite"
#define _MC_X_ISEXEC            "isexec"
#define _MC_X_PERM              "perm"
#define _MC_X_OPERM             "operm"
#define _MC_X_GPERM             "gperm"
#define _MC_X_UPERM             "uperm"
#define _MC_X_SPARSE            "sparse"
#define _MC_X_CRC32             "crc32"
#define _MC_X_DCRC32            "dec-crc32"
#define _MC_X_BASEPATH          "basepath"
#define _MC_X_DIRPATH           "dirpath"
#define _MC_X_PATH              "path"
#define _MC_X_UID               "uid"
#define _MC_X_GID               "gid"

#define F_XRF_DO_STAT           (a32 << 1)
#define F_XRF_GET_DT_MODE       (a32 << 2)
#define F_XRF_GET_READ          (a32 << 3)
#define F_XRF_GET_WRITE         (a32 << 4)
#define F_XRF_GET_EXEC          (a32 << 5)
#define F_XRF_GET_UPERM         (a32 << 6)
#define F_XRF_GET_GPERM         (a32 << 7)
#define F_XRF_GET_OPERM         (a32 << 8)
#define F_XRF_GET_PERM          (a32 << 9)
#define F_XRF_GET_CRC32         (a32 << 10)
#define F_XRF_GET_CTIME         (a32 << 11)
#define F_XRF_GET_MINOR         (a32 << 12)
#define F_XRF_GET_MAJOR         (a32 << 13)
#define F_XRF_GET_SPARSE        (a32 << 14)
#define F_XRF_GET_STCTIME       (a32 << 15)

#define F_PD_RECURSIVE                  (a32 << 1)
#define F_PD_MATCHDIR                   (a32 << 2)
#define F_PD_MATCHREG                   (a32 << 3)

#define F_PD_MATCHTYPES                 (F_PD_MATCHDIR|F_PD_MATCHREG)

#define F_XRF_ACCESS_TYPES  (F_XRF_GET_READ|F_XRF_GET_WRITE|F_XRF_GET_EXEC)
#define F_XRF_PERM_TYPES        (F_XRF_GET_UPERM|F_XRF_GET_GPERM|F_XRF_GET_OPERM|F_XRF_GET_PERM)

#define F_ENUMD_ENDFIRSTOK              (a32 << 1)
#define F_ENUMD_BREAKONBAD              (a32 << 2)
#define F_ENUMD_NOXDEV                  (a32 << 3)
#define F_ENUMD_NOXBLK                  (a32 << 4)

#define F_EDS_ROOTMINSET                (a32 << 1)
#define F_EDS_KILL                      (a32 << 2)

/* Get or fake the disk device blocksize.
 Usually defined by sys/param.h (if at all).  */
#ifndef DEV_BSIZE
# ifdef BSIZE
#  define DEV_BSIZE BSIZE
# else /* !BSIZE */
#  define DEV_BSIZE 4096
# endif /* !BSIZE */
#endif /* !DEV_BSIZE */

/* Extract or fake data from a `struct stat'.
 ST_BLKSIZE: Preferred I/O blocksize for the file, in bytes.
 ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
 ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
#ifndef HAVE_STRUCT_STAT_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined _POSIX_SOURCE || !defined BSIZE /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0) : 0)
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? st_blocks ((statbuf).st_size) : 0)
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_STRUCT_STAT_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
                               ? (statbuf).st_blksize : DEV_BSIZE)
# if defined hpux || defined __hpux__ || defined __hpux
/* HP-UX counts st_blocks in 1024-byte units.
 This loses when mixing HP-UX and BSD filesystems with NFS.  */
#  define ST_NBLOCKSIZE 1024
# else /* !hpux */
#  if defined _AIX && defined _I386
/* AIX PS/2 counts st_blocks in 4K units.  */
#   define ST_NBLOCKSIZE (4 * 1024)
#  else /* not AIX PS/2 */
#   if defined _CRAY
#    define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks * ST_BLKSIZE(statbuf)/ST_NBLOCKSIZE : 0)
#   endif /* _CRAY */
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_STRUCT_STAT_ST_BLOCKS */

#ifndef ST_NBLOCKS
# define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks : 0)
#endif

#ifndef ST_NBLOCKSIZE
# define ST_NBLOCKSIZE 512
#endif

#ifdef major                    /* Might be defined in sys/types.h.  */
#define HAVE_MAJOR
#endif
#ifndef HAVE_MAJOR
#define major(dev)  (((dev) >> 8) & 0xff)
#define minor(dev)  ((dev) & 0xff)
#endif
#undef HAVE_MAJOR

typedef struct ___d_xref_ct
{
  uint8_t active;
  time_t curtime;
  int ct_off;
} _d_xref_ct, *__d_xref_ct;

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

typedef struct ___g_eds
{
  uint32_t flags;
  uint32_t r_minor, r_major;
  struct stat st;
  off_t depth;
} _g_eds, *__g_eds;

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

typedef int
__d_enum_cb(char *, unsigned char, void *, __g_eds);
int
g_l_fmode_n(char *path, size_t max_size, char *output);
int
g_l_fmode(char *path, size_t max_size, char *output);

__g_proc_rv dt_rval_x_path, dt_rval_x_basepath, dt_rval_x_dirpath, dt_rval_c,
    dt_rval_x_size, dt_rval_x_mode, dt_rval_x_devid, dt_rval_x_minor, major,
    dt_rval_x_inode, dt_rval_x_links, dt_rval_x_uid, dt_rval_x_gid,
    dt_rval_x_blksize, dt_rval_x_blocks, dt_rval_x_atime, dt_rval_x_ctime,
    dt_rval_x_mtime, dt_rval_x_isread, dt_rval_x_iswrite, dt_rval_x_isexec,
    dt_rval_x_uperm, dt_rval_x_gperm, dt_rval_x_operm, dt_rval_x_perm,
    dt_rval_x_sparse, dt_rval_x_crc32, dt_rval_x_deccrc32;

void
g_xproc_rc(char *name, void *aa_rh, __g_eds eds);
int
g_xproc_m(unsigned char type, char *name, __std_rh aa_rh, __g_eds eds);
void
g_preproc_dm(char *name, __std_rh aa_rh, unsigned char type);

_d_rtv_lk ref_to_val_lk_x;
__d_ref_to_pval ref_to_val_ptr_x;

int
g_process_directory(char *name, unsigned char type, void *arg, __g_eds eds);

size_t
d_xref_ct_fe(__d_xref_ct input, size_t sz);

typedef int
(*__d_edscb)(char *, unsigned char, void *, __g_eds);

int
enum_dir(char *dir, __d_edscb callback_f, void *arg, int f, __g_eds eds);

int
ref_to_val_x(void *arg, char *match, char *output, size_t max_size, void *mppd);

int
g_dump_ug(char *ug);
int
g_dump_gen(char *root);

float
file_sparseness(const struct stat *p);

#endif /* XREF_H_ */
