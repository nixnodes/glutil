#!/bin/sh
#@VERSION:0
#@REVISION:2
#GLUTIL=/bin/glutil-chroot
SQLITE=/bin/sqlite3
#
FILE_LOG_PATH=/ftp-data/glutil/db/filelog.db
#
############################################
#

echo "${@}" | egrep -q "^[-._\%\*\(\) a-zA-Z0-9]+$" || exit 2

echo "${@}" | egrep -q "^$" && exit 2

qry=`echo "${@}" | tr '*' '%'`

${SQLITE} -init `dirname ${0}`/fsearch.sqlite3 ${FILE_LOG_PATH} "select path AS Path, (round(size/1024.0,2)) AS 'Size (K)', \
        uid AS UID, gid AS GID from filelog where path LIKE '%${qry}%';" .

exit 0
