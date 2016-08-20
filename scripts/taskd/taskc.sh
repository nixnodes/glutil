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
#@REVISION:12
#
#@MACRO:taskc-installch:{exe} -noop --preexec `! updatedb -e "{glroot}" -o /tmp/glutil.mlocate.$$.db && echo "updatedb failed" && exit 1 ; li="/bin/nc /bin/socat"; for lli in $li; do lf=$(locate -d /tmp/glutil.mlocate.$$.db "$lli" | head -1) && l=$(ldd "$lf" | awk '\{print $3\}' | grep -v ')' | sed '/^$/d' ) && for f in $l ; do [ -f "$f" ] && dn="/glftpd$(dirname $f)" && ! [ -d $dn ] && mkdir -p "$dn"; [ -f "{glroot}$f" ] || if cp --preserve=all "$f" "{glroot}$f"; then echo "$lf: {glroot}$f"; fi; done; [ -f "{glroot}/bin/$(basename "$lf")" ] || if cp --preserve=all "$lf" "{glroot}/bin/$(basename "$lf")"; then echo "{glroot}/bin/$(basename "$lf")"; fi; done; rm -f /tmp/glutil.mlocate.$$.db`
#
#########################################################################

BASE_PATH=`dirname "${0}"`

[ -f "${BASE_PATH}/taskc_config" ] && . "${BASE_PATH}/taskc_config" || {
	echo "ERROR: ${BASE_PATH}/taskc_config: configuration file missing"
	exit 1
}

MODE="${1}"

[ -z "${MODE}" ] && {
	echo "ERROR: missing mode"
	exit 1
}

echo "${MODE}" | egrep -q '^[0-9]+$' || {
	echo "ERROR: invalid mode"
	exit 1
}

out_U1="${MODE}"
out_U2="${2}"

echo "${out_U1}" | egrep -q '^[0-9]+$' || {
	echo "ERROR: u1: invalid argument"
	exit 1
}

NL="
"

out_GE1="${3}"
out_GE2="${4}"
out_GE3="${5}"
out_GE4="${6}"

[ -n "${out_GE1}" ] && b_proc="${b_proc}ge1 ${out_GE1}${NL}"
[ -n "${out_GE2}" ] && b_proc="${b_proc}ge2 ${out_GE2}${NL}"
[ -n "${out_GE3}" ] && b_proc="${b_proc}ge3 ${out_GE3}${NL}"
[ -n "${out_GE4}" ] && b_proc="${b_proc}ge4 ${out_GE4}${NL}"

if [ ${SSL} -eq 1 ]; then 
	echo -e "u1 ${out_U1}\nu2 ${out_U2}\nge8 ${PASS}\n${b_proc}\n" | 
		${GLUTIL} -z ge2 --raw | ${SOCAT} -t 60 stdio "openssl-connect:${CONNECT_IP}:${CONNECT_PORT},verify=${SSL_VERIFY}"
else
	echo -e "u1 ${out_U1}\nu2 ${out_U2}\nge8 ${PASS}\n${b_proc}\n" | 
		${GLUTIL} -z ge2 --raw | ${NC} ${CONNECT_IP} ${CONNECT_PORT}
fi

exit 0