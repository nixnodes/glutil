/*
 * log_io.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>

#include <log_shm.h>
#include <log_io.h>
#include <memory_t.h>
#include <l_error.h>
#include <log_op.h>
#include <x_f.h>
#include <m_general.h>
#include <exec_t.h>
#include <log_shm.h>
#include <sort_hdr.h>
#include <g_modes.h>
#include <misc.h>
#include <fp_types.h>
#include <str.h>
#include <exech.h>
#include <errno_int.h>

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#ifdef HAVE_ZLIB_H

#include <zlib.h>

#endif

long long int db_max_size = DB_MAX_SIZE;

static _emr EMR_m_load_input_n[] =
  {
    { .code = -1, .err_msg = _E_MSG_MLI_NOREF, .has_errno = 0 },
    { .code = -2, .err_msg = _E_MSG_MLI_NOMEMB, .has_errno = 0 },
    { .code = -9, .err_msg = _E_MSG_MLI_NORES, .has_errno = 0 },
    { .code = 1, .err_msg = _E_MSG_MLI_UTERM, .has_errno = 0 },
    { .code = 3, .err_msg = _E_MSG_MLI_MALF, .has_errno = 0 },
    { .code = 0, .err_msg = _E_MSG_MLI_DEF, .has_errno = 0 } };

static _emr EMR_flush_data_md[] =
  {
    { .code = 2, .err_msg = _E_MSG_FDM_NOW, .has_errno = 0 },
    { .code = 8, .err_msg = _E_MSG_FDM_NOWC, .has_errno = 0 },
    { .code = 14, .err_msg = _E_MSG_FDM_WF, .has_errno = 0 },
    { .code = 13, .err_msg = _E_MSG_FDM_WFC, .has_errno = 0 },
    { .code = 5, .err_msg = _E_MSG_FDM_NORECW, .has_errno = 0 },
    { .code = 0, .err_msg = _E_MSG_FDM_DEF, .has_errno = 0 } };

#ifdef HAVE_ZLIB_H
static void
g_set_compression_opts(uint8_t level, __g_handle hdl)
{
  snprintf(hdl->w_mode, sizeof(hdl->w_mode), "wb%hhu", level);
}
#endif

int
g_fopen(char *file, char *mode, uint32_t flags, __g_handle hdl)
{
  g_setjmp(0, "g_fopen", NULL, NULL);
  int r = 0;
  uint8_t b = 0;
  if (flags & F_DL_FOPEN_SHM)
    {
      if ((r = g_map_shm(hdl, SHM_IPC)))
        {
          return r;
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
      if (!(r = g_buffer_into_memory(file, hdl)))
        {
          if (g_proc_mr(hdl))
            {
              return 24;
            }

          if (!(flags & F_DL_FOPEN_FILE))
            {
              return 0;
            }
          b = 1;
        }
      else
        {
          if (!(hdl->flags & F_GH_NOMEM))
            {
              return r;
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

  strncpy(hdl->file, file, strlen(file));

#ifdef HAVE_ZLIB_H
  if (gfl0 & F_OPT_GZIP)
    {
      hdl->flags |= F_GH_IO_GZIP;
      g_set_compression_opts(comp_level, hdl);
    }

  gzFile gz_fh;
  if ((gz_fh = gzopen(file, "rb")) != NULL)
    {
      if (gzdirect(gz_fh) == 0)
        {
          hdl->flags |= F_GH_IS_GZIP;
        }
    }

  gzclose(gz_fh);
#endif

  FILE *fd;

  if (b != 1)
    {
      if (determine_datatype(hdl, hdl->file))
        {
          print_str(MSG_GEN_NODFILE, file, "could not determine data-type");
          return 3;
        }

      if (!(gfl0 & F_OPT_STDIN))
        {
          hdl->total_sz = get_file_size(hdl->file);

          if (!hdl->total_sz)
            {
              print_str(MSG_GEN_NODFILE, file, "zero-byte data file");
            }

          if (!(hdl->flags & F_GH_IO_GZIP) && !(hdl->flags & F_GH_IS_GZIP))
            {
              if (hdl->total_sz % hdl->block_sz)
                {
                  print_str(
                  MSG_GEN_DFCORRUW, file, (ulint64_t) hdl->total_sz,
                      hdl->block_sz);
                }
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

    }
  else
    {
      if (!(gfl0 & F_OPT_STDIN))
        {
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
    }

  hdl->fh = fd;
#ifdef HAVE_ZLIB_H
  if (hdl->flags & F_GH_IS_GZIP)
    {
      if (NULL == strchr(mode, 0x2B))
        {
          if ((hdl->gz_fh = gzdopen(dup(fileno(hdl->fh)), mode)) == NULL)
            {
              return 36;
            }
        }
      else
        {
          print_str(
              "ERROR: mode '%s': reading and writing to the same gzip file is not supported\n",
              mode);
          return 37;
        }
    }
#endif

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

  if (__pf_eof(hdl->fh))
    {
      return NULL;
    }

  size_t fr;

#ifdef HAVE_ZLIB_H
  if ((fr = gzread(hdl->gz_fh, buffer, size)) != size)
#else
  if ((fr = fread(buffer, 1, size, hdl->fh)) != size)
#endif
    {
      if (fr == 0 ||
#ifdef HAVE_ZLIB_H
          gzeof(hdl->gz_fh)
#else
              feof(hdl->fh) || ferror(hdl->fh)
#endif
              )
        {
          return NULL;
        }
      return NULL;
    }

  hdl->br += fr;
  hdl->offset++;

  return buffer;
}

int
g_close(__g_handle hdl)
{
  g_setjmp(0, "g_close", NULL, NULL);
  bzero(&dl_stats, sizeof(struct d_stats));
  dl_stats.br += hdl->br;
  dl_stats.bw += hdl->bw;
  dl_stats.rw += hdl->rw;

#ifdef HAVE_ZLIB_H
  if (hdl->gz_fh)
    {
      gzclose(hdl->gz_fh);
    }
  hdl->gz_fh = NULL;
  if (hdl->gz_fh1)
    {
      gzclose(hdl->gz_fh1);
    }
  hdl->gz_fh1 = NULL;
#endif

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

int
g_cleanup(__g_handle hdl)
{
  int r = 0;

  r += md_g_free(&hdl->buffer);
  r += md_g_free(&hdl->w_buffer);
  r += md_g_free(&hdl->print_mech);
  r += md_g_free(&hdl->_accumulator);
  r += md_g_free(&hdl->uuid_stor);
  r += md_g_free(&hdl->guid_stor);

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
          md_g_free(&g_ptr->dtr.math);
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
          md_g_free_cb(&ach->mech, g_claf_mech);
          ptr = ptr->next;
        }

      if (hdl->exec_args.argv_c)
        {
          free(hdl->exec_args.argv_c);
        }

      r += md_g_free(&hdl->exec_args.ac_ref);
    }

  md_g_free_cb(&hdl->exec_args.mech, g_claf_mech);

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

int
g_claf_mech(void *ptr)
{
  __d_exec_ch ach = (__d_exec_ch) ptr;
  regfree(&ach->dtr.preg);
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
#ifdef HAVE_ZLIB_H
          gzrewind(hdl->gz_fh);
#endif
          rewind(hdl->fh);

          hdl->offset = 0;
        }
    }
  return 0;
}

static int
g_load_data_check_of(void *f, void *nf, __g_handle hdl)
{

  uint8_t _xof = 0;

#ifdef HAVE_ZLIB_H
  if (hdl->flags & F_GH_IS_GZIP)
    {
      if (f == NULL)
        {
          return 1;
        }
      _xof = gzeof((gzFile) f);
    }
  else
    {
      _xof = (feof((FILE*) nf) || ferror((FILE*) nf));
    }
#else
  if (f == NULL)
    {
      return 1;
    }
  _xof = (feof((FILE*) f) || ferror((FILE*) f));
#endif
  return _xof;
}

size_t
g_load_data_md(void *output, size_t max, char *file, __g_handle hdl)
{
  g_setjmp(0, "g_load_data_md", NULL, NULL);
  size_t fr = 0;
  off_t c_fr = 0;
  FILE *fh;
#ifdef HAVE_ZLIB_H
  gzFile gz_fh = NULL;
#endif

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      if (NULL == (fh = fopen(file, "rb")))
        {
          return 0;
        }
    }
  else
    {
      fh = stdin;
    }

#ifdef HAVE_ZLIB_H

  if ((gz_fh = gzdopen(dup(fileno(fh)), "rb")) == NULL)
    {
      return 0;
    }

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      if (gzdirect(gz_fh) == 0)
        {
          hdl->flags |= F_GH_IS_GZIP;
        }
      gzrewind(gz_fh);
    }
  else
    {
      hdl->flags |= F_GH_IS_GZIP;
    }
#endif

  uint8_t *b_output = (uint8_t*) hdl->data;
#ifdef HAVE_ZLIB_H
  while (!g_load_data_check_of((void*) gz_fh, fh, hdl))
#else
  while (!g_load_data_check_of((void*) fh, NULL, hdl))
#endif
    {

      if ((hdl->flags & F_GH_FROMSTDIN) || (hdl->flags & F_GH_IS_GZIP))
        {
          if (!fr && !(hdl->total_sz - c_fr))
            {
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
#ifdef HAVE_ZLIB_H
      if (hdl->flags & F_GH_IS_GZIP)
        {

          fr = gzread(gz_fh, &b_output[c_fr], hdl->total_sz - c_fr);
          if (fr == -1)
            {
              c_fr = 0;
              break;
            }

        }
      else
        {
          fr = fread(&b_output[c_fr], 1, hdl->total_sz - c_fr, fh);
        }

#else
      fr = fread(&b_output[c_fr], 1, hdl->total_sz - c_fr, fh);
#endif

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
#ifdef HAVE_ZLIB_H
      if (gz_fh)
        {
          gzclose(gz_fh);
        }
#endif
      fclose(fh);
    }

  return c_fr;
}

int
load_data_md(pmda md, char *file, __g_handle hdl)
{
  g_setjmp(0, "load_data_md", NULL, NULL);
  errno = 0;
  int r = 0;
  off_t count = 0;

  if (!hdl->block_sz)
    {
      return 20102;
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

      if (!hdl->shmcflags)
        {
          hdl->shmcflags = S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP;
        }

      hdl->data = shmap(hdl->ipc_key, &hdl->ipcbuf, (size_t) hdl->total_sz,
          &sh_ret, &hdl->shmid, hdl->shmcflags, hdl->shmatflags);

      if (sh_ret & R_SHMAP_FAILED_ATTACH)
        {
          return 20103;
        }
      if (sh_ret & R_SHMAP_FAILED_SHMAT)
        {
          return 20104;
        }
      if (!hdl->data)
        {
          return 20105;
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
          return 20106;
        }
      uint32_t bdiff;

      if (hdl->total_sz < hdl->block_sz)
        {
          hdl->total_sz = hdl->block_sz;
        }
      else if ((bdiff = (hdl->total_sz % hdl->block_sz)))
        {
          hdl->total_sz -= bdiff;
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
              % hdl->block_sz)
            {
              md_g_free(md);
              if (!b_read)
                {
                  return 20109;
                }
              return 20110;
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
      return 20107;
    }

  md->flags |= F_MDA_REFPTR;
  cb(hdl, md, count);

  if (gfl0 & F_OPT_ARR_DIST)
    {
      md->flags |= F_MDA_ARR_DIST;
      md_relink_n(md, 100);
    }

  g_setjmp(0, "load_data_md", NULL, NULL);

  if (!md->count)
    {
      return 20108;
    }

  return 0;
}

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

#ifdef HAVE_ZLIB_H
  if (gfl0 & F_OPT_GZIP)
    {
      hdl->flags |= F_GH_IO_GZIP;
      g_set_compression_opts(comp_level, hdl);
    }

  gzFile gz_fh;
  if ((gz_fh = gzopen(file, "rb")) != NULL)
    {
      if (gzdirect(gz_fh) == 0)
        {
          hdl->flags |= F_GH_IS_GZIP;
        }
    }

  gzclose(gz_fh);

#endif

  struct stat st =
    { 0 };

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      if (stat(file, &st) == -1)
        {
          if (!(gfl & F_OPT_SHAREDMEM))
            {
              print_str("ERROR: %s: [%s]\n", file, strerror(errno));
              return 20201;
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

          /*if (st.st_size > db_max_size)
           {
           print_str(
           "WARNING: %s: disabling memory buffering, file too big (%lld MB max)\n",
           file, db_max_size / 1048576);
           hdl->flags |= F_GH_NOMEM;
           return 20202;
           }*/

          hdl->total_sz = st.st_size;
        }
    }
  else
    {
      if ((gfl & F_OPT_SHAREDMEM))
        {
          //flags |= F_GBM_SHM_NO_DATAFILE;
        }
    }

  if (!hdl->file[0])
    {
      strcp_s(hdl->file, PATH_MAX, file);
    }

  if (!(hdl->flags & F_GH_ISTYPE))
    {
      if (determine_datatype(hdl, hdl->file))
        {
          print_str(MSG_BAD_DATATYPE, file);
          return 20203;
        }
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
      if (!(hdl->flags & F_GH_IS_GZIP))
        {
          if (st.st_size % hdl->block_sz)
            {
              print_str(MSG_GEN_DFCORRUW, file, (ulint64_t) st.st_size,
                  hdl->block_sz);
            }
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
                          "WARNING: %s: [IPC: 0x%.8X] [%s] unable to destroy shared memory segment\n",
                          hdl->file, hdl->ipc_key, strerror(errno));
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
                  "ERROR: %s: [IPC: 0x%.8X]: [%s]: could not get shared memory segment information from kernel\n",
                  hdl->file, hdl->ipc_key, strerror(errno));
              return 20205;
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
              "ERROR: %s: [IPC: 0x%.8X]: failed loading data into shared memory segment: [%s]: no shared memory segment or data file available to load\n",
              hdl->file, hdl->ipc_key, strerror(errno));
          return 20206;
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
          print_str("NOTICE: %s: %s [%llu kB]\n", hdl->file,
              (hdl->flags & F_GH_SHM) ?
                  hdl->shmid == -1 && !(hdl->flags & F_GH_SHMRB) ?
                      "loading data into shared memory segment" :
                  (hdl->flags & F_GH_SHMRB) ?
                      "re-loading data into shared memory segment" :
                      "mapping shared memory segment"
                  :
                  "loading data file into memory",

              (hdl->flags & F_GH_SHM) && hdl->shmid != -1 ?
                  (ulint64_t) hdl->ipcbuf.shm_segsz / 1024 :
                  (ulint64_t) hdl->total_sz / 1024);

        }
    }

  errno = 0;
  int r;
  if ((r = load_data_md(&hdl->buffer, hdl->file, hdl)))
    {
      print_str(
          "ERROR: %s: [%llu/%llu] [%llu] [%u] could not load data!%s [%d] [%s]\n",
          hdl->file, (ulint64_t) hdl->buffer.count,
          (ulint64_t) (hdl->total_sz / hdl->block_sz), hdl->total_sz,
          hdl->block_sz,
          (hdl->flags & F_GH_SHM) ? " [shared memory segment]" : "", r,
          strerror(errno));

      return 20209;
    }
  else
    {
      if (!(hdl->flags & F_GH_IS_GZIP) && !(flags & F_GBM_SHM_NO_DATAFILE)
          && !(hdl->flags & F_GH_FROMSTDIN))
        {
          if (hdl->total_sz < tot_sz)
            {
              print_str(
                  "WARNING: %s: [%llu/%llu] actual data loaded was not the same size as source data file\n",
                  hdl->file, (uint64_t) hdl->total_sz, (uint64_t) tot_sz);
              if (!(gfl & F_OPT_FORCE))
                {
                  gfl |= F_OPT_LOADQ;
                }
            }

        }
      if (gfl & F_OPT_VERBOSE2)
        {
#ifdef HAVE_ZLIB_H
          if (hdl->flags & F_GH_IS_GZIP)
            {
              print_str(
                  "NOTICE: %s: loaded %llu records [compression ratio: %f, %llu/%llu kB]\n",
                  hdl->file, (uint64_t) hdl->buffer.offset,
                  (double) hdl->total_sz / (double) tot_sz,
                  (unsigned long long int) hdl->total_sz / 1024,
                  (unsigned long long int) tot_sz / 1024);
            }
          else
            {
              print_str(MSG_LL_RC, hdl->file, (uint64_t) hdl->buffer.offset);
            }
#else
          print_str(MSG_LL_RC, hdl->file, (uint64_t) hdl->buffer.offset);
#endif
        }
    }
  return 0;
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

