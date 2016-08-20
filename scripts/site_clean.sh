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
#@VERSION:00
#@REVISION:12
#@MACRO:site-clean|Usage\: -m site-clean [-arg1=<config file>]:{exe} -noop --postexec "{spec1} {arg1}"
#
## Dependencies:    glutil-2.6.2
#
GLUTIL="/bin/glutil"
DF=/bin/df

[ -n "${1}" ] && {
	. ${1}
} || {
	. `dirname ${0}`/site_clean_config
}

########################################

d_used()
{
	${DF} | egrep "${1}" | awk '{print $3}'
}

d_free()
{
	${DF} | egrep "${1}" | awk '{print $4}'
}

proc_tgmk_str()
{
	mod=0
	case ${1} in
		*K)			
			mod=1024
		;;
		*M)			
			mod=1048576
		;;
		*G)			
			mod=1073741824
		;;
		*T)
			mod=1099511627776			
		;;
	esac
	p=`echo "${1}" | sed -r 's/[^0-9]//'`
	echo `expr ${p} \* ${mod}`
}

pinc() {
	echo `expr ${1} + 1`
}

get_action()
{
	c=0
	for l in "${ACTIONS[@]}"; do
		[ `expr ${c} \% 3` -gt 0 ] && c=`pinc ${c}` && continue
		[ "${l}" = "${1}" ] && {
			echo ${ACTIONS[`pinc ${c}`]}
			break
		}
		c=`pinc ${c}`
	done
}

get_post_action()
{
	c=0
	for l in "${ACTIONS[@]}"; do
		[ `expr ${c} \% 3` -gt 0 ] && c=`pinc ${c}` && continue
		[ "${l}" = "${1}" ] && {
			echo ${ACTIONS[`expr ${c} + 2`]}
			break
		}
		c=`pinc ${c}`
	done
}

MIN_FREE=`proc_tgmk_str ${MIN_FREE}`

free=`expr $(d_free ${DEVICE}) \* 1024`

[ ${ALWAYS_RUN} -eq 0 ] && 
	[ ${free} -gt ${MIN_FREE} ] && exit 0

used=`expr $(d_used ${DEVICE}) \* 1024`
total=`expr ${free} + ${used}`

max=`expr ${total} - ${MIN_FREE}`

[ ${max} -lt 0 ] && echo "${path}: negative maximum, MIN_FREE shouldn't exceed device size" && exit 2


for i in "${SECTIONS[@]}"; do
	path=`echo ${i} | cut -d " " -f1`	
	[ -z "${path}" ] && continue	
	percent=`echo ${i} | cut -d " " -f2`	
	[ -z "${percent}" ] && echo "${path}: missing percentage value" && continue	
	action=`echo ${i} | cut -d " " -f3`			
	[ -z "${action}" ] && echo "${path}: missing action" && continue	
	action_cmd=`get_action "${action}"`	
	[ -z "${action_cmd}" ] && echo "${path}: missing command (${action})" && continue	
	action_post_cmd=`get_post_action "${action}"`
		
	max_p=`expr $(expr ${max} / 100) \* ${percent}`
			
	${GLUTIL} -x ${ROOT}/${path} -R -xdev --ftime -postprint \
	"${ROOT}/${path}: {?L:(u64glob2) != 0:(?m:u64glob1/(1024^3)):(?p:0)}/{?m:(u64glob0/(1024^3))} G purged total" \
	-lom "u64glob0 += size" -and \( -lom "(u64glob0) > ${max_p}" -and -lom "u64glob1 += size" \) -and \( -lom "mode = 4 || mode = 8" -and -lom "u64glob2 += 1" \) \
	 -execv "${action_cmd}"  -lom "depth=1" --sort desc,mtime --postexec "${action_post_cmd}"
done

exit 0
