#!/bin/bash

CONFIG_PATH="/glftpd/ftp-data/glutil/precheck-config"
DATA_PATH="/ftp-data/glutil/precheck-data"
GLUTIL="/bin/glutil"

####


get_opts() {
case ${1} in
    *i* )
        g_icase=1
        ;;
esac
return 0
}

get_log_int() {
case ${1} in
    imdb )
        return 1
        ;;
    tvrage )
        return 2
        ;;
esac
return 0
}

get_inv() {
case ${1} in
    allow )
        return 1
        ;;    
esac
return 0
}

get_lom_comp() {
case ${1} in
    isequal )
        return 1
        ;;
    ishigher )
        return 2
        ;;
    islower )
        return 3
        ;;
    islowerorequal )
        return 4
        ;;
    ishigherorequal )
        return 5
        ;;
    isnotequal )
        return 6
        ;;
esac
return 0
}

get_m_type() {
get_log_int "${1}"
g_mode=$?
get_lom_comp "${2}"
g_lom=$?
! [ ${g_lom} -eq 0 ] && return 2

case ${3} in
    exec )    	
        return 3
        ;;
esac
get_opts ${2}
return 1
}

tokenize_s_left() {
	tsl_t="${@}"
	#g_field=`echo ${1} | egrep -o '\(.+\)'`
	IFS="_"
	set -- `echo "${tsl_t}" | cut -f 1 -d"="`
	g_match=`echo "${tsl_t}" | cut -f 2- -d"="`
	g_icase=0;g_mode=0; g_type=0; g_inv=0; g_lom=0; unset g_field	
	if echo ${1} | egrep -q '^(allow|deny|do)$'; then		
		get_inv ${1}; g_inv=$?
		get_m_type "-" "-" "${2}"; g_type=$?
		if [ ${g_type} -eq 1 ]; then
			get_opts "${2}"
		else
			get_opts "${3}"
		fi
	else		
		get_m_type "${1}" "${4}" "${3}"; g_type=$?
		get_inv "${2}"; g_inv=$?
		! [ ${g_type} -eq 3 ] && g_field="${3}"
	fi
}

proc_s_entry() {
	while read ir_l; do
		[ -z "${ir_l}" ] && continue		
		
		if echo "${ir_l}" | egrep -q 'msg([ ]+=|=)'; then
			g_msg=`echo "${ir_l}" | cut -f 2- -d"="`
		elif echo "${ir_l}" | egrep -q 'path([ ]+=|=)'; then
			if [ -z "${t_path}" ]; then
				c_path=`echo "${ir_l}" | cut -f 2- -d"="`
				t_path="${t_path}${c_path}"
			else
				c_path=`echo "${ir_l}" | cut -f 2- -d"="`
				t_path="${t_path}|${c_path}"
			fi
		else
			tokenize_s_left "${ir_l}"
			echo -e "match ${g_match}\nfield ${g_field}\ntype ${g_type}\nint ${g_mode}\ninvert ${g_inv}\nlcomp ${g_lom}\nicase ${g_icase}\nmsg ${g_msg}\n"
			
			unset g_msg
		fi
	done < ${1}
}

[ -f /tmp/glutil.$$.pce ] && rm -f /tmp/glutil.$$.pce

trap "rm /tmp/glutil.$$.pce; exit 2" 2 15 9 6

rm -Rf "${DATA_PATH}/*"

for ir_f in ${CONFIG_PATH}/*; do
	[ -f "${ir_f}" ] || continue
	echo "${ir_f}" | egrep -q '\/gconf$' && continue
	unset c_path
	unset g_msg
	proc_s_entry "${ir_f}" >> /tmp/glutil.$$.pce
	d_path=`dirname "${c_path}"`
	[[ "${d_path}" != "." ]] && {
		mkdir -p "${DATA_PATH}/${d_path}"
	}
	#cat /tmp/glutil.$$.pce
	${GLUTIL} -z sconf --raw  < /tmp/glutil.$$.pce > "${DATA_PATH}/${c_path}" && 
		echo "BUILD: '${c_path}': OK"
	rm -f /tmp/glutil.$$.pce
done

g_path="/(${t_path})$"

cat "${CONFIG_PATH}/gconf" | sed -r '/^$/d' > /tmp/glutil.$$.pce
echo "paths ${g_path}" >> /tmp/glutil.$$.pce
echo >> /tmp/glutil.$$.pce

${GLUTIL} -z gconf --raw  < /tmp/glutil.$$.pce > "${DATA_PATH}/gconf" && 
		echo "BUILD: 'GCONF': OK"

${GLUTIL} -q gconf --shmem --shmdestroy --silent
${GLUTIL} -q gconf --shmem --shmreload --silent

exit 0
