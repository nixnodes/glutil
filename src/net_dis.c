/*
 * net_dis.c
 *
 *  Created on: Oct 20, 2015
 *      Author: reboot
 */

#include "str.h"
#include "misc.h"
#include "thread.h"
#include "g_crypto.h"
#include "net_io.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "net_dis.h"

_dis di_base =
  { 0 };

/* Create a new hashtable. */
hashtable_t *
ht_create (int size)
{

  hashtable_t *hashtable = NULL;
  int i;

  if (size < 1)
    return NULL;

  /* Allocate the table itself. */
  if ((hashtable = malloc (sizeof(hashtable_t))) == NULL)
    {
      return NULL;
    }

  /* Allocate pointers to the head nodes. */
  if ((hashtable->table = malloc (sizeof(entry_t *) * size)) == NULL)
    {
      return NULL;
    }
  for (i = 0; i < size; i++)
    {
      hashtable->table[i] = NULL;
    }

  hashtable->size = size;

  return hashtable;
}

/* Hash a string for a particular hash table. */
int
ht_hash (hashtable_t *hashtable, unsigned char *key, size_t keyLength)
{

  size_t hash;
  int i = 0;

  /* jenkins algo  */
  for (hash = i = 0; i < keyLength; ++i)
    {
      hash += key[i], hash += (hash << 10), hash ^= (hash >> 6);
    }
  hash += (hash << 3), hash ^= (hash >> 11), hash += (hash << 15);

  return hash % hashtable->size;
}

/* Create a key-value pair. */
entry_t *
ht_newpair (unsigned char *key, size_t k_size, void *value, size_t size)
{
  entry_t *newpair;

  if ((newpair = malloc (sizeof(entry_t))) == NULL)
    {
      return NULL;
    }

  if ((newpair->key = malloc (k_size)) == NULL)
    {
      return NULL;
    }

  memcpy (newpair->key, (void*) key, k_size);

  newpair->value = value;

  newpair->next = NULL;

  return newpair;
}

/* Insert a key-value pair into a hash table. */
void
ht_set (hashtable_t *hashtable, unsigned char *key, size_t k_size, char *value,
	size_t size)
{
  int bin = 0;
  entry_t *newpair = NULL;
  entry_t *next = NULL;
  entry_t *last = NULL;

  bin = ht_hash (hashtable, key, k_size);

  next = hashtable->table[bin];

  while (next != NULL && next->key != NULL
      && memcmp (key, next->key, k_size) > 0)
    {
      last = next;
      next = next->next;
    }

  /* There's already a pair.  Let's replace that string. */
  if (next != NULL && next->key != NULL && memcmp (key, next->key, k_size) == 0)
    {

      free (next->value);
      next->value = strdup (value);

      /* Nope, could't find it.  Time to grow a pair. */
    }
  else
    {
      newpair = ht_newpair (key, k_size, value, size);

      /* We're at the start of the linked list in this bin. */
      if (next == hashtable->table[bin])
	{
	  newpair->next = next;
	  hashtable->table[bin] = newpair;

	}
      else if (next == NULL)
	{
	  /* We're at the end of the linked list in this bin. */
	  last->next = newpair;

	}
      else /* We're in the middle of the list. */
	{
	  newpair->next = next;
	  last->next = newpair;
	}
    }
}

/* Retrieve a key-value pair from a hash table. */
void *
ht_get (hashtable_t *hashtable, unsigned char *key, size_t k_size)
{
  int bin = 0;
  entry_t *pair;

  bin = ht_hash (hashtable, key, k_size);

  /* Step through the bin, looking for our value. */
  pair = hashtable->table[bin];
  while (pair != NULL && pair->key != NULL && memcmp (key, pair->key, k_size))
    {

      pair = pair->next;
    }

  /* Did we actually find anything? */
  if (pair == NULL || pair->key == NULL || memcmp (key, pair->key, k_size) != 0)
    {
      return NULL;

    }
  else
    {
      return pair->value;
    }

}

//////////////////////////////////////////////////////////////////

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

  mutex_init (&pool->mutex, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);

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
  pthread_mutex_unlock (&di_base.index_linked.mutex);

  print_str ("DEBUG: created pool: %s [%d]\n", key, l);

  return pool;
}

