#!/bin/sh
#
GLUTIL=/bin/glutil-chroot
FILE_LOG_PATH=/ftp-data/glutil/db/filelog
#
###################################
#

g_file=`echo "${1}" | cut -f 2- -d " "`
g_user=${2}
g_path=`pwd`/${g_file}

${GLUTIL} -q altlog --altlog ${FILE_LOG_PATH} --silent --imatch "file,${g_path}" --nobuffer && {
	${GLUTIL} -q altlog --altlog ${FILE_LOG_PATH} -vvvv --gz 2 --raw --match "file,${g_path}" --nobuffer > /tmp/mki.d.$$.tmp
	mv /tmp/mki.d.$$.tmp ${FILE_LOG_PATH}
}

exit 0
