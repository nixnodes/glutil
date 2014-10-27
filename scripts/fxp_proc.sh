#!/bin/bash

GLUTIL="${1}"


write_lfxp()
{
echo "set ftp:ssl-force ${SSL_FORCE};
set ssl:verify-certificate ${SSL_VERIFY_CERT};
set ftp:use-fxp ${SSL_FXP};
set ftp:fxp-passive-source true;
set ftp:ssl-protect-fxp true;
set net:max-retries 15;
set net:reconnect-interval-base ${RECONNECT_INTERVAL};
set net:reconnect-interval-multiplier 1;
set xfer:clobber false;
set net:connection-takeover false;
set net:reconnect-interval-max 5;
set mirror:sort-by name;
set mirror:order "${PRIORITY_FILES}";
set mirror:parallel-transfer-count ${PARALLEL_TRANSFERS};
set mirror:no-empty-dirs true;
set mirror:skip-noaccess true;
set ftp:use-stat-for-list true;
debug ${LFTP_DEBUG};
set ftp:use-size true;
open -p ${PORT} ${IP};
user ${USER} ${PASS};"
. ${2}/atxfer/${r_site}/vars
echo "mirror --only-missing --no-empty-dirs --continue --ignore-time \
		--verbose=9  --no-symlinks --size-range 1-99999999999999 --loop \
		--depth-first \
		${1} ftp://${USER}:${PASS}@${IP}:${PORT}${f_remote_basepath}/${f_l_basepath}"
}

echoerr() { echo "$@" 1>&2; }

em_kill()
{	
	[ -n "${2}" ] && rm -f "${2}"
	pkill -9 -P ${1}
	kill -9 ${1}
}

do_fxp()
{	 
	! [ -f "${1}/atxfer/${2}/vars" ] && echoerr "ERROR: missing configuration file '${1}/atxfer/${2}/vars'" && return 1

	. ${1}/atxfer/${2}/vars
		
	write_lfxp ${3} ${1} | ${LFTP} && {
		trap "em_kill $?" 2 15 9 6
		return 0
	}
		
	return 1
}

# [site] [path] [basedir] [value] [field]
set_processed() {
	${GLUTIL} -q ge3 --ge3log "${3}/atxfer/${1}/data" --silent --match "ge1,${2}" and --lom "${5} != ${4}" --imatchq -E > /tmp/glutil.fxpp.$$ && {	
		cat /tmp/glutil.fxpp.$$ | sed -r "s/^${5} [0-1]$/${5} ${4}/" > /tmp/glutil.fxppt.$$
		cat /tmp/glutil.fxppt.$$ > /tmp/glutil.fxpp.$$
		
		rm -f /tmp/glutil.fxppt.$$
		if ${GLUTIL} -e ge3 --ge3log "${3}/atxfer/${1}/data" --silent --nobackup --noglconf --nofq ! --match "ge1,${2}"; then
			${GLUTIL} -z ge3 --ge3log "${3}/atxfer/${1}/data" --nobackup --noglconf --silent < /tmp/glutil.fxpp.$$ || {
				echoerr "ERROR: ${2} unable to write log!!"
			} && {
				${GLUTIL} -e ge3 --ge3log "${3}/atxfer/${1}/data" --sort asc,u1 --silent --nostats --nobackup --noglconf
			}

		else
			echoerr "ERROR: could not erase old record"
		fi
	}
	rm -f /tmp/glutil.fxpp.$$
}

. ${4}/atxfer/config
f_sect="${6}"
f_l_basepath="${7}"
r_site="${8}"

! [ -n "${r_site}" ] && exit 2

! [ -f "${4}/atxfer/${r_site}/sections" ] && echoerr "ERROR: sections reference file for ${r_site} doesn't exist" && exit 2

f_remote_sect=`${GLUTIL} -q ge3 --ge3log "${4}/atxfer/${r_site}/sections" --match "ge2,${f_sect}" --maxres=1 -printf {?rd:ge1:\/$}`

f_remote_basepath=${f_remote_sect}

[ -z "${f_remote_basepath}" ] && echoerr "ERROR: could not get remote section" && exit 2

eval "sleep ${FXP_MAIN_TIMEOUT}; em_kill $$ \"${4}/atxfer/${r_site}/proc.lock\"" &
kproc_pid=$!

case ${5} in
	-)		
		echo "${2}: ${3}:  clearing processed.."
		set_processed ${2} ${3} ${4} 0 i1
	;;
	*)
		if ${GLUTIL} -q ge3 --ge3log "${4}/atxfer/${2}/data" --silent --imatchq --match "ge1,${3}" and --lom "i1 == 0"; then
			echo "${2}: ${3}: starting FXP.."
			i_rc=0
			while [ ${i_rc} -lt 3 ]; do 
				[ ${i_rc} -gt 0 ] && echo "${2}: ${3}: retrying FXP (${i_rc}).."
				
				do_fxp ${4} ${2} ${3} && {					
					set_processed ${2} ${3} ${4} 1 i1
					set_processed ${2} ${3} ${4} 0 i2
					echo "${2}: ${3}: mirroring completed successfully"
					break
				}			
				i_rc=`expr ${i_rc} + 1`
			done
			
			[ ${i_rc} -eq 3 ] && {
				f_fac=`${GLUTIL} -q ge3 --ge3log "${4}/atxfer/${2}/data" --maxres=1 --silent --imatchq --match "ge1,${3}" -printf "{?m:i2+1}"`
				set_processed ${2} ${3} ${4} ${f_fac} i2
				[ ${f_fac} -ge ${FXP_PROC_MAX_FAIL} ] && {
					echoerr "${2}: ${3}: mirroring process failed too many times (${f_fac}), ignoring.."
					set_processed ${2} ${3} ${4} 1 i1
				} || {					
					echoerr "${2}: ${3}: mirroring process failed (${f_fac}), moving on.."
				}
			}
			
			rm -f /tmp/glutil.dfxp.$$
		else
			f_fac=`${GLUTIL} -q ge3 --ge3log "${4}/atxfer/${2}/data" --maxres=1 --silent  --match "ge1,${3}" -printf "{?m:i2+1}"`
			[ ${f_fac} -ge ${FXP_PROC_MAX_FAIL} ] && {
				echo "${2}: ${3}:  ignoring (failed ${f_fac}x).."
			} || {
				echo "${2}: ${3}:  ignoring (suceeded).."
			}
		fi
	;;
esac

kill -9 ${kproc_pid}

exit 0