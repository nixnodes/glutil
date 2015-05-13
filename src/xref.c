/*
 * xref.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"
#include <t_glob.h>
#include <l_sb.h>
#include <cfgv.h>
#include <lref_gen.h>
#include <x_f.h>
#include <mc_glob.h>
#include "xref.h"
#include <omfp.h>
#include <m_comp.h>
#include <str.h>
#include <lref.h>
#include <log_op.h>
#include <m_general.h>
#include <log_io.h>
#include <misc.h>
#include <errno_int.h>
#include <sort_hdr.h>
#include <arg_proc.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <math.h>
#include <errno.h>

void
dt_set_x(__g_handle hdl)
{
  hdl->flags |= F_GH_ISFSX;
  hdl->block_sz = sizeof(_d_xref);
  hdl->d_memb = 1;
  hdl->g_proc0 = NULL;
  hdl->g_proc1_lookup = ref_to_val_lk_x;
  hdl->g_proc2 = ref_to_val_ptr_x;
  hdl->g_proc3 = x_format_block;
  hdl->g_proc3_batch = x_format_block;
  hdl->g_proc3_export = x_format_block;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_X;
  hdl->jm_offset = (size_t) &((__d_xref) NULL)->name;
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

static int
g_legacy_guser(__d_xref data, size_t max_size, char *output, __d_drt_h mppd)
{
  int r;
  if ((r = r_preload_guid_data(&mppd->hdl->uuid_stor, DEFPATH_PASSWD)))
    {
      /*if (r == 1)
       {
       print_str(MSG_GEN_NOFACC, GLROOT, DEFPATH_PASSWD);
       }*/
      return 1;
    }

  struct stat st;

  if (lstat(data->name, &st))
    {
      return 2;
    }

  p_gu_n pgn = search_xuid_id(&mppd->hdl->uuid_stor, (uint32_t) st.st_uid);
  if (pgn != NULL)
    {
      snprintf(output, max_size, "%s", pgn->name);
      return 0;
    }

  strncpy(output, MSG_X_UMISSING, sizeof(MSG_X_UMISSING));
  return 0;
}

static int
g_legacy_ggroup(__d_xref data, size_t max_size, char *output, __d_drt_h mppd)
{
  int r;
  if ((r = r_preload_guid_data(&mppd->hdl->guid_stor, DEFPATH_GROUP)))
    {
      /*if (r == 1)
       {
       print_str(MSG_GEN_NOFACC, GLROOT, DEFPATH_GROUP);
       }*/
      return 1;
    }

  struct stat st;

  if (lstat(data->name, &st))
    {
      return 2;
    }

  p_gu_n pgn = search_xuid_id(&mppd->hdl->guid_stor, (uint32_t) st.st_gid);
  if (pgn != NULL)
    {
      snprintf(output, max_size, "%s", pgn->name);
      return 0;
    }

  strncpy(output, MSG_X_GMISSING, sizeof(MSG_X_GMISSING));
  return 0;
}

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
  else if (!strncmp(match, _MC_X_ST_MODE, 6))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", st.st_mode);
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
#if defined HAVE_STRUCT_STAT_ST_BLKSIZE
  else if (!strncmp(match, _MC_X_BLKSIZE, 7))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_blksize);
    }
#endif
#if defined HAVE_STRUCT_STAT_ST_BLOCKS
  else if (!strncmp(match, _MC_X_BLOCKS, 6))
    {

      struct stat st;
      if (lstat(data->name, &st))
        {
          return 1;
        }
      snprintf(output, max_size, "%u", (uint32_t) st.st_blocks);
    }
