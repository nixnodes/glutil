AC_PREREQ(2.63)


AC_INIT([glutil], [2.6.7], [devel@nixnodes.net],,[http://glutil.nixnodes.net/]) 
AC_CONFIG_SRCDIR([src/main.c]) 

AM_INIT_AUTOMAKE([subdir-objects silent-rules]) 
AM_MAINTAINER_MODE([disable])
AM_PROG_CC_C_O
AM_SILENT_RULES([yes])

AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_FILES([Makefile src/Makefile]) 
AC_CONFIG_SRCDIR([src])

AC_PREFIX_DEFAULT(["/glftpd"])

AC_DEFINE(_LARGE_FILES, [1], [Large files])
AC_DEFINE(_LARGEFILE_SOURCE, [1], [Large files])
AC_DEFINE(_LARGEFILE_SOURCE64, [1],[Large files])
AC_DEFINE(_FILE_OFFSET_BITS, [64],[Large files])

AC_ARG_ENABLE(chroot,
[  --enable-chroot		Build glutil for chrooted environment [default=no]],
glutil_chroot="$enableval", glutil_chroot="no")

AC_ARG_ENABLE(chroot-ownbin,
[  --enable-chroot-ownbin	Build glutil for chrooted environment as a separate binary [default=no]],
glutil_chroot_ownbin="$enableval", glutil_chroot_ownbin="no")

AC_ARG_ENABLE(gzip,
[  --enable-gzip			Enable gzip compression support [default=no]],
glutil_enable_gzip="yes", glutil_enable_gzip="no" )

AC_ARG_ENABLE(gfind,
[  --enable-gfind		Compile glutil-based search tool [default=no]],
glutil_enable_sbin="yes", glutil_enable_sbin="no" )

AC_ARG_ENABLE(networking,
[  --enable-networking		Enable network subsystem (MT engine) [default=no]],
glutil_enable_net="yes", glutil_enable_net="no" )

AC_ARG_ENABLE(crypto,
[  --enable-crypto		Enable OpenSSL cryptography functions [default=no]],
glutil_enable_crypto="yes", glutil_enable_crypto="no" )

AC_ARG_ENABLE(precheck,
[  --enable-precheck		Build glutil-precheck [default=no]],
glutil_precheck="$enableval", glutil_precheck="no")

AC_ARG_ENABLE(precheck-nonchroot,
[  --enable-precheck-nonchrot	Build glutil-precheck for non-chrooted environment [default=no]],
glutil_precheck_nonchroot="$enableval", glutil_precheck_nonchroot="no")

if [ test "$glutil_precheck_nonchroot" = "yes" ]; then	
	AC_DEFINE_UNQUOTED([PATH_PREFIX], ["$prefix"])	
fi

if [ test "$glutil_enable_gzip" = "yes" ]; then	
	LDFLAGS="${LDFLAGS} -lz"
fi

if [ test "$glutil_enable_sbin" = "yes" ]; then
	AC_DEFINE([_MAKE_SBIN], [1], [//])
	glutil_enable_gzip="no"
fi

if [ test "$glutil_enable_net" = "yes" ]; then
 	if [ test "$glutil_enable_sbin" = "no" ]; then
 		AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_FAILURE([could not find ssl libs])])
 		AC_CHECK_HEADER([openssl/ssl.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (openssl/ssl.h)])])
 		AC_CHECK_HEADER([openssl/err.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (openssl/err.h)])]) 		
		AC_DEFINE([_G_SSYS_NET], [1], [//])
		AC_DEFINE([_G_SSYS_THREAD], [1], [//])				
		AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_FAILURE([could not find libpthread])])
		AC_CHECK_HEADER([pthread.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (pthread.h)])])
		glutil_enable_crypto="yes"
	else
		glutil_enable_net="no"
	fi
fi

