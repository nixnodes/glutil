#!/bin/bash
#
#  Copyright (C) 2013 NixNodes
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:1
#@REVISION:0
#@MACRO:loc-dm:{m:exe} --daemon --glroot "{m:glroot}" -x "{m:glroot}/io/glud" iregex "basepath,gld\.in\." --ilom "mode=1" --preexec `mkdir -p {m:glroot}/io/glud; chmod 777 {m:glroot}/io/glud` --loop 0 --usleep 300000 --silent -execv "{m:spec1} {path} {?rd:basepath:^gld\.in\.} {m:exe} {glroot}"
## Install script dependencies + libs into glftpd root, preserving library paths (requires mlocate)
#
#@MACRO:loc-dm-installch:{m:exe} noop --preexec `! updatedb -e "{glroot}" -o /tmp/glutil.mlocate.db && echo "updatedb failed" && exit 1 ; li="/bin/mkfifo"; for lli in $li; do lf=$(locate -d /tmp/glutil.mlocate.db "$lli" | head -1) && l=$(ldd "$lf" | awk '{print $3}' | grep -v ')' | sed '/^$/d' ) && for f in $l ; do [ -f "$f" ] && dn="/glftpd$(dirname $f)" && ! [ -d $dn ] && mkdir -p "$dn"; [ -f "{glroot}$f" ] || if cp --preserve=all "$f" "{glroot}$f"; then echo "$lf: {glroot}$f"; fi; done; [ -f "{glroot}/bin/$(basename "$lf")" ] || if cp --preserve=all "$lf" "{glroot}/bin/$(basename "$lf")"; then echo "{glroot}/bin/$(basename "$lf")"; fi; done; rm -f /tmp/glutil.mlocate.db`
#
## Offers functionality to processes that would otherwise require higher priviledges.
## The tasks it does are constricted and injection checks performed against user-
## specified input.
## Communicates with other processes via 'loc_cl.sh' using named pipes (FIFO)
#
## glutil -x searches for FIFO's in /io/glud in the background, this script
## is executed only when something is found.
#
## Avoid running this as root, use a normal user and give it access to what
## resources are needed.
#
#########################################################################

g_f="${1}"
g_pid=${2}
GLUTIL="${3}"
glroot=${4}

trap "rm -f ${g_f}; exit 2" 2 15 9 6

async_reply() {
	ar_bp="${glroot}/io/glud/gld.out.${g_pid}"
	${GLUTIL} noop --silent --preexec "{exe} noop --daemon --preexec \"sleep 5; kill -9 {procid}; rm -f $ar_bp\"" --fork "mkfifo \"${ar_bp}\"; echo \"${1}\" > \"${ar_bp}\"; rm -f \"${ar_bp}\""
	return 0
}

process_query(){	
	case ${1} in
    imdb-lookup ) # <query> <imdb log path> 
        ${GLUTIL} -m imdb-e -arg1 "${2}" --imdblog "${3}" &&
        	${GLUTIL} -q imdb --imdblog "${3}" -vvvv --shmem --shmdestroy --loadq &&
        	async_reply OK || async_reply BAD
        ;;
    tvrage-lookup ) # <query> <tvrage log path> 
        ${GLUTIL} -m tvrage-e -arg1 "${2}" --i "${3}" &&
        	${GLUTIL} -q tvrage --tvlog "${3}" -vvvv --shmem --shmdestroy --loadq &&
        	async_reply OK || async_reply BAD
        ;;
    *)
    	async_reply UNKNOWN
    	;;
    esac
}

e_m=`cat ${1}`

echo "${e_m}" | egrep -q '[\$\`]' && async_reply "ERROR" && {
	rm -f "${g_f}"
	exit 3
}

eval process_query ${e_m}

rm -f "${g_f}"