#!/bin/bash
#@VERSION:0
#@REVISION:7
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
dupefile_values=(kl.s12e08.720p.hdtv.x264-la.mkv user 1372468945)
dupefile_lom_fields=("time")
dupefile_lom_values=(1372468945)

game_fields=("dir time score")
game_values=(/test/kl.s12e08.720p.hdtv.x264-la.mkv 1372468945 5.1)
game_lom_fields=("time score")
game_lom_values=(1372468945 5.1)

oneliners_fields=("user group tag time msg")
oneliners_values=(SomeUser SomeGrp nothing 1372468945 test123)
oneliners_lom_fields=("time")
oneliners_lom_values=(1372468945)

ge1_fields=("i32 ge1 ge2 ge3 ge4 ge5 ge6 ge7 ge8")
ge1_values=(1172414945 test1 test2 test3 test4 test5 test6 test7 test8)
ge1_lom_fields=("i32")
ge1_lom_values=(1172414945)

ge2_fields=("i1 i2 i3 i4 u1 u2 u3 u4 f1 f2 f3 f4 ul1 ul2 ul3 ul4 ge1 ge2 ge3 ge4 ge5 ge6 ge7 ge8")
ge2_values=(1172414945 434151 34351 12323 43441 654674 23473 6743 1.111111 1.111111 1.111111 1.111111 34515321412 434314351434 143343 3144 test1 test2 test3 test4 test5 test6 test7 test8)
ge2_lom_fields=("i1 i2 i3 i4 u1 u2 u3 u4 f1 f2 f3 f4 ul1 ul2 ul3 ul4")
ge2_lom_values=(1172414945 434151 34351 12323 43441 654674 23473 6743 1.111111 1.111111 1.111111 1.111111 34515321412 434314351434 143343 3144)

ge3_fields=("u1 u2 ge1 ge2 i1 i2 ul1 ul2 ge3 ge4")
ge3_values=(11232	325211	bla	33f3	3432545	-2323	3435153343	2434325345	ggf	yyy)
ge3_lom_fields=("u1 u2 i1 i2 ul1 ul2")
ge3_lom_values=(11232	325211	3432545	-2323	3435153343 2434325345)

ge4_fields=("u1 u2 ge1 ge2 i1 i2 ul1 ul2 ge3 ge4")
ge4_values=(11232	325211	fd	ffffff3	3432545	-2323	3435153343	2434325345	ggf	yyy)
ge4_lom_fields=("u1 u2 i1 i2 ul1 ul2")
ge4_lom_values=(11232	325211	3432545	-2323	3435153343 2434325345)

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

#########################################################################

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

get_exec_str() {
	f=$1[@]; a=("${!f}");
	echo "${a[@]}" | sed -r 's/[a-z0-9A-Z\-]+/{\0}/g' 
}

get_exec_vals() {
	f=$1[@]; a=("${!f}");
	echo "${a[@]}"
}

