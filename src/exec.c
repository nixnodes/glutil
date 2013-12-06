/*
 * exec.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */
#include "m_comp.h"
#include <glutil.h>

#include <fp_types.h>
#include <exec_t.h>
#include <memory_t.h>
#include <exech.h>
#include <arg_proc.h>
#include <l_error.h>

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <stdlib.h>

long amax = 0;

char *exec_str = NULL;
char **exec_v = NULL;
int exec_vc = 0;

__d_exec exc = NULL;

int
g_build_argv_c(__g_handle hdl)
{
  md_init(&hdl->exec_args.ac_ref, hdl->exec_args.argc);

  hdl->exec_args.argv_c = calloc(amax / sizeof(char*), sizeof(char**));
  int i, r;
  __d_argv_ch ach;
  char *ptr;
  for (i = 0; i < hdl->exec_args.argc && hdl->exec_args.argv[i]; i++)
    {
      ptr = strchr(hdl->exec_args.argv[i], 0x7B);
      if (ptr)
        {
          hdl->exec_args.argv_c[i] = (char*) calloc(8192, 1);
          size_t t_l = strlen(hdl->exec_args.argv[i]);
          strncpy(hdl->exec_args.argv_c[i], hdl->exec_args.argv[i],
              t_l > 8191 ? 8191 : t_l);
          ach = md_alloc(&hdl->exec_args.ac_ref, sizeof(_d_argv_ch));
          ach->cindex = i;
          if ((r = g_compile_exech(&ach->mech, hdl, hdl->exec_args.argv[i])))
            {
              return r;
            }
        }
      else
        {
          hdl->exec_args.argv_c[i] = hdl->exec_args.argv[i];
        }
    }

  if (!i)
    {
      return -1;
    }

  return 0;
}


int
opt_execv(void *arg, int m)
{
  int c = 0;

#ifdef _SC_ARG_MAX
  amax = sysconf(_SC_ARG_MAX);
#else
#ifdef ARG_MAX
  val = ARG_MAX;
#endif
#endif

  if (!amax)
    {
      amax = LONG_MAX;
    }

  long count = amax / sizeof(char*);

  exec_str = g_pd(arg, m, MAX_EXEC_STR);

  if (!exec_str)
    {
      return 9008;
    }

  if (!strlen(exec_str))
    {
      return 9009;
    }

  char **ptr = build_argv(exec_str, count, &c);

  if (!c)
    {
      return 9001;
    }

  if (c > count / 2)
    {
      return 9002;
    }

  exec_vc = c;

  exec_v = ptr;
  exc = g_do_exec_v;

  return 0;
}

int
g_do_exec_v(void *buffer, void *callback, char *ex_str, void * p_hdl)
{
  __g_handle hdl = (__g_handle) p_hdl;
  process_execv_args(buffer, hdl);
  return l_execv(hdl->exec_args.exec_v_path, hdl->exec_args.argv_c);
}


int
g_do_exec(void *buffer, void *callback, char *ex_str, void *hdl)
{
  if (callback)
    {
      char *e_str;
      if (ex_str)
        {
          e_str = ex_str;
        }
      else
        {
          if (!exec_str)
            {
              return -1;
            }
          e_str = exec_str;
        }

      if (process_exec_string(e_str, b_glob, MAX_EXEC_STR, callback, buffer))
        {
          bzero(b_glob, MAX_EXEC_STR + 1);
          return -2;
        }

      return system(b_glob);
    }
  else if (ex_str)
    {
      if (strlen(ex_str) > MAX_EXEC_STR)
        {
          return -1;
        }
      return system(ex_str);
    }
  else
    {
      return -1;
    }
}


int
prep_for_exec(void)
{
  const char inputfile[] = "/dev/null";

  if (close(0) < 0)
    {
      fprintf(stdout, "ERROR: could not close stdin\n");
      return 1;
    }
  else
    {
      if (open(inputfile, O_RDONLY
#if defined O_LARGEFILE
          | O_LARGEFILE
#endif
          ) < 0)
        {
          fprintf(stdout, "ERROR: could not open %s\n", inputfile);
        }
    }

  if (execv_stdout_redir != -1)
    {
      dup2(execv_stdout_redir, STDOUT_FILENO);
    }

  return 0;
}