#endif
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
      snprintf(output, max_size, "%huu", (uint8_t) !(access(data->name, R_OK)));
    }
  else if (!strncmp(match, _MC_X_ISWRITE, 7))
    {
      snprintf(output, max_size, "%huu", (uint8_t) !(access(data->name, W_OK)));
    }
  else if (!strncmp(match, _MC_X_ISEXEC, 6))
    {
      snprintf(output, max_size, "%huu", (uint8_t) !(access(data->name, X_OK)));
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
          snprintf(output, max_size, "%hhu",
              (uint8_t) ((st.st_mode & S_IRWXU) >> 6));
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
          snprintf(output, max_size, "%hhu",
              (uint8_t) ((st.st_mode & S_IRWXG) >> 3));
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
          snprintf(output, max_size, "%hhu",
              (uint8_t) ((st.st_mode & S_IRWXO)));
        }
    }
  else if (!strncmp(match, _MC_X_SPERM, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          strcp_s(output, max_size, "-1");
        }
      else
        {
          snprintf(output, max_size, "%hhu",
              (uint8_t) ((st.st_mode & (S_ISUID | S_ISGID | S_ISVTX))));
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
          snprintf(output, max_size, "%hu%hu%hu%hu",
              (uint16_t) ((st.st_mode & (S_ISUID | S_ISGID | S_ISVTX))) >> 9,
              (uint16_t) ((st.st_mode & S_IRWXU) >> 6),
              (uint16_t) ((st.st_mode & S_IRWXG) >> 3),
              (uint16_t) ((st.st_mode & S_IRWXO)));
        }
    }
  else if (!strncmp(match, _MC_X_PBITS, 5))
    {
      struct stat st;
      if (lstat(data->name, &st))
        {
          strcp_s(output, max_size, "-1");
        }
      else
        {
          char buffer[13];
          snprintf(output, max_size, "%s",
              g_bitstr(
                  (uint64_t) st.st_mode
                      & (S_ISUID | S_ISGID | S_ISVTX | S_IRWXO | S_IRWXG
                          | S_IRWXU), 12, buffer));
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
          snprintf(output, max_size, "%f", file_sparseness(&st));
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
  else if (!strncmp(match, _MC_X_USER, 4))
    {
      return g_legacy_guser(data, max_size, output, (__d_drt_h ) mppd);
    }
  else if (!strncmp(match, _MC_X_GROUP, 5))
    {
      return g_legacy_ggroup(data, max_size, output, (__d_drt_h ) mppd);
    }
  else if (!strncmp(match, _MC_X_DEPTH, 5))
    {
      snprintf(output, max_size, "%llu", (long long unsigned int) data->depth);
    }
  else
    {
      output[0] = 0x0;
      return 1;
    }

  return 0;
}

void *
ref_to_val_ptr_x(void *arg, char *match, int *output)
{
  __d_xref data = (__d_xref) arg;

  if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      *output = sizeof(data->st.st_size);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_size;
    }
  else if (!strncmp(match,_MC_GLOB_MODE, 4))
    {
      *output = sizeof(data->type);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_DT_MODE|F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->type;
    }
  else if (!strncmp(match,_MC_X_ST_MODE, 6))
    {
      *output = sizeof(data->st.st_mode);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_mode;
    }
  else if (!strncmp(match, _MC_X_ISREAD, 6))
    {
      *output = ~((int) sizeof(data->r));
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_READ;
        }
      return &((__d_xref) NULL)->r;
    }
  else if (!strncmp(match, _MC_X_ISWRITE, 7))
    {
      *output = ~((int) sizeof(data->w));
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_WRITE;
        }
      return &((__d_xref) NULL)->w;
    }
  else if (!strncmp(match, _MC_X_ISEXEC, 6))
    {
      *output = ~((int) sizeof(data->x));
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_EXEC;
        }
      return &((__d_xref) NULL)->x;
    }
  else if (!strncmp(match, _MC_X_UPERM, 5))
    {
      *output = sizeof(data->uperm);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_UPERM | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->uperm;
    }
  else if (!strncmp(match, _MC_X_GPERM, 5))
    {
      *output = sizeof(data->gperm);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_GPERM | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->gperm;
    }
  else if (!strncmp(match, _MC_X_OPERM, 5))
    {
      *output = sizeof(data->operm);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_OPERM | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->operm;
    }
  else if (!strncmp(match, _MC_X_SPERM, 5))
    {
      *output = sizeof(data->operm);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_SPERM | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->sperm;
    }
  else if (!strncmp(match, _MC_X_PBITS, 5))
    {
      *output = sizeof(data->pbits);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_PBITS | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->pbits;
    }
  else if (!strncmp(match, _MC_X_DEVID, 5))
    {
      *output = sizeof(data->st.st_dev);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_dev;
    }
  else if (!strncmp(match, _MC_X_MINOR, 5))
    {
      *output = sizeof(data->minor);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_MINOR | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->minor;
    }
  else if (!strncmp(match, _MC_X_MAJOR, 5))
    {
      *output = sizeof(data->major);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_MAJOR | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->major;
    }
  else if (!strncmp(match, _MC_X_SPARSE, 6))
    {
      *output = -32;
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_SPARSE | F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->sparseness;
    }
  else if (!strncmp(match, _MC_X_INODE, 5))
    {
      *output = sizeof(data->st.st_ino);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_ino;
    }
  else if (!strncmp(match, _MC_X_LINKS, 5))
    {
      *output = sizeof(data->st.st_nlink);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_nlink;
    }
  else if (!strncmp(match, _MC_X_UID, 3))
    {
      *output = sizeof(data->st.st_uid);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_uid;
    }
  else if (!strncmp(match, _MC_X_GID, 3))
    {
      *output = sizeof(data->st.st_gid);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_gid;
    }
#if defined HAVE_STRUCT_STAT_ST_BLKSIZE
  else if (!strncmp(match, _MC_X_BLKSIZE, 7))
    {
      *output = sizeof(data->st.st_blksize);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_blksize;
    }
#endif
#if defined HAVE_STRUCT_STAT_ST_BLOCKS
  else if (!strncmp(match, _MC_X_BLOCKS, 6))
    {
      *output = sizeof(data->st.st_blocks);
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_blocks;
    }
#endif
  else if (!strncmp(match, _MC_X_ATIME, 5))
    {
      *output = ~((int) sizeof(data->st.st_atime));
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_atime;
    }
  else if (!strncmp(match, _MC_X_CTIME, 5))
    {
      *output = ~((int) sizeof(data->st.st_ctime));
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT|F_XRF_GET_STCTIME;
        }
      return &((__d_xref) NULL)->st.st_ctime;
    }
  else if (!strncmp(match, _MC_X_MTIME, 5))
    {
      *output = ~((int) sizeof(data->st.st_mtime));
      if (NULL != data)
        {
          data->flags |= F_XRF_DO_STAT;
        }
      return &((__d_xref) NULL)->st.st_mtime;
    }
  else if (!strncmp(match, _MC_X_CRC32, 5))
    {
      *output = sizeof(data->crc32);
      if (NULL != data)
        {
          data->flags |= F_XRF_GET_CRC32;
        }
      return &((__d_xref) NULL)->crc32;
    }
  else if (!strncmp(match, _MC_X_DEPTH, 5))
    {
      *output = sizeof(data->depth);
      if (NULL != data)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_DEPTH;
        }
      return &((__d_xref) NULL)->depth;
    }
  /*else if (!strncmp(match, "curtime", 7))
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
   }*/

  return NULL;
}

