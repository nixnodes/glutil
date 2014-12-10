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
#@REVISION:30
#@MACRO:script-update-self|Update script-update.sh:{exe} -noop --preexec `B="https://raw.githubusercontent.com/nixnodes/glutil/master/scripts/";D="{?d:(spec1)}";[ -d "$\{D\}" ] || mkdir "$\{D\}"; if curl --silent "$\{B\}"script_update.sh > "$\{D\}/script_update.sh"; then echo -e "$\{D\}/script_update.sh \t\tv$(cat "$\{D\}/script_update.sh" | egrep "^\#\@VERSION\:" | cut -d ":" -f 2).$(cat "$\{D\}/script_update.sh" | egrep "^\#\@REVISION\:" | cut -d ":" -f 2) \tOK"; chmod 755 "$\{D\}/script_update.sh"; else echo "$\{D\}/script_update.sh \tFAILED"; fi; `
#@MACRO:script-update|Install/update native scripts <-arg 1 <install|update>> [ -arg 2 <regex filter> ]:{exe} -noop --preexec `{spec1} {glroot} "{arg1}" "{arg2}"`
#
## Requires: - glutil-2.5 or above
##             curl
#
## Automatically updates scripts from a web repository
#
## Usage: './glutil -m script-update -arg 1 <install|update> [-arg 2 "<script1|script2|..>]"'
#
## <script1|script2|..> is matched against the first token of each element
## in ${SOURCES} array (see 'script_update.d' folder)
#
BASE_PATH=`dirname "${0}"`

GLROOT="${1}"

script_get_version()
{
	[ -f "${1}" ] || {
		echo 0
		return 0
	}
	g_vers=`cat "${1}" | egrep "^#@VERSION" | sed -r 's/(^#@VERSION:)//g'`
	
	[ -z "${g_vers}" ] && {
		g_vers=0
	}
	
	echo -n ${g_vers}
}