int
determine_temp_path(char *file, char *output, size_t max_out)
{
  char *f_n = g_basename(file);
  if (!strlen(f_n))
    {
      return 1;
    }

  snprintf(output, max_out, "%s.%d.dtm", file, getpid());

  FILE *fh;

  if (!(fh = fopen(output, "w")))
    {
      snprintf(output, max_out, "/tmp/glutil_%s.%d.dtm", f_n, getpid());
    }
  else
    {
      fclose(fh);
      remove(output);
    }

  return 0;
}

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

  if (determine_temp_path(file, hdl->s_buffer, PATH_MAX))
    {
      print_str("ERROR: %s: unable to get a writable temp. path\n", file);
    }

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

#ifdef HAVE_ZLIB_H
  /*  if (hdl->flags & F_GH_D_WRITE)
   {
   gzFile gz_fh;
   if ((gz_fh = gzopen(file, "r")) == NULL)
   {
   goto skip;
   }

   int gzd = gzdirect(gz_fh);

   gzclose(gz_fh);

   if ((gfl0 & F_OPT_GZIP))
   {
   if (gzd == 1)
   {
   if (hdl->flags & F_GH_IO_GZIP)
   {
   hdl->flags ^= F_GH_IO_GZIP;
   gfl0 ^= F_OPT_GZIP;
   }
   if (gfl & F_OPT_VERBOSE2)
   {
   print_str("WARNING: %s: target is not compressed, forcing gzip off\n",
   file);
   }
   }
   else if (gzd == 0)
   {
   RDF_SFAC(hdl, comp_level)
   }
   }
   else
   {
   if (gzd == 0)
   {
   if (gfl & F_OPT_VERBOSE2)
   {
   print_str("WARNING: %s: target is compressed, forcing gzip on\n",
   file);
   }
   comp_level = 2;
   RDF_SFAC(hdl, comp_level)
   } else {
   RDF_SFAC(hdl, comp_level)
   }
   }

   }
   else
   {
   skip:*/
  if (gfl0 & F_OPT_GZIP)
    {
      hdl->flags |= F_GH_IO_GZIP;
      g_set_compression_opts(comp_level, hdl);
    }

