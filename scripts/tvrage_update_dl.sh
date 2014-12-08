#!/bin/bash
#
#  Copyright (C) 2014 NixNodes
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
#@VERSION:0
#@REVISION:9
#
GLUTIL=/bin/glutil-chroot
TVRAGE_LOG=/ftp-data/glutil/db/tvrage.dlog
#
BASEDIR=`dirname "${0}"`

[ -f "${BASEDIR}/tvrage_update_dl_config" ] && { 
	. "${BASEDIR}/tvrage_update_dl_config"
}

GLROOT="${1}"

TVRAGE_LOG="${GLROOT}${TVRAGE_LOG}"

[ -f "${BASEDIR}/common" ] || { 
	echo "ERROR: ${BASEDIR}/common missing"
	exit 1 
}

. "${BASEDIR}/common"


I_DIR="${2}"
I_TIME="${3}"
I_SHOWID="${4}"
I_NAME="${5}"
I_LINK="${6}"
I_COUNTRY="${7}"
I_AIRTIME="${8}"
I_AIRDAY="${9}"
I_RUNTIME="${10}"
I_STARTED="${11}"
I_ENDED="${12}"
I_STARTYEAR="${13}"
I_ENDYEAR="${14}"
I_SEASONS="${15}"
I_CLASS="${16}"
I_GENRE="${17}"
I_NETWORK="${18}"
I_STATUS="${19}"

[ -z "${I_DIR}" ] && exit 1

try_lock_r 12 tvr_lk "`echo "${TVRAGE_LOG}" | md5sum | cut -d' ' -f1`" 120 "ERROR: could not obtain lock"

if [ -f "${TVRAGE_LOG}" ]; then	
	
	if ! ${GLUTIL} -e tvrage -ff --silent --tvlog "${TVRAGE_LOG}" --nofq -l: dir ! -match "${I_DIR}" --rev; then	
		echo "ERROR: -e failed:  ${I_DIR}, ${TVRAGE_LOG}"
		exit 1
	fi
	
	if ${GLUTIL} -q tvrage --tvlog "${TVRAGE_LOG}" -l: dir -match "${I_DIR}" --rev --silent; then
		echo "ERROR: old record still exists:  ${I_DIR}, ${TVRAGE_LOG}"
		exit 1
	fi
else
	f_create=1
fi

echo -e "dir ${I_DIR}\ntime ${I_TIME}\nshowid ${I_SHOWID}\nname ${I_NAME}\nlink ${I_LINK}\ncountry ${I_COUNTRY}\nairtime ${I_AIRTIME}\nairday ${I_AIRDAY}\nruntime ${I_RUNTIME}\nstarted ${I_STARTED}\nended ${I_ENDED}\nstartyear ${I_STARTYEAR}\nendyear ${I_ENDYEAR}\nseasons ${I_SEASONS}\nclass ${I_CLASS}\ngenre ${I_GENRE}\nnetwork ${I_NETWORK}\nstatus ${I_STATUS}\n" |
	${GLUTIL} -z tvrage --tvlog "${TVRAGE_LOG}" || {
		echo "ERROR: could not write ${TVRAGE_LOG}"
		exit 1
	}
	
[ -n "${f_create}" ] && {
	chmod -f 666 "${TVRAGE_LOG}"
	touch "${TVRAGE_LOG}.bk"
	chmod -f 666 "${TVRAGE_LOG}.bk"
}
	
exit 0