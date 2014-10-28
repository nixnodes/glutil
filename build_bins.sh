#!/bin/bash
OUT="bin"

build() 
{
	make clean || return 2
	./configure ${1} || return 2
	make LDFLAGS="${2}" || return 2
	for i in ${3}; do
		cp src/${i} ${OUT}/${i}${4} || return 2
	done
}

rm -f ${OUT}/*

build --enable-precheck -static "glutil glutil-precheck" || exit 2
build --enable-gfind -static "gfind" || exit 2
build --enable-chroot-ownbin -static "glutil-chroot" || exit 2

build --enable-precheck "" "glutil glutil-precheck" "-dynamic" || exit 2
build --enable-gfind "" "gfind" "-dynamic" || exit 2
build --enable-chroot-ownbin "" "glutil-chroot" "-dynamic" || exit 2

exit 0
