/*
 * net_dis.c
 *
 *  Created on: Oct 20, 2015
 *      Author: reboot
 */

#include "memory.h"
#include "hasht.h"
#include "str.h"
#include "misc.h"
#include "thread.h"
#include "g_crypto.h"
#include "net_io.h"

#include "net_dis.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

_dis di_base =
  {{{ 0 }}};

static int
d_find_first_free_hslot (__ipr_a hosts)
{
  int i = 1;

  while (i < DIS_MAX_HOSTS_GLOBAL)
    {
      if (0 == hosts[i].links)
	{
	  return i;
	}
      i++;
    }

  return -1;
}

int
d_register_host (__ipr endpoint, __ipr_a hosts)
{
  int hslot = d_find_first_free_hslot (hosts);

  if (-1 == hslot)
    {
      return -1;
    }

  _ipr_a tmp;

  tmp.ipr = *endpoint;
  tmp.links = 0;

  hosts[hslot] = tmp;

  md_alloc_le (&di_base.hosts_linked, 0, 0, (void*) &hosts[hslot]);

  return hslot;

}

int
d_find_host (__ipr endpoint, __ipr_a hosts)
{
  int i = 1;

  while (i < DIS_MAX_HOSTS_GLOBAL)
    {
      if (!memcmp (&hosts[i].ipr, endpoint, sizeof(_ipr)))
	{
	  return i;
	}
      i++;
    }

  return 0;
}

static int
d_find_first_free_rhslot (uint16_t *hosts)
{
  int i = 1;

  while (i < DIS_MAX_HOSTS_GLOBAL)
    {
      if (0 == hosts[i])
	{
	  return i;
	}
      i++;
    }

  return -1;
}

static int
d_find_relhost (uint16_t hindex, uint16_t *hosts)
{
  int i = 0;

  while (i < DIS_MAX_HLINKS_PER_FILE)
    {
      if (hindex == hosts[i])
	{
	  return i;
	}
      i++;
    }

  return -1;
}

int
nd_pool_entry_set (__do pool, char *p, int l)
{
  size_t pc_size = strlen (p) + 1;

  if (!l)
    {
      pool->d = (void*) ht_create (256);

      if (!pool->d)
	{
	  return 1;
	}
    }

  //mutex_init (&pool->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);

  pool->path_c = malloc (pc_size);
  strncpy (pool->path_c, p, pc_size);

  return 0;
}

static __do
nd_pool_create (__do pool_p, char *key, uint8_t flags, int l)
{
  __do pool = calloc (1, sizeof(_do));

  pool->flags |= flags;

  if (nd_pool_entry_set (pool, key, l))
    {
      print_str (
	  "ERROR: d_import_entry: nd_pool_entry_set failed (out of memory)\n");
      abort ();
    }

  ht_set ((hashtable_t*) pool_p->d, (unsigned char*) key, strlen (key) + 1,
	  (void*) pool, sizeof(_do));
  pool->p_pool = pool_p;

  mutex_lock (&di_base.index_linked.mutex);

  di_base.index_linked.lref_ptr = (void*) pool;
  md_alloc (&di_base.index_linked, 0);
  pool->link = di_base.index_linked.pos;

  pthread_mutex_unlock (&di_base.index_linked.mutex);

  print_str ("DEBUG: created pool: %s [%d]\n", key, l);

  return pool;
}

static int
nd_pool_destroy (__do pool)
{

  if ( NULL != pool->p_pool)
    {
      if (ht_remove ((hashtable_t*) pool->p_pool->d,
		     (unsigned char*) pool->path_c, strlen (pool->path_c) + 1))
	{
	  print_str ("ERROR: nd_pool_destroy: orphaned pool, fix this!\n");
	  abort ();
	}
    }

  if (NULL != pool->link)
    {
      mutex_lock (&di_base.index_linked.mutex);
      md_unlink_le (&di_base.index_linked, pool->link);
      pthread_mutex_unlock (&di_base.index_linked.mutex);
    }

  print_str ("DEBUG: destroyed pool: %s\n", pool->path_c);

  free (pool->d);
  free (pool->path_c);
  free (pool);

  return 0;
}

static int
d_check_fe_update (__ipr host)
{
  if (!memcmp (&host, &di_base.host, sizeof(_ipr)))
    {
      print_str (
	  "WARNING: d_update_file_entry: recieved own update, ignoring..\n");
      return 1;
    }
  return 0;
}

typedef int
(*_d_ufe) (char *p, __do pool, __do_xfd fdata, __ipr host, uint32_t flags);

#define F_D_UPD_CHANGED		(uint8_t)1 << 1