char *
dt_rval_x_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_x_guid(&((__d_drt_h) mppd)->hdl->uuid_stor, ((__d_xref) arg)->st.st_uid)
  return output;
}

char *
dt_rval_x_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_x_guid(&((__d_drt_h) mppd)->hdl->guid_stor, ((__d_xref) arg)->st.st_gid)
  return output;
}

char *
dt_rval_x_path(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((__d_xref) arg)->name;
}

char *
dt_rval_x_depth(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (unsigned long long int) ((__d_xref) arg)->depth);
  return output;
}

char *
dt_rval_x_data(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  FILE *fp = fopen(((__d_xref) arg)->name, "rb");

  if ( NULL == fp )
    {
      char err_buf[1024];
      print_str("ERROR: dt_rval_x_data: error opening file '%s': %s\n", ((__d_xref) arg)->name,
      strerror_r(errno, err_buf, sizeof(err_buf)));
    }

  size_t read, r;

  for (read = 0; !feof(fp) && !ferror(fp) && read < max_size;)
    {
      if ((r = fread(&((unsigned char*) output)[read], 1, max_size - read, fp))
      < 1)
        {
          break;
        }
      read += r;
    }

  if ( ferror(fp) )
    {
      char err_buf[1024];
      print_str("ERROR: dt_rval_x_data: while reading file '%s': %s\n", ((__d_xref) arg)->name,
      strerror_r(errno, err_buf, sizeof(err_buf)));
    }

  if ( !feof(fp) && read == max_size)
    {
      print_str("ERROR: dt_rval_x_data: data size exceeded available buffer space\n");
      memset(output, 0x0, max_size);
    }

  if ( fclose(fp) )
    {
      char err_buf[1024];
      print_str("ERROR: dt_rval_x_data: failed closing file descriptor '%s': %s\n", ((__d_xref) arg)->name,
      strerror_r(errno, err_buf, sizeof(err_buf)));
    }

  ((__d_drt_h ) mppd)->ret_len = (uint64_t)read;

  return output;
}

char *
dt_rval_x_rlink(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  uint8_t d_mode = IFTODT(((__d_xref) arg)->st.st_mode);

  if (d_mode != DT_LNK)
    {
      output[0] = 0x0;
      return output;
    }

  ssize_t b_spl_l;
  if ((b_spl_l = readlink(((__d_xref) arg)->name, output, max_size - 1)) != (ssize_t) -1 )
    {
      output[b_spl_l] = 0x0;
    }
  else
    {
      output[0] = 0x0;
    }

  return output;
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
dt_rval_c(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  char p_b0[128];
  int ic = 0;
  match = ((__d_drt_h ) mppd)->match;
  while (match[ic] != 0x7D && match[ic] != 0x2C && ic < 127 && match[ic])
    {
      p_b0[ic] = match[ic];
      ic++;
    }

  p_b0[ic] = 0x0;

  return g_rtval_ex((char *) (arg + (((__d_drt_h ) mppd)->vp_off2)), p_b0,
      max_size, output,
      F_CFGV_BUILD_FULL_STRING);
}

char *
dt_rval_x_size(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (ulint64_t) ((__d_xref) arg)->st.st_size);
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
dt_rval_x_st_mode(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->st.st_mode);
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

#if defined HAVE_STRUCT_STAT_ST_BLKSIZE
char *
dt_rval_x_blksize(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_blksize);
  return output;
}
#endif

#if defined HAVE_STRUCT_STAT_ST_BLOCKS
char *
dt_rval_x_blocks(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint32_t) ((__d_xref) arg)->st.st_blocks);
  return output;
}
#endif

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
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint8_t) ((__d_xref) arg)->uperm);
  return output;
}

char *
dt_rval_x_gperm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint8_t) ((__d_xref) arg)->gperm);
  return output;
}

char *
dt_rval_x_operm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint8_t) ((__d_xref) arg)->operm);
  return output;
}

char *
dt_rval_x_sperm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, (uint8_t) ((__d_xref) arg)->sperm);
  return output;
}

char *
dt_rval_x_perm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, "%hhu%hhu%hhu%hhu", ((__d_xref) arg)->sperm, ((__d_xref) arg)->uperm, ((__d_xref) arg)->gperm, ((__d_xref) arg)->operm);
  return output;
}

char *
dt_rval_x_pbits(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_bitstr((uint64_t) (((__d_xref) arg)->pbits), 12, output);
  return output;
}

char *
dt_rval_x_sparse(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->sparseness);
  return output;
}

char *
dt_rval_x_crc32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->crc32);
  return output;
}

char *
dt_rval_x_deccrc32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, ((__d_xref) arg)->crc32);
  return output;
}

int
opt_xref_depth(void *arg, int m, void *opt)
{
  gfl0 |= F_OPT_DEPTH;
  return 0;
}

