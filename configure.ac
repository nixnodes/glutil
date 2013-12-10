dnl Process this file with autoconf to produce a configure script.

#AC_PREREQ(2.13)

AC_INIT(glutil, [2.3.1d], [devel@nixnodes.net])

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_CC
AC_PROG_CC_STDC

CFLAGS="-g -Wall"

AC_CONFIG_FILES(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src/])

AC_OUTPUT



dnl Process this file with autoconf to produce a configure script.


#AC_CONFIG_HEADERS([config.h])

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdarg.h unistd.h errno.h dirent.h time.h \
				  	signal.h string.h regex.h libgen.h utime.h  limits.h setjmp.h \
				  	fcntl.h sys/stat.h sys/wait.h sys/types.h sys/shm.h inttypes.h \
				  	stdint.h "$PWD/src/glconf.h"],,AC_MSG_ERROR([missing required header file!]))
				  	

AC_SUBST(CFLAGS)  	

# Checks for typedefs, structures, and compiler characteristics.
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


# Checks for library functions.
AC_CHECK_FUNCS([bzero memcpy memmove memset _exit exit strcmp strncmp \
				regcomp strerror usleep sleep syscall \
				printf vsprintf vprintf fflush daemon waitpid \
				regcomp regexec snprintf strtoull strtoul strtoll \
				strtol strtof  strlen calloc sysconf dup2 execv \
				signal sigaction sigemptyset sigfillset readdir opendir fopen system fprintf \
				fclose shmget shmat shmctl dirname basename],,
				AC_MSG_ERROR([missing procedure!]))