int
l_execv(char *exec, char **argv)
{
  pid_t c_pid;

  fflush(stdout);
  fflush(stderr);

  c_pid = fork();

  if (c_pid == (pid_t) -1)
    {
      fprintf(stderr, "ERROR: %s: fork failed\n", exec);
      return 1;
    }

  if (!c_pid)
    {
      if (prep_for_exec())
        {
          _exit(1);
        }
      else
        {
          execv(exec, argv);
          fprintf(stderr, "ERROR: %s: execv failed to execute [%d]\n", exec,
          errno);
          _exit(1);
        }
    }
  int status;
  while (waitpid(c_pid, &status, 0) == (pid_t) -1)
    {
      if (errno != EINTR)
        {
          fprintf(stderr,
              "ERROR: %s:failed waiting for child process to finish [%d]\n",
              exec, errno);
          return 2;
        }
    }

  return status;
}


int
process_execv_args(void *data, __g_handle hdl)
{
  g_setjmp(0, "process_execv_args", NULL, NULL);

  p_md_obj ptr = md_first(&hdl->exec_args.ac_ref);

  __d_argv_ch ach;
  char *s_ptr;
  while (ptr)
    {

      ach = (__d_argv_ch) ptr->ptr;

      if (!(s_ptr = g_exech_build_string(data, &ach->mech,
                  hdl, hdl->exec_args.argv_c[ach->cindex], 8191)))
        {

          hdl->exec_args.argv_c[ach->cindex][0] = 0x0;
        }

      /*if (process_exec_string(hdl->exec_args.argv[ach->cindex],
       hdl->exec_args.argv_c[ach->cindex], 8191, (void*) hdl->g_proc1, data))
       {
       }*/

      ptr = ptr->next;
    }

  return 0;
}

int
g_do_exec_fb(void *buffer, void *callback, char *ex_str, void *hdl)
{
  char *ptr;
  if (!(ptr = g_exech_build_string(buffer, &((__g_handle ) hdl)->exec_args.mech,
      (__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      b_glob[0] = 0x0;
      return -2;
    }

  return system(b_glob);
}

int
process_exec_string(char *input, char *output, size_t max_size, void *callback,
    void *data)
{

  int
  (*call)(void *, char *, char *, size_t) = callback;

  size_t blen = strlen(input);

  if (!blen || blen > MAX_EXEC_STR)
    {
      return 1;
    }

  size_t b_l_1;
  char buffer[255] =
    { 0 }, buffer2[MAX_VAR_LEN] =
    { 0 };
  int i, i2, pi, r = 0, f;

  for (i = 0, pi = 0; i < blen; i++, pi++)
    {
      if (input[i] == 0x7B)
        {
          for (i2 = 0, i++, f = 0, r = 0; i < blen && i2 < 255; i++, i2++)
            {
              if (input[i] == 0x7D)
                {
                  buffer[i2] = 0x0;
                  if (!i2 || strlen(buffer) > MAX_VAR_LEN
                      || (r = call(data, buffer, buffer2, MAX_VAR_LEN)))
                    {
                      if (r)
                        {
                          b_l_1 = strlen(buffer);
                          snprintf(&output[pi],
                          MAX_EXEC_STR - pi - 2, "%c%s%c", 0x7B, buffer, 0x7D);

                          pi += b_l_1 + 2;
                        }
                      f |= 0x1;
                      break;
                    }
                  b_l_1 = strlen(buffer2);
                  memcpy(&output[pi], buffer2, b_l_1);

                  pi += b_l_1;
                  f |= 0x1;
                  break;
                }
              buffer[i2] = input[i];
            }

          if ((f & 0x1))
            {
              pi -= 1;
              continue;
            }
        }
      output[pi] = input[i];
    }

  output[pi] = 0x0;

  return 0;
}