int
opt_xref_sl_dat(void *arg, int m, void *opt)
{
  char *buffer = g_pg(arg, m);

  if (NULL == buffer)
    {
      return 24155;
    }

  _d_xref xr_dummy =
    {
      { 0 } };
  _d_drt_h ddr_dummy =
    { 0 };

  xr_dummy.flags = 0x0;

  mda sp_i =
    { 0 };
  md_init(&sp_i, 32);

  int r_c = split_string(buffer, 0x7C, &sp_i);

  if (r_c < 1)
    {
      print_str("ERROR: opt_xref_sl_dat: must contain atleast one argument\n");
      md_g_free(&sp_i);
      return 24156;
    }

  p_md_obj ptr = md_first(&sp_i);

  while (ptr)
    {

      if ( NULL
          == ref_to_val_lk_x((void*) &xr_dummy, (char*) ptr->ptr, NULL, 0,
              (void*) &ddr_dummy))
        {
          print_str("ERROR: opt_xref_sl_dat: invalid option '%s'\n",
              (char*) ptr->ptr);
        }
      ptr = ptr->next;
    }

  xref_flags = xr_dummy.flags;

  md_g_free(&sp_i);

  return 0;
}

void*
ref_to_val_lk_x(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  PROC_SH_EX(match)

  void *ptr;
  if ((ptr = ref_to_val_lk_generic(arg, match, output, max_size, mppd)))
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
          ((__d_xref) arg)->flags |= F_XRF_GET_DT_MODE|F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_mode ,(__d_drt_h)mppd, "%u");
    }
  else if (!strncmp(match, _MC_X_ST_MODE, 6))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_st_mode ,(__d_drt_h)mppd, "%u");
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
#if defined HAVE_STRUCT_STAT_ST_BLKSIZE
  else if (!strncmp(match, _MC_X_BLKSIZE, 7))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_blksize ,(__d_drt_h)mppd, "%u");
    }
#endif
#if defined HAVE_STRUCT_STAT_ST_BLOCKS
  else if (!strncmp(match, _MC_X_BLOCKS, 6))
    {
      if ( arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_blocks ,(__d_drt_h)mppd, "%u");
    }
#endif
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
      return as_ref_to_val_lk(match, dt_rval_x_uperm ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_GPERM, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_GPERM | F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_gperm ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_OPERM, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_OPERM | F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_operm ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_SPERM, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_SPERM | F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_sperm ,(__d_drt_h)mppd, "%hhu");
    }
  else if (!strncmp(match, _MC_X_PERM, 4))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT | F_XRF_GET_GPERM | F_XRF_GET_UPERM | F_XRF_GET_OPERM | F_XRF_GET_SPERM;
        }
      return as_ref_to_val_lk(match, dt_rval_x_perm ,(__d_drt_h)mppd, "%hhu%hhu%hhu%hhu");
    }
  else if (!strncmp(match, _MC_X_PBITS, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT | F_XRF_GET_PBITS;
        }
      return as_ref_to_val_lk(match, dt_rval_x_pbits ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, _MC_X_SPARSE, 6))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT | F_XRF_GET_SPARSE;
        }
      return as_ref_to_val_lk(match, dt_rval_x_sparse ,(__d_drt_h)mppd, "%f");
    }
  else if (!strncmp(match, _MC_X_CRC32, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_CRC32;
        }
      return as_ref_to_val_lk(match, dt_rval_x_crc32 ,(__d_drt_h)mppd, "%.8X");
    }
  else if (!strncmp(match, _MC_X_DCRC32, 9))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_CRC32;
        }
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
  else if (!strncmp(match, _MC_X_DEPTH, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_GET_DEPTH;
        }
      return as_ref_to_val_lk(match, dt_rval_x_depth ,(__d_drt_h)mppd, "%llu");
    }
  else if (!strncmp(match, _MC_X_DATA, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_x_data ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, _MC_X_RLINK, 5))
    {
      if (arg)
        {
          ((__d_xref) arg)->flags |= F_XRF_DO_STAT;
        }
      return as_ref_to_val_lk(match, dt_rval_x_rlink ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, _MC_X_USER, 4))
    {
      int r;
      if ((r=r_preload_guid_data(&((__d_drt_h) mppd)->hdl->uuid_stor, DEFPATH_PASSWD)))
        {
          if ( r == 1 )
            {
              print_str (MSG_GEN_NOFACC, GLROOT, DEFPATH_PASSWD);
            }
          return NULL;
        }
      ((__d_xref) arg)->flags |= F_XRF_DO_STAT;

      return as_ref_to_val_lk(match, dt_rval_x_user ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, _MC_X_GROUP, 5))
    {
      int r;
      if ((r=r_preload_guid_data(&((__d_drt_h) mppd)->hdl->guid_stor, DEFPATH_GROUP)))
        {
          if ( r == 1 )
            {
              print_str (MSG_GEN_NOFACC, GLROOT, DEFPATH_GROUP);
            }
          return NULL;
        }

      ((__d_xref) arg)->flags |= F_XRF_DO_STAT;

      return as_ref_to_val_lk(match, dt_rval_x_group ,(__d_drt_h)mppd, "%s");
    }
  else if (!strncmp(match, "c:", 2))
    {
      ((__d_drt_h ) mppd)->vp_off2 = (size_t)((__d_xref) NULL)->name;
      ((__d_drt_h ) mppd)->match = match+2;
      return dt_rval_c;
    }
  else
    {
      print_str("ERROR: ref_to_val_lk_x: invalid option: '%s'\n", match);
    }

  return NULL;
}

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