if [ test "$glutil_enable_crypto" = "yes" ]; then
	AC_CHECK_LIB([crypto], [CRYPTO_set_id_callback], [], [AC_MSG_FAILURE([could not find libcrypto])])
	AC_DEFINE([_G_SSYS_CRYPTO], [1], [//])
fi

if [ test "$glutil_chroot" = "no" ] && [ test "$glutil_chroot_ownbin" = "no" ]; then
	AS_IF([test "$prefix" = "NONE"], 
		[AC_DEFINE_UNQUOTED([CONF_PREFIX], ["$ac_default_prefix"]) ], 
		[AC_DEFINE_UNQUOTED([CONF_PREFIX], ["$prefix"] )] \
	)	
	AS_IF([test "$prefix" = "NONE"], AS_VAR_SET(prefix, [$ac_default_prefix]))
else 
	AC_DEFINE_UNQUOTED([CONF_PREFIX], [""], [//])
	AC_DEFINE([_GLCONF_NOROOTPATH], [], [//])
	AS_IF([test "$glutil_chroot_ownbin" = "yes"], 
		[AC_MSG_NOTICE(compiling for chrooted environment (glutil-chroot)..)],
		[AC_MSG_NOTICE(compiling for chrooted environment..)]) 
fi

if [ test "$glutil_enable_sbin" = "yes" ]; then
	AC_DEFINE([_G_MODE_GFIND])
fi

AM_CONDITIONAL(ENABLE_CHROOT_OWNBIN, test "$glutil_chroot_ownbin" = "yes")

AS_VAR_SET(datadir, ["$prefix/scripts"])

if echo $host | egrep -q "arm|eabi"; then
	AC_DEFINE([NO_SHAREDMEM])
	AC_DEFINE([H_ISARM])
fi

AM_CONDITIONAL(ENABLE_PRECHECK_NONCHROOT, test "$glutil_precheck_nonchroot" = "yes")	
AM_CONDITIONAL(ENABLE_PRECHECK, test "$glutil_precheck" = "yes")
AM_CONDITIONAL(ENABLE_SEARCHBIN, test "$glutil_enable_sbin" = "yes")
AM_CONDITIONAL(ENABLE_NET_SUBSYS, test "$glutil_enable_net" = "yes")
AM_CONDITIONAL(ENABLE_CRYPTO, test "$glutil_enable_crypto" = "yes")
AM_CONDITIONAL(ENABLE_ARM, echo $host | egrep -q "arm|eabi")

AC_SUBST(GLUTIL_PREFIX, [$prefix])

CFLAGS="-Wall -O2 -fcommon"
AC_SUBST(CFLAGS)

AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_HEADER_MAJOR

GLUTIL_MSG_REQH="missing required header file"

AS_IF([test "$glutil_enable_gzip" = "yes"], \
AC_CHECK_HEADERS([zlib.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (zlib.h); install zlib-devel or remove --enable-gzip])]) \
)

AC_CHECK_HEADER([fcntl.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (fnctl.h)])])
AC_CHECK_HEADER([limits.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (limits.h)])])
AC_CHECK_HEADER([utime.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (utime.h)])])
AC_CHECK_HEADER([errno.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (errno.h)])])

if [ ! test "$host" ] && [ test "$glutil_enable_sbin" = "no" ]; then
	AC_CHECK_FILE(["$PWD/src/glconf.h"],, \
	AC_CHECK_FILE(["$prefix/bin/sources/glconf.h"], \
		[AC_MSG_NOTICE(copying '$prefix/bin/sources/glconf.h' to 'src/' folder..); \
		AS_IF([cp "$prefix/bin/sources/glconf.h" "$PWD/src/glconf.h"],, \
		[AC_MSG_ERROR([could not copy "$prefix/bin/sources/glconf.h" to "$PWD/src/glconf.h"])]) \
	]) \
	)
	
	AC_CHECK_HEADER([$PWD/src/glconf.h],,[AC_MSG_ERROR([$GLUTIL_MSG_REQH (src/glconf.h)])])
fi


AC_HEADER_STDC

GLUTIL_MSG_FH="missing procedure"

if [ ! test "$host" ]; then
	AC_CHECK_FUNCS([bzero],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (bzero)])])
fi

AC_CHECK_FUNCS([dup2],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (dup2)])])
AC_CHECK_FUNCS([memmove],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (memmove)])])
AC_CHECK_FUNCS([memset],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (memset)])])
AC_CHECK_FUNCS([regcomp],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (regcomp)])])
AC_CHECK_FUNCS([strchr],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strchr)])])
AC_CHECK_FUNCS([strdup],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strdup)])])
AC_CHECK_FUNCS([strerror],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strerror)])])
AC_CHECK_FUNCS([strrchr],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strrchr)])])
AC_CHECK_FUNCS([strstr],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strstr)])])
AC_CHECK_FUNCS([strtol],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strtol)])])
AC_CHECK_FUNCS([strtoul],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strtoul)])])
AC_CHECK_FUNCS([strtoull],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (strtoull)])])
AC_CHECK_FUNCS([utime],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (utime)])])

#AS_IF([test "$glutil_enable_gzip" = "yes"], \
#AC_CHECK_FUNCS([gzread],,[AC_MSG_ERROR([$GLUTIL_MSG_FH (gzread)])])
#)

AC_FUNC_FORK
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC


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

AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SSIZE_T

AC_OUTPUT
