/*
 * cfgv.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "cfgv.h"

#include <l_sb.h>
#include <glutil.h>
#include <memory_t.h>
#include <str.h>
#include <l_error.h>
#include <x_f.h>



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