static int
d_update_file_entry (char *p, __do pool, __do_xfd fdata, __ipr host,
		     uint32_t flags)
{
  uint8_t intflags = 0;

  if ( NULL == pool->d)
    {
      pool->d = calloc (1, sizeof(_do_fp));
    }

  __do_fp fp = (__do_fp) pool->d;

  /*if ( memcmp(&fp->fd.digest, &fdata->fd.digest, sizeof(fp->fd.digest)) )
   {
   print_str("WARNING: d_update_file_entry: collision alert: %s\n", p);
   return 1;
   }*/
  if (flags & F_DUUF_UPDATE_ORIGIN_NETWORK)
    {
      if (d_check_fe_update (host))
	{
	  return 127;
	}
    }

  uint16_t hindex = (uint16_t) d_find_host (host, di_base.hosts);

  if (0 == hindex)
    {
      hindex = d_register_host (host, di_base.hosts);

      if (-1 == hindex)
	{
	  print_str ("ERROR: d_update_file_entry: d_register_host failed\n");
	  abort ();
	}

      print_str ("D5: d_update_file_entry: registered host %hu\n", hindex);
    }
  else
    {
      print_str ("D6: d_update_file_entry: host already registered : %hu\n",
		 hindex);
    }

  if ( NULL == fp->hosts)
    {
      print_str ("D6: d_update_file_entry: allocating fp->hosts %hu\n", hindex);
      fp->hosts = calloc (2, DIS_MAX_HLINKS_PER_FILE);

      fp->hosts[0] = hindex;
      di_base.hosts[hindex].links++;
      intflags |= F_D_UPD_CHANGED;

      print_str ("D5: d_update_file_entry: linked file %s to host : %hu [0]\n",
		 p, hindex);
    }
  else
    {
      int relindex;
      if (-1 == (relindex = d_find_relhost (hindex, fp->hosts)))
	{
	  relindex = d_find_first_free_rhslot (fp->hosts);
	  if (-1 == relindex)
	    {
	      print_str ("WARNING: d_update_file_entry: HR for %s full\n", p);
	    }
	  else
	    {

	      fp->hosts[relindex] = hindex;
	      di_base.hosts[hindex].links++;
	      intflags |= F_D_UPD_CHANGED;

	      print_str (
		  "D5: d_update_file_entry: linked file %s to host : %hu [%d]\n",
		  p, hindex, relindex);

	    }
	}
      else
	{
	  print_str (
	      "D6: d_update_file_entry: %s already linked to host %hu [%d]\n",
	      p, hindex, relindex);
	}
    }

  if (memcmp (&fp->fd, &fdata->fd, sizeof(fdata->fd))
      || !(flags & F_DUUF_UPDATE_ORIGIN_NETWORK))
    {
      fp->fd = fdata->fd;
      intflags |= F_D_UPD_CHANGED;
    }

  if (intflags & F_D_UPD_CHANGED)
    {
      print_str ("DEBUG: d_update_file_entry: %s changed [%hu]\n", p, hindex);
      return 0;
    }
  else
    {
      print_str ("D6: d_update_file_entry: %s nothing changed [%hu]\n", p,
		 hindex);
      return 127;
    }

  return -1;

}

static int
d_fe_count_hlinks (uint16_t *hosts)
{
  int i = 0, c = 0;

  while (i < DIS_MAX_HLINKS_PER_FILE)
    {
      if (hosts[i])
	{
	  c++;
	}
      i++;
    }

  return c;
}

static int
d_remove_file_entry (char *p, __do pool, __do_xfd fdata, __ipr host,
		     uint32_t flags)
{
  if ( NULL == pool->d)
    {
      print_str ("WARNING: d_remove_file_entry: target file not found\n");
      return 127;

    }

  if (flags & F_DUUF_UPDATE_ORIGIN_NETWORK)
    {
      if (d_check_fe_update (host))
	{
	  return 127;
	}
    }

  __do_fp fp = (__do_fp) pool->d;

  if ( NULL == fp->hosts)
    {
      print_str (
	  "WARNING: d_remove_file_entry: ignoring non-existant record\n");
      return 127;
    }

  uint16_t hindex = (uint16_t) d_find_host (host, di_base.hosts);

  if (0 == hindex)
    {
      print_str ("WARNING: d_remove_file_entry: target host not found\n");
      return 127;
    }

  int relindex;
  if (-1 == (relindex = d_find_relhost (hindex, fp->hosts)))
    {
      print_str (
	  "WARNING: d_remove_file_entry: trying to remove non-existant record %d [%hu]\n",
	  relindex, hindex);
      return 127;
    }

  fp->hosts[relindex] = 0;
  di_base.hosts[hindex].links--;

  print_str ("DEBUG: d_remove_file_entry: %d removed\n", relindex);

  if (!di_base.hosts[hindex].links)
    {
      memset (&di_base.hosts[hindex].ipr, 0x0, sizeof(_ipr));
      print_str (
	  "WARNING: d_remove_file_entry: host index %hu has no more links (orphaned), removing\n",
	  hindex);
    }

  if (0 == d_fe_count_hlinks (fp->hosts))
    {
      nd_pool_destroy (pool);
      print_str (
	  "DEBUG: d_remove_file_entry: %s has been destroyed globally\n");
      return 71;
    }

  return 0;
}

#include "xref.h"
#include "x_f.h"

static int
d_td_uloc (char *dir, __d_edscb_d callback_f, void *arg, int f, __g_eds eds,
	   DIR *dp)
{

  struct dirent _dirp, *dirp;
  char buf[PATH_MAX];

  while ((dirp = readdir (dp)))
    {

      size_t d_name_l = strlen (dirp->d_name);

      if ((d_name_l == 1 && dirp->d_name[0] == 0x2E)
	  || (d_name_l == 2 && dirp->d_name[0] == 0x2E
	      && dirp->d_name[1] == 0x2E))
	{
	  continue;
	}

      snprintf (buf, PATH_MAX, "%s/%s", dir, dirp->d_name);

      remove_repeating_chars (buf, 0x2F);

      if (dirp->d_type == DT_UNKNOWN)
	{
	  _dirp.d_type = get_file_type (buf);
	}
      else
	{
	  _dirp.d_type = dirp->d_type;
	}

      strncpy (_dirp.d_name, dirp->d_name, strlen (dirp->d_name) + 1);

      callback_f (buf, &_dirp, arg, eds);

    }

  return 0;

}

