#!/bin/sh
#@VERSION:0
#@REVISION:2
#
GLUTIL=/bin/glutil-chroot
#
FILE_LOG_PATH=/ftp-data/glutil/db/filelog
#
REMOVE_EXISTING=0
#
###################################
#

g_file=`echo "${1}" | cut -f 2- -d " "`
g_user=${2}
g_path=`pwd`/${g_file}

[ ${REMOVE_EXISTING} -gt 0 ] && {
	${GLUTIL} -q altlog --altlog ${FILE_LOG_PATH} --silent --imatch "file,${g_path}" --nobuffer && {
		${GLUTIL} -q altlog --altlog ${FILE_LOG_PATH} -vvvv --gz 2 --raw --match "file,${g_path}" --nobuffer > /tmp/mki.d.$$.tmp
		mv /tmp/mki.d.$$.tmp ${FILE_LOG_PATH}
	}
}

${GLUTIL} --silent -x "${g_path}" \
 	-print "file {path}{:n}size {size}{:n}time {mtime}{:n}user {uid}{:n}group {gid}{:n}status 0{:n}files 0{:n}" \
 	--noglconf | ${GLUTIL} -z altlog --nobackup --noglconf --altlog ${FILE_LOG_PATH} --gz 2 --silent

exit 0

