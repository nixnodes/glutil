#!/bin/sh
#################################################
# 
# Simple script, runs commands in $CHAIN in
# order. If and when a command fails, the
# script breaks and returns the exit code
# of that command.
#
# Usefull for attaching multiple commands
# to a single ftp command via 'cscript'.
# Obviously this is a low-performance solution,
# in short - bash is slow
#
#################################################
#

CHAIN=(	
	"/bin/postdel"
	"/bin/scripts/flog/un_index.sh"
)

for exe in "${CHAIN[@]}"; do
	${exe} "${1}" "${2}" "${3}" || exit $?
done


