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
#@REVISION:12
#@MACRO:script-update-self|Update script-update.sh:{exe} -noop --preexec `B="https://raw.githubusercontent.com/nixnodes/glutil/master/scripts/";D="{?d:(spec1)}";[ -d "$\{D\}" ] || mkdir "$\{D\}"; if curl --silent "$\{B\}"script_update.sh > "$\{D\}/script_update.sh"; then echo -e "$\{D\}/script_update.sh \t\tv$(cat "$\{D\}/script_update.sh" | egrep "^\#\@VERSION\:" | cut -d ":" -f 2).$(cat "$\{D\}/script_update.sh" | egrep "^\#\@REVISION\:" | cut -d ":" -f 2) \tOK"; chmod 755 "$\{D\}/script_update.sh"; else echo "$\{D\}/script_update.sh \tFAILED"; fi; `
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

script_register_update()
{
	SOURCES+=("${1}")
}

script_inject_sources()
{
	declare -a input_ar=("${!1}")
	for item in "${input_ar[@]}"; do
		script_register_update "${item}"
	done
}

[ -f "${BASE_PATH}/script_update_config" ] && . ${BASE_PATH}/script_update_config || {
	echo "ERROR: "${BASE_PATH}/script_update_config": missing configuration file"
	exit 1
}

[ -z "${GLROOT}" ] && {
	echo "ERROR: could not get glroot" 
	exit 1
}

[ -d "${GLROOT}/${BASE_SEARCHDIR}" ] || {
	echo "ERROR: glftpd environment not sane: missing root directory '${GLROOT}/${BASE_SEARCHDIR}'"
	exit 1
}

script_process_source()
{
	spc_ret=0
	declare -a input_source_ar=("${!1}")
	for item in "${input_source_ar[@]}"; do
		name=`echo "${item}" | cut -d' ' -f1`
		
		[ -z "${name}" ] && {
			echo "ERROR: empty source definition in config"
			exit 2
		}
		
		path=`echo "${item}" | cut -d' ' -f2`
		
		[ -z "${path}" ] && {
			echo "ERROR: bad configuration, missing script path: ${name}"
			exit 2
		}
		
		opt=`echo "${item}" | cut -d' ' -f4`
		
		if [[ ${opt} -eq 2 ]]; then
			[ -d "${GLROOT}/${BASE_SEARCHDIR}" ] &&
				touch "${GLROOT}/${BASE_SEARCHDIR}/${path}"
			[ ${VERBOSE} -gt 0 ] && echo "NOTICE: ${path}: updates disabled"
		elif [[ ${opt} -eq 1 ]]; then
			[ -f "${GLROOT}${BASE_SEARCHDIR}/${path}" ] && {
				[ ${VERBOSE} -gt 0 ] &&  echo "WARNING: ${path}: configuration file already exists"
				continue
			}
		fi
		
		echo "${name}" | egrep -q "^${2}$" && {
			${CURL} ${CURL_FLAGS} "${BASE_URL}/${path}" > /tmp/glutil.script_update.$$.tmp || {
				echo "ERROR: ${CURL}: could not fetch: '${BASE_URL}/${path}'"
				spc_ret=2
				continue
			}
			
			[ -f /tmp/glutil.script_update.$$.tmp ] || {
				echo "ERROR: ${path}: missing temp file ( ${BASE_URL}/${path} -> /tmp/glutil.script_update.$$.tmp )"
				spc_ret=2
				continue
			}
			
			b_size=`stat -c %s /tmp/glutil.script_update.$$.tmp`
			
			[ ${b_size} -eq 0 ] && {
				echo "ERROR: ${path}: recieved no data ( ${BASE_URL}/${path} -> /tmp/glutil.script_update.$$.tmp )"
				spc_ret=2
				continue
			}
			
			[ ${b_size} -lt 50 ] && {
				echo "ERROR: ${path}: recieved invalid data ( ${BASE_URL}/${path} -> /tmp/glutil.script_update.$$.tmp )"
				spc_ret=2
				continue
			}
			
			TARGET_DIR=`dirname "${GLROOT}/${BASE_SEARCHDIR}/${path}"`
			[ -d "${TARGET_DIR}" ] || {
				mkdir -p "${TARGET_DIR}" || {
					echo "ERROR: could not create '${GLROOT}/${BASE_SEARCHDIR}/${path}'"
					spc_ret=$[spc_ret+1]
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
					[ ${VERBOSE} -gt 0 ] && echo "WARNING: ${path}: already installed (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` = `script_join_verstring /tmp/glutil.script_update.$$.tmp`), not updating.."
					continue
				fi
				
				echo "${mode}: ${path}: (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` -> `script_join_verstring /tmp/glutil.script_update.$$.tmp`)"
			else
				mode="INSTALL"
				echo "${mode}: ${path}: (`script_join_verstring /tmp/glutil.script_update.$$.tmp`)"
			fi
					
			cat /tmp/glutil.script_update.$$.tmp > "${GLROOT}${BASE_SEARCHDIR}/${path}" || {
				spc_ret=2
				echo "ERROR: ${path}: update failed ( /tmp/glutil.script_update.$$.tmp )"
				continue
			}
			
			perm_mask=`echo "${item}" | cut -d' ' -f3`
		
			[ -z "${perm_mask}" ] && {		
				perm_mask=755
			}
			
			chmod ${perm_mask} "${GLROOT}/${BASE_SEARCHDIR}/${path}"
			
			opt_a=`echo "${item}" | cut -d' ' -f5`
			
			[[ ${opt_a} -eq 1 ]] && { 
				. "${GLROOT}/${BASE_SEARCHDIR}/${path}" || {
					echo "ERROR: ${in_source}: failed loading source"
					return 1
				}
				script_process_source INPUT_SOURCES[@] "${match}"
			}
		}
	done
	
	return ${spc_ret}
}

SOURCES=()

INIT_SOURCES=(
	"init/00main scripts/script_update.d/00main 644 0"	
)

trap "rm -f /tmp/glutil.script_update.$$.tmp" EXIT

if [ -d "${BASE_PATH}/script_update.d" ]; then
	script_process_source INIT_SOURCES[@] ".*"
else
	script_process_source INIT_SOURCES[@] ".*" || {
		echo "ERROR: could not initialize '${BASE_PATH}/script_update.d'"
		exit 1
	}
	
	mkdir -p "${BASE_PATH}/script_update.d"
fi

if [ "${1}" = all ]; then
	match=".*"
else
	match="${1}"
fi

for in_source in "${BASE_PATH}/script_update.d"/*; do
	. "${in_source}" || {
		echo "ERROR: ${in_source}: failed loading source"
		exit 1
	}
	script_inject_sources INPUT_SOURCES[@]
done

script_process_source SOURCES[@] "${match}"

