/*
 * log_io.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <t_glob.h>
#include <im_hdr.h>
#include <glutil.h>
#include <l_sb.h>
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

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

long long int db_max_size = DB_MAX_SIZE;

int
g_fopen(char *file, char *mode, uint32_t flags, __g_handle hdl)
{
  g_setjmp(0, "g_fopen", NULL, NULL);
  int r;
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

  strncpy(hdl->file, file, strlen(file) + 1);

  if (determine_datatype(hdl, hdl->file))
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

int
g_cleanup(__g_handle hdl)
{
  int r = 0;

  r += md_g_free(&hdl->buffer);
  r += md_g_free(&hdl->w_buffer);
  r += md_g_free(&hdl->print_mech);

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
      hdl->data = shmap(hdl->ipc_key, &hdl->ipcbuf, (size_t) hdl->total_sz,
          &sh_ret, &hdl->shmid);

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
              return 20109;
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

  struct stat st =
    { 0 };

  if (!(hdl->flags & F_GH_FROMSTDIN))
    {
      if (stat(file, &st) == -1)
        {
          if (!(gfl & F_OPT_SHAREDMEM))
            {
              print_str(
                  "ERROR: %s: [%s] unable to get information from data file\n",
                  file,
                  strerror(errno));
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

          if (st.st_size > db_max_size)
            {
              print_str(
                  "WARNING: %s: disabling memory buffering, file too big (%lld MB max)\n",
                  file, db_max_size / 1048576);
              hdl->flags |= F_GH_NOMEM;
              return 20202;
            }

          hdl->total_sz = st.st_size;
        }
    } else {
        if ((gfl & F_OPT_SHAREDMEM)) {
            flags |= F_GBM_SHM_NO_DATAFILE;
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
      if (st.st_size % hdl->block_sz)
        {
          print_str(MSG_GEN_DFCORRU, file, (ulint64_t) st.st_size,
              hdl->block_sz);
          return 20204;
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
              print_str("ERROR: %s: [%s] renaming temporary file failed!\n",
                  hdl->s_buffer,
                  strerror(errno));
              ret = 4;
            }
          goto end;
        }

      cleanup:

      if ((r = remove(hdl->s_buffer)))
        {
          print_str(
              "WARNING: %s: [%s] deleting temporary file failed (remove manually)\n",
              hdl->s_buffer,
              strerror(errno));
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
  fflush(fh);

  if (!(gfl & F_OPT_MODE_RAWDUMP))
    {
      fclose(fh);

      if (hdl->st_mode)
        {
          chmod(outfile, hdl->st_mode);
        }

    }

  return ret;
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
          if (rw >= hdl->d_memb)
            {
              if (hdl->gcb_post_proc)
                {
                  hdl->gcb_post_proc(st_buffer, (void*) hdl);
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
      else if (bd == -1)
        {
          rf = 4;
        }
      else
        {
          rw += bd;
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