static int
d_update_file_local (char *dir, struct dirent *dp, void *arg, __g_eds eds)
{
  __do pool = (__do) arg, pool_p = pool;

  switch (dp->d_type)
    {
      case DT_DIR:
      ;

      pool = (__do) ht_get((hashtable_t*)pool->d, (unsigned char*)dp->d_name, strlen(dp->d_name) + 1);

      if ( NULL == pool)
	{
	  pool = nd_pool_create(pool_p, dp->d_name, F_DO_DIR, 0);

	}

      d_enum_dir_bare (dir, d_update_file_local, (void*) pool, 0, eds, d_td_uloc);
      break;
      case DT_REG:;

      struct stat st;

      if ( -1 == stat(dir, &st) )
	{
	  char eb[1024];
	  print_str("WARNING: d_update_local: stat failed: %s [%s]\n", dir, g_strerr_r(errno, eb, 1024));
	  break;
	}

      _do_xfd fdata =
	{
	    {
		{
		    {
		      0}}}};

      errno = 0;

      fdata.fd.digest = c_get_file_sha1(dir);

      if ( 0 != errno )
	{
	  char eb[1024];
	  print_str("ERROR: d_update_local: unable to open %s [%s]\n", dir, g_strerr_r(errno, eb, 1024));
	  break;
	}

      fdata.fd.size = (uint64_t) st.st_size;

      //fdata.fd.

      pool = (__do) ht_get((hashtable_t*)pool->d, (unsigned char*)dp->d_name, strlen(dp->d_name) + 1);

      if ( NULL == pool)
	{
	  pool = nd_pool_create(pool_p, dp->d_name, F_DO_FILE, 1);

	}

      if ( NULL == pool->d)
	{
	  pool->d = calloc (1, sizeof(_do_fp));
	}

      d_update_file_entry (dp->d_name, pool, &fdata, &di_base.host, 0);

      printf ("%s\n", dir);
      break;
    }

  return 0;
}

