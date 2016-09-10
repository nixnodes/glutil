/*
 * main.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "config.h"

#include <t_glob.h>
#include <l_error.h>
#include <cfgv.h>
#include <g_modes.h>
#include <macros_t.h>
#include <arg_proc.h>
#include <misc.h>
#include <signal_t.h>
#include <log_op.h>
#include <arg_opts.h>
#include <dirlog.h>
#include <log_io.h>
#include <exec_t.h>
#include <m_general.h>
#include <sort_hdr.h>
#include <misc.h>
#include <str.h>
#include <xref.h>
#include <x_f.h>
#include <omfp.h>
#include "lref_generic.h"
#ifdef _G_SSYS_NET
#include <glutil_net.h>
#endif
//#include <imdb_pload.h>
#include <g_help.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#ifdef _G_SSYS_THREAD
#include <pthread.h>
#endif

#include <lref.h>

int
(*print_str) (const char * volatile buf, ...) = NULL;

int
g_setxid (void)
{
  char e_buffer[1024];

  if (gfl0 & F_OPT_SETGID)
    {
      mda guid_stor =
	{ 0 };

      md_init (&guid_stor, 32);

      int r;
      if ((r = load_guid_info (&guid_stor, DEFPATH_GROUP)))
	{
	  print_str (MSG_GEN_NOFACC, "", DEFPATH_GROUP);
	  md_free (&guid_stor);
	  return 1;
	}

      p_gu_n pgn = search_xuid_name (&guid_stor, G_GROUP);
      if (pgn == NULL)
	{
	  print_str ("ERROR: group '%s' not found\n", G_GROUP);
	  md_free (&guid_stor);
	  return 1;
	}

      if (setgid (pgn->id) == -1)
	{
	  print_str ("ERROR: g_setxid: setgid failed: %s\n",
		     strerror_r (errno, e_buffer, sizeof(e_buffer)));
	  md_free (&guid_stor);
	  return 1;
	}

      if (gfl & F_OPT_VERBOSE)
	{
	  print_str ("NOTICE: setgid: %s, gid: %u\n", pgn->name, pgn->id);
	}

      md_free (&guid_stor);
      gfl0 ^= F_OPT_SETGID;
    }

  if (gfl0 & F_OPT_SETUID)
    {
      mda uuid_stor =
	{ 0 };

      md_init (&uuid_stor, 32);

      int r;
      if ((r = load_guid_info (&uuid_stor, DEFPATH_PASSWD)))
	{
	  print_str ( MSG_GEN_NOFACC, "", DEFPATH_PASSWD);
	  md_free (&uuid_stor);
	  return 1;
	}

      p_gu_n pgn = search_xuid_name (&uuid_stor, G_USER);
      if (pgn == NULL)
	{
	  print_str ("ERROR: user '%s' not found\n", G_USER);
	  md_free (&uuid_stor);
	  return 1;
	}

      if (setuid (pgn->id) == -1)
	{
	  print_str ("ERROR: g_setxid: setuid failed: %s\n",
		     strerror_r (errno, e_buffer, sizeof(e_buffer)));
	  md_free (&uuid_stor);
	  return 1;
	}
      if (gfl & F_OPT_VERBOSE)
	{
	  print_str ("NOTICE: setuid: %s, uid: %u\n", pgn->name, pgn->id);
	}

      md_free (&uuid_stor);
      gfl0 ^= F_OPT_SETUID;

    }
  return 0;
}

#include <arg_opts.h>



int
main (int argc, char *argv[])
{

  //((__o_dbent ) glob_dbe[0x2D].n_dbent)->l = 10;

  //register_option (opt_rec_upd_records, 0, "-r", glob_dbe);
  //register_option (opt_glroot, 1, "-bla", glob_dbe);
  //register_option (opt_dirlog_rb_full, 0, "--full", glob_dbe);

  //proc_option ("-test");

  //parse_args (argc, argv, NULL, NULL, 0);

  char **p_argv = _p_argv = (char**) argv;
  int r;

#ifdef _G_SSYS_THREAD
  mutex_init (&mutex_glob00, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST);
#endif

  print_str = g_print_str;
  __pf_eof = g_feof;
  fd_out = fileno (stdout);

  g_setjmp (0, "main", NULL, NULL);
  if ((r = setup_sighandlers ()))
    {
      print_str ("ERROR: could not setup signal handlers [%d]\n", r);
      return 2;
    }

  _p_macro_argc = argc;
  char **l_arg = NULL;

  if ((r = parse_args (argc, argv, gg_prio_f_ref, (char***) &l_arg,
  F_PARSE_ARG_SILENT | F_PARSE_ARG_IGNORE_NOT_FOUND)) > 0)
    {
      print_str (MSG_INIT_CMDLINE_ERROR, r);
      EXITVAL = 2;
      g_shutdown (NULL);
    }

  enable_logging ();

  switch (updmode)
    {
    case PRIO_UPD_MODE_MACRO:
      ;
      uint64_t gfl_s =
	  (gfl
	      & (F_OPT_WBUFFER | F_OPT_PS_LOGGING | F_OPT_NOGLCONF
		  | F_OPT_DAEMONIZE));
      char **ptr;
      ptr = process_macro (prio_argv_off, NULL);
      if (NULL != ptr)
	{
	  _p_macro_argv = p_argv = ptr;
	  gfl = gfl_s;
	}
      else
	{
	  g_shutdown (NULL);
	}
      break;
    case PRIO_UPD_MODE_INFO:
      g_print_info ();
      g_shutdown (NULL);
      break;
    case UPD_MODE_NOOP:
      goto exit_;
    }

  updmode = 0;

  g_init (_p_macro_argc, p_argv, l_arg);

  exit_: ;

  g_shutdown (NULL);

  return EXITVAL;
}

static void
g_throw_arerr (int r)
{
  if (r == -1)
    {
      print_version_long (NULL, 0, NULL);
    }
#ifdef _MAKE_SBIN
  print_str (HSTR_GFIND_USAGE);
#endif
  print_str ("See --help for more info\n");
  EXITVAL = 4;
}

int
g_init (int argc, char **argv, char **l_arg)
{


  g_setjmp (0, "g_init", NULL, NULL);

/*  htest();
     exit(0);*/

  int r = 0;

  if (strlen (LOGFILE))
    {
      gfl |= F_OPT_PS_LOGGING;
    }