#define F_D_UPD_CHANGED		(uint8_t)1 << 1

static int
d_update_file_entry (char *p, __do pool, __do_xfd fdata, __ipr host)
{
  uint8_t intflags = 0;

  __do_fp fp = (__do_fp) pool->d;

  /*if ( memcmp(&fp->fd.digest, &fdata->fd.digest, sizeof(fp->fd.digest)) )
   {
   print_str("WARNING: d_update_file_entry: collision alert: %s\n", p);
   return 1;
   }*/

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

  if (memcmp (&fp->fd, &fdata->fd, sizeof(fdata->fd)))
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

#include "xref.h"
#include "x_f.h"

static int
d_td_uloc (char *dir, __d_edscb_d callback_f, void *arg, int f, __g_eds eds,
	   DIR *dp)
{

  struct dirent _dirp, *dirp;
  char buf[PATH_MAX];

  int r = 0, ir;

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
	{ 0};

      errno = 0;

      fdata.fd.digest = c_get_file_sha1(dir);

      if ( 0 != errno )
	{
	  char eb[1024];
	  print_str("ERROR: d_update_local: unable to open %s [%s]\n", dir, g_strerr_r(errno, eb, 1024));
	  break;
	}

      fdata.fd.size = st.st_size;
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

      d_update_file_entry (dp->d_name, pool, &fdata, &di_base.host);

      printf ("%s\n", dir);
      break;
    }

  return 0;
}

int
d_update_file (char *path, __do pool, uint8_t flags, __do_xfd fdata,
	       __ipr hosts_in, size_t ipr_count)
{
  mda path_b =
    { 0 };

  md_init_le (&path_b, 256);

  int nd = split_string_l_le (path, 0x2F, &path_b, (size_t) path_b.count);

  if (nd < 1)
    {
      md_g_free (&path_b);
      return 1;
    }

  int r = 2;

  __do base_pool = pool;

  mutex_lock (&base_pool->mutex);

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
		  "ERROR: d_import_entry: nd_pool_entry_set failed (out of memory)\n");
	      abort ();
	    }

	  ht_set ((hashtable_t*) pool_p->d, (unsigned char*) p, strlen (p) + 1,
		  (void*) pool, sizeof(_do));

	  print_str ("DEBUG: created pool: %s\n", p);
	}

      if (NULL == ptr->next)
	{
	  print_str ("DEBUG: d_import_entry: hit last: %s\n", p);
	  pool->flags |= flags;
	  if ( NULL == pool->d)
	    {
	      pool->d = calloc (1, sizeof(_do_fp));
	    }

	  int i, r_t;
	  for (i = 0, r = 127; i < ipr_count; i++)
	    {
	      if ((r_t = d_update_file_entry (p, pool, fdata, &hosts_in[i])))
		{
		  if (r_t != 127)
		    {
		      r = r_t;
		      break;
		    }

		}
	      else
		{
		  r = 0;
		}
	    }
	  break;
	}

      ptr = ptr->next;
    }

  pthread_mutex_unlock (&base_pool->mutex);

  md_g_free (&path_b);

  return r;
}

__do
d_lookup_path (char *path, __do pool, uint8_t flags)
{
  mda path_b =
    { 0 };

  md_init_le (&path_b, 256);

  int nd = split_string_l_le (path, 0x2F, &path_b, (size_t) path_b.count);

  if (nd < 1)
    {
      return NULL;
    }

  __do base_pool = pool;
  mutex_lock (&base_pool->mutex);

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
	  pthread_mutex_unlock (&base_pool->mutex);
	  return pool;
	}

      depth++;

      ptr = ptr->next;
    }

  exit: ;

  pthread_mutex_unlock (&base_pool->mutex);

  md_g_free (&path_b);

  return NULL;
}

int
d_enum_index (__do base_pool, pmda lindex, void *data, d_enum_i_cb call)
{
  mutex_lock (&base_pool->mutex);

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

  pthread_mutex_unlock (&base_pool->mutex);

  exit: ;

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
	  "ERROR: net_dis_process_inbound_update: [%u] invalid ip data field size\n",
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

  int ret = d_update_file (path, &di_base.nd_pool, 1, &puex->xfd, pipr,
			   ipr_count);

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

  return 0;

}

