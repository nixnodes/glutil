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

if ENABLE_ARM
SHM_SRC = 
X64_LD = 
else
SHM_SRC = log_shm.c
X64_LD = -lrt -lm
endif

bin_PROGRAMS=$(GLUTIL_APP) $(PRECHECK_APP)

if ENABLE_NET_SUBSYS
GLUTIL_DEFAULTSRC=signal.c g_help.c memory.c hasht.c thread.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c taskd.c net_io.c net_proto.c net_gfs.c net_fs.c net_dis.c net_cmdproc.c net_ftp.c net_fsjproc.c glutil_net.c g_crypto.c log_io.c $(SHM_SRC)  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c base64.c xref.c lref.c lref_generic.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_imdb_old.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c
else
if ENABLE_CRYPTO
GLUTIL_DEFAULTSRC=signal.c g_help.c memory.c hasht.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c  $(SHM_SRC)  gv_off.c cfgv.c m_general.c sort.c m_string.c g_crypto.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c base64.c xref.c lref.c lref_generic.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_imdb_old.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c
else
GLUTIL_DEFAULTSRC=signal.c g_help.c memory.c hasht.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c  $(SHM_SRC)  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c base64.c xref.c lref.c lref_generic.c lref_dirlog.c lref_nukelog.c lref_dupefile.c lref_lastonlog.c lref_onliners.c lref_online.c lref_imdb.c lref_imdb_old.c lref_tvrage.c lref_game.c lref_gen1.c lref_gen2.c lref_gen3.c lref_gen4.c lref_sconf.c lref_gconf.c lref_altlog.c log_op.c omfp.c  dirlog.c
endif
endif

GLUTIL_PRECHECK_SRC=pce/pce_signal.c g_help.c memory.c hasht.c pce/pce_init.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c  $(SHM_SRC)  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c base64.c xref.c lref.c lref_generic.c lref_online.c lref_imdb.c lref_imdb_old.c lref_tvrage.c lref_game.c lref_sconf.c lref_gconf.c log_op.c omfp.c pce/pce_proc.c pce/pce_misc.c

glutil_SOURCES=$(GLUTIL_DEFAULTSRC)
glutil_chroot_SOURCES=$(GLUTIL_DEFAULTSRC)
glutil_precheck_SOURCES=$(GLUTIL_PRECHECK_SRC)
glutil_precheck_nonchroot_SOURCES=$(GLUTIL_PRECHECK_SRC)

if ENABLE_CRYPTO
gfind_SOURCES=signal.c g_help.c memory.c hasht.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c $(SHM_SRC)  gv_off.c cfgv.c m_general.c sort.c m_string.c g_crypto.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c base64.c xref.c lref.c lref_generic.c log_op.c omfp.c 
else
gfind_SOURCES=signal.c g_help.c memory.c hasht.c arg_opts.c init.c main.c macros.c l_error.c errno_int.c str.c time.c misc.c lc_oper.c x_f.c log_io.c $(SHM_SRC)  gv_off.c cfgv.c m_general.c sort.c m_string.c lref_glob.c m_lom.c i_math.c  arg_proc.c exech.c exec.c base64.c xref.c lref.c lref_generic.c log_op.c omfp.c 
endif

GLUTIL_DEFAULTLN=

glutil_LDADD=$(GLUTIL_DEFAULTLN) $(X64_LD)
glutil_chroot_LDADD=$(GLUTIL_DEFAULTLN) $(X64_LD)
glutil_precheck_LDADD=$(GLUTIL_DEFAULTLN) $(X64_LD)
glutil_precheck_nonchroot_LDADD=$(GLUTIL_DEFAULTLN) $(X64_LD)
gfind_LDADD=$(GLUTIL_DEFAULTLN) $(X64_LD)

AM_CPPFLAGS=-DGIT_SHORTREV=\"`git rev-parse --short=8 HEAD`\"