int
x_format_block(void *iarg, char *output)
{
  __d_xref ptr = (__d_xref) iarg;
  return print_str("%s\n", ptr->name);
}

void
g_preproc_xhdl(__std_rh ret)
{
  if (gfl & F_OPT_MODE_RAWDUMP)
    {
#ifdef HAVE_ZLIB_H
      if (gfl0 & F_OPT_GZIP)
        {
          g_set_compression_opts(comp_level, &ret->hdl);
          ret->hdl.flags |= F_GH_IO_GZIP;
        }

#endif

      ret->xproc_out = g_omfp_raw;
      if (0 != xref_flags)
        {
          ret->p_xref.flags |= xref_flags;
        }
      else
        {
          ret->p_xref.flags |= F_XRF_ALL_STAT | F_XRF_GET_DT_MODE
              | F_XRF_GET_READ | F_XRF_GET_WRITE | F_XRF_GET_EXEC;
        }
    }
  else if ((gfl & F_OPT_FORMAT_BATCH))
    {
      ret->xproc_out = g_xproc_print;
    }
  else if ((gfl0 & (F_OPT_PRINT | F_OPT_PRINTF)))
    {
      ret->xproc_out = g_omfp_eassemble;
      if ((gfl0 & F_OPT_PRINTF))
        {
          ret->hdl.w_d = g_omfp_write;
        }
      else
        {
          ret->hdl.w_d = g_omfp_write_nl;
        }

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

  if ((ret->flags & F_PD_RECURSIVE))
    {
      if (gfl0 & F_OPT_DEPTH)
        {
          ret->xproc_rcl0 = g_xproc_rc;
        }
      else
        {
          ret->xproc_rcl1 = g_xproc_rc;
        }
    }

  ret->hdl.flags |= F_GH_ISFSX;
  ret->hdl.g_proc1_lookup = ref_to_val_lk_x;
  ret->hdl.jm_offset = (size_t) ((__d_xref ) NULL)->name;
  ret->hdl.g_proc2 = ref_to_val_ptr_x;
  ret->hdl._x_ref = &ret->p_xref;
  ret->hdl.block_sz = sizeof(_d_xref);
  strncpy(ret->hdl.file, "FILESYSTEM", 12);
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

  eds.tp_cb = (void*) tp_default;

  int r = enum_dir(buffer, g_process_directory, &ret, 0, &eds, tp_default);

  if (r < 0)
    {
      print_str("ERROR: %s: %s\n", buffer, ie_tl(r, EMR_enum_dir));
    }

  return r;
}

static void
g_proc_out_once(__std_rh ret, __g_eds eds, void *tp_cb)
{

  int r = enum_dir(ret->root, g_process_directory, (void*) ret, 0, eds, tp_cb);

  if (r < 0)
    {
      print_str("ERROR: %s: %s\n", ret->root, ie_tl(r, EMR_enum_dir));
    }

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

  ret.edsp = &eds;
  ret.flags = flags_udcfg;
  g_preproc_xhdl(&ret);

  if (g_proc_mr(&ret.hdl))
    {
      return 1;
    }

  __d_is_wb w_d_s = ret.hdl.w_d;

  ret.hdl.w_d = ret.hdl.w_d_pr;

  g_do_ppprint(&ret.hdl, F_GH_PRE_PRINT, &ret.hdl.pre_print_mech,
      ret.hdl.g_proc4_pr);

  ret.hdl.w_d = w_d_s;

  if (!(ret.flags & F_PD_MATCHTYPES))
    {
      ret.flags |= F_PD_MATCHTYPES;
    }

  ret.rt_m = 1;

  if (!file_exists(root))
    {
      ret.flags ^= F_PD_MATCHTYPES;
      ret.flags |= F_PD_MATCHREG;
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str("NOTICE: %s is a file\n", root);
        }
      g_process_directory(root, DT_REG, &ret, &eds);
      goto end;
    }

  if ((gfl & F_OPT_CDIRONLY) && !dir_exists(root))
    {
      ret.flags |= F_PD_MATCHTYPES;
      ret.flags ^= F_PD_MATCHTYPES;
      ret.flags |= F_PD_MATCHDIR;
      ret.xproc_rcl0 = NULL;
      ret.xproc_rcl1 = NULL;
      if (ret.flags & F_PD_RECURSIVE)
        {
          ret.flags ^= F_PD_RECURSIVE;
        }
      if (gfl & F_OPT_VERBOSE5)
        {
          print_str("NOTICE: %s is a directory\n", root);
        }
      g_process_directory(root, DT_DIR, &ret, &eds);
      goto end;
    }

  snprintf(ret.root, PATH_MAX, "%s", root);
  remove_repeating_chars(ret.root, 0x2F);

  int r;
  void *tp_cb;

  if (gfl & F_OPT_SORT)
    {
      if (0 != (r = g_f_sort(g_sort_flags, &eds, &ret.hdl)))
        {
          print_str("ERROR: [%d]: sort settings preproc failed!\n", r);
          goto end;
        }

      eds.flags |= F_EDS_SORT;
      if (gfl0 & F_OPT_FSROOT)
        {
          eds.tp_cb = (void*) tp_sorted;
        }
      else
        {
          eds.tp_cb = (void*) tp_default;
        }
      tp_cb = (void*) tp_sorted;
    }
  else
    {
      eds.tp_cb = (void*) tp_default;
      tp_cb = tp_default;
    }

  enter: ;

  if (gfl0 & F_OPT_DEPTH)
    {
      g_proc_out_once(&ret, &eds, tp_cb);
    }

  int xpm_ret = 0;

  if (ret.flags & F_PD_MATCHDIR)
    {
      if (0 == (xpm_ret = g_xproc_m(DT_DIR, ret.root, &ret, &eds)))
        {
          if (ret.xproc_out)
            {
              g_preproc_dm(ret.root, &ret.p_xref, DT_DIR, &ret);
              ret.xproc_out(&ret.hdl, (void*) &ret.p_xref,
                  (void*) ret.p_xref.name);
            }
        }

    }

  if (!(gfl0 & F_OPT_DEPTH))
    {
      if (!((0 != xpm_ret) && (ret.hdl.flags & F_GH_TFD_PROCED)))
        {
          g_proc_out_once(&ret, &eds, tp_cb);
        }
      else
        {
          if (ret.hdl.flags & F_GH_TFD_PROCED)
            {
              ret.hdl.flags ^= F_GH_TFD_PROCED;
            }
        }
    }

  if ((((gfl0 & F_OPT_XRETRY) && !ret.st_1) || (gfl0 & F_OPT_XLOOP))
      && !(gfl & F_OPT_KILL_GLOBAL))
    {
      if (g_sleep)
        {
          sleep(g_sleep);
        }
      if (g_usleep)
        {
          usleep(g_usleep);
        }
      goto enter;
    }

  ret.hdl.w_d = ret.hdl.w_d_po;

  g_do_ppprint(&ret.hdl, F_GH_POST_PRINT, &ret.hdl.post_print_mech,
      ret.hdl.g_proc4_po);

  if (!(gfl & F_OPT_FORMAT_BATCH) && (gfl0 & F_OPT_STATS))
    {
      print_str("STATS: %s: OK: %llu/%llu\n", ret.root,
          (unsigned long long int) ret.st_1,
          (unsigned long long int) ret.st_1 + ret.st_2);
    }

  end:

  g_cleanup(&ret.hdl);

  return ret.rt_m;
}

