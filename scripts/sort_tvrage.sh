#!/bin/sh
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
#@REVISION:10
#@MACRO:tvrage-sort|Build sorted links based on TVRage data:{exe} --silent -q tvrage --sort asc,time --tvlog "{?q:tvrage@file}" -execv `{spec1} \{dir\} none \{?p:\} \{siterootb\} "{arg1} " \{genre\} \{startyear\} \{country\} \{airtime\} \{airday\} \{seasons\} \{class\} \{runtime\} \{network\} \{status\} \{(?L:endyear>=startyear:(?m:(endyear-startyear)):(?p:0))\}`
#
##
BASE_DIR=/glftpd/site/_sorted
#
##########################################

BASEDIR=`dirname ${0}`

[ -f "${BASEDIR}/common" ] || { 
	echo "ERROR: ${BASEDIR}/common missing"
	exit 2
}

. ${BASEDIR}/common

[ -f "${BASEDIR}/sort_common" ] || { 
	echo "ERROR: ${BASEDIR}/sort_common missing"
	exit 2
}

. ${BASEDIR}/sort_common

[ ${#5} -gt 1 ] && BASE_DIR=`echo "${5}" | sed -r 's/^[ ]+//' | sed -r 's/[ ]+$//'`

R_PATH="${1}"
R_ARG=${2}
R_GLROOT="${3}"
R_SITEROOT="${4}"
R_GENRE="${6}"
R_STARTYEAR="${7}"
R_COUNTRY="${8}"
R_AIRTIME="${9}"
R_AIRDAY="${10}"
R_SEASONS="${11}"
R_CLASS="${12}"
R_RUNTIME="${13}"
R_NETWORK="${14}"
R_STATUS="${15}"
RS_YEARRUN="${16}"


[ "${R_ARG}" = "mute" ] && OUT_PRINT=0

C_SITEROOT=`echo "${R_SITEROOT}" | sed -r 's/\//\\\\\//g'`


T_PATH=`echo "${R_PATH}" | sed -r "s/^${C_SITEROOT}//"`

[ -z "${T_PATH}" ] && print_str "failed extracting base target path" && exit 0

! [ -d "${BASE_DIR}" ] && { 
	mkdir -p "${BASE_DIR}" && 
		chmod 777 "${BASE_DIR}" || exit 2
	
}

BT_PATH=`dirname "${T_PATH}"`

[ -z "${BT_PATH}" ] || [ "${BT_PATH}" = "/" ] && print_str "failed extracting directory part of base path - ${BT_PATH}" && exit 0

DT_PATH="${BASE_DIR}${BT_PATH}"

! [ -d "${DT_PATH}" ] && {
 	mkdir -p "${DT_PATH}"  && 
 		 chmod 777 "${DT_PATH}"  ||	exit 2
	
}

B_PATH=`basename "${T_PATH}"`

C_GLROOT=`echo "${R_GLROOT}" | sed -r 's/\//\\\\\//g'`

CR_PATH=`echo "${R_PATH}" | sed -r "s/^${C_GLROOT}//"`

[ -n "${R_GENRE}" ] && proc_sort Genre "${R_GENRE}"
[ -n "${R_STARTYEAR}" ] && proc_sort Startyear ${R_STARTYEAR}
[ -n "${R_COUNTRY}" ] && proc_sort Country "${R_COUNTRY}"
[ -n "${R_AIRTIME}" ] && proc_sort Airtime "${R_AIRTIME}"
[ -n "${R_AIRDAY}" ] && proc_sort Airday "${R_AIRDAY}"
[ -n "${R_SEASONS}" ] && proc_sort Seasons "${R_SEASONS}"
[ -n "${R_CLASS}" ] && proc_sort Classification "${R_CLASS}"
[ -n "${R_RUNTIME}" ] && proc_sort Runtime "${R_RUNTIME}"
[ -n "${R_NETWORK}" ] && proc_sort Network "${R_NETWORK}"
[ -n "${R_STATUS}" ] && proc_sort Status "${R_STATUS}"
[ -n "${RS_YEARRUN}" ] && proc_sort Years-on-air "${RS_YEARRUN}"

chmod -Rf 777 "${BASE_DIR}" &> /dev/null

exit 0