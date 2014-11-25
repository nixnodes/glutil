if ENABLE_PRECHECK_NONCHROOT
PRECHECK_APP = glutil-precheck-nonchroot
endif

if ENABLE_PRECHECK
PRECHECK_APP = glutil-precheck
endif

if ENABLE_CHROOT_OWNBIN
GLUTIL_APP = glutil-chroot
endif

if ENABLE_SEARCHBIN
GLUTIL_APP = gfind
else
GLUTIL_APP = glutil
endif

bin_PROGRAMS=$(GLUTIL_APP) $(PRECHECK_APP)

if ENABLE_NET_SUBSYS
glutil_SOURCES=signal.c g_help.c memory.c thread.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c net_io.c net_proto.c glutil_net.c log_io.c log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c
glutil_chroot_SOURCES=signal.c g_help.c memory.c thread.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c net_io.c net_proto.c glutil_net.c log_io.c log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c 
glutil_precheck_SOURCES=pce/pce_signal.c g_help.c memory.c thread.c pce/pce_init.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c net_io.c net_proto.c  log_io.c  log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_sconf.c lref_gconf.c log_op.c omfp.c pce/pce_proc.c pce/pce_misc.c
glutil_precheck_nonchroot_SOURCES=pce/pce_signal.c g_help.c memory.c thread.c pce/pce_init.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c net_io.c net_proto.c log_io.c  log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_sconf.c lref_gconf.c log_op.c omfp.c pce/pce_proc.c pce/pce_misc.c
else
glutil_SOURCES=signal.c g_help.c memory.c thread.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c  log_io.c log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c
glutil_chroot_SOURCES=signal.c g_help.c memory.c thread.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c 
glutil_precheck_SOURCES=pce/pce_signal.c g_help.c memory.c thread.c pce/pce_init.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c  log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_sconf.c lref_gconf.c log_op.c omfp.c pce/pce_proc.c pce/pce_misc.c
glutil_precheck_nonchroot_SOURCES=pce/pce_signal.c g_help.c memory.c thread.c pce/pce_init.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.clog_io.c  log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c lref_online.c lref_imdb.c lref_tvrage.c lref_game.c lref_sconf.c lref_gconf.c log_op.c omfp.c pce/pce_proc.c pce/pce_misc.c
endif

gfind_SOURCES=signal.c g_help.c memory.c thread.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c log_shm.c  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c xref.c lref.c lref_gen.c log_op.c omfp.c 


glutil_LDADD=-lm 
glutil_chroot_LDADD=-lm 
glutil_precheck_LDADD=-lm 
glutil_precheck_nonchroot_LDADD=-lm 
gfind_LDADD=-lm