int
g_preproc_dm(char *name, __d_xref p_xref, unsigned char type, __std_rh aa_rh)
{
  size_t s_l = strlen(name);
  s_l >= PATH_MAX ? s_l = PATH_MAX - 1 : s_l;
  strncpy(p_xref->name, name, s_l);
  p_xref->name[s_l] = 0x0;

  int ret = 0;

  if (p_xref->flags & F_XRF_DO_STAT)
    {
      if (lstat(name, &p_xref->st) == -1)
        {
          bzero(&p_xref->st, sizeof(struct stat));
          ret = 1;
          print_str("ERROR: '%s': could not stat [%s]\n", name,
              g_strerr_r(errno, aa_rh->hdl.strerr_b,
                  sizeof(aa_rh->hdl.strerr_b)));
        }
      else
        {
          if (p_xref->flags & F_XRF_GET_STCTIME)
            {
              p_xref->st.st_ctime = get_file_creation_time(&p_xref->st);
            }
          if (p_xref->flags & F_XRF_GET_UPERM)
            {
              p_xref->uperm = (p_xref->st.st_mode & S_IRWXU) >> 6;
            }
          if (p_xref->flags & F_XRF_GET_GPERM)
            {
              p_xref->gperm = (p_xref->st.st_mode & S_IRWXG) >> 3;
            }
          if (p_xref->flags & F_XRF_GET_OPERM)
            {
              p_xref->operm = (p_xref->st.st_mode & S_IRWXO);
            }
          if (p_xref->flags & F_XRF_GET_SPERM)
            {
              p_xref->sperm = (p_xref->st.st_mode
                  & (S_ISUID | S_ISGID | S_ISVTX)) >> 9;
            }
          if (p_xref->flags & F_XRF_GET_PBITS)
            {
              p_xref->pbits = p_xref->st.st_mode
                  & (S_ISUID | S_ISGID | S_ISVTX | S_IRWXO | S_IRWXG | S_IRWXU);
            }
          if (p_xref->flags & F_XRF_GET_MINOR)
            {
              p_xref->minor = minor(p_xref->st.st_dev);
            }
          if (p_xref->flags & F_XRF_GET_MAJOR)
            {
              p_xref->major = major(p_xref->st.st_dev);
            }
          if (p_xref->flags & F_XRF_GET_SPARSE)
            {
              p_xref->sparseness = file_sparseness(&p_xref->st);
            }
          if (p_xref->flags & F_XRF_GET_DT_MODE)
            {
              p_xref->type = IFTODT(p_xref->st.st_mode);
            }
        }
    }

  if (p_xref->flags & F_XRF_GET_READ)
    {
      p_xref->r = (uint8_t) !(access(p_xref->name, R_OK));
    }
  if (p_xref->flags & F_XRF_GET_WRITE)
    {
      p_xref->w = (uint8_t) !(access(p_xref->name, W_OK));
    }
  if (p_xref->flags & F_XRF_GET_EXEC)
    {
      p_xref->x = (uint8_t) !(access(p_xref->name, X_OK));
    }
  if (p_xref->flags & F_XRF_GET_CRC32)
    {
      file_crc32(p_xref->name, &p_xref->crc32);
    }
  if (p_xref->flags & F_XRF_GET_DEPTH)
    {
      if ( NULL != aa_rh)
        {
          p_xref->depth = (uint64_t) aa_rh->edsp->depth;
        }
    }

  return ret;
}

