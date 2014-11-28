#!/bin/sh
#@VERSION:0
#@REVISION:5
#@MACRO:filelog-rebuild|Rebuild the filelog based on filesystem info:{m:exe} -noop --preexec `gl_root={siteroot};{m:exe} -x {siteroot} -R --preexec "echo \"CREATE TABLE filelog(ID INTEGER PRIMARY KEY, path TEXT, size INTEGER, time DATETIME, uid INTEGER, gid INTEGER, status INTEGER);\"" lom "mode!=8" or regex "(basepath)=^\." -printf "INSERT INTO filelog values(NULL,\"{?rd:path:^$gl_root}\",{size},{mtime},{uid},{gid},0);" | sqlite3 /tmp/glutil.frb.$$.tmp; mv /tmp/glutil.frb.$$.tmp {m:q:altlog@file}; chmod 666 {m:q:altlog@file}`
#@MACRO:flog-installch|Install required libraries into glFTPd root (mk_index.sh):{m:exe} -noop --preexec `! updatedb -e "{glroot}" -o /tmp/glutil.mlocate.db && echo "updatedb failed" && exit 1 ; li="/bin/sqlite3 /bin/cut"; for lli in $li; do lf=$(locate -d /tmp/glutil.mlocate.db "$lli" | head -1) && l=$(ldd "$lf" | awk '{print $3}' | grep -v ')' | sed '/^$/d' ) && for f in $l ; do [ -f "$f" ] && dn="/glftpd$(dirname $f)" && ! [ -d $dn ] && mkdir -p "$dn"; [ -f "{glroot}$f" ] || if cp --preserve=all "$f" "{glroot}$f"; then echo "$lf: {glroot}$f"; fi; done; [ -f "{glroot}/bin/$(basename "$lf")" ] || if cp --preserve=all "$lf" "{glroot}/bin/$(basename "$lf")"; then echo "{glroot}/bin/$(basename "$lf")"; fi; done; rm -f /tmp/glutil.mlocate.db`
#
GLUTIL=/bin/glutil-chroot
SQLITE=/bin/sqlite3
#
FILE_LOG_PATH=/ftp-data/glutil/db/filelog.db
#
REMOVE_EXISTING=0
#
###################################
#

g_file=`echo "${1}" | cut -f 2- -d " "`
#g_user=${2}
g_path=`pwd`/${g_file}

#[ ${REMOVE_EXISTING} -gt 0 ] && {

#}

${GLUTIL} --silent -x "${g_path}" \
        -print "INSERT INTO filelog values(NULL,\"{?rd:path:^\/[^\/]+}\",{size},{mtime},{uid},{gid},0);" \
        --noglconf | ${SQLITE} ${FILE_LOG_PATH}

exit 0