#ifdef _MAKE_SBIN

  flags_udcfg |= F_PD_RECURSIVE;

  if ( argc < 2 )
    {
      //g_throw_arerr(-1);
      p_argv_off = ".";

    }
  else
    {
      r = parse_args(2, argv, gg_f_ref, NULL, F_PARSE_ARG_SILENT);
      size_t off = 0;

      if ( (r == -1 || r == -2) )
	{
	  p_argv_off = argv[1];
	  off++;
	}
      else
	{
	  p_argv_off = argv[argc - 1];
	}

      argc--;

      r = parse_args(argc, &argv[off], gg_f_ref, NULL, 0);

      if ( r == -1 )
	{
	  r = 0;
	}
    }

  if ( r == 0 )
    {
      updmode = UPD_MODE_DUMP_GEN;
      gfl |= F_OPT_VERBOSE;
    }
#else
  r = parse_args (argc, argv, gg_f_ref, NULL, 0);
#endif

  if (r == -2 || r == -1)
    {
      g_throw_arerr (r);
      return EXITVAL;
    }

  if (r > 0)
    {
      print_str (MSG_INIT_CMDLINE_ERROR, r);
      EXITVAL = 2;
      return EXITVAL;
    }

  enable_logging ();

  if (1 == g_setxid ())
    {
      EXITVAL = 1;
      return EXITVAL;
    }

  if (ofl & F_ESREDIRFAILED)
    {
      print_str ("ERROR: could not open file to redirect execv stdout to\n");
      EXITVAL = 2;
      return EXITVAL;
    }

  if (updmode && updmode != UPD_MODE_NOOP && !(gfl & F_OPT_FORMAT_BATCH)
      && !(gfl & F_OPT_FORMAT_COMP) && (gfl & F_OPT_VERBOSE2))
    {
      print_str ("INFO: %s_%s starting [PID: %d]\n",
      PACKAGE_NAME,
      BASE_VERSION, getpid ());
    }