int
g_xproc_m(unsigned char type, char *name, __std_rh aa_rh, __g_eds eds)
{
  if ((gfl & F_OPT_MINDEPTH) && eds->depth < min_depth)
    {
      return 1;
    }

  g_preproc_dm(name, &aa_rh->p_xref, type, aa_rh);

  if ((g_bmatch((void*) &aa_rh->p_xref, &aa_rh->hdl, &aa_rh->hdl.buffer)))
    {
      aa_rh->st_2++;
      return 2;
    }

  omfp_timeout;

  aa_rh->rt_m = 0;
  aa_rh->st_1++;
  return 0;
}

void
g_xproc_rc(char *name, void *aa_rh, __g_eds eds)
{
  if (!((gfl & F_OPT_MAXDEPTH) && eds->depth >= max_depth))
    {
      int r = enum_dir(name, g_process_directory, aa_rh, 0, eds,
          (__d_edir ) eds->tp_cb);
      if (r < 0)
        {
          print_str("ERROR: %s: %s\n", name, ie_tl(r, EMR_enum_dir));
        }
    }
}

int
g_process_directory(char *name, unsigned char type, void *arg, __g_eds eds)
{
  __std_rh aa_rh = (__std_rh) arg;

  switch (type)
    {
      case DT_DIR:;

      if ((gfl & F_OPT_KILL_GLOBAL) )
        {
          break;
        }

      if (NULL != aa_rh->xproc_rcl0)
        {
          aa_rh->xproc_rcl0(name, (void*)aa_rh, eds);
        }

      int ret = 0;

      if (aa_rh->flags & F_PD_MATCHDIR)
        {
          if (0 == (ret = g_xproc_m(type, name, aa_rh, eds)))
            {
              if (aa_rh->xproc_out)
                {
                  aa_rh->xproc_out(&aa_rh->hdl, (void*) &aa_rh->p_xref, (void*)aa_rh->p_xref.name);
                }
            }

        }

      if (NULL != aa_rh->xproc_rcl1)
        {
          if ( !((0 != ret) && (aa_rh->hdl.flags & F_GH_TFD_PROCED)))
            {
              aa_rh->xproc_rcl1(name, (void*)aa_rh, eds);
            }
          else
            {
              if ( aa_rh->hdl.flags & F_GH_TFD_PROCED)
                {
                  aa_rh->hdl.flags ^= F_GH_TFD_PROCED;
                }
            }
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
                  print_str("ERROR: %s -> %s [%s]\n", name, b_spl,
                      g_strerr_r(errno, aa_rh->hdl.strerr_b, sizeof(aa_rh->hdl.strerr_b)));
                  break;
                }

              uint8_t dt_mode = IFTODT(aa_rh->p_xref.st.st_mode);

              if (dt_mode == DT_DIR && (p_spl=strstr(name, b_spl)) && p_spl == name)
                {
                  print_str("ERROR: %s: filesystem loop detected inside '%s'\n", name, b_spl);
                  break;
                }

              g_process_directory(name, dt_mode, arg, eds);
            }
          else
            {
              if ( b_spl_l == -1 )
                {
                  print_str("ERROR: %s: [%s] could not read link\n", name,
                      g_strerr_r(errno, aa_rh->hdl.strerr_b, sizeof(aa_rh->hdl.strerr_b)));
                }
            }

        }
      break;
      default:;
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
    }

  return 0;
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

static int
e_comp_dir(const struct dirent **a, const struct dirent **b)
{

  if ((*a)->d_type == DT_DIR)
    {
      return 1;
    }

  return 0;
}

int
enum_dir(char *dir, __d_edscb callback_f, void *arg, int f, __g_eds eds,
    __d_edir point_cb)
{

  int r = 0;

  DIR *dp = opendir(dir);

  if (NULL == dp)
    {
      return -2;
    }

  if (eds)
    {
      int fddp = dirfd(dp);

      if (-1 == fddp)
        {
          r = -4;
          goto end;
        }

      if (fstat(fddp, &eds->st))
        {
          r = -3;
          goto end;
        }

      if (!(eds->flags & F_EDS_ROOTMINSET))
        {
          eds->r_minor = minor(eds->st.st_dev);
          eds->flags |= F_EDS_ROOTMINSET;
        }

      if (!(f & F_ENUMD_NOXBLK) && (gfl & F_OPT_XBLK)
          && major(eds->st.st_dev) != 8)
        {
          r = 0;
          goto end;
        }

      if ((gfl & F_OPT_XDEV) && minor(eds->st.st_dev) != eds->r_minor)
        {
          r = 0;
          goto end;
        }

      eds->depth++;
    }

  r = point_cb(dir, callback_f, arg, f, eds, dp);

  if (eds)
    {
      eds->depth--;
    }

  end:

  closedir(dp);
  return r;
}