int
d_update_file (char *path, __do pool, uint8_t flags, __do_xfd fdata,
	       __ipr hosts_in, size_t ipr_count, uint32_t ufe_flags,
	       _d_ufe upd_call)
{
  mda path_b =
    { 0 };

  md_init_le (&path_b, 256);

  int nd = split_string_l_le (path, 0x2F, &path_b, (size_t) path_b.count);

  if (nd < 1)
    {
      md_g_free_l (&path_b);
      return 1;
    }

  int r = 2;

  mutex_lock (&di_base.nd_pool.mutex);

  p_md_obj ptr = path_b.first;
  int depth = 0;

  while (ptr)
    {
      char *p = (char*) ptr->ptr;

      depth++;

      __do pool_p = pool;

      pool = (__do) ht_get((hashtable_t*)pool->d, (unsigned char*) p, strlen(p) + 1);

      if ( NULL == pool)
	{
	  if (ufe_flags & F_DUUF_UPDATE_DESTROY)
	    {
	      print_str ("D2: d_update_file: no such path exists: %s\n", path);
	      break;
	    }

	  pool = calloc (1, sizeof(_do));

	  int l;

	  if ( NULL == ptr->next)
	    {
	      l = 1;
	    }
	  else
	    {
	      pool->flags |= F_DO_DIR;
	      l = 0;
	    }

	  if (nd_pool_entry_set (pool, p, l))
	    {
	      print_str (
		  "ERROR: d_update_file: nd_pool_entry_set failed (out of memory)\n");
	      abort ();
	    }

	  ht_set ((hashtable_t*) pool_p->d, (unsigned char*) p, strlen (p) + 1,
		  (void*) pool, sizeof(_do));

	  print_str ("DEBUG: d_update_file: created pool: %s\n", p);
	}

      if (NULL == ptr->next)
	{
	  //mutex_lock (&pool->mutex);
	  //print_str ("DEBUG: d_import_entry: hit last: %s\n", p);
	  pool->flags |= flags;

	  int i, r_t;
	  for (i = 0, r = 127; i < ipr_count; i++)
	    {
	      if ((r_t = upd_call (p, pool, fdata, &hosts_in[i], ufe_flags)))
		{
		  if (r_t != 127)
		    {
		      r = r_t;
		      break;
		    }
		  if (r_t == 71)
		    {
		      r = 0;
		      break;
		    }
		}
	      else
		{
		  r = 0;
		}
	    }

	  //pthread_mutex_unlock (&pool->mutex);

	  break;
	}

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  md_g_free_l (&path_b);

  return r;
}

__do
d_lookup_path (char *path, __do pool, uint8_t flags)
{

  if (path[0] == 0x2F && path[1] == 0x0)
    {
      return &di_base.nd_pool.pool;
    }

  mda path_b =
    { 0 };

  md_init_le (&path_b, 256);

  int nd = split_string_l_le (path, 0x2F, &path_b, (size_t) path_b.count);

  if (nd < 1)
    {
      return NULL;
    }

  mutex_lock (&di_base.nd_pool.mutex);

  p_md_obj ptr = path_b.first;
  int depth = 0;

  while (ptr)
    {
      char *p = (char*) ptr->ptr;

      pool = (__do) ht_get((hashtable_t*)pool->d, (unsigned char*) p, strlen(p) + 1);

      if ( NULL == pool)
	{
	  goto exit;
	}

      if (NULL == ptr->next)
	{
	  md_g_free_l (&path_b);
	  pthread_mutex_unlock (&di_base.nd_pool.mutex);
	  return pool;
	}

      depth++;

      ptr = ptr->next;
    }

  exit: ;

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  md_g_free_l (&path_b);

  return NULL;
}

int
d_enum_index (__do base_pool, pmda lindex, void *data, d_enum_i_cb call)
{
  mutex_lock (&di_base.nd_pool.mutex);
  mutex_lock (&lindex->mutex);

  p_md_obj ptr = md_first (lindex);
  int count = 0;

  while (ptr)
    {
      __do pool = (__do) ptr->ptr;

      if ( call(pool, data) )
	{
	  break;
	}

      count++;

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&lindex->mutex);
  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  return count;

}

static char *
net_dis_decompile_update (__do_base_h_enc packet, __do_updex *puex, __ipr *pipr)
{
  *puex = (__do_updex) ((void*)packet + sizeof(_do_base_h_enc));
  char *path = ((char*) *puex) + sizeof(_do_updex);
  *pipr = (__ipr)(path + packet->body.ex_len1);
  return path;
}

static int
net_dis_broadcast_check (__sock_o pso, void *arg, void *data)
{
  __ipr ipr_d = &((__do_sst ) pso->va_p3)->ipr, ipr_s = (__ipr) arg;

  if ( !memcmp(ipr_d, ipr_s, sizeof(_ipr)))
    {
      print_str ("D6: net_dis_broadcast_check: [%d] skipping originating socket\n",
	  pso->sock);

      return 1;
    }

  return 0;
}

static int
net_dis_process_inbound_update_msgl (__sock_o pso, __do_updex updex)
{
  int ret = 0;

  mutex_lock (&di_base.msg_log.mutex);

  mutex_lock (&di_base.msg_log.links.mutex);

  if (di_base.msg_log.links.offset >= DIS_RMSGL_MAX)
    {
      if (ht_remove (di_base.msg_log.ht,
		     (unsigned char*) di_base.msg_log.links.first->ptr,
		     sizeof(_pid_sha1)))
	{
	  print_str (
	      "ERROR: net_dis_process_inbound_update_msgl: [%d] could not clean hashtable\n",
	      pso->sock);
	  abort ();
	}
      free (di_base.msg_log.links.first->ptr);
      md_unlink_le (&di_base.msg_log.links, di_base.msg_log.links.first);

      print_str (
	  "D6: net_dis_process_inbound_update_msgl: removed oldest update\n");
    }

  void * h_dig = ht_get (di_base.msg_log.ht, (unsigned char *) &updex->digest,
			 sizeof(updex->digest));
  char buffer[128];

  if ( NULL != h_dig)
    {

      print_str (
	  "WARNING: net_dis_process_inbound_update_msgl: [%d] [%s] duplicate update recieved\n",
	  pso->sock,
	  bb_to_ascii (updex->digest.data, sizeof(updex->digest.data), buffer));
      ret = -2;
      goto exit;
    }

  __pid_sha1 dc = malloc (sizeof(_pid_sha1));

  *dc = updex->digest;

  char d = 1;

  ht_set (di_base.msg_log.ht, dc->data, sizeof(_pid_sha1), &d, 1);
  md_alloc_le (&di_base.msg_log.links, 0, 0, dc);

  print_str (
      "D3: net_dis_process_inbound_update_msgl: [%d] [%s] cached update\n",
      pso->sock, bb_to_ascii (dc->data, sizeof(_pid_sha1), buffer));

  exit: ;

  pthread_mutex_unlock (&di_base.msg_log.links.mutex);
  pthread_mutex_unlock (&di_base.msg_log.mutex);

  return ret;
}

static int
net_dis_process_inbound_update (__sock_o pso, pmda threadr,
				__do_base_h_enc packet)
{

  if (packet->body.ex_len1 > PATH_MAX)
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] path data field too large\n",
	  packet->body.code);
      return 1;
    }

  if (0 == packet->body.ex_len1)
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] path data field size 0\n",
	  packet->body.code);
      return 1;
    }

  if (packet->body.ex_len2 % sizeof(_ipr))
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] corrupt header, invalid remote endpoint data field size\n",
	  packet->body.code);
      return 1;
    }

  size_t ipr_count = packet->body.ex_len2 / sizeof(_ipr);

  if (ipr_count > DIS_MAX_HLINKS_PER_FILE)
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] too many remote endpoint records\n",
	  packet->body.code);
      return 1;
    }

  if (0 == ipr_count)
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] no remote endpoint information was recieved in update\n",
	  packet->body.code);
      return 1;
    }

  __do_updex puex;
  __ipr pipr;
  char *path = net_dis_decompile_update (packet, &puex, &pipr);

  size_t path_len = strlen (path);

  if (0 == path_len)
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] update path string null\n",
	  packet->body.code);
      return 1;
    }

  if (path_len > PATH_MAX + 1)
    {
      print_str (
	  "ERROR: net_dis_process_inbound_update: [%u] unterminated path string\n",
	  packet->body.code);
      return 1;
    }

  if (net_dis_process_inbound_update_msgl (pso, puex))
    {
      return 0;
    }

  _d_ufe upd_call;
  uint32_t ufe_flags = F_DUUF_UPDATE_ORIGIN_NETWORK;

  if (packet->body.m00_8 & F_DH_UPDATE_DESTROY)
    {
      upd_call = d_remove_file_entry;
      ufe_flags |= F_DUUF_UPDATE_DESTROY;
    }
  else
    {
      upd_call = d_update_file_entry;
    }

  int ret = d_update_file (path, &di_base.nd_pool.pool, 1, &puex->xfd, pipr,
			   ipr_count, ufe_flags, upd_call);

  if (ret == 127)
    {
      print_str (
	  "WARNING: net_dis_process_inbound_update: [%d] [%u] recieved redundant update for %s\n",
	  pso->sock, packet->body.code, path);
      return 0;
    }
  else if (ret)
    {
      return ret;
    }

  print_str ("DEBUG: net_dis_process_inbound_request: [%d] recieved update\n",
	     pso->sock);

  _ipr ipr = ((__do_sst ) pso->va_p3)->ipr;

  pthread_mutex_unlock (&pso->mutex);

  net_broadcast (pso->host_ctx, packet, packet->head.content_length,
		 net_dis_broadcast_check, &ipr, 0);

  mutex_lock (&pso->mutex);

  return 0;

}

