/*
 * im_hdr.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef IM_HDR_H_
#define IM_HDR_H_

#include <fp_types.h>

#define F_GH_NOMEM                      (a64 << 1)
#define F_GH_ISDIRLOG                   (a64 << 2)
#define F_GH_EXEC                       (a64 << 3)
#define F_GH_ISNUKELOG                  (a64 << 4)
#define F_GH_FFBUFFER                   (a64 << 5)
#define F_GH_WAPPEND                    (a64 << 6)
#define F_GH_DFWASWIPED                 (a64 << 7)
#define F_GH_DFNOWIPE                   (a64 << 8)
#define F_GH_ISDUPEFILE                 (a64 << 9)
#define F_GH_ISLASTONLOG                (a64 << 10)
#define F_GH_ISONELINERS                (a64 << 11)
#define F_GH_ONSHM                      (a64 << 12)
#define F_GH_ISONLINE                   (a64 << 13)
#define F_GH_ISIMDB                     (a64 << 14)
#define F_GH_ISGAME                     (a64 << 15)
#define F_GH_ISFSX                      (a64 << 16)
#define F_GH_ISTVRAGE                   (a64 << 17)
#define F_GH_ISGENERIC1                 (a64 << 18)
#define F_GH_SHM                        (a64 << 19)
#define F_GH_SHMRB                      (a64 << 20)
#define F_GH_SHMDESTROY                 (a64 << 21)
#define F_GH_SHMDESTONEXIT              (a64 << 22)
#define F_GH_FROMSTDIN                  (a64 << 23)
#define F_GH_HASLOM                     (a64 << 24)
#define F_GH_HASMATCHES                 (a64 << 25)
#define F_GH_HASEXC                     (a64 << 26)
#define F_GH_APFILT                     (a64 << 27)
#define F_GH_HASMAXRES                  (a64 << 28)
#define F_GH_HASMAXHIT                  (a64 << 29)
#define F_GH_IFRES                      (a64 << 30)
#define F_GH_IFHIT                      (a64 << 31)
#define F_GH_ISGENERIC2                 (a64 << 32)
#define F_GH_HASSTRM                    (a64 << 33)
#define F_GH_ISGENERIC3                 (a64 << 34)
#define F_GH_ISGENERIC4                 (a64 << 35)
#define F_GH_TOSTDOUT                   (a64 << 36)
#define F_GH_ISSCONF                    (a64 << 37)
#define F_GH_LOCKED                     (a64 << 38)
#define F_GH_ISGCONF                    (a64 << 39)
#define F_GH_ARR_DIST                   (a64 << 40)
#define F_GH_IO_GZIP                    (a64 << 41)
#define F_GH_D_WRITE                    (a64 << 42)
#define F_GH_IS_GZIP                    (a64 << 43)
#define F_GH_IS_ALTLOG                  (a64 << 44)
#define F_GH_NO_BACKUP                  (a64 << 45)
#define F_GH_POST_PRINT                 (a64 << 46)
#define F_GH_PRE_PRINT                  (a64 << 47)
#define F_GH_NO_ACCU                    (a64 << 48)
#define F_GH_W_NSSYS                    (a64 << 49)
#define F_GH_EXECRD_PIPE_OUT            (a64 << 50)
#define F_GH_EXECRD_HAS_STD_PIPE     (a64 << 51)
#define F_GH_PRINT                      (a64 << 52)
#define F_GH_EXECRD_PIPE_IN             (a64 << 53)
#define F_GH_EXECRD_WAS_PIPED           (a64 << 54)
#define F_GH_TFD_PROCED                 (a64 << 55)
#define F_GH_SPEC_SQ01                  (a64 << 56)
#define F_GH_SPEC_SQ02                  (a64 << 57)
#define F_GH_HAS_LEXEC_WPID_CB          (a64 << 58)

#define F_GH_STATUS_FLAGS               (F_GH_TFD_PROCED)

/* these bits determine log type */
#define F_GH_ISTYPE                     (F_GH_ISFSX|F_GH_ISGCONF|F_GH_ISSCONF|F_GH_ISGENERIC4|F_GH_ISGENERIC3|F_GH_ISGENERIC2|F_GH_ISGENERIC1|F_GH_ISNUKELOG|F_GH_ISDIRLOG|F_GH_ISDUPEFILE|F_GH_ISLASTONLOG|F_GH_ISONELINERS|F_GH_ISONLINE|F_GH_ISIMDB|F_GH_ISGAME|F_GH_ISFSX|F_GH_ISTVRAGE|F_GH_IS_ALTLOG)

