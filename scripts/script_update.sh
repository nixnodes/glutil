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
#@REVISION:2
#@MACRO:script-update|Install/update native scripts:{exe} -noop --preexec `{spec1} "{arg1}" {glroot}`
#
## Requires: - glutil-2.5 or above
##             curl
#
## Automatically updates scripts from a web repository
#
## Usage: './glutil -m script-update -arg 1 "<script1|script2|..>"'
#
## <script1|script2|..> is matched against the first token of each element
## in ${SOURCES} array, see see 'script_update_config' file
#
## Run './glutil -m script-update -arg 1 all' to update everything configured
## in ${SOURCES}
#
BASE_PATH=`dirname "${0}"`

GLROOT="${2}"

script_get_version()
{
	g_vers=`cat "${1}" | egrep "^#@VERSION" | sed -r 's/(^#@VERSION:)//g'`
	
	[ -z "${g_vers}" ] && {
		g_vers=0
	}
	
	echo -n ${g_vers}
}

script_get_revision()
{
	g_rev=`cat "${1}" | egrep "^#@REVISION" | sed -r 's/(^#@REVISION:)//g'`
	
	[ -z "${g_rev}" ] && {
		g_rev=0
	}
	echo -n ${g_rev}
}

script_join_verstring()
{
	echo "`script_get_version "${1}"`.`script_get_revision "${1}"`"
}

[ -z "${GLROOT}" ] && {
	echo "ERROR: could not get glroot" 
	exit 1
}

[ -d "${GLROOT}/${BASE_SEARCHDIR}" ] || {
	echo "ERROR: glftpd environment not sane: missing root directory '${GLROOT}/${BASE_SEARCHDIR}'"
	exit 1
}

if [ "${1}" = all ]; then
	match=".*"
else
	match="${1}"
fi

[ -f "${BASE_PATH}/script_update_config" ] && . ${BASE_PATH}/script_update_config

trap "rm -f /tmp/glutil.script_update.$$.tmp" 2 15 9 6 EXIT

for item in "${SOURCES[@]}"; do
	name=`echo "${item}" | cut -d' ' -f1`
	
	[ -z "${name}" ] && {
		echo "ERROR: empty source definition in config"
		exit 2
	}
	
	path=`echo "${item}" | cut -d' ' -f2`
	
	[ -z "${path}" ] && {
		echo "WARNING: missing script path: ${name}"
		continue
	}
	

	echo "${name}" | egrep -q "^${match}$" && {
		${CURL} ${CURL_FLAGS} "${BASE_URL}/${path}" > /tmp/glutil.script_update.$$.tmp || {
			echo "ERROR: ${CURL}: could not fetch: '${BASE_URL}/${path}'"
			continue
		}
		
		[ -f /tmp/glutil.script_update.$$.tmp ] || {
			echo "ERROR: ${path}: missing temp file ( ${BASE_URL}/${path} -> /tmp/glutil.script_update.$$.tmp )"
			continue
		}
		
		b_size=`stat -c %s /tmp/glutil.script_update.$$.tmp`
		
		[ ${b_size} -eq 0 ] && {
			echo "ERROR: ${path}: recieved no data ( ${BASE_URL}/${path} -> /tmp/glutil.script_update.$$.tmp )"
			continue
		}
		
		[ ${b_size} -lt 50 ] && {
			echo "ERROR: ${path}: recieved invalid data ( ${BASE_URL}/${path} -> /tmp/glutil.script_update.$$.tmp )"
			continue
		}
		
		TARGET_DIR=`dirname "${GLROOT}/${BASE_SEARCHDIR}/${path}"`
		[ -d "${TARGET_DIR}" ] || {
			mkdir -p "${TARGET_DIR}" || {
				echo "ERROR: could not create '${GLROOT}/${BASE_SEARCHDIR}/${path}'"
				continue
			}
		}
		
		if [ -f "${GLROOT}/${BASE_SEARCHDIR}/${path}" ]; then 
			mode="UPDATE"
			current_ver=`script_get_version "${GLROOT}/${BASE_SEARCHDIR}/${path}"``script_get_revision "${GLROOT}/${BASE_SEARCHDIR}/${path}"`
			upgrade_ver=`script_get_version /tmp/glutil.script_update.$$.tmp``script_get_revision /tmp/glutil.script_update.$$.tmp`
			
			if [ ${upgrade_ver} -lt ${current_ver} ]; then
				echo "WARNING: ${path}: current version is newer than remote (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` > `script_join_verstring /tmp/glutil.script_update.$$.tmp`), not updating.."
				continue
			fi
			
			if [ ${upgrade_ver} -eq ${current_ver} ]; then
				echo "WARNING: ${path}: already installed (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` = `script_join_verstring /tmp/glutil.script_update.$$.tmp`), not updating.."
				continue
			fi
			
			echo "${mode}: ${path}: (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` -> `script_join_verstring /tmp/glutil.script_update.$$.tmp`)"
		else
			mode="INSTALL"
			echo "${mode}: ${path}: (`script_join_verstring /tmp/glutil.script_update.$$.tmp`)"
		fi
		cat /tmp/glutil.script_update.$$.tmp > "${GLROOT}${BASE_SEARCHDIR}/${path}" || {
			echo "ERROR: ${path}: update failed!"
			continue
		}
		
		perm_mask=`echo "${item}" | cut -d' ' -f3`
	
		[ -z "${perm_mask}" ] && {		
			perm_mask=755
		}
		
		chmod ${perm_mask} "${GLROOT}/${BASE_SEARCHDIR}/${path}"
	}
done