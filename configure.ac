dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)


AC_INIT(glutil, [2.3.4d], [devel@nixnodes.net],,["http://glutil.nixnodes.net/"])
#LT_INIT

AC_CANONICAL_SYSTEM
AC_CANONICAL_BUILD

AM_INIT_AUTOMAKE([subdir-objects])

AM_PROG_CC_C_O
AC_PROG_CC_STDC

AC_CONFIG_FILES(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src])

AC_PREFIX_DEFAULT(["/glftpd"])

CFLAGS="-g -Wall"
AC_SUBST(CFLAGS)

AS_IF([test "$prefix" = "NONE"], [AC_DEFINE_UNQUOTED([CONF_PREFIX], "$ac_default_prefix") ], [AC_DEFINE_UNQUOTED([CONF_PREFIX], "$prefix" )])
AS_IF([test "$prefix" = "NONE"], AS_VAR_SET(prefix, [$ac_default_prefix]))

AC_CHECK_FILE( \
["$PWD/src/glconf.h"],, \
AC_CHECK_FILE(["$prefix/bin/sources/glconf.h"], \
	[AC_MSG_NOTICE(copying '$prefix/bin/sources/glconf.h' to 'src/' folder..); \
	AS_IF([cp "$prefix/bin/sources/glconf.h" "$PWD/src/glconf.h"],, \
	[AC_MSG_ERROR([could not copy "$prefix/bin/sources/glconf.h" to "$PWD/src/glconf.h"])]) \
]) \
)

AC_OUTPUT

AC_CHECK_HEADERS([stdio.h stdarg.h unistd.h errno.h dirent.h time.h \
				  	signal.h string.h regex.h libgen.h utime.h  limits.h setjmp.h \
				  	fcntl.h sys/stat.h sys/wait.h sys/types.h sys/shm.h inttypes.h \
				  	stdint.h "$PWD/src/glconf.h"],,[AC_MSG_ERROR([missing required header file!])])
		 	

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


AC_CHECK_FUNCS([bzero memcpy memmove memset _exit exit strcmp strncmp \
				regcomp strerror usleep sleep syscall \
				printf vsprintf vprintf fflush daemon waitpid \
				regcomp regexec snprintf strtoull strtoul strtoll \
				strtol strtof  strlen calloc sysconf dup2 execv \
				signal sigaction sigemptyset sigfillset readdir opendir fopen system fprintf \
				fclose shmget shmat shmctl dirname basename],,
				AC_MSG_ERROR([missing procedure!]))



