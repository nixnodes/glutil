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
#@REVISION:3
#
GLUTIL=/bin/glutil
IMDB_LOG=/ftp-data/glutil/db/imdb.dlog
#
BASEDIR=`dirname "${0}"`

GLROOT="${1}"

IMDB_LOG="${GLROOT}${IMDB_LOG}"

[ -f "${BASEDIR}/common" ] || { 
	echo "ERROR: ${BASEDIR}/common missing"
	exit 1 
}

. "${BASEDIR}/common"


I_DIR="${2}"
I_TIME="${3}"
I_IMDBID="${4}"
I_SCORE="${5}"
I_VOTES="${6}"
I_GENRE="${7}"
I_RATED="${8}"
I_TITLE="${9}"
I_DIRECTOR="${10}"
I_ACTORS="${11}"
I_RELEASED="${12}"
I_RUNTIME="${13}"
I_YEAR="${14}"
I_PLOT="${15}"

[ -z "${I_DIR}" ] && exit 1

[ -f "${IMDB_LOG}" ] && {
	try_lock_r 12 imdb_lk "`echo "${IMDB_LOG}" | md5sum | cut -d' ' -f1`" 120 "ERROR: could not obtain lock"
	
	if ! ${GLUTIL} -e imdb --imdblog "${IMDB_LOG}" --nofq -l: dir ! -match "${I_DIR}" --rev; then	
		echo "ERROR: -e failed:  ${I_DIR}, ${IMDB_LOG}"
	fi
	
	if ${GLUTIL} -q imdb --imdblog "${IMDB_LOG}" -l: dir -match "${I_DIR}" --rev --silent; then
		echo "ERROR: old record still exists:  ${I_DIR}, ${IMDB_LOG}"
		exit 2	
	fi
}

echo -e "dir ${I_DIR}\ntime ${I_TIME}\nimdbid ${I_IMDBID}\nscore ${I_SCORE}\nvotes ${I_VOTES}\ngenre ${I_GENRE}\nrated ${I_RATED}\ntitle ${I_TITLE}\ndirector ${I_DIRECTOR}\nactors ${I_ACTORS}\nreleased ${I_RELEASED}\nruntime ${I_RUNTIME}\nyear ${I_YEAR}\nplot ${I_PLOT}\n" |
	${GLUTIL} -z imdb --imdblog "${IMDB_LOG}" || {
		echo "ERROR: could not write ${IMDB_LOG}"
		exit 2
	}
	
exit 0