#ifndef _MAKE_SBIN
  if (!(gfl & F_OPT_NOGLCONF))
    {
      if (strlen (GLCONF_I))
	{
	  if ((r = load_cfg (&glconf, GLCONF_I, F_LCONF_NORF, NULL))
	      && (gfl & F_OPT_VERBOSE))
	    {
	      print_str ("WARNING: %s: could not load GLCONF file [%d]\n",
			 GLCONF_I, r);
	    }

	  if ((gfl & F_OPT_VERBOSE4) && glconf.offset)
	    {
	      print_str ("NOTICE: %s: loaded %d config lines into memory\n",
			 GLCONF_I, (int) glconf.offset);
	    }

	  p_md_obj ptr = get_cfg_opt ("ipc_key", &glconf, NULL);

	  if (ptr && !(ofl & F_OVRR_IPC))
	    {
	      SHM_IPC = (key_t) strtol (ptr->ptr, NULL, 16);
	    }

#ifndef _GLCONF_NOROOTPATH
	  ptr = get_cfg_opt ("rootpath", &glconf, NULL);

	  if (ptr && !(ofl & F_OVRR_GLROOT))
	    {
	      snprintf (GLROOT, PATH_MAX, "%s", (char*) ptr->ptr);
	      if ((gfl & F_OPT_VERBOSE5))
		{
		  print_str ("NOTICE: GLCONF: using 'rootpath': %s\n", GLROOT);
		}
	    }
#endif
	  /*ptr = get_cfg_opt("min_homedir", &glconf, NULL);

	   if (ptr && !(ofl & F_OVRR_SITEROOT))
	   {
	   snprintf(SITEROOT_N, PATH_MAX, "%s", (char*) ptr->ptr);
	   if ((gfl & F_OPT_VERBOSE5))
	   {
	   print_str("NOTICE: GLCONF: using 'min_homedir': %s\n",
	   SITEROOT_N);
	   }
	   }*/

	  ptr = get_cfg_opt ("ftp-data", &glconf, NULL);

	  if (ptr)
	    {
	      snprintf (FTPDATA, PATH_MAX, "%s", (char*) ptr->ptr);
	      if ((gfl & F_OPT_VERBOSE5))
		{
		  print_str ("NOTICE: GLCONF: using 'ftp-data': %s\n", FTPDATA);
		}
	    }

	  ptr = get_cfg_opt ("nukedir_style", &glconf, NULL);

	  if (ptr)
	    {
	      NUKESTR = string_replace (ptr->ptr, "%N", "%s", NUKESTR, 255);
	      if ((gfl & F_OPT_VERBOSE5))
		{
		  print_str ("NOTICE: GLCONF: using 'nukedir_style': %s\n",
			     NUKESTR);
		}
	      ofl |= F_OVRR_NUKESTR;
	    }
	}
      else
	{
	  print_str ("WARNING: GLCONF not defined in glconf.h\n");
	}
    }

#ifndef _GLCONF_NOROOTPATH
  if (!strlen (GLROOT))
    {
      print_str ("ERROR: glftpd root directory not specified!\n");
      return 2;
    }
#endif

  if (!strlen (SITEROOT_N))
    {
      print_str ("ERROR: glftpd site root directory not specified!\n");
      return 2;
    }

  snprintf (SITEROOT, PATH_MAX, "%s%s", GLROOT, SITEROOT_N);
  remove_repeating_chars (SITEROOT, 0x2F);

  if (dir_exists (SITEROOT) && !dir_exists (SITEROOT_N))
    {
      strcp_s (SITEROOT, PATH_MAX, SITEROOT_N);
    }

  if ((gfl & F_OPT_VERBOSE) && dir_exists (SITEROOT))
    {
      print_str ("WARNING: siteroot '%s' not found\n", SITEROOT);
    }

  if (!updmode && (gfl & F_OPT_SFV))
    {
      updmode = UPD_MODE_RECURSIVE;
      if (!(gfl & F_OPT_NOWRITE))
	{
	  gfl |= F_OPT_FORCEWSFV | F_OPT_NOWRITE;
	}
      if (ofl & F_OVRR_GLROOT)
	{
	  print_str (MSG_INIT_PATH_OVERR, "GLROOT", GLROOT);
	}

      if (ofl & F_OVRR_SITEROOT)
	{
	  print_str (MSG_INIT_PATH_OVERR, "SITEROOT", SITEROOT);
	}
      if ((gfl & F_OPT_VERBOSE))
	{
	  print_str (
	      "NOTICE: switching to non-destructive filesystem rebuild mode\n");
	}
    }