__do_base_h_enc
d_assemble_update (__do pool, __do base_pool, char *path, uint8_t flags)
{
  __do_base_h_enc packet = NULL;

  mutex_lock (&di_base.nd_pool.mutex);

  if (!(pool->flags & F_DO_FILE))
    {
      print_str ("ERROR: d_assemble_update: %s is not a file\n", path);
      goto exit;
    }

  _ipr ipr[DIS_MAX_HLINKS_PER_FILE];

  __do_fp fp = (__do_fp) pool->d;

  int i, c = 0;

  if (flags & F_DH_UPDATE_DESTROY)
    {
      c = 1;
      ipr[0] = di_base.host;
    }
  else
    {
      for (i = 0; i < DIS_MAX_HLINKS_PER_FILE; i++)
	{
	  uint16_t idx = fp->hosts[i];

	  if (idx)
	    {
	      ipr[c] = di_base.hosts[idx].ipr;
	      c++;
	    }
	}
    }

  if (!c)
    {
      print_str (
	  "ERROR: d_assemble_update: %s endpoint information could not be assembled\n",
	  path);
      abort ();
    }

  _do_xfd xfd;

  xfd.fd = fp->fd;

  packet = net_dis_compile_update (CODE_DIS_UPDATE, path, ipr, (size_t) c, &xfd,
				   flags);

  exit: ;

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  return packet;
}

__do_base_h_enc
net_assemble_update (__sock_o pso, __do basepool, char *path, uint8_t flags)
{
  mutex_lock (&di_base.nd_pool.mutex);

  __do pool = d_lookup_path (path, basepool, 0);

  __do_base_h_enc packet = NULL;

  if ( NULL == pool)
    {
      print_str ("ERROR: net_assemble_update: [%d] %s not found\n", pso->sock,
		 path);
      goto exit_np;
    }

  packet = d_assemble_update (pool, basepool, path, flags);

  exit_np: ;
  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  return packet;

}

int
d_build_path_index (__do pool, char *out)
{
  char tpath[PATH_MAX];
  char *pd = "";
  int i = 0;
  while ( NULL != pool)
    {
      char *pp;

      if (NULL == pool->path_c)
	{
	  pp = pd;
	}
      else
	{
	  pp = pool->path_c;
	}

      if (!i)
	{
	  snprintf (tpath, PATH_MAX, "%s", pp);
	}
      else
	{
	  snprintf (tpath, PATH_MAX, "%s/%s", pp, out);
	}
      strncpy (out, tpath, strlen (tpath) + 1);

      i++;
      pool = pool->p_pool;
    }

  return i;
}

static int
net_de_send_update (__do pool, void *data)
{
  mutex_lock (&di_base.nd_pool.mutex);
  if (!(pool->flags & F_DO_FILE))
    {
      pthread_mutex_unlock (&di_base.nd_pool.mutex);
      return 0;
    }

  __sock_o pso = (__sock_o) data;

  char b[PATH_MAX];

  if (!d_build_path_index (pool, b))
    {
      print_str (
	  "ERROR: net_de_send_update: [%d]: could not extrapolate path\n",
	  pso->sock);
      pthread_mutex_unlock (&di_base.nd_pool.mutex);
      return 0;
    }

  __do_base_h_enc packet = d_assemble_update (pool, &di_base.nd_pool.pool, b,
					      0);

  if ( NULL == packet)
    {
      pthread_mutex_unlock (&di_base.nd_pool.mutex);
      return 0;
    }

  int r;
  if ((r = net_send_direct (pso, (void*) packet, packet->head.content_length)))
    {
      print_str (
	  "ERROR: net_dis_process_inbound_request: [%d]: net_send_direct failed: [%d]\n",
	  pso->sock, r);
      pso->flags |= F_OPSOCK_TERM;
    }

  free (packet);

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  return r;
}

#define	F_NS_THREAD_ONLINE 	(uint8_t) 1 << 1

struct net_smu
{
  pthread_t pt;
  __sock_o pso;
  pthread_mutex_t mutex;
  uint8_t status;
};

