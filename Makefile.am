SUBDIRS=src

SCRIPTS_DIST           	= 	scripts
GLDATA_DIST				= 	ftp-data
MISC_DIST				= 	misc

dist-hook:
		cp -Rf $(SCRIPTS_DIST) $(distdir)/; \
	 	chmod 755 $(distdir)/$(SCRIPTS_DIST)/*.sh; \
	 	cp -Rf $(GLDATA_DIST) $(distdir)/; \
	 	cp -Rf $(MISC_DIST) $(distdir)/; \
	 	chmod 755 $(distdir)/$(MISC_DIST)/macros.glutil