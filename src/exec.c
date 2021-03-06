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
#include <misc.h>
#include <str.h>
#include <x_f.h>
#include <errno_int.h>

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
g_build_argv_c (__g_handle hdl)
{
  md_init (&hdl->exec_args.ac_ref, hdl->exec_args.argc);

  hdl->exec_args.argv_c = calloc (amax / sizeof(char*), sizeof(char**));
  int i, r;
  __d_argv_ch ach;
  char *ptr;
  for (i = 0; i < hdl->exec_args.argc && hdl->exec_args.argv[i]; i++)
    {
      ptr = strchr (hdl->exec_args.argv[i], 0x7B);
      if (ptr)
	{
	  hdl->exec_args.argv_c[i] = (char*) calloc (8192, 1);
	  size_t t_l = strlen (hdl->exec_args.argv[i]);
	  strncpy (hdl->exec_args.argv_c[i], hdl->exec_args.argv[i],
		   t_l > 8191 ? 8191 : t_l);
	  ach = md_alloc (&hdl->exec_args.ac_ref, sizeof(_d_argv_ch));
	  ach->cindex = i;
	  if ((r = g_compile_exech (&ach->mech, hdl, hdl->exec_args.argv[i])))
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
g_build_argv_c_bare (__execv exec_args, __g_handle hdl)
{
  md_init (&exec_args->ac_ref, exec_args->argc);

  exec_args->argv_c = calloc (amax / sizeof(char*), sizeof(char**));
  int i, r;
  __d_argv_ch ach;
  char *ptr;
  for (i = 0; i < exec_args->argc && exec_args->argv[i]; i++)
    {
      ptr = strchr (exec_args->argv[i], 0x7B);
      if (ptr)
	{
	  exec_args->argv_c[i] = (char*) calloc (8192, 1);
	  size_t t_l = strlen (exec_args->argv[i]);
	  strncpy (exec_args->argv_c[i], exec_args->argv[i],
		   t_l > 8191 ? 8191 : t_l);
	  ach = md_alloc (&exec_args->ac_ref, sizeof(_d_argv_ch));
	  ach->cindex = i;
	  if ((r = g_compile_exech (&ach->mech, hdl, exec_args->argv[i])))
	    {
	      return r;
	    }
	}
      else
	{
	  exec_args->argv_c[i] = exec_args->argv[i];
	}
    }

  if (!i)
    {
      return -1;
    }

  return 0;
}

int
g_init_execv_bare (__execv exec_args, __g_handle hdl, char *i_exec_str)
{
  int r;

#ifdef _SC_ARG_MAX
  amax = sysconf (_SC_ARG_MAX);
#else
#ifdef ARG_MAX
  amax = ARG_MAX;
#endif
#endif

  if (!amax)
    {
      amax = LONG_MAX;
    }

  long count = amax / sizeof(char*);

  if (!i_exec_str)
    {
      return 9008;
    }

  if (!strlen (i_exec_str))
    {
      return 9009;
    }

  int c = 0;

  char **ptr = build_argv (i_exec_str, count, &c);

  if (!c)
    {
      return 9001;
    }

  if (c > count / 2)
    {
      return 9002;
    }

  exec_args->argv = ptr;
  exec_args->argc = c;
  exec_args->exc = g_do_exec_v;

  if (!exec_args->argc)
    {
      return 2001;
    }

  if ((r = g_build_argv_c_bare (exec_args, hdl)))
    {
      return r;
    }

  if ((r = find_absolute_path (exec_args->argv_c[0], exec_args->exec_v_path)))
    {
      snprintf (exec_args->exec_v_path, PATH_MAX, "%s", exec_args->argv_c[0]);
    }

  return 0;

}

int
opt_execv_stdin (void *arg, int m, void *opt)
{
  char *b_in, *fn_in;
  FILE *fh_in;

  O_FI_STDIN(arg, m, 6420, 6421)

  exec_str = calloc (MAX_EXEC_STR, 1);
  if (read_file (fn_in, exec_str, MAX_EXEC_STR, 0, fh_in))
    {
      return opt_execv (exec_str, 3, opt);
    }

  return 6426;
}

int
opt_execv (void *arg, int m, void *opt)
{
  int c = 0;

#ifdef _SC_ARG_MAX
  amax = sysconf (_SC_ARG_MAX);
#else
#ifdef ARG_MAX
  amax = ARG_MAX;
#endif
#endif

  if (!amax)
    {
      amax = LONG_MAX;
    }

  long count = amax / sizeof(char*);

  if (m == 3)
    {
      exec_str = (char*) arg;
    }
  else
    {
      exec_str = g_pd (arg, m, MAX_EXEC_STR);
    }

  if (!exec_str)
    {
      return 9008;
    }

  if (!strlen (exec_str))
    {
      return 9009;
    }

  char **ptr = build_argv (exec_str, count, &c);

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
g_do_exec_v (void *buffer, void *callback, char *ex_str, void * p_hdl)
{
  __g_handle hdl = (__g_handle) p_hdl;
  process_execv_args(buffer, hdl);
  return l_execv(hdl->exec_args.exec_v_path, hdl->exec_args.argv_c, hdl);
}

int
g_do_exec (void *buffer, void *callback, char *ex_str, void *hdl)
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

      if (process_exec_string (e_str, b_glob, MAX_EXEC_STR, callback, buffer))
	{
	  bzero (b_glob, MAX_EXEC_STR);
	  return -2;
	}

      return system (b_glob);
    }
  else if (ex_str)
    {
      if (strlen (ex_str) > MAX_EXEC_STR)
	{
	  return -1;
	}
      return system (ex_str);
    }
  else
    {
      return -1;
    }
}

#define MSG_PFE_DUP2ERR         "ERROR: prep_for_exec: dup2 failed [%s] (%s)\n"

static int
prep_for_exec (__g_handle hdl)
{
  const char inputfile[] = "/dev/null";

  if (close (STDIN_FILENO) < 0)
    {
      print_str ("ERROR: could not close stdin\n");
      return 1;
    }
  else
    {
      if (open (inputfile, O_RDONLY
#if defined O_LARGEFILE
		    | O_LARGEFILE
#endif
		) < 0)
	{
	  print_str ("ERROR: could not open %s\n", inputfile);
	}
    }

  if (hdl->flags & F_GH_EXECRD_PIPE_IN)
    {
      close (hdl->pipe.pfd_in[1]);
      if (dup2 (hdl->pipe.pfd_in[0], STDIN_FILENO) == -1)
	{
	  print_str (MSG_PFE_DUP2ERR,
		     g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)),
		     "IN");
	  return 1;
	}
    }

  if (hdl->flags & F_GH_EXECRD_PIPE_OUT)
    {
      close (hdl->pipe.pfd_out[0]);

      if (dup2 (hdl->pipe.pfd_out[1], STDOUT_FILENO) == -1)
	{
	  print_str (MSG_PFE_DUP2ERR,
		     g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)),
		     "OUT");
	  return 1;
	}

    }
  else if (execv_stdout_redir != -1)
    {
      if (dup2 (execv_stdout_redir, STDOUT_FILENO) == -1)
	{
	  print_str (MSG_PFE_DUP2ERR,
		     g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  return 1;
	}

      if (dup2 (execv_stdout_redir, STDERR_FILENO) == -1)
	{
	  print_str (MSG_PFE_DUP2ERR,
		     g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  return 1;
	}
    }

  return 0;
}

void
l_post_exec_cleanup (__g_handle hdl)
{
  if (hdl->flags & F_GH_EXECRD_PIPE_IN)
    {
      close (hdl->pipe.pfd_in[0]);
    }
  if (hdl->flags & F_GH_EXECRD_PIPE_OUT)
    {
      close (hdl->pipe.pfd_out[1]);
    }
}

int
l_waitpid_def (pid_t c_pid, void *arg)
{
  __g_handle hdl = (__g_handle) arg;

  int status = -2;

  while (waitpid(c_pid, &status, 0) == (pid_t) -1)
    {
      if (errno != EINTR)
	{
	  fprintf(stderr,
	      "ERROR: [%d]: failed waiting for child process to finish [%s]\n",
	      (uint32_t)c_pid, g_strerr_r(errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  l_post_exec_cleanup(hdl);
	  return -1;
	}
    }

  l_post_exec_cleanup(hdl);

  return status;
}

int
l_execv (char *exec, char **argv, __g_handle hdl)
{
  pid_t c_pid;

  fflush (stdout);
  fflush (stderr);

  if (hdl->flags & F_GH_EXECRD_PIPE_IN)
    {
      if (pipe (hdl->pipe.pfd_in) == -1)
	{
	  print_str ("ERROR: l_execv: could not open STDIN pipe [%s]\n",
		     g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  return 1;
	}
      hdl->flags |= F_GH_EXECRD_HAS_STD_PIPE;
    }

  if (hdl->flags & F_GH_EXECRD_PIPE_OUT)
    {

      if (pipe (hdl->pipe.pfd_out) == -1)
	{
	  print_str ("ERROR: l_execv: could not open STDOUT pipe [%s]\n",
		     g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  return 1;
	}
      hdl->flags |= F_GH_EXECRD_HAS_STD_PIPE;

    }

  if ((c_pid = fork ()) == (pid_t) -1)
    {
      fprintf (stderr, "ERROR: %s: fork failed [%s]\n", exec,
	       g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
      return 1;
    }

  if (0 == c_pid)
    {
      if (prep_for_exec (hdl))
	{
	  _exit (1);
	}
      else
	{
	  execv (exec, argv);
	  fprintf (stderr, "ERROR: %s: execv failed to execute [%s]\n", exec,
		   g_strerr_r (errno, hdl->strerr_b, sizeof(hdl->strerr_b)));
	  _exit (1);
	}
    }

  int status = hdl->execv_wpid_fp (c_pid, (void*) hdl);

  return status;
}

int
process_execv_args (void *data, __g_handle hdl)
{
  g_setjmp (0, "process_execv_args", NULL, NULL);

  p_md_obj ptr = md_first (&hdl->exec_args.ac_ref);

  __d_argv_ch ach = NULL;

  while (ptr)
    {
      ach = (__d_argv_ch) ptr->ptr;

      if (-1 == (g_exech_build_string(data, &ach->mech,
		  hdl, hdl->exec_args.argv_c[ach->cindex], 8191)))
	{

	  hdl->exec_args.argv_c[ach->cindex][0] = 0x0;
	}

      ptr = ptr->next;
    }

  if (!ach)
    {
      return 1;
    }

  return 0;
}

int
process_execv_args_bare (void *data, __g_handle hdl, __execv exec_args)
{
  g_setjmp (0, "process_execv_args", NULL, NULL);

  p_md_obj ptr = md_first (&exec_args->ac_ref);

  __d_argv_ch ach;

  while (ptr)
    {

      ach = (__d_argv_ch) ptr->ptr;

      if (-1 == (g_exech_build_string(data, &ach->mech,
		  hdl, exec_args->argv_c[ach->cindex], 8191)))
	{

	  exec_args->argv_c[ach->cindex][0] = 0x0;
	}

      ptr = ptr->next;
    }

  return 0;
}

int
g_do_exec_fb (void *buffer, void *callback, char *ex_str, void *hdl)
{
  if (-1
      == (g_exech_build_string (buffer, &((__g_handle ) hdl)->exec_args.mech,
				(__g_handle) hdl, b_glob, MAX_EXEC_STR)))
    {
      b_glob[0] = 0x0;
      return -2;
    }

  return system (b_glob);
}

int
process_exec_string_n (char *input, char *output, size_t max_size,
		       _d_rtv_lk callback, void *data, void *mppd)
{
  __g_proc_v dt_rval;
  size_t ow = 0;
  char *op = output;
  char vb[MAX_VAR_LEN];

  while (input[0] && ow < max_size)
    {
      while (input[0] != 0x7B && input[0])
	{
	  op[0] = input[0];
	  input++;
	  op++;
	  ow++;
	}

      if (input[0] != 0x7B)
	{
	  break;
	}

      input++;

      dt_rval = callback (data, input, vb, MAX_VAR_LEN, mppd);

      if (dt_rval)
	{
	  char *dp = dt_rval (data, input, vb, MAX_VAR_LEN, mppd);
	  if (dp)
	    {
	      size_t dp_l = strlen (dp);
	      if (dp_l)
		{
		  strcp_s (op, max_size - ow, dp);
		  op = &op[dp_l];
		}
	    }
	}

      while (input[0] != 0x7D && input[0])
	{
	  input++;
	}

      if (!input[0])
	{
	  break;
	}

      input++;

    }

  op[0] = 0x0;

  return 0;
}

int
process_exec_string (char *input, char *output, size_t max_size, void *callback,
		     void *data)
{

  int
  (*call) (void *, char *, char *, size_t) = callback;

  size_t blen = strlen (input);

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
		  if (!i2 || strlen (buffer) > MAX_VAR_LEN
		      || (r = call (data, buffer, buffer2, MAX_VAR_LEN)))
		    {
		      if (r)
			{
			  b_l_1 = strlen (buffer);
			  snprintf (&output[pi],
			  MAX_EXEC_STR - pi - 2,
				    "%c%s%c", 0x7B, buffer, 0x7D);

			  pi += b_l_1 + 2;
			}
		      f |= 0x1;
		      break;
		    }
		  b_l_1 = strlen (buffer2);
		  memcpy (&output[pi], buffer2, b_l_1);

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
