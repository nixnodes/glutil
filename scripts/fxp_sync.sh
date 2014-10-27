#!/bin/bash

LFTP_SYNC_OPTS="set ssl:verify-certificate off;
set ssl:verify-certificate off;
set ftp:use-stat-for-list true;
set ftp:use-size false;
set net:max-retries 2;
set net:persist-retries 2;
"

. ${BASE_DIR}/fxp_lftp.sh

#[ip] [port] [user] [pass] [section]
write_lftpf()
{
echo "${LFTP_SYNC_OPTS}"
lftp_connect "${1}" "${2}" "${3}" "${4}"
echo "cls /${5} -1 --date --time-style=%s"
}

write_lftpf2()
{
echo "${LFTP_SYNC_OPTS}"
lftp_connect "${1}" "${2}" "${3}" "${4}"
}

#[site] [section]
sync_section() {
	GLUTIL=${4}
	trap "rm -f /tmp/glutil.fx.$$.*" 2 15 9 6 SIGTERM SIGINT
	
	eval "sleep 360; em_kill $$ \"${BASE_DIR}/atxfer/${1}/sync.lock\"" &
	kproc_pid=$!
	
	! [ -f "${BASE_DIR}/atxfer/${1}/vars" ] && echoerr "ERROR: missing configuration file '${BASE_DIR}/atxfer/${1}/vars'" && {
		kill -9 ${kproc_pid}
		return 1
	}
	
	. ${BASE_DIR}/atxfer/${1}/vars
	. ${BASE_DIR}/atxfer/config
	. ${BASE_DIR}/fxp_sections.sh

	echo "retrieving data from '${2}' on '${1}'.."	
	
	echo -e "`write_lftpf2 ${IP} ${PORT} ${USER} "${PASS}"`\nls -ltr /${2}" | ${LFTP} | tail -n +3 | egrep -v "${SYNC_DEFAULT_FILTER_REGEX}" |  tail -20 > /tmp/glutil.fx.$$.ap
	
	rm -f /tmp/glutil.fx.$$.a
	i=0
	
	while read ml; do
		name=`echo ${ml} | awk '{for(i=9;i<NF;i++)printf "%s",$i OFS; if (NF) printf "%s",$NF; printf ORS}'`
		time=$(expr `date "+%s"` + ${i})
		i=`expr ${i} + 1`
		echo "${time} /${2}/${name}" >> /tmp/glutil.fx.$$.a
	done < /tmp/glutil.fx.$$.ap
	
	! [ -f "/tmp/glutil.fx.$$.a" ] && echo "no listing was retrieved" && {
		rm -f /tmp/glutil.fx.$$.*
		pkill -TERM -P $$
		return 0
	}
	

	l_c=`cat /tmp/glutil.fx.$$.a | wc -l`
	c=0; i=0
		
	rm -f /tmp/glutil.fx.$$.b
	
	[ -f "${3}/atxfer/${1}/data" ] || touch "${3}/atxfer/${1}/data" || { 
		rm -f /tmp/glutil.fx.$$.*
		pkill -TERM -P $$
		return 2
	}
	
	section=`do_lookup ${2}`	

	[ -z "${section}" ] && echo "could not extract section" && {
		rm -f /tmp/glutil.fx.$$.*
		pkill -TERM -P $$
		return 2
	}
	
	while read ml; do
		line=`echo ${ml} | cut -d " " -f2- | sed -r 's/\/$//'`
		size=0		
		time=`echo ${ml} | cut -d " " -f1`
		c=`expr ${c} + 1`
		#printf "\rprocessing (${2}): ${c}/${l_c}.."		
		${GLUTIL} --rev -q ge3 --ge3log "${BASE_DIR}/atxfer/${1}/data" --match "ge1,${line}" --silent --imatchq  || {			
			echo -e "u1 ${time}\nge1 ${line}\nge2 ${section}\nu2 ${size}\ni1 0\ni2 0\nge3  /\nge4  /\nul1 0\nul2 0\n" >> /tmp/glutil.fx.$$.b
			i=`expr ${i} + 1`			
		}
		
	done < /tmp/glutil.fx.$$.a

	! [ ${i} -eq 0 ] && {		
		${GLUTIL} -z ge3 --noglconf --stats --nobackup --ge3log "${BASE_DIR}/atxfer/${1}/data" < /tmp/glutil.fx.$$.b	
		${GLUTIL} -e ge3 --noglconf --ge3log "${BASE_DIR}/atxfer/${1}/data" --sort asc,u1 --silent --nostats
	} || {
		echo "up to date."
	}	
	
	rm -f /tmp/glutil.fx.$$.*
	
	pkill -TERM -P $$
	
	return 0 
}

#[site]
sync_sections() {
	trap "rm -f /tmp/glutil.fx.$$.*" 2 15 9 6
	
	[ -f ${BASE_DIR}/atxfer/${1}/vars ] || { 
		echo "no configuration could be loaded for ${1}" 
		return 2
	}
	
	. ${BASE_DIR}/atxfer/${1}/vars
	. ${BASE_DIR}/fxp_sections.sh
	
	l_dirs=`echo "${LFTP_SYNC_OPTS}
	$(lftp_connect ${IP} ${PORT} ${USER} "${PASS}")
	cls / -1" | ${LFTP}`
	
	rm -f /tmp/glutil.fx.$$.b

	i=0
	
	for s in ${l_dirs}; do
		sct=`lookup_section "${s}-"`
		[ -n "${sct}" ] || continue;
		#${GLUTIL} -q ge3 --ge3log "${BASE_DIR}/atxfer/${1}/sections" --match "ge1,${s}" --silent --imatchq  || {	
			echo -e "u1 0\nge1 ${s}\nge2 ${sct}\nu2 0\ni1 0\ni2 0\nge3  /\nge4  /\nul1 0\nul2 0\n" >> /tmp/glutil.fx.$$.b
			i=`expr ${i} + 1`	
		#}		
	done
	
	! [ ${i} -eq 0 ] && {
		rm -f "${BASE_DIR}/atxfer/${1}/sections"
		${GLUTIL} -z ge3 --noglconf --nobackup -vvvv --ge3log "${BASE_DIR}/atxfer/${1}/sections" < /tmp/glutil.fx.$$.b	
		${GLUTIL} -e ge3 --noglconf --ge3log "${BASE_DIR}/atxfer/${1}/sections" --sort ge1 -vvvv		
	} || {
		echo "${1}: up to date."
	}	
	
	rm -f /tmp/glutil.fx.$$.b
}


