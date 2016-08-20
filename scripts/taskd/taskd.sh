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
#########################################################################
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:00
#@REVISION:21
#@MACRO:taskd-sha1gen|Generate SHA1 password:{exe} --noglconf -noop --preexec `[ -z "{?Q:(\{arg1\})}" ] && echo "Missing input" && exit 2;  echo {?S:(?Q:(\{arg1\}))}`
#@MACRO:taskd|taskd:{exe} -vvvv --user "{?x:(?Q:(\{?d:spec1\}/taskd_server_config)):RUN_USER}" --group "{?x:(?Q:(\{?d:spec1\}/taskd_server_config)):RUN_GROUP}" -netctl "threads_recv={?x:(?Q:(\{?d:spec1\}/taskd_server_config)):NET_THREADS}" -listen "mode=1|log=ge2|{?x:(?Q:(\{?d:spec1\}/taskd_server_config)):NET_OPTIONS}" {?x:(?Q:(\{?d:spec1\}/taskd_server_config)):LISTEN_ADDR} {?x:(?Q:(\{?d:spec1\}/taskd_server_config)):LISTEN_PORT} (  -l: "(?s:(?Q:(\{ge1\}\{ge2\}\{ge3\}\{ge4\}\{ge5\}\{ge6\}\{ge7\}\{ge8\})):ascii)" -match "1" -l: "(?Q:(\{ge1\}\{ge2\}\{ge3\}\{ge4\}\{ge5\}\{ge6\}\{ge7\}\{ge8\}))" ! -regex "[\$\`]" -l: "(?X:(isread):(?Q:(\{?d:spec1\}/taskd_server_config)))" -match "1" -l: "ge8" -regex "^......" -l: "(?S:(ge8))" -regexi "^{?x:(?Q:(\{?d:spec1\}/taskd_server_config)):SHA1_PW}$" ) --maxres 1 -execv `{spec1} {glroot} {exe} {procid} \{u1\} \{u2\} \{ge1\} \{ge2\} \{ge3\} \{ge4\}` --glroot "{glroot}" --noglconf {arg1}
#
## Offers functionality to processes that would otherwise require higher priviledges.
## The tasks it does are constricted and injection checks performed against user-
## specified input.
## Communicates with other processes via 'taskc.sh' using TCP/IP
#
## This is meant to run outside the chrooted environment.
#
#########################################################################

BASE_PATH=`dirname "${0}"`
GLROOT="${1}"
GLUTIL="${2}"
GL_HOST_PID=${3}

MODULES=()

[ -f "${BASE_PATH}/taskd_config" ] && . "${BASE_PATH}/taskd_config" || {
	echo "ERROR: ${BASE_PATH}/taskd_config: configuration file missing"
	exit 1
}

. ${BASE_PATH}/../common || {
	echo "ERROR: loading '${BASE_PATH}/../common' failed"
	exit 2
}

[ -z "${4}" ] && {
	exit 1
}

( [ ${4} -gt -1 ] && [ ${4} -lt 128 ] ) ||  {
	echo "ERROR: mode out of range"
	exit 1
}

MODE=${4}

register_module()
{
	MODULES[${1}]="${2}"
}

version_get()
{
	echo hello
}

check_illegal_string()
{
	[ -z "${1}" ] && return 0
	echo "${1}" | egrep -q '[^-_.,/()= a-zA-Z0-9]' && {
		echo "ERROR: illegal query"
		exit 1
	}
}

check_illegal_string "${5}"
check_illegal_string "${6}"
check_illegal_string "${7}"
check_illegal_string "${8}"

register_module 1 version_get

for mod in "${BASE_PATH}/${MODULES_PATH}"/${4}*; do
	. "${mod}" || {
		echo "ERROR: ${mod}: module load failed"
	}
done

[ -z ${MODULES[${MODE}]} ] && {
	echo "ERROR: unrecognized query ${MODE}"
	exit 1
}

${MODULES[${MODE}]} "${5}" "${6}" "${7}" "${8}"

exit $?