#endif

  if ((gfl & F_OPT_VERBOSE))
    {
      if (gfl & F_OPT_NOBUFFER)
	{
	  print_str ("NOTICE: disabling memory buffering\n");
	  if (gfl & F_OPT_SHAREDMEM)
	    {
	      print_str (
		  "WARNING: --shmem: shared memory segment buffering option is invalid when --nobuffer specified\n");
	    }
	}
      if (SHM_IPC && SHM_IPC != shm_ipc)
	{
	  print_str ("NOTICE: IPC key set to '0x%.8X'\n", SHM_IPC);
	}

      if ((gfl & F_OPT_VERBOSE4) && (gfl & F_OPT_PS_LOGGING))
	{
	  print_str ("NOTICE: Logging enabled: %s\n", LOGFILE);
	}
    }

  if ((gfl & F_OPT_VERBOSE3) && (gfl & F_OPT_NOWRITE))
    {
      print_str ("WARNING: performing dry run, no writing will be done\n");
    }

  if (gfl & F_OPT_DAEMONIZE)
    {
      print_str ("NOTICE: forking into background.. [PID: %d]\n", getpid ());
      if (daemon (1, 0) == -1)
	{
	  print_str (
	      "ERROR: [%s] could not fork into background, terminating..\n",
	      strerror (errno));
	  EXITVAL = errno;
	  return errno;
	}
    }

  if (updmode && (gfl & F_OPT_PREEXEC))
    {
      if (gfl & F_OPT_VERBOSE)
	{
	  print_str ("PREEXEC: running: '%s'\n", GLOBAL_PREEXEC);
	}
      int r_e = 0;
      if ((r_e = g_do_exec (NULL, ref_to_val_generic, GLOBAL_PREEXEC, NULL))
	  == -1 || WEXITSTATUS(r_e))
	{
	  if (gfl & F_OPT_VERBOSE5)
	    {
	      print_str ("WARNING: [%d]: PREEXEC returned non-zero: '%s'\n",
			 WEXITSTATUS(r_e), GLOBAL_PREEXEC);
	    }
	  EXITVAL = WEXITSTATUS(r_e);
	  return 1;
	}
    }

  uint64_t mloop_c = 0;
  char m_b1[128];
  int m_f = 0x1;

  g_setjmp (0, "main(start)", NULL, NULL);

  enter:

  if (g_sleep)
    {
      sleep (g_sleep);
    }

  if (g_usleep)
    {
      usleep (g_usleep);
    }

  if ((m_f & 0x1))
    {
      snprintf (m_b1, 127, "main(loop) [c:%llu]",
		(long long unsigned int) mloop_c);
      g_setjmp (0, m_b1, NULL, NULL);
      m_f ^= 0x1;
    }

  switch (updmode)
    {
#ifndef _MAKE_SBIN
    case UPD_MODE_RECURSIVE:
      EXITVAL = rebuild_dirlog ();
      break;
    case UPD_MODE_SINGLE:
      EXITVAL = dirlog_update_record (argv_off);
      break;
    case UPD_MODE_CHECK:
      EXITVAL = dirlog_check_records ();
      break;
    case UPD_MODE_DUMP:
      EXITVAL = g_print_stats (DIRLOG, 0, 0);
      break;
    case UPD_MODE_DUMP_NUKE:
      EXITVAL = g_print_stats (NUKELOG, 0, 0);
      break;
    case UPD_MODE_DUMP_DUPEF:
      EXITVAL = g_print_stats (DUPEFILE, 0, 0);
      break;
    case UPD_MODE_DUMP_LON:
      EXITVAL = g_print_stats (LASTONLOG, 0, 0);
      break;
    case UPD_MODE_DUMP_ONEL:
      EXITVAL = g_print_stats (ONELINERS, 0, 0);
      break;
    case UPD_MODE_DUMP_IMDB:
      EXITVAL = g_print_stats (IMDBLOG, 0, 0);
      break;
    case UPD_MODE_DUMP_IMDBO:
      EXITVAL = g_print_stats (IMDBLOG_O, 0, 0);
      break;
    case UPD_MODE_DUMP_GAME:
      EXITVAL = g_print_stats (GAMELOG, 0, 0);
      break;
    case UPD_MODE_DUMP_TV:
      EXITVAL = g_print_stats (TVLOG, 0, 0);
      break;
    case UPD_MODE_DUMP_GENERIC:
      EXITVAL = d_gen_dump (p_argv_off);
      break;
    case UPD_MODE_DUPE_CHK:
      EXITVAL = dirlog_check_dupe ();
      break;
    case UPD_MODE_REBUILD:
      EXITVAL = rebuild (p_argv_off);
      break;
    case UPD_MODE_DUMP_ONL:
      EXITVAL = g_print_stats ("ONLINE USERS", F_DL_FOPEN_SHM, ON_SZ);
      break;
    case UPD_MODE_FORK:
      if (p_argv_off)
	{
	  int sret = system (p_argv_off), EXITVAL = WEXITSTATUS(sret);
	  if (0 != EXITVAL)
	    {
	      if (gfl & F_OPT_VERBOSE)
		{
		  print_str ("WARNING: '%s': command failed, code %d\n",
			     p_argv_off, EXITVAL);
		}
	    }
	}
      break;
    case UPD_MODE_BACKUP:
      EXITVAL = data_backup_records (g_dgetf (p_argv_off));
      break;
    case UPD_MODE_DUMP_USERS:
      EXITVAL = g_dump_ug (DEFPATH_USERS);
      break;
    case UPD_MODE_DUMP_GRPS:
      EXITVAL = g_dump_ug (DEFPATH_GROUPS);
      break;
    case UPD_MODE_WRITE:
      EXITVAL = d_write ((char*) p_argv_off);
      break;
    case PRIO_UPD_MODE_INFO:
      EXITVAL = g_print_info ();
      break;
    case UPD_MODE_LIST_MACROS:
      EXITVAL = list_macros ();
      break;
#endif
    case UPD_MODE_NOOP:
      break;
    case UPD_MODE_DUMP_GEN:
      EXITVAL = g_dump_gen (p_argv_off);
      break;
#ifdef _G_SSYS_NET
    case UPD_MODE_NETWORK:
      EXITVAL = net_deploy ();
      break;
#endif
    default:
      print_help (NULL, -1, NULL);
      print_str ("ERROR: no mode specified\n");
      break;
    }

  if (updmode && (gfl & F_OPT_POSTEXEC))
    {
      if (gfl & F_OPT_VERBOSE)
	{
	  print_str ("POSTEXEC: running: '%s'\n", GLOBAL_POSTEXEC);
	}
      if (g_do_exec (NULL, ref_to_val_generic, GLOBAL_POSTEXEC, NULL) == -1)
	{
	  if (gfl & F_OPT_VERBOSE)
	    {
	      print_str ("WARNING: POSTEXEC failed: '%s'\n", GLOBAL_POSTEXEC);
	    }
	}
    }

  if ((gfl & F_OPT_LOOP) && !(gfl & F_OPT_KILL_GLOBAL)
      && (!loop_max || mloop_c < loop_max - 1))
    {
      g_cleanup (&g_act_1);
      g_cleanup (&g_act_2);
      free_cfg_rf (&cfg_rf);

      /*if (gfl & F_OPT_LOOPEXEC)
       {
       g_do_exec(NULL, ref_to_val_generic, LOOPEXEC, NULL);
       }*/
      mloop_c++;
      goto enter;
    }

  return EXITVAL;
}