launch_test() {
	rm -f ${g_td}
	lt_log=${1}
	r=0
	if [ "${lt_log}" = "tvrage" ]; then
		lt_ft="tvlog"
	elif [ "${lt_log}" = "imdb" ]; then
		lt_ft="imdblog"
	elif [ "${lt_log}" = "game" ]; then
		lt_ft="gamelog"
	elif [ "${lt_log}" = "ge1" ]; then
		lt_ft="ge1log"
	elif [ "${lt_log}" = "ge2" ]; then
		lt_ft="ge2log"
	elif [ "${lt_log}" = "ge3" ]; then
		lt_ft="ge3log"
	elif [ "${lt_log}" = "ge4" ]; then
		lt_ft="ge4log"
	else
		lt_ft=${lt_log}
	fi
	echo -n "$1: creating test log.. "
	create_packet "${lt_log}_fields" "${lt_log}_values" | ${GLUTIL} silent --nobackup -z ${lt_log} --${lt_ft} ${g_td} -vvvv &&
	create_packet "${lt_log}_fields" "${lt_log}_values" | ${GLUTIL} silent --nobackup -z ${lt_log} --${lt_ft} ${g_td} -vvvv && {
		echo -e "       \tOK"
	} || {
		echo -e "       \tFAILED"
		return 1
	}
	echo -n "$1: eval literal matching:  "
	lt_ms=`create_match_s "${lt_log}_fields" "${lt_log}_values" imatch`

	${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} ${lt_ms} silent && {
		echo -e "   \tOK"
	} || {
		echo -e "   \tFAILED"
		${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} --batch
		echo ${lt_ms}
		r=1
	}
	echo -n "$1: eval regex matching:  "
	lt_ms=`create_match_s "${lt_log}_fields" "${lt_log}_values" iregex`
	${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} ${lt_ms} silent && {
		echo -e "      \tOK"
	} || {
		echo -e "      \tFAILED"
		echo ${lt_ms}
		r=2
	}
	echo -n "$1: eval lom matching:  "
	lt_ms=`create_match_lom "${lt_log}_lom_fields" "${lt_log}_lom_values"`

	${GLUTIL} -q ${lt_log} --nofq --${lt_ft} ${g_td} ilom "${lt_ms}" silent && {
		echo -e "\t\tOK"
	} || {
		echo -e "\t\tFAILED"
		echo ${lt_ms}
		r=3
	}	
	
	echo -n "$1: exec field translate:  "
	lt_ms=`get_exec_str "${lt_log}_fields"`
	
	lt_rf=`${GLUTIL} -q ${lt_log} --${lt_ft} ${g_td} -exec "echo \"${lt_ms}\"" --imatchq silent`
	lt_rv=`get_exec_vals "${lt_log}_values"`
	[ "$lt_rf" == "$lt_rv" ]  && {
		echo -e "    \tOK"
	} || {
		echo -e "    \tFAILED"
		echo $lt_ms
		echo $lt_rf
		echo $lt_rv
		return 1
	}	
	echo -n "$1: execv field translate:  "
	lt_rf=`${GLUTIL} -q ${lt_log} --${lt_ft} ${g_td} -execv "echo \"${lt_ms}\"" --imatchq silent`
	lt_rv=`get_exec_vals "${lt_log}_values"`
	[ "$lt_rf" == "$lt_rv" ]  && {
		echo -e "   \tOK"
	} || {
		echo -e "   \tFAILED"
		echo $lt_ms
		echo $lt_rf
		echo $lt_rv
		return 1
	}	
	
	echo -n "$1: print field translate:  "
	lt_rf=`${GLUTIL} -q ${lt_log} --${lt_ft} ${g_td} -print "${lt_ms}" --imatchq silent`
	lt_rv=`get_exec_vals "${lt_log}_values"`
	[ "$lt_rf" == "$lt_rv" ]  && {
		echo -e "   \tOK"
	} || {
		echo -e "   \tFAILED"
		echo $lt_ms
		echo $lt_rf
		echo $lt_rv
		return 1
	}	
	
	return ${r}
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
launch_test oneliners || t_quit ${?}
launch_test game || t_quit ${?}
launch_test ge1 || t_quit ${?}
launch_test ge2 || t_quit ${?}
launch_test ge3 || t_quit ${?}
launch_test ge4 || t_quit ${?}

# End 

# Filesystem

echo -n "FS: directory tree matching tests.. "
[ -d "${fs_td}" ] && rm -Rf "${fs_td}"
mkdir "${fs_td}" && {
	echo "t1" > "${fs_td}/t1"
	echo "t2" > "${fs_td}/t2"
	echo "t3" > "${fs_td}/t3"
	[ $(${GLUTIL} -x ${fs_td}/ --batch ilom "ctime > curtime-2000 && uid == `id -u`" | wc -l) -eq 3 ] && {
		echo -e " \tOK "
	} || {
		echo -e " \tBAD"
		exit 1		
	}
} || { 
	echo "COULD NOT CREATE TEMP DIR"
}
rm -fR ${fs_td}
# End

t_quit 0