script_get_revision()
{
	[ -f "${1}" ] || {
		echo 0
		return 0
	}
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

script_fetch()
{
	${CURL} ${CURL_FLAGS} "${1}" > /tmp/glutil.script_update.$$.tmp || {
		echo "ERROR: ${name}: ${CURL}: could not fetch: '${BASE_URL}/${path}'"
		return 1
	}
	[ -f /tmp/glutil.script_update.$$.tmp ] || {
		echo "ERROR: ${name}: ${path}: missing temp file ( ${1} -> /tmp/glutil.script_update.$$.tmp )"		
		return 1
	}
	b_size=`stat -c %s /tmp/glutil.script_update.$$.tmp`
		
	[ ${b_size} -eq 0 ] && {
		echo "ERROR: ${name}: ${path}: recieved no data ( ${1} -> /tmp/glutil.script_update.$$.tmp )"
		return 1
	}
	
	[ ${b_size} -lt 50 ] && {
		echo "ERROR: ${name}: ${path}: recieved invalid data ( ${1} -> /tmp/glutil.script_update.$$.tmp )"
		return 1
	}
	return 0
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

script_do_proc()
{
	[ -n "${INPUT_DEPENDENCIES}" ] && {
		[ ${VERBOSE} -gt 0 ] && echo "DEPS: ${1}: processing dependencies.."
		script_process_source INPUT_DEPENDENCIES[@] ".*" 0 || {
			echo "ERROR: ${in_source}: failed processing dependencies"
			return 1
		}
	}	
	
	script_process_source INPUT_SOURCES[@] "${2}" ${3}
	
	return $?
}

script_process_source()
{
	add_filt=`echo "${add_filt}" | sed -r 's/(\|$)|(^\|)//g'`
	
	spc_ret=0
	declare -a input_source_ar=("${!1}")
	for item in "${input_source_ar[@]}"; do
		name=`echo "${item}" | cut -d' ' -f1`
		
		[ -z "${name}" ] && {
			echo "ERROR: empty source definition in config"
			exit 1
		}
		
		echo "${name}" | egrep -q "^${2}$" || {
			[ ${VERBOSE} -gt 1 ] && echo "NOTICE: ${name}: ${path}: filter caught item, skipping.."
			continue
		}
				
		path=`echo "${item}" | cut -d' ' -f2`
		
		[ -z "${path}" ] && {
			echo "ERROR: ${name}: bad configuration, missing script path"
			exit 1
		}
		
		opt=`echo "${item}" | cut -d' ' -f4`
		
		force=0
		skip_fetch=0
		
		if [[ ${opt} -eq 2 ]]; then
			[ -d "${GLROOT}/${BASE_SEARCHDIR}" ] &&
				touch "${GLROOT}/${BASE_SEARCHDIR}/${path}"
			[ ${VERBOSE} -gt 0 ] && echo "NOTICE: ${name}: ${path}: updates disabled"
			continue
		elif [[ ${opt} -eq 4 ]]; then
			[ -f "${GLROOT}/${BASE_SEARCHDIR}/${path}" ] && 
				rm -f "${GLROOT}/${BASE_SEARCHDIR}/${path}" &&
				echo "DELETE: ${name}: ${path}"
			continue
		elif [[ ${opt} -eq 1 ]]; then
			[ -f "${GLROOT}${BASE_SEARCHDIR}/${path}" ] && {
				[ ${VERBOSE} -gt 1 ] &&  echo "WARNING: ${name}: ${path}: configuration file already exists"
				continue
			}
		elif [[ ${opt} -eq 8 ]]; then
			[ -d "${GLROOT}${BASE_SEARCHDIR}/${path}" ] && {
				[ ${VERBOSE} -gt 1 ] &&  echo "WARNING: ${name}: ${path}: folder already exists"
				continue
			}
			mkdir -p "${GLROOT}${BASE_SEARCHDIR}/${path}"
			perm_mask=`echo "${item}" | cut -d' ' -f3`
			
			[ -z "${perm_mask}" ] && {
				echo "ERROR: ${name}: ${path}: could not get folder perms"
				continue
			}			
			
			chmod ${perm_mask} "${GLROOT}${BASE_SEARCHDIR}/${path}"
			
			continue
		elif [[ ${opt} -eq 6 ]]; then			
			req_version=`echo "${item}" | cut -d' ' -f6`
			[ -z "${req_version}" ] && {
				echo "ERROR: ${name}: bad configuration, missing version string for dependency"
				exit 1
			}
			current_ver=`script_get_version "${GLROOT}/${BASE_SEARCHDIR}/${path}"``script_get_revision "${GLROOT}/${BASE_SEARCHDIR}/${path}"`
			
			if [ ${current_ver} -lt ${req_version} ]; then
				[ ${VERBOSE} -gt 3 ] && echo "NOTICE: ${name}: updating for dependency ( ${current_ver} < ${req_version} )"
	
				force=1
				
				script_fetch "${BASE_URL}/${path}" || {
					spc_ret=2
					continue
				}

				skip_fetch=1
				
				upgrade_ver=`script_get_version /tmp/glutil.script_update.$$.tmp``script_get_revision /tmp/glutil.script_update.$$.tmp`
				
				[ ${upgrade_ver} -lt ${req_version} ] && {
					echo "ERROR: ${name}: ${path}: unmet dependency ( ${upgrade_ver} < ${req_version} )"
					spc_ret=2
					continue
				}
				
				[ ${VERBOSE} -gt 0 ] && echo "NOTICE ${name}: ${path}: installing for dependencies ( ${upgrade_ver} >= ${req_version} )"
			else
				[ ${VERBOSE} -gt 0 ] && echo "NOTICE ${name}: ${path}: dependencies met ( ${current_ver} >= ${req_version} )"
				continue			
			fi			
		fi
		
		echo "${3}" | egrep -q '1' && [ ${force} -eq 0 ] && {
			[ -f "${GLROOT}/${BASE_SEARCHDIR}/${path}" ] || {
				[ ${VERBOSE} -gt 0 ] && echo "NOTICE: ${name}: ${path}: doesn't exist, skipping.."
				continue
			}
		}
		
		[ -n "${add_filt}" ] && [ ${force} -eq 0 ] && {
			echo "${name}" | egrep -q "^(${add_filt})$" && {
				[ ${VERBOSE} -gt 2 ] && echo "NOTICE: ${name}: ${path}: already processed"
				continue
			}
		}
		
		add_filt="${add_filt}|${name}"
		
		[ ${VERBOSE} -gt 3 ] && echo "NOTICE: ${name}: ${path}: processing.."
		
		[ ${skip_fetch} -eq 0 ] && {
			script_fetch "${BASE_URL}/${path}" || {
				spc_ret=2
				continue
			}
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
				echo "WARNING: ${name}: ${path}: current version is newer than remote (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` > `script_join_verstring /tmp/glutil.script_update.$$.tmp`), not updating.."
				continue
			fi
			
			if [ ${upgrade_ver} -eq ${current_ver} ]; then
				[ ${VERBOSE} -gt 1 ] && echo "WARNING: ${name}: ${path}: already installed (`script_join_verstring "${GLROOT}/${BASE_SEARCHDIR}/${path}"` = `script_join_verstring /tmp/glutil.script_update.$$.tmp`), not updating.."
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
			INPUT_SOURCES=()
			. "${GLROOT}/${BASE_SEARCHDIR}/${path}" || {
				echo "ERROR: ${in_source}: failed loading source"
				exit 1
			}
			
			[ -n "${INPUT_SOURCES}" ] && [ ${VERBOSE} -gt 0 ] && {
				echo "NOTICE: ${name}: ${path}: processing sources"
				
				p_flags="${3}"
				
				echo "${3}" | egrep -q '3' && {
					echo "${3}" | egrep -q '1' || {
						p_flags="${p_flags}1"
					}
				}
				
				script_do_proc "${name}" "${2}" ${p_flags} || {
					spc_ret=2
					continue
				}
				
				#script_process_source INPUT_SOURCES[@] "${2}" ${p_flags}
			}
		}
		
	done
		
	return ${spc_ret}
}

SOURCES=()

INIT_SOURCES=(
	"init/00main scripts/script_update.d/00main 644 0 1"	
)

trap "rm -f /tmp/glutil.script_update.$$.tmp" EXIT


add_filt=""

if [ -d "${BASE_PATH}/script_update.d" ]; then
	script_process_source INIT_SOURCES[@] ".*" 3
else
	script_process_source INIT_SOURCES[@] ".*" 3 || {
		echo "ERROR: could not initialize '${BASE_PATH}/script_update.d'"
		exit 1
	}
	
	mkdir -p "${BASE_PATH}/script_update.d"
fi

flags=""

if [ "${2}" = "update" ]; then	
	flags="${flags}1"
elif [ "${2}" = "install" ]; then
	flags="${flags}0"
else
	echo "ERROR: invalid argument: '${2}'"
	exit 1
fi

[ -n "${3}" ] && match="${3}" || match=".*"

[ ${VERBOSE} -gt 0 ] && echo "NOTICE: processing main sources"

for in_source in "${BASE_PATH}/script_update.d"/*; do
	INPUT_SOURCES=()
	INPUT_DEPENDENCIES=()
	. "${in_source}" || {
		echo "ERROR: ${in_source}: failed loading source"
		exit 1
	}
	
	script_do_proc "${in_source}" "${match}" ${flags} 
		
	#[ -n "${INPUT_SOURCES}" ] && script_inject_sources INPUT_SOURCES[@]
done




exit 0