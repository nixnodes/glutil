#!/bin/sh
#
#@VERSION:00
#@REVISION:04
#
GLUTIL=/bin/glutil-chroot
SQLITE=/bin/sqlite3
#
FILE_LOG_PATH=/ftp-data/glutil/db/filelog.db
#
###################################
#

g_file=`echo "${1}" | cut -f 2- -d " "`
g_user=${2}
g_path=`pwd`/${g_file}

${SQLITE} ${FILE_LOG_PATH} "DELETE FROM filelog WHERE path='${g_path}';"

exit 0

