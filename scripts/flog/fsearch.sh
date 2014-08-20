#!/bin/sh
#@VERSION:0
#@REVISION:3
#
#GLUTIL=/bin/glutil-chroot
SQLITE=/bin/sqlite3
#
FILE_LOG_PATH=/ftp-data/glutil/db/filelog.db
#
############################################
#

echo "${@}" | egrep -q "([\`\>\<\|]|\$\()" && exit 2

[ -z "${@}" ] && exit 2

${SQLITE} -separator '    ' ${FILE_LOG_PATH} "select path, (round(size/1024,2)), uid, gid from filelog where path LIKE '%${@}%';"

exit 0