int
net_send_mass_updates (__sock_o pso)
{

  //mutex_lock (&nsmu->pso->mutex);

  /*if (nsmu->pso->flags & F_OPSOCK_TERM)
   {
   goto exit;
   }*/

  //nsmu->status |= F_NS_THREAD_ONLINE;
  //pthread_mutex_unlock (&nsmu->mutex);
  //mutex_lock (&nsmu->pso->mutex);
  d_enum_index (&di_base.nd_pool.pool, &di_base.index_linked, pso,
		net_de_send_update);

  __do_base_h_enc packet = net_dis_compile_genreq (CODE_DIS_NOTIFY,
  F_DH_UPDATE_EOS,
						   NULL, 0);

  int r;
  if ((r = net_send_direct (pso, (void*) packet, packet->head.content_length)))
    {
      print_str (
	  "ERROR: net_send_mass_updates: [%d]: net_send_direct failed: [%d]\n",
	  pso->sock, r);
      pso->flags |= F_OPSOCK_TERM;
      free (packet);
      return 1;
    }

  free (packet);

  //pid_t _tid = (pid_t) syscall (SYS_gettid);

  /*print_str (
   "DEBUG: net_send_mass_updates: [%d] [%d] thread shutting down [%d processed]\n",
   _tid, nsmu->pso->sock, c);*/

  //exit: ;
  //nsmu->pso->flags ^= (nsmu->pso->flags & F_OPSOCK_PERSIST);
  //pthread_mutex_unlock (&nsmu->pso->mutex);
  return 0;
}

static int
net_dis_process_inbound_request (__sock_o pso, pmda threadr,
				 __do_base_h_enc packet)
{
  if (packet->body.m00_8 & F_DH_REQUPD_ALL)
    {
      print_str (
	  "DEBUG: net_dis_process_inbound_request: [%d] peer requesting all updates\n",
	  pso->sock);

      //struct net_smu *nsmu = malloc (sizeof(struct net_smu));

      //nsmu->pso = pso;

      net_send_mass_updates (pso);

      __do_sst sst = (__do_sst) pso->va_p3;

      sst->status |= F_DO_SSTATE_INITSYNC;

      /*mutex_init (&nsmu->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);

       int r;
       if ((r = pthread_create (&nsmu->pt, NULL, net_send_mass_updates,
       (void *) nsmu)))
       {
       free (nsmu);
       print_str (
       "ERROR: net_dis_process_inbound_request: [%d] pthread_create failed: %d\n",
       pso->sock, r);
       return 1;
       }
       */
      /*time_t s = time (NULL);

       while (time (NULL) - s < 15)
       {
       mutex_lock (&nsmu->mutex);
       if (nsmu->status & F_NS_THREAD_ONLINE)
       {
       print_str (
       "DEBUG: net_dis_process_inbound_request: [%d] [%X] net_send_mass_updates thread online\n",
       pso->sock, nsmu->pt);
       break;
       }
       pthread_mutex_unlock (&nsmu->mutex);
       usleep (100000);
       }
       */
    }
  else
    {

    }

  return 0;
}

static int
net_dis_process_inbound_notify (__sock_o pso, pmda threadr,
				__do_base_h_enc packet)
{
  if (packet->body.m00_8 & F_DH_UPDATE_EOS)
    {
      __do_sst sst = (__do_sst) pso->va_p3;

      if ( !(sst->status & F_DO_SSTATE_INITSYNC))
	{

	  sst->status |= F_DO_SSTATE_INITSYNC;

	  print_str (
	      "DEBUG: net_dis_process_inbound_notify: [%d] end of remote update stream, sending ourown..\n",
	      pso->sock);

	  net_send_mass_updates (pso);

	}
    }
  else
    {

    }

  return 0;
}

static int
net_dis_check_header (__sock_o pso, __do_base_h_enc input)
{
  size_t total = input->body.ex_len1 + input->body.ex_len2 + input->body.ex_len
      + sizeof(_do_base_h_enc);

  if (total != input->head.content_length)
    {
      print_str (
	  "ERROR: net_baseline_dis: corrupt header on [%d] [%zu / %zu]\n",
	  pso->sock, input->head.content_length, total);
      pthread_mutex_unlock (&pso->mutex);
      return 4;
    }

  return 0;
}

int
net_baseline_dis (__sock_o pso, pmda base, pmda threadr, void *data)
{
  mutex_lock (&pso->mutex);

  if (pso->counters.b_read < pso->unit_size)
    {
      pthread_mutex_unlock (&pso->mutex);
      return -2;
    }

  __do_base_h_enc input = (__do_base_h_enc ) data;

  if (net_dis_check_header (pso, input))
    {
      return 4;
    }

  int ret;

  switch (input->body.code)
    {
    case CODE_DIS_UPDATE:
      ;
      ret = net_dis_process_inbound_update (pso, threadr, input);
      if (ret)
	{
	  print_str (
	      "ERROR: net_baseline_dis: [%d] [%u]: an error occured while processing update [%d]\n",
	      pso->sock, input->body.code, ret);
	  pthread_mutex_unlock (&pso->mutex);
	  ret = 5;
	  goto exit;
	}
      break;
    case CODE_DIS_REQUPD:
      ;
      ret = net_dis_process_inbound_request (pso, threadr, input);
      if (ret)
	{
	  print_str (
	      "ERROR: net_baseline_dis: [%d] [%u]: an error occured while processing request [%d]\n",
	      pso->sock, input->body.code, ret);
	  pthread_mutex_unlock (&pso->mutex);
	  ret = 6;
	  goto exit;
	}

      break;
    case CODE_DIS_NOTIFY:
      ;
      ret = net_dis_process_inbound_notify (pso, threadr, input);
      if (ret)
	{
	  print_str (
	      "ERROR: net_baseline_dis: [%d] [%u]: an error occured while processing notify [%d]\n",
	      pso->sock, input->body.code, ret);
	  pthread_mutex_unlock (&pso->mutex);
	  ret = 7;
	  goto exit;
	}
      break;
    default:
      print_str ("ERROR: net_baseline_dis: unknown header code %u\n",
		 input->body.code);
      pthread_mutex_unlock (&pso->mutex);
      ret = 10;
      goto exit;
    }

  //exit_reset: ;

  net_proto_reset_to_baseline (pso);

  exit: ;

  pthread_mutex_unlock (&pso->mutex);

  return ret;
}