#endif

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

  if (gfl & F_OPT_VERBOSE4)
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

  if (gfl & F_OPT_VERBOSE3)
    {
      print_str("NOTICE: %s: using mode %o\n", hdl->s_buffer, hdl->st_mode);
#ifdef HAVE_ZLIB_H
      if ((hdl->flags & F_GH_IO_GZIP))
        {
          print_str("NOTICE: %s: gzip compression enabled, level '%hhu'\n",
              hdl->s_buffer, comp_level);
        }
#endif
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
          print_str("ERROR: 'flush_data_md': %s: [%d]: %s\n", hdl->s_buffer, r,
              ie_tl(r, EMR_flush_data_md));
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

  sz_r = get_file_size(hdl->s_buffer);

  if (gfl & F_OPT_VERBOSE2)
    {
#ifdef HAVE_ZLIB_H
      uint64_t n0_sz = (ulint64_t) (hdl->bw - bw_o);
      if ((hdl->flags & F_GH_IO_GZIP))
        {
          print_str(
              "NOTICE: %s: flushed %llu records, %llu kB [compression ratio: %f, %llu/%llu kB]\n",
              hdl->file, (ulint64_t) (hdl->rw - rw_o), sz_r / 1024,
              (double) n0_sz / (double) sz_r,
              (unsigned long long int) n0_sz / 1024,
              (unsigned long long int) sz_r / 1024);
        }
      else
        {
          print_str(MSG_GEN_FLUSHED, hdl->s_buffer,
              (ulint64_t) (hdl->rw - rw_o), (ulint64_t) (hdl->bw - bw_o));
        }
#else
      print_str(MSG_GEN_FLUSHED, hdl->s_buffer, (ulint64_t) (hdl->rw - rw_o),
          (ulint64_t) (hdl->bw - bw_o));
#endif
    }

  g_setjmp(0, "rebuild_data_file(5)", NULL, NULL);

  if (!(hdl->flags & F_GH_IO_GZIP) && !(gfl & F_OPT_NOWRITE)
      && sz_r < hdl->block_sz)
    {
      print_str(
          "WARNING: %s: [%u/%u] generated data file is smaller than a single record\n",
          hdl->s_buffer, (uint32_t) sz_r, (uint32_t) hdl->block_sz);
    }

  g_setjmp(0, "rebuild_data_file(6)", NULL, NULL);

  if (!(gfl & F_OPT_NOWRITE)
      && !((hdl->flags & F_GH_WAPPEND) && (hdl->flags & F_GH_DFWASWIPED)))
    {
      if (!(hdl->flags & F_GH_NO_BACKUP) && data_backup_records(file))
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
#ifdef HAVE_ZLIB_H
          if (hdl->gz_fh)
            {
              gzclose(hdl->gz_fh);
            }
          hdl->gz_fh = NULL;
#endif
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
          if (strncmp(hdl->s_buffer, "/tmp/", 5))
            {
              if ((r = rename(hdl->s_buffer, file)))
                {
                  print_str("ERROR: %s: [%s] renaming temporary file failed!\n",
                      hdl->s_buffer, strerror(errno));
                  ret = 4;
                }
              goto end;
            }
          else
            {
              errno = 0;
              if ((r = (int) file_copy(hdl->s_buffer, file, "wb",
              F_FC_MSET_SRC)) < 1)
                {
                  print_str("ERROR: %s: [%d] [%s] copying temp file failed!\n",
                      hdl->s_buffer, r, strerror(errno));
                  ret = 21;
                }
            }

        }

      cleanup:

      if ((r = remove(hdl->s_buffer)))
        {
          print_str(
              "WARNING: %s: [%s] deleting temporary file failed (remove manually)\n",
              hdl->s_buffer, strerror(errno));
          ret = 5;
        }

    }

  end:

  return ret;
}