int
g_shutdown (void *arg)
{
  g_setjmp (0, "g_shutdown", NULL, NULL);

  g_cleanup (&g_act_1);
  g_cleanup (&g_act_2);
  free_cfg_rf (&cfg_rf);
  free_cfg (&glconf);

  if (-1 != fd_log)
    {
      close (fd_log);
    }

  if (_p_macro_argv)
    {
      free (_p_macro_argv);
    }

  if (GLOBAL_PREEXEC)
    {
      free (GLOBAL_PREEXEC);
    }

  if (GLOBAL_POSTEXEC)
    {
      free (GLOBAL_POSTEXEC);
    }

  if (LOOPEXEC)
    {
      free (LOOPEXEC);
    }

  if (exec_str)
    {
      free (exec_str);
    }

  if (_cl_print_ptr)
    {
      free (_cl_print_ptr);
    }

  if (exec_v)
    {
      int i;

      for (i = 0; i < exec_vc && exec_v[i]; i++)
	{
	  free (exec_v[i]);
	}
      free (exec_v);

    }

  if (execv_stdout_redir != -1)
    {
      close (execv_stdout_redir);
    }

  md_free (&_match_rr);
  md_free (&_md_gsort);
#ifdef _G_SSYS_NET
  md_free (&_boot_pca);
#endif

  _p_macro_argc = 0;

  exit (EXITVAL);
}
