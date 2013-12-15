SUBDIRS=src

include_HEADERS=src/arg_opts.h src/arg_proc.h src/cfgv.h src/config.h src/dirlog.h src/exech.h src/exec_t.h src/fp_types.h src/g_conf.h src/g_help.h src/glc.h src/glconf.h src/glutil.h src/g_modes.h src/g_types.h src/gv_off.h src/i_math.h src/im_hdr.h src/lc_oper.h src/l_error.h src/log_io.h src/log_op.h src/log_shm.h src/lref_dirlog.h src/lref_dupefile.h src/lref_game.h src/lref_gconf.h src/lref_gen1.h src/lref_gen2.h src/lref_gen3.h src/lref_gen4.h src/lref_gen.h src/lref.h src/lref_imdb.h src/lref_lastonlog.h src/lref_nukelog.h src/lref_online.h src/lref_onliners.h src/lref_sconf.h src/lref_tvrage.h src/l_sb.h src/macros_t.h src/mc_glob.h src/m_comp.h src/memory_t.h src/m_general.h src/misc.h src/m_lom.h src/m_string.h src/omfp.h src/signal_t.h src/sort_hdr.h src/str.h src/t_glob.h src/timeh.h src/x_f.h src/xref.h

SCRIPTS_DIST           	= 	scripts
GLDATA_DIST				= 	ftp-data
MISC_DIST				= 	misc

dist-hook:			
		cp -Rf $(SCRIPTS_DIST) $(distdir)/; \	 	
	 	chmod 755 $(distdir)/$(SCRIPTS_DIST)/*.sh; \	 	
	 	cp -Rf $(GLDATA_DIST) $(distdir)/; \
	 	cp -Rf $(MISC_DIST) $(distdir)/; \
	 	chmod 755 $(distdir)/$(MISC_DIST)/macros.glutil