int
flush_data_md(__g_handle hdl, char *outfile)
{
  g_setjmp(0, "flush_data_md", NULL, NULL);

  if (!(gfl & F_OPT_MODE_RAWDUMP) && (gfl & F_OPT_NOWRITE))
    {
      return 0;
    }

  FILE *fh = NULL;
#ifdef HAVE_ZLIB_H
  gzFile gz_fh = NULL;
#endif
  size_t bw = 0;
  unsigned char *buffer = NULL;
  char *mode = "wb";

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

  if (!(gfl & F_OPT_MODE_RAWDUMP))
    {
      if ((fh = fopen(outfile, mode)) == NULL)
        {
          return 2;
        }
    }
  else
    {
      fh = stdout;
    }

#ifdef HAVE_ZLIB_H
  if (hdl->flags & F_GH_IO_GZIP)
    {
      if ((gz_fh = gzdopen(dup(fileno(fh)), hdl->w_mode)) == NULL)
        {
          return 8;
        }
    }

#endif

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
#ifdef HAVE_ZLIB_H
      if (hdl->flags & F_GH_IO_GZIP)
        {
          if ((bw = gzwrite(gz_fh, ptr->ptr, hdl->block_sz)) != hdl->block_sz)
            {
              ret = 13;
              break;
            }
        }
      else
        {

          if ((bw = fwrite(ptr->ptr, hdl->block_sz, 1, fh)) != 1)
            {

              ret = 14;
              break;
            }
        }
#else
      if ((bw = fwrite(ptr->ptr, hdl->block_sz, 1, fh)) != 1)
        {
          ret = 14;
          break;
        }
#endif

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
  fflush(fh);
#ifdef HAVE_ZLIB_H

  if (hdl->flags & F_GH_IO_GZIP)
    {
      if (gz_fh)
        {
          gzclose(gz_fh);
        }
    }
#endif

  if (!(gfl & F_OPT_MODE_RAWDUMP))
    {
      if (fh)
        {
          fclose(fh);
        }

      if (hdl->st_mode)
        {
          chmod(outfile, hdl->st_mode);
        }
    }

  return ret;
}

