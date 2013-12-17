#!/bin/bash
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:1
#@REVISION:0
#
#  Copyright (C) 2013 NixNodes
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
#########################################################################
#
## 
#
###########################[ BEGIN OPTIONS ]#############################
#
## glutil executable
GLUTIL="/bin/glutil"
#
## glFTPd root path
GLROOT=""
#
#TIMEOUT=60
#
###########################[  END OPTIONS  ]#############################

g_io="${GLROOT}/io/glud"

#echo "${@}" | egrep -q '[\$\`]' && exit 3 

! [ -p "${g_io}/gld.in.$$" ] && mkfifo "${g_io}/gld.in.$$"

[ -p "${g_io}/gld.in.$$" ] && echo "${@}" > "${g_io}/gld.in.$$"

ec=1

! [ -p "${g_io}/gld.out.$$" ] && { 
	mkfifo "${g_io}/gld.out.$$"
	trap "rm -f ${g_io}/gld.out.$$; exit 2" 2 15 9 6
	#${GLUTIL} noop --silent --sleep ${TIMEOUT} --fork "kill -9 $$"

	while read line; do
		echo "${line}"
		echo "${line}" | egrep -q "^OK$" && ec=0 && break;
		echo "${line}" | egrep -q "^ERROR$" && ec=2	&& break;
	done < "${g_io}/gld.out.$$"	 
}

[ -p "${g_io}/gld.out.$$" ] && rm -f "${g_io}/gld.out.$$"

exit ${ec}