#!/bin/sh
#@VERSION:0
#@REVISION:1
#
## Simple debugger script
#
###########################[ BEGIN OPTIONS ]#############################
#
GLUTIL="/bin/glutil"
#
###########################[  END OPTIONS  ]#############################

dirlog_fields=("dir time files size user group status")
dirlog_lom_fields=("time files size user group status")
dirlog_lom_values=(34151334 3341 14351514 700 500 1)
dirlog_values=(test 34151334 3341 14351514 700 500 1)

lastonlog_fields=("user group tag logon logoff upload download stats")
lastonlog_lom_fields=("logon logoff upload download")
lastonlog_lom_values=(1384893704 1384893724 23658496 0)
lastonlog_values=(siska test No 1384893704 1384893724 23658496 0 gfg)

dupefile_fields=("file user time")
dupefile_lom_fields=("time")
dupefile_lom_values=(1372468945)
dupefile_values=(kl.s12e08.720p.hdtv.x264-la.mkv user 1372468945)

nukelog_fields=("dir reason mult size nuker unnuker nukee time status")
nukelog_lom_fields=("mult size time status")
nukelog_lom_values=(3 1.111111 1372583206 1)
nukelog_values=(Afrojack blabla 3 1.111111 test bla test 1372583206 1)

tvrage_fields=("dir time name class showid link status airday airtime runtime started ended genre country seasons startyear endyear network")
tvrage_lom_fields=("time showid started ended startyear endyear seasons")
tvrage_lom_values=(1384575576 17 378687600 417222000 1982 1983 1)
tvrage_values=(- 1384575576 Seven Scripted 17 http://www.tvrage.com/shows/id-17 Canceled/Ended Wednesday 20:00 60 378687600 417222000 Drama,Music US 1 1982 1983 CBS)

imdb_fields=("dir title time imdbid score votes genre year released runtime rated actors director plot")
imdb_lom_fields=("time score votes year released runtime")
imdb_lom_values=(1385027223 6.6 47 2008 1 14)
imdb_values=(- One 1385027223 tt1302196 6.6 47 Short,Comedy 2008 1 14 blabla Joe Hattie WePlot)


create_packet() {
	f=$1[@]; v=$2[@]
    a=("${!f}"); b=("${!v}")
	
	y=0
	for i in ${a[@]}; do	
		echo "$i ${b[$y]}"
		y=`expr $y + 1`
	done
	echo
	y=0
	for i in ${a[@]}; do	
		echo "$i ${b[$y]}"
		y=`expr $y + 1`
	done
	echo
}

create_match_s() {
	f=$1[@]; v=$2[@]
    a=("${!f}"); b=("${!v}")
	y=0
	for i in ${a[@]}; do	
		echo -n "$3 $i,${b[$y]} "
		y=`expr $y + 1`
	done

}

create_match_lom() {
	f=$1[@]; v=$2[@]
    a=("${!f}"); b=("${!v}")
	y=0
	for i in ${a[@]}; do	
		[ $y -gt 0 ] && echo -n " && "
		
		echo -n "$i == ${b[$y]}" 
		y=`expr $y + 1`
	done

}


launch_test() {
	rm -f ${g_td}
	lt_log=${1}
	if [ "${lt_log}" = "tvrage" ]; then
		lt_ft="tvlog"
	elif [ "${lt_log}" = "imdb" ]; then
		lt_ft="imdblog"
	else
		lt_ft=${lt_log}
	fi
	echo -n "$1: creating test log.. "
	create_packet "${lt_log}_fields" "${lt_log}_values" | ${GLUTIL} silent --nobackup -z ${lt_log} --${lt_ft} ${g_td} -vvvv &&
	create_packet "${lt_log}_fields" "${lt_log}_values" | ${GLUTIL} silent --nobackup -z ${lt_log} --${lt_ft} ${g_td} -vvvv && {
		echo -e " \t\tOK"
	} || {
		echo -e " \t\tFAILED"
		return 1
	}
	echo -n "$1: eval literal matching..  "
	lt_ms=`create_match_s "${lt_log}_fields" "${lt_log}_values" imatch`

	${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} ${lt_ms} silent && {
		echo -e " \tOK"
	} || {
		echo -e " \tFAILED"
		return 1
	}
	echo -n "$1: eval regex matching..  "
	lt_ms=`create_match_s "${lt_log}_fields" "${lt_log}_values" iregex`
	${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} ${lt_ms} silent && {
		echo -e "\t\tOK"
	} || {
		echo -e "\t\tFAILED"
		return 1
	}
	echo -n "$1: eval lom matching..  "
	lt_ms=`create_match_lom "${lt_log}_lom_fields" "${lt_log}_lom_values"`

	${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} ilom "${lt_ms}" silent && {
		echo -e " \t\tOK"
	} || {
		echo -e " \t\tFAILED"
		return 1
	}	
	return 0
}

t_quit() {
	rm -f ${g_td}
	exit ${1}
}

g_td="/tmp/glutil.tl.$$"
fs_td="/tmp/mkt.$$"

[ -f "${g_td}" ] && rm -f ${g_td}

trap "rm -f ${g_td}; rm -fR ${fs_td}; exit 2" 2 15 9 6 3 15

# Begin log tests..

launch_test dirlog || t_quit ${?}
launch_test nukelog || t_quit ${?}
launch_test tvrage || t_quit ${?}
launch_test imdb || t_quit ${?}
launch_test dupefile || t_quit ${?}
launch_test lastonlog || t_quit ${?}

# End 

# Filesystem

echo -n "FS: performing directory tree parser tests.. "
[ -d "${fs_td}" ] && rm -Rf "${fs_td}"
mkdir "${fs_td}" && {
	echo "t1" > "${fs_td}/t1"
	echo "t2" > "${fs_td}/t2"
	echo "t3" > "${fs_td}/t3"
	[ $(${GLUTIL} -x ${fs_td}/ --batch ilom "ctime > curtime-2000 && uid == `id -u $USER`" | wc -l) -eq 3 ] && {
		echo -n "#1 OK "
	} || {
		echo "#1 BAD"
		exit 1		
	}
	echo
} || { 
	echo "COULD NOT CREATE TEMP DIR"
}
rm -fR ${fs_td}
# End

t_quit 0