static void
d_write_stats(char *datafile)
{
  if (((gfl0 & F_OPT_STATS) || (gfl & F_OPT_VERBOSE4))
      && !(gfl0 & F_OPT_NOSTATS))
    {

#ifdef HAVE_ZLIB_H
      if (g_act_1.flags & F_GH_IO_GZIP)
        {
          fprintf(stderr, MSG_GEN_WROTE2, datafile,
              (double) get_file_size(datafile) / 1024.0,
              (double) g_act_1.bw / 1024.0,
              (long long unsigned int) g_act_1.rw);
        }
      else
        {
          OPLOG_OUTPUT_NSTATS(datafile, g_act_1)
        }
#else
      OPLOG_OUTPUT_NSTATS(datafile, g_act_1)
#endif
    }
}

int
d_write(char *arg)
{
  g_setjmp(0, "d_write", NULL, NULL);

  int ret = 0;

  if (NULL == arg)
    {
      print_str("ERROR: "MSG_GEN_MISSING_DTARG" (-z <log>)\n");
      return 1;
    }

  char *a_ptr = (char*) arg;
  char *datafile = g_dgetf(a_ptr);

  if (!datafile)
    {
      print_str(MSG_UNRECOGNIZED_DATA_TYPE, a_ptr);
      return 2;
    }

  //off_t f_sz = get_file_size(datafile);

  g_act_1.flags |= F_GH_FFBUFFER | F_GH_D_WRITE | F_GH_WAPPEND | F_GH_DFNOWIPE;

#ifdef HAVE_ZLIB_H
  if (gfl0 & F_OPT_GZIP)
    {
      g_act_1.flags |= F_GH_IO_GZIP;
      g_set_compression_opts(comp_level, &g_act_1);
    }
#endif

  /*if (f_sz > 0)
   {
   g_act_1.flags |= F_GH_WAPPEND | F_GH_DFNOWIPE;
   }*/

  strncpy(g_act_1.file, datafile, strlen(datafile));

  if (determine_datatype(&g_act_1, g_act_1.file))
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
      sprintf(infile_p, "stdin");
      g_act_1.flags |= F_GH_FROMSTDIN;
    }

  int r;

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: %s: loading input data..\n", infile_p);
    }

  if ((r = g_proc_mr(&g_act_1)))
    {
      goto end;
    }

  if (!(gfl & F_OPT_MODE_BINARY))
    {
      g_act_1.gcb_post_proc = g_d_post_proc_gcb;

      data_backup_records(datafile);

      g_act_1.flags |= F_GH_NO_BACKUP;

      if ((r = m_load_input_n(&g_act_1, in)))
        {
          print_str("ERROR: DATA IMPORT: %s: [%d]: %s\n", datafile, r,
              ie_tl(r, EMR_m_load_input_n));
          ret = 5;
          goto end;
        }

      if (g_act_1.w_buffer.offset > 0 && rebuild_data_file(datafile, &g_act_1))
        {
          print_str(MSG_GEN_DFRFAIL, datafile);
          ret = 16;
          goto end;
        }
      d_write_stats(datafile);
      /*print_str("ERROR: %s: no records were loaded, aborting..\n", datafile);
       ret = 15;
       goto end;*/
      goto end;

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
              "ERROR: DATA IMPORT: %s: [%d]: could not load input data (binary source)\n",
              datafile, r);
          ret = 12;
          goto end;
        }
    }

  if (g_act_1.flags & F_GH_FROMSTDIN)
    {
      g_act_1.flags ^= F_GH_FROMSTDIN;
    }

  if (gfl & F_OPT_VERBOSE)
    {
      print_str("NOTICE: '%s': parsed and loaded %llu records\n", a_ptr,
          (unsigned long long int) g_act_1.w_buffer.offset);
    }

  if ((gfl & F_OPT_ZPRUNEDUP) && (gfl & F_OPT_NOBUFFER))
    {
      print_str("ERROR: '%s': pruning can't be done with buffering disabled\n",
          datafile);
    }

  if ((gfl & F_OPT_ZPRUNEDUP) && !(gfl & F_OPT_NOBUFFER)
      && !access(datafile, R_OK)
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
              if (!(m = g_memcomp(ptr_r->ptr, ptr_w->ptr,
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
      d_write_stats(datafile);
    }

  end:

  if (pf_infile)
    {
      fclose(pf_infile);
    }

  return ret;
}

int
g_load_record(__g_handle hdl, pmda w_buffer, const void *data, off_t max,
    uint16_t flags)
{
  void *buffer = NULL;

  if (flags & F_LOAD_RECORD_DATA)
    {

      buffer = md_alloc(w_buffer, hdl->block_sz);

      if (!buffer)
        {
          return 2;
        }

      memcpy(buffer, data, hdl->block_sz);
    }

  if (flags & F_LOAD_RECORD_FLUSH)
    {
      if (w_buffer->offset == max)
        {
          w_buffer->flags |= F_MDA_FREE;
          if (rebuild_data_file(hdl->file, hdl))
            {
              print_str(MSG_GEN_DFRFAIL, hdl->file);
              return 1;
            }

          p_md_obj ptr = w_buffer->objects, ptr_s;

          while (ptr)
            {
              ptr_s = ptr->next;
              free(ptr->ptr);
              ptr = ptr_s;
            }
          bzero(w_buffer->objects,
              (size_t) sizeof(md_obj) * (size_t) w_buffer->count);
          w_buffer->pos = w_buffer->objects;
          w_buffer->offset = 0;

        }
    }

  if ((gfl0 & F_OPT_PROGRESS) && hdl->t_rw != hdl->rw)
    {
      fprintf(stderr, "writing records: %llu..\r",
          (unsigned long long int) hdl->rw);
      hdl->t_rw = hdl->rw;
    }

  return 0;
}

int
g_d_post_proc_gcb(void *buffer, void *p_hdl)
{
  __g_handle hdl = (__g_handle ) p_hdl;

  int r;

  if (g_bmatch(buffer, hdl, &hdl->w_buffer))
    {
      md_unlink(&hdl->w_buffer, hdl->w_buffer.pos);
      return 1;
    }

  if ((r = g_load_record(hdl, &hdl->w_buffer, (const void*) buffer,
  MAX_BWHOLD_BYTES / hdl->block_sz,
  F_LOAD_RECORD_FLUSH)))
    {
      printf("ERROR: could not flush data to storage device\n");
      return 2;
    }

  return 0;
}

int
m_load_input_n(__g_handle hdl, FILE *input)
{
  g_setjmp(0, "m_load_input_n", NULL, NULL);

  if (!hdl->w_buffer.objects)
    {
      md_init(&hdl->w_buffer, 256);
    }

  if (NULL == hdl->g_proc0)
    {
      return -1;
    }

  if (!hdl->d_memb)
    {
      return -2;
    }

  char *buffer = malloc(MAX_SDENTRY_LEN + 1);
  buffer[0] = 0x0;

  int rf = -9;

  char *l_ptr;
  void *st_buffer = NULL;
  uint32_t rw = 0, c = 0;

  while ((l_ptr = fgets(buffer, MAX_SDENTRY_LEN, input)) != NULL)
    {
      rf = 1;

      if (buffer[0] == 0x23)
        {
          continue;
        }

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
          if (rw >= hdl->d_memb)
            {
              if (NULL != hdl->gcb_post_proc)
                {
                  if (0 != hdl->gcb_post_proc(st_buffer, (void*) hdl))
                    {
                      break;
                    }
                }
              rw = 0;
              rf = 0;
              c++;
            }
          else
            {
              print_str(
                  "ERROR: DATA IMPORT: [%d/%d] missing mandatory parameters\n",
                  rw, hdl->d_memb);
              break;
            }

          continue;
        }

      if (!rw || !st_buffer)
        {
          st_buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);
        }

      char *s_p1 = l_ptr;

      while (l_ptr[0] && l_ptr[0] != 0x20 && l_ptr[0] != 0x9)
        {
          l_ptr++;
        }

      l_ptr[0] = 0x0;
      l_ptr++;

      while (l_ptr[0] == 0x20 || l_ptr[0] == 0x9)
        {
          l_ptr++;
        }

      size_t s_p1_l = strlen(l_ptr);
      if (l_ptr[s_p1_l - 1] == 0xA)
        {
          l_ptr[s_p1_l - 1] = 0x0;
          s_p1_l--;
        }

      if (!s_p1_l)
        {
          print_str("WARNING: DATA IMPORT: null value '%s'\n", s_p1);
        }

      int bd = hdl->g_proc0(st_buffer, s_p1, l_ptr);
      if (!bd)
        {
          print_str("ERROR: DATA IMPORT: failed extracting '%s'\n", s_p1);
          rf = 3;
        }
      else if (bd == -1)
        {
          rf = 4;
          rw++;
        }
      else
        {
          rw++;
        }
    }

  free(buffer);

  return rf;
}

int
g_enum_log(_d_enuml callback, __g_handle hdl, off_t *nres, void *arg)
{
  g_setjmp(0, "g_enum_log", NULL, NULL);

  *nres = 0;

  void *buffer = calloc(1, hdl->block_sz), *ptr;
  int r = 1;

  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      while ((ptr = g_read(buffer, hdl, hdl->block_sz)))
        {
          if ((r = callback((void*) hdl, ptr, arg)))
            {
              if (r == -1)
                {
                  *nres = 1;
                  break;
                }
              else if (r == -2)
                {
                  goto end;
                }

              continue;
            }
          *nres = *nres + 1;
        }
    }

  if (*nres)
    {
      r = 0;
    }

  end:

  free(buffer);

  return r;
}

void
dt_set_dummy(__g_handle hdl)
{
  return;
}