int
g_f_sort(uint32_t flags, __g_eds eds, __g_handle hdl)
{
  if (NULL == g_sort_field)
    {
      return 2;
    }

  switch (gfl0 & F_OPT_SMETHOD)
    {
  case F_OPT_SMETHOD_HEAP:
    eds->sort_proc = g_heapsort;
    break;
  case F_OPT_SMETHOD_Q:
    eds->sort_proc = g_qsort;
    eds->srd.off_right = 1;
    break;
  default:
    eds->sort_proc = g_qsort;
    eds->srd.off_right = 1;
    break;
    }

  g_invert_sort_order(&flags);

  eds->srd.mppd.hdl = hdl;

  int ret;

  if (!(flags & F_GSORT_TYPE))
    {
      if (!g_check_is_data_numeric(hdl, g_sort_field))
        {
          flags |= F_GSORT_NUMERIC;
        }
      else
        {
          flags |= F_GSORT_STRING;
        }
    }

  switch (flags & F_GSORT_TYPE)
    {
  case F_GSORT_NUMERIC:
    ;
    ret = g_sort_numeric(hdl, g_sort_field, flags, &eds->srd);
    break;
  case F_GSORT_STRING:
    ;
    ret = g_sort_string(hdl, g_sort_field, flags, &eds->srd);
    break;
  default:
    return 11;
    }

  if (0 != ret)
    {
      return ret;
    }

  return ret;
}

static __d_xref *
build_xarr(char *dir, struct dirent **namelist, int n, __std_rh aa_rh,
    char *field)
{
  struct dirent *dirp;
  __d_xref *x_arr = calloc(n, sizeof(void*));

  char buf[PATH_MAX];

  while (n--)
    {
      dirp = namelist[n];
      x_arr[n] = calloc(1, sizeof(_d_xref));
      x_arr[n]->flags |= ((__d_xref ) aa_rh->hdl._x_ref)->flags;
      x_arr[n]->dirp = dirp;
      snprintf(buf, PATH_MAX, "%s/%s", dir, dirp->d_name);
      remove_repeating_chars(buf, 0x2F);
      g_preproc_dm(buf, x_arr[n], dirp->d_type, aa_rh);
    }

  return x_arr;
}

static void
tp_clean_xarr(__d_xref *x_arr, int n)
{
  while (n--)
    {
      free(x_arr[n]->dirp);
      free(x_arr[n]);
    }
}

int
tp_sorted(char *dir, __d_edscb callback_f, void *arg, int f, __g_eds eds,
    DIR *dp)
{
  struct dirent _dirp, *dirp;
  char buf[PATH_MAX];

  int r = 0, ir;

  __std_rh aa_rh = (__std_rh) arg;

  struct dirent **namelist;

  int n = scandir(dir, &namelist, NULL, e_comp_dir);

  if (n == -1)
    {
      return -3;
    }

  __d_xref *x_arr = build_xarr(dir, namelist, n, aa_rh, g_sort_field);

  free(namelist);

  eds->srd.sp_0 = dir;
  eds->sort_proc((void**) x_arr, 0, ((int64_t) n) - eds->srd.off_right,
      &eds->srd);

  while (n--)
    {
      if ((gfl & F_OPT_KILL_GLOBAL) || (gfl & F_OPT_TERM_ENUM))
        {
          tp_clean_xarr(x_arr, n);
          break;
        }

      dirp = x_arr[n]->dirp;
      size_t d_name_l = strlen(dirp->d_name);

      if ((d_name_l == 1 && dirp->d_name[0] == 0x2E)
          || (d_name_l == 2 && dirp->d_name[0] == 0x2E
              && dirp->d_name[1] == 0x2E))
        {
          free(dirp);
          free(x_arr[n]);
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

      free(dirp);
      free(x_arr[n]);

      if (!(ir = callback_f(buf, _dirp.d_type, arg, eds)))
        {
          if (f & F_ENUMD_ENDFIRSTOK)
            {
              tp_clean_xarr(x_arr, n);
              r = 0;
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
              tp_clean_xarr(x_arr, n);
              //r = ir;
              break;
            }
        }
    }

  free(x_arr);

  return r;
}

int
tp_default(char *dir, __d_edscb callback_f, void *arg, int f, __g_eds eds,
    DIR *dp)
{
  struct dirent _dirp, *dirp;
  char buf[PATH_MAX];

  int r = 0, ir;

  while ((dirp = readdir(dp)))
    {
      //dirp = namelist[n];
      if ((gfl & F_OPT_KILL_GLOBAL) || (gfl & F_OPT_TERM_ENUM))
        {
          break;
        }

      size_t d_name_l = strlen(dirp->d_name);

      if ((d_name_l == 1 && dirp->d_name[0] == 0x2E)
          || (d_name_l == 2 && dirp->d_name[0] == 0x2E
              && dirp->d_name[1] == 0x2E))
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

      //free(dirp);

      if (!(ir = callback_f(buf, _dirp.d_type, arg, eds)))
        {
          if (f & F_ENUMD_ENDFIRSTOK)
            {
              r = 0;
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
              //r = ir;
              break;
            }
        }
    }

  return r;
}

float
file_sparseness(const struct stat *p)
{
#if defined HAVE_STRUCT_STAT_ST_BLOCKS
  if (0 == p->st_size)
    {
      if (0 == p->st_blocks)
        return 1.0;
      else
        return p->st_blocks < 0 ? -HUGE_VAL : HUGE_VAL;
    }
  else
    {
      double blklen = ST_NBLOCKSIZE * (double) p->st_blocks;
      return (float) (blklen / p->st_size);
    }
#else
  return 1.0;
#endif
}
