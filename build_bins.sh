#!/bin/sh
OUT="bin"
INSTALL=true

####################################################

[ -n "${4}" ] && INSTALL="${4}"

build() 
{
	make clean || return 2
	./configure ${1} || return 2
	make -j2 LDFLAGS="${LDFLAGS} ${2}" || return 2
	[ "${INSTALL}" = "true" ] && {
		make install || return 2	
	}
	return 0
}

rm -f "${OUT}"/*

build "--enable-precheck ${1}" "${3}" || exit 2
build "${1} ${2}" "${3}" || exit 2
build "--enable-gfind ${1}" "${3}" || exit 2
build "--enable-chroot-ownbin ${1} ${2}" "${3}" || exit 2

exit 0
