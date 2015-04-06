/*
 * l_error.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <t_glob.h>
#include <l_sb.h>
#include <l_error.h>

#include <unistd.h>

void
e_pop(p_sigjmp psm)
{
  memcpy(psm->p_env, psm->env, sizeof(sigjmp_buf));
  psm->pid = psm->id;
  psm->pflags = psm->flags;
  psm->pci = psm->ci;
}

void
e_push(p_sigjmp psm)
{
  memcpy(psm->env, psm->p_env, sizeof(sigjmp_buf));
  psm->id = psm->pid;
  psm->flags = psm->pflags;
  psm->ci = psm->pci;
}

void
g_setjmp(uint32_t flags, char *type, void *callback, void *arg)
{
  if (flags)
    {
      g_sigjmp.flags = flags;
    }
  g_sigjmp.id = ID_SIGERR_UNSPEC;

  //bzero(g_sigjmp.type, 32);
  size_t t_len = strlen(type) + 1;

  if (t_len >= sizeof(g_sigjmp.type))
    {
      t_len = sizeof(g_sigjmp.type) - 1;
    }

  strncpy(g_sigjmp.type, type, t_len);

  g_sigjmp.type[t_len] = 0x0;

  return;
}
#ifdef _G_SSYS_THREAD
#include <unistd.h>
#include <sys/syscall.h>
#endif

#define MSG_DEF_UNKN1   "(unknown)"

void
sighdl_error(int sig, siginfo_t* siginfo, void* context)
{

  char *s_ptr1 = MSG_DEF_UNKN1, *s_ptr2 = MSG_DEF_UNKN1, *s_ptr3 = "";
  char buffer1[4096] =
    { 0 };

  switch (sig)
    {
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

  snprintf(buffer1, 4096, ", fault address: 0x%.16llX",
      (ulint64_t)  siginfo->si_addr);

  switch (g_sigjmp.id)
    {
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

  if (g_sigjmp.flags & F_SIGERR_CONTINUE)
    {
      s_ptr3 = ", resuming execution..";
    }


  fprintf(stderr, "EXCEPTION: "
#ifdef _G_SSYS_THREAD
      "[%ld] "
#endif
      "%s: [%s] [%s] [%s]%s%s\n",
#ifdef _G_SSYS_THREAD
      syscall(SYS_gettid),
#endif
      s_ptr1, g_sigjmp.type,
      s_ptr2, strerror(siginfo->si_errno), buffer1, s_ptr3);

  //usleep(450000);

  g_sigjmp.ci++;

  if (g_sigjmp.flags & F_SIGERR_CONTINUE)
    {
      siglongjmp(g_sigjmp.env, 0);
    }

  g_sigjmp.ci = 0;
  g_sigjmp.flags = 0;

  exit(siginfo->si_errno);
}

void *
g_memcpy(void *dest, const void *src, size_t n)
{
  void *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags = 0;
  g_sigjmp.id = ID_SIGERR_MEMCPY;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = memcpy(dest, src, n);
    }
  e_push(&g_sigjmp);
  return ret;
}

void *
g_memmove(void *dest, const void *src, size_t n)
{
  void *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags = 0;
  g_sigjmp.id = ID_SIGERR_MEMMOVE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = memmove(dest, src, n);
    }
  e_push(&g_sigjmp);
  return ret;
}

char *
g_strncpy(char *dest, const char *src, size_t n)
{
  char *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags = 0;
  g_sigjmp.id = ID_SIGERR_STRCPY;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = strncpy(dest, src, n);
    }
  e_push(&g_sigjmp);
  return ret;
}

void
g_free(void *ptr)
{
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FREE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      free(ptr);
    }
  e_push(&g_sigjmp);
}

size_t
g_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t ret = 0;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FREAD;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fread(ptr, size, nmemb, stream);
    }
  e_push(&g_sigjmp);
  return ret;
}

size_t
g_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t ret = 0;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FWRITE;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fwrite(ptr, size, nmemb, stream);
    }
  e_push(&g_sigjmp);
  return ret;
}

FILE *
gg_fopen(const char *path, const char *mode)
{
  FILE *ret = NULL;
  e_pop(&g_sigjmp);
  g_sigjmp.flags |= F_SIGERR_CONTINUE;
  g_sigjmp.id = ID_SIGERR_FOPEN;
  if (!sigsetjmp(g_sigjmp.env, 1))
    {
      ret = fopen(path, mode);
    }
  e_push(&g_sigjmp);
  return ret;
}