int
net_dis_socket_dc_cleanup (__sock_o pso)
{
  mutex_lock (&pso->mutex);

  if ( NULL != pso->va_p3)
    {
      free (pso->va_p3);
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

static int
net_dis_initialize_sts (__sock_o pso)
{
  if ( NULL == pso->va_p3)
    {
      pso->va_p3 = calloc (1, sizeof(_do_sst));
    }

  __do_sst sst = (__do_sst) pso->va_p3;

  return net_addr_to_ipr (pso, &sst->ipr);
}

int
net_dis_socket_init1_accept (__sock_o pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;

      if (pso->flags & F_OPSOCK_TERM)
	{
	  print_str (
	      "DEBUG: net_dis_socket_init1_accept: [%d]: skipping initialization (socket shutting down)\n",
	      pso->sock);
	  break;
	}

      if (net_dis_initialize_sts (pso))
	{
	  pso->flags |= F_OPSOCK_TERM;
	  print_str (
	      "ERROR: net_dis_socket_init1_accept: [%d]: net_dis_initialize_sts failed\n",
	      pso->sock);
	  break;
	}

      break;
    case SOCKET_OPMODE_LISTENER:
      ;

      __sock_ca ca = (__sock_ca) pso->sock_ca;

      if ( ca->ca_flags & F_CA_MISC03)
	{
	  mutex_lock(&di_base.mutex);

	  struct addrinfo *aip;
	  struct addrinfo hints =
	    { 0};

	  _sock_o dummy;

	  hints.ai_flags = AI_ALL | AI_ADDRCONFIG;
	  hints.ai_socktype = SOCK_STREAM;
	  hints.ai_protocol = IPPROTO_TCP;

	  int ret;

	  if (getaddrinfo (ca->b4, ca->port, &hints, &aip))
	    {
	      ret = net_addr_to_ipr(pso, &di_base.host);
	    }
	  else
	    {
	      dummy.res = *aip;
	      ret = net_addr_to_ipr(&dummy, &di_base.host);
	    }

	  freeaddrinfo(aip);

	  di_base.status |= F_DIS_ACTIVE;

	  if (ret )
	    {
	      print_str (
		  "ERROR: [%d]: net_dis_socket_init1_accept: net_addr_to_ipr failed\n",
		  pso->sock);
	      pso->flags |= F_OPSOCK_TERM;
	      pthread_mutex_unlock(&di_base.mutex);
	      break;
	    }

	  print_str (
	      "INFO: [%d]: DIS server online: %hhu.%hhu.%hhu.%hhu\n",
	      pso->sock, di_base.host.ip[0], di_base.host.ip[1], di_base.host.ip[2] , di_base.host.ip[3]);

	  pthread_mutex_unlock(&di_base.mutex);

	}
      else
	{
	  print_str (
	      "ERROR: [%d]: net_dis_socket_init1_accept: missing settings\n",
	      pso->sock);
	  abort();
	}

      break;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_dis_socket_init1_connect (__sock_o pso)
{
  mutex_lock (&pso->mutex);
  switch (pso->oper_mode)
    {
    case SOCKET_OPMODE_RECIEVER:
      ;

      if (pso->flags & F_OPSOCK_TERM)
	{
	  print_str (
	      "DEBUG: net_dis_socket_init1_connect: [%d]: skipping initialization (socket shutting down)\n",
	      pso->sock);
	  break;
	}

      if (net_dis_initialize_sts (pso))
	{
	  pso->flags |= F_OPSOCK_TERM;
	  print_str (
	      "ERROR: net_dis_socket_init1_connect: [%d]: net_dis_initialize_sts failed\n",
	      pso->sock);
	  break;
	}

      __do_base_h_enc requpd_packet = net_dis_compile_genreq (CODE_DIS_REQUPD,
      F_DH_REQUPD_ALL,
							      NULL, 0);

      int r;
      if ((r = net_push_to_sendq (pso, (void*) requpd_packet,
				  requpd_packet->head.content_length, 0)))
	{
	  print_str (
	      "ERROR: net_fs_send_xfer_req: [%d]: net_push_to_sendq failed: [%d]\n",
	      pso->sock, r);
	  pso->flags |= F_OPSOCK_TERM;
	  free (requpd_packet);
	  break;
	}

      free (requpd_packet);

      break;
    }

  pthread_mutex_unlock (&pso->mutex);

  return 0;
}

int
net_dis_compile_gp (__do_base_h_enc packet)
{
  struct timespec tp;
  if (-1 == clock_gettime (CLOCK_REALTIME, &tp))
    {
      char eb[1024];
      print_str ("ERROR: net_dis_compile_gp: clock_gettime failed: [%s]\n",
		 strerror_r (errno, eb, sizeof(eb)));
      return 1;
    }

  packet->body.ts.tv_nsec = (uint32_t) tp.tv_nsec;
  packet->body.ts.tv_sec = (uint32_t) tp.tv_sec;

  packet->body.rand = (uint32_t) rand_r ((unsigned int*) &di_base.seed);

  return 0;
}

__do_base_h_enc
net_dis_compile_update (int code, char *data, __ipr ipr, size_t ipr_count,
			__do_xfd xfd, uint8_t f)
{

  size_t p_len = strlen ((char*) data) + 1;

  if (0 == p_len)
    {
      print_str ("ERROR: net_fs_compile_update: missing path\n");
      return NULL;
    }

  if (p_len > PATH_MAX - 1)
    {
      print_str ("ERROR: net_fs_compile_update: path too long\n");
      return NULL;
    }

  if (ipr_count > DIS_MAX_HLINKS_PER_FILE)
    {
      print_str ("ERROR: net_fs_compile_update: ipr_count too large\n");
      return NULL;
    }

  size_t ipr_len = sizeof(_ipr) * ipr_count;
  size_t dat_len = sizeof(_do_updex) + p_len + ipr_len;
  size_t req_len = sizeof(_do_base_h_enc) + dat_len;
  __do_base_h_enc request = calloc (1, req_len);

  if (net_dis_compile_gp (request))
    {
      free (request);
      return NULL;
    }

  request->head.prot_code = PROT_CODE_DIS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = CODE_DIS_UPDATE;
  request->body.m00_8 |= f;

  request->body.ex_len = sizeof(_do_updex);

  request->body.ex_len1 = (uint32_t) p_len;
  request->body.ex_len2 = (uint32_t) ipr_len;

  void *ptr = (void*) request + sizeof(_do_base_h_enc);

  __do_updex updex = (__do_updex) ptr;

  updex->xfd = *xfd;

  ptr += sizeof(_do_updex);

  memcpy (ptr, data, p_len);

  ptr += p_len;

  memcpy (ptr, ipr, ipr_len);

  SHA1 ((unsigned char *) request, req_len, updex->digest.data);

  return request;
}

__do_base_h_enc
net_dis_compile_genreq (int code, uint8_t f, void *data, size_t size)
{
  size_t req_len = sizeof(_do_base_h_enc) + size;
  __do_base_h_enc request = calloc (1, req_len);

  if (net_dis_compile_gp (request))
    {
      free (request);
      return NULL;
    }

  request->head.prot_code = PROT_CODE_DIS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = code;
  request->body.m00_8 |= f;

  if ( NULL != data)
    {
      request->body.ex_len1 = (uint32_t) size;
      memcpy ((((void*) request) + sizeof(_do_base_h_enc)), data, size);
    }

  return request;
}

int
dis_rescan (void *arg)
{

  mutex_lock (&di_base.mutex);

  if (!c_get_urandom_bytes ((void*) &di_base.seed, sizeof(di_base.seed),
			    di_base.fh_urandom))
    {
      print_str ("ERROR: net_dis_compile_gp: c_get_urandom_bytes failed\n");
      pthread_mutex_unlock (&di_base.mutex);
      kill (getpid (), SIGINT);
      return 0;
    }

  if (!(di_base.status & F_DIS_ACTIVE))
    {
      print_str ("ERROR: DIS requested but could not be initialized\n");
      pthread_mutex_unlock (&di_base.mutex);
      kill (getpid (), SIGINT);
      return 0;
    }

  if (!strlen (di_base.root))
    {
      print_str (
	  "WARNING: DIS enabled but no root directory was defined, using /\n");
    }

  pthread_mutex_unlock (&di_base.mutex);

  _g_eds eds =
    { 0 };
  mutex_lock (&di_base.nd_pool.mutex);

  int ret = d_enum_dir_bare ("/", d_update_file_local, &di_base.nd_pool.pool, 0,
			     &eds, d_td_uloc);

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  return ret;
}

int
htest ()
{
  /*
   //exit (0);
   hashtable_t *hashtable = ht_create (10000);

   ht_set (hashtable, (unsigned char*) "key1", 5, "inky", strlen ("inky") + 1);
   ht_set (hashtable, (unsigned char*) "key11", 6, "343434",
   strlen ("343434") + 1);
   ht_set (hashtable, (unsigned char*) "key2", 5, "pinky", strlen ("pinky") + 1);
   ht_set (hashtable, (unsigned char*) "key3", 5, "blinky",
   strlen ("blinky") + 1);

   printf ("%s\n", ht_get (hashtable, "key1", 5));
   printf ("%s\n", ht_get (hashtable, "key11", 6));
   printf ("%s\n", ht_get (hashtable, "key2", 5));
   printf ("%s\n", ht_get (hashtable, "key3", 5));

   ht_remove (hashtable, (unsigned char*) "key11", 6);
   ht_remove (hashtable, (unsigned char*) "key1", 5);
   ht_remove (hashtable, (unsigned char*) "key2", 5);
   ht_remove (hashtable, (unsigned char*) "key3", 5);

   ht_set (hashtable, (unsigned char*) "key1", 5, "gdffdgdhg",
   strlen ("gdffdgdhg") + 1);

   ht_set (hashtable, (unsigned char*) "key2", 5, "aaaagdffdgdhg",
   strlen ("aaaagdffdgdhg") + 1);

   ht_set (hashtable, (unsigned char*) "key11", 6, "443ggg43gdt43344",
   strlen ("443ggg43gdt43344") + 1);

   ht_set (hashtable, (unsigned char*) "key3", 5, "gdfg43gw32rfd",
   strlen ("gdfg43gw32rfd") + 1);

   printf("::\n::\n");

   printf ("%s\n", ht_get (hashtable, "key1", 5));
   printf ("%s\n", ht_get (hashtable, "key11", 6));
   printf ("%s\n", ht_get (hashtable, "key2", 5));
   printf ("%s\n", ht_get (hashtable, "key3", 5));

   exit (0);

   */
  return 0;

}

