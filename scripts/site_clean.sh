#!/bin/bash

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

MIN_FREE=`proc_tgmk_str ${MIN_FREE}`

used=`expr $(d_used ${DEVICE}) \* 1024`
free=`expr $(d_free ${DEVICE}) \* 1024`
total=`expr ${free} + ${used}`
max=`expr ${total} - ${MIN_FREE}`

[ ${max} -lt 0 ] && echo "${path}: negative maximum, MIN_FREE shouldn't exceed device size" && exit 2

for i in "${SECTIONS[@]}"; do
	path=`echo ${i} | cut -d " " -f1`
	percent=`echo ${i} | cut -d " " -f2`
		
	max_p=`expr $(expr ${max} / 100) \* ${percent}`
			
	${GLUTIL} -x ${SITEROOT}/${path} -R -xdev --ftime -postprint \
	"${SITEROOT}/${path}: {?L:(u64glob2) != 0:?m:u64glob1/(1024^2)}{?L:(u64glob2) = 0:?p:0}/{?m:(u64glob0/(1024^2))} M purged total" \
	 lom "u64glob0 += size" and lom "(u64glob0) > ${max_p}" and lom "u64glob1 += size" and lom "mode = 4" and lom "u64glob2 += 1" \
	 -execv "echo purging(test) {mtime}: {path}"  lom "depth=1" --sort desc,mtime
done