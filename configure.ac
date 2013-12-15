AC_PREREQ(2.63)


AC_INIT([glutil], [2.3.5d], [devel@nixnodes.net],,["http://glutil.nixnodes.net/"]) 
AC_CONFIG_SRCDIR([src/main.c]) 
AM_INIT_AUTOMAKE([subdir-objects]) 

#LT_INIT


AM_PROG_CC_C_O
AC_PROG_CC_STDC

AC_CONFIG_FILES(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src])

AC_PREFIX_DEFAULT(["/glftpd"])

AM_MAINTAINER_MODE([disable])

AC_DEFINE(_LARGE_FILES, [1], [Large files])
AC_DEFINE(_LARGEFILE_SOURCE, [1], [Large files])
AC_DEFINE(_LARGEFILE_SOURCE64, [1],[Large files])
AC_DEFINE(_FILE_OFFSET_BITS, [64],[Large files])

AC_ARG_ENABLE(chroot,
[  --enable-chroot	  build glutil for chrooted environment [default=no]],
glutil_chroot="$enableval", glutil_chroot="no")

AC_ARG_ENABLE(chroot-ownbin,
[  --enable-chroot-ownbin	  build glutil for chrooted environment as a separate binary [default=no]],
glutil_chroot_ownbin="$enableval", glutil_chroot_ownbin="no")

if [ test "$glutil_chroot" = "no" ] && [ test "$glutil_chroot_ownbin" = "no" ]; then
	AS_IF([test "$prefix" = "NONE"], 
		[AC_DEFINE_UNQUOTED([CONF_PREFIX], "$ac_default_prefix") ], 
		[AC_DEFINE_UNQUOTED([CONF_PREFIX], "$prefix" )] \
	)	
	AS_IF([test "$prefix" = "NONE"], AS_VAR_SET(prefix, [$ac_default_prefix]))
else 
	AC_DEFINE_UNQUOTED([CONF_PREFIX], "")
	AC_DEFINE([_GLCONF_NOROOTPATH])
	AS_IF([test "$glutil_chroot_ownbin" = "yes"],
		[AC_MSG_NOTICE(compiling for chrooted environment (glutil-chroot)..)],
		[AC_MSG_NOTICE(compiling for chrooted environment..)]) 
fi

AM_CONDITIONAL(ENABLE_CHROOT_OWNBIN, test "$glutil_chroot_ownbin" = "yes")

AC_CHECK_FILE( \
["$PWD/src/glconf.h"],, \
AC_CHECK_FILE(["$prefix/bin/sources/glconf.h"], \
	[AC_MSG_NOTICE(copying '$prefix/bin/sources/glconf.h' to 'src/' folder..); \
	AS_IF([cp "$prefix/bin/sources/glconf.h" "$PWD/src/glconf.h"],, \
	[AC_MSG_ERROR([could not copy "$prefix/bin/sources/glconf.h" to "$PWD/src/glconf.h"])]) \
]) \
)

AS_VAR_SET(datadir, ["$prefix/scripts"])

AC_ARG_ENABLE(precheck,
[  --enable-precheck	  build glutil-precheck [default=no]],
glutil_precheck="$enableval", glutil_precheck="no")

AM_CONDITIONAL(ENABLE_PRECHECK, test "$glutil_precheck" = "yes")

AC_SUBST(GLUTIL_PREFIX, [$prefix])

CFLAGS="-g -Wall"
AC_SUBST(CFLAGS)

AC_OUTPUT

GLUTIL_MSG_REQH="missing required header file"
AC_CHECK_HEADER([stdarg.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (stdarg.h)])])
AC_CHECK_HEADER([fcntl.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (fnctl.h)])])
AC_CHECK_HEADER([limits.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (limits.h)])])
AC_CHECK_HEADER([utime.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (utime.h)])])
AC_CHECK_HEADER([errno.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (errno.h)])])
AC_CHECK_HEADER([signal.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (signal.h)])])
AC_CHECK_HEADER([dirent.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (dirent.h)])])
AC_CHECK_HEADER([time.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (time.h)])])
AC_CHECK_HEADER([utime.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (utime.h)])])
AC_CHECK_HEADER([inttypes.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (inttypes.h)])])
AC_CHECK_HEADER([sys/shm.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (sys/shm.h)])])
AC_CHECK_HEADER([sys/types.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (sys/types.h)])])
AC_CHECK_HEADER([sys/wait.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (sys/wait.h)])])
AC_CHECK_HEADER([sys/stat.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (sys/stat.h)])])

#AC_CHECK_HEADER([stdio.h],,[AC_MSG_ERROR([missing required header file!])])


#AC_CHECK_HEADERS([stdio.h stdarg.h unistd.h errno.h dirent.h time.h \
#				  	signal.h string.h regex.h libgen.h utime.h  limits.h setjmp.h \
#				  	fcntl.h sys/stat.h sys/wait.h sys/types.h sys/shm.h inttypes.h \
#				  	stdint.h "$PWD/src/glconf.h"],,[AC_MSG_ERROR([missing required header file!])])
		 	


AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UNSIGNED_LONG_LONG_INT
AC_TYPE_LONG_LONG_INT
AC_TYPE_SIGNAL

AC_FUNC_FORK
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_HEADER_MAJOR
AC_STRUCT_ST_BLOCKS
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SSIZE_T

AC_CHECK_FUNCS([bzero memcpy memmove memset _exit exit strcmp strncmp \
				regcomp strerror usleep sleep syscall \
				printf vsprintf vprintf fflush daemon waitpid \
				regcomp regexec snprintf strtoull strtoul strtoll utime \
				strtol strtof  strlen calloc sysconf dup2 execv strstr \
				signal sigaction sigemptyset sigfillset readdir opendir fopen system fprintf \
				fclose shmget shmat shmctl dirname basename strchr strdup strrchr],,
				AC_MSG_ERROR([missing procedure!]))


AC_CHECK_MEMBERS([struct stat.st_blksize])