#define F_GH_ISSHM                      (F_GH_SHM|F_GH_ONSHM)
#define F_GH_ISMP                       (F_GH_HASMATCHES|F_GH_HASMAXRES|F_GH_HASMAXHIT)


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

#include <zlib.h>

#define F_ST_PIPEO_OUT                  (a32 << 1)
#define F_ST_PIPEO_IN                   (a32 << 2)

typedef struct ___pipe_object
{
  uint32_t status;
  pid_t child;
  ssize_t data_in, data_out;
  int pfd_in[2], pfd_out[2];
} _pipe_obj, *__pipe_obj;

typedef struct g_handle
{
  FILE *fh;
#ifdef HAVE_ZLIB_H
  char w_mode[6];
  gzFile gz_fh, gz_fh1;
#endif
  off_t offset, bw, br, total_sz;
  off_t rw, t_rw;
  uint32_t block_sz;
  uint64_t flags, status;
  mda buffer, w_buffer;
  mda _match_rr;
  mda _accumulator;
  off_t max_results, max_hits;
  __g_ipcbm ifrh_l0, ifrh_l1;
  _execv exec_args;
  mda print_mech;
  mda post_print_mech;
  mda pre_print_mech;
  pmda act_mech;
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
  _d_proc3 g_proc3, g_proc3_batch, g_proc3_export, g_proc3_extra;
  _d_gcb_pp_hook gcb_post_proc;
  _d_omfp g_proc4, g_proc4_pr, g_proc4_po;
  __d_is_wb w_d, w_d_pr, w_d_po;
  size_t j_offset, jm_offset;
  int d_memb;
  void *_x_ref;
  int shmcflags;
  int shmatflags;
  mda guid_stor;
  mda uuid_stor;
  int h_errno;
  int h_errno_gz;
  const char *h_errstr_gz;
  _pipe_obj pipe;
  char strerr_b[1024];
  void *v_b0;
  size_t v_b0_sz;
  __d_wpid_cb execv_wpid_fp;
#ifdef _G_SSYS_NET
  void *pso_ref;
#endif
} _g_handle, *__g_handle;

#define G_HDL_ERRNO_DL_READ        1
#define G_HDL_ERRNO_DL_FOPEN       2
#define G_HDL_ERRNO_DL_FOPEN_GZ    3
#define G_HDL_ERRNO_ALLOC          4

typedef void
_dt_set(__g_handle hdl);
typedef void
(*__dt_set)(__g_handle hdl);

#ifdef _MAKE_SBIN
_dt_set dt_set_dummy;

#else
_dt_set dt_set_dummy, dt_set_dirlog, dt_set_nukelog, dt_set_dupefile,
    dt_set_lastonlog, dt_set_oneliners, dt_set_imdb, dt_set_game, dt_set_tvrage,
    dt_set_gen1, dt_set_gen2, dt_set_gen3, dt_set_gen4, dt_set_gconf,
    dt_set_sconf, dt_set_altlog, dt_set_online, dt_set_x;

__dt_set pdt_set_dirlog, pdt_set_nukelog, pdt_set_dupefile, pdt_set_lastonlog,
    pdt_set_oneliners, pdt_set_imdb, pdt_set_game, pdt_set_tvrage, pdt_set_gen1,
    pdt_set_gen2, pdt_set_gen3, pdt_set_gen4, pdt_set_gconf, pdt_set_sconf,
    pdt_set_altlog, pdt_set_online, pdt_set_x;
#endif

#endif /* IM_HDR_H_ */