__do_base_h_enc
d_assemble_update (__do pool, char *path)
{
  __do_base_h_enc packet = NULL;

  mutex_lock (&pool->mutex);

  if (!(pool->flags & F_DO_FILE))
    {
      print_str ("ERROR: d_assemble_update: %s is not a file\n", path);
      goto exit;
    }

  _ipr ipr[DIS_MAX_HLINKS_PER_FILE];

  __do_fp fp = (__do_fp) pool->d;

  int i, c;

  for (i = 0, c = 0; i < DIS_MAX_HLINKS_PER_FILE; i++)
    {
      uint16_t idx = fp->hosts[i];

      if (idx)
	{
	  ipr[c] = di_base.hosts[idx].ipr;
	  c++;
	}

    }

  if (!c)
    {
      print_str (
	  "ERROR: d_assemble_update: %s endpoint information could not be assembled\n",
	  path);
      goto exit;
    }

  _do_xfd xfd;

  xfd.fd = fp->fd;

  packet = net_dis_compile_update (CODE_DIS_UPDATE, path, ipr, (size_t) c,
				   &xfd);

  exit: ;

  pthread_mutex_unlock (&pool->mutex);

  return packet;
}

__do_base_h_enc
net_assemble_update (__sock_o pso, __do basepool, char *path)
{
  mutex_lock (&basepool->mutex);

  __do pool = d_lookup_path (path, basepool, 0);

  __do_base_h_enc packet = NULL;

  if ( NULL == pool)
    {
      print_str ("ERROR: net_assemble_update: [%d] %s not found\n", pso->sock,
		 path);
      goto exit_np;
    }

  packet = d_assemble_update (pool, path);

  exit_np: ;
  pthread_mutex_unlock (&basepool->mutex);

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
  mutex_lock (&pool->mutex);
  if (!(pool->flags & F_DO_FILE))
    {
      pthread_mutex_unlock (&pool->mutex);
      return 0;
    }

  __sock_o pso = (__sock_o) data;

  char b[PATH_MAX];

  if (!d_build_path_index (pool, b))
    {
      print_str (
	  "ERROR: net_de_send_update: [%d]: could not extrapolate path\n",
	  pso->sock);
      pthread_mutex_unlock (&pool->mutex);
      return 0;
    }

  __do_base_h_enc packet = d_assemble_update (pool, b);

  if ( NULL == packet)
    {
      pthread_mutex_unlock (&pool->mutex);
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

  pthread_mutex_unlock (&pool->mutex);

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

void *
net_send_mass_updates (void *args)
{
  struct net_smu *nsmu = (struct net_smu *) args;

  //mutex_lock (&nsmu->mutex);

  //nsmu->status |= F_NS_THREAD_ONLINE;

  //pthread_mutex_unlock (&nsmu->mutex);

  //mutex_lock (&nsmu->pso->mutex);

  int c = d_enum_index (&di_base.nd_pool, &di_base.index_linked, nsmu->pso,
			net_de_send_update);

  //pid_t _tid = (pid_t) syscall (SYS_gettid);

  /*print_str (
   "DEBUG: net_send_mass_updates: [%d] [%d] thread shutting down [%d processed]\n",
   _tid, nsmu->pso->sock, c);*/

  //pthread_mutex_unlock (&nsmu->pso->mutex);
  free (args);

  return NULL;
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

      struct net_smu *nsmu = malloc (sizeof(struct net_smu));

      nsmu->pso = pso;

      net_send_mass_updates ((void*) nsmu);

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

       time_t s = time (NULL);

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
       }*/

    }
  else
    {

    }

  return 0;
}

