#!/bin/bash

get_section_from_path()
{
	dirname "${@}" | sed -r 's/^\///'
}

do_lookup()
{
	[ -z "${1}" ] && return 2;
	sct=${1}
	while read f; do	
		f_base=`echo "${f}" | cut -f 1 -d ":"`
		for s in `echo ${f} | sed 's/:/ /g'`; do
			echo "${s}" | egrep -q "^(${sct})$" && {
				echo -n ${f_base}
				return 0
			}
		done
	done < ${BASE_DIR}/atxfer/sections
	return 1
}

lookup_section()
{
	[ -z "${@}" ] && return 2;
	sct=`get_section_from_path "${1}"`
	do_lookup ${sct}
	return $?
}