int
net_addr_to_ipr (__sock_o pso, __ipr out)
{
  uint16_t *port_data;
  uint8_t *ip_data;
  int len;
  switch (pso->res->ai_family)
    {
    case AF_INET:
      ;
      ip_data = (uint8_t*) &((struct sockaddr_in*) pso->res->ai_addr)->sin_addr;
      port_data =
	  (uint16_t*) &((struct sockaddr_in*) pso->res->ai_addr)->sin_port;
      len = 4;
      break;
    case AF_INET6:
      ;
      ip_data =
	  (uint8_t*) &((struct sockaddr_in6*) pso->res->ai_addr)->sin6_addr;
      port_data =
	  (uint16_t*) &((struct sockaddr_in6*) pso->res->ai_addr)->sin6_port;
      len = 16;
      break;
    default:
      ;
      return 1;
    }

  out->port = ntohs (*port_data);

  int i;
  for (i = 0; i < len && i < sizeof(out->ip); i++)
    {
      out->ip[i] = ip_data[i];
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

  __do_base_h_enc input = (__do_base_h_enc ) data, packet = NULL;

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

  int ret;

  switch (input->body.code)
    {
    case CODE_DIS_UPDATE:
      ;
      ret = net_dis_process_inbound_update (pso, threadr, input);
      if (ret)
	{
	  print_str (
	      "ERROR: net_baseline_dis: [%u]: an error occured while processing update [%d]\n",
	      input->body.code, ret);
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
	      "ERROR: net_baseline_dis: [%u]: an error occured while processing request [%d]\n",
	      input->body.code, ret);
	  pthread_mutex_unlock (&pso->mutex);
	  ret = 6;
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
	      dummy.res = aip;
	      ret = net_addr_to_ipr(&dummy, &di_base.host);
	    }

	  di_base.status |= F_DIS_ACTIVE;
	  pthread_mutex_unlock(&di_base.mutex);

	  if (ret )
	    {
	      print_str (
		  "ERROR: [%d]: net_dis_socket_init1_accept: net_addr_to_ipr failed\n",
		  pso->sock);
	      pso->flags |= F_OPSOCK_TERM;
	      break;
	    }

	  print_str (
	      "INFO: [%d]: DIS server online: %hhu.%hhu.%hhu.%hhu\n",
	      pso->sock, di_base.host.ip[0], di_base.host.ip[1], di_base.host.ip[2] , di_base.host.ip[3]);

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

      __sock_ca ca = (__sock_ca) pso->sock_ca;

      if ( ca->ca_flags & F_CA_MISC00)
	{

	}

      __do_base_h_enc requpd_packet = net_dis_compile_genreq(CODE_DIS_REQUPD, F_DH_REQUPD_ALL, NULL, 0);

      int r;
      if ((r = net_push_to_sendq (pso, (void*) requpd_packet, requpd_packet->head.content_length,
		  0)))
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

__do_base_h_enc
net_dis_compile_update (int code, char *data, __ipr ipr, size_t ipr_count,
			__do_xfd xfd)
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

  request->head.prot_code = PROT_CODE_DIS;
  request->head.content_length = (uint32_t) req_len;
  request->body.code = CODE_DIS_UPDATE;

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

  if (!(di_base.status & F_DIS_ACTIVE))
    {
      print_str ("ERROR: DIS requested but could not be initialized\n");
      pthread_mutex_unlock (&di_base.mutex);
      kill (getpid (), SIGINT);
      return 0;
    }

  if (!strlen (di_base.root))
    {
      print_str ("ERROR: DIS enabled but no root directory was defined\n");

    }

  pthread_mutex_unlock (&di_base.mutex);

  _g_eds eds =
    { 0 };
  mutex_lock (&di_base.nd_pool.mutex);

  int ret = d_enum_dir_bare ("/", d_update_file_local, &di_base.nd_pool, 0,
			     &eds, d_td_uloc);

  pthread_mutex_unlock (&di_base.nd_pool.mutex);

  return ret;
}

int
htest ()
{

  //exit (0);
  /*hashtable_t *hashtable = ht_create (10000);

   ht_set (hashtable, (unsigned char*) "key1", 5, "inky", strlen ("inky") + 1);
   ht_set (hashtable, (unsigned char*) "key2", 5, "pinky", strlen ("pinky") + 1);
   ht_set (hashtable, (unsigned char*) "key3", 5, "blinky",
   strlen ("blinky") + 1);

   printf ("%s\n", ht_get (hashtable, "key1", 5));
   printf ("%s\n", ht_get (hashtable, "key2", 5));
   printf ("%s\n", ht_get (hashtable, "key3", 5));


   */
  _do_xfd fdata =
    { 0 };

  _ipr h[16] =
    { 0 };

  h[0].ip[0] = 43;
  h[0].port = 434;

  h[1].ip[0] = 75;
  h[1].port = 6546;

  return 0;
}
