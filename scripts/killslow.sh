#!/bin/bash
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
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:1
#@REVISION:12
#@MACRO:killslow|Kills any matched transfer that is under $MINRATE bytes/s for a minimum duration of $MAXSLOWTIME (see inside script file):{m:exe} -w --loop=1 --silent --daemon --loglevel=3 -execv "{m:spec1} {bxfer} {lupdtime} {user} {pid} {rate} {status} {exe} {FLAGS} {dir} {usroot} {logroot} {time} {host} {ndir} {glroot}"
#
## Kills any matched transfer that is under $MINRATE bytes/s for a minimum duration of $MAXSLOWTIME
#
## Requires: - glutil-1.9-68 or above
##           - date, kill, expr, sleep, stat
## Usage (manual): /glroot/bin/glutil -w --loop=1 --silent --daemon --loglevel=3 -exec "/glroot/bin/scripts/killslow.sh '{bxfer}' '{lupdtime}' '{user}' '{pid}' '{rate}' '{status}' '{exe}' '{FLAGS}' '{dir}' '{usroot}'"
#
## Usage (macro): ./glutil -m killslow
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
##  NOTE: this macro forks glutil into background, executing this every --loop=<x> seconds
##  If necessary, change interval on the third line (one starting with "#@MACRO:..")
#
## See ./glutil --help for more info about options
#
###########################[ BEGIN OPTIONS ]#############################
#
## Minimum allowed transfer rate (bytes per second)
MINRATE=512000
#
# Maximum time a transfer can be under the speed limit, before killing it (seconds)
MAXSLOWTIME=7
#
## Enforce only after transfer is atleast this amount of seconds old
WAIT=3
#
## File to log to
LOG="/var/log/killslow.log"
#
## Verbose output
VERBOSE=0
#
## Ban user after violating minimum speed limit (seconds)
## Setting 0 disables ban after kick
BANUSER=20 
#
## Exempt users list
EXEMPTUSERS="user1|user2"
#
## Do NOT enforce limit on siteops
EXEMPTSITEOPS=1
#
## Enforce only on files matching this expression
FILES_ENFORCED="\.(r([0-9]{1,3}|ar)|mkv|avi|mp((e|())g|[34]))$"
#
## Do NOT enforce paths matching this expression,
## case-insensitive
PATHS_FILTERED="\/(sample|cover(s|())|proof)(($)|\/)"
#
## Do not enforce when only one user is uploading
## into a certain directory
IGNORE_LONE_RANGER=0
#
## Remove file the violator was uploading, immediately
## after sending process the kill signal
WIPE_FILE=1
#
## Remove all file records the violator was uploading
## from the dupelog
WIPE_FROM_DUPELOG=1
#
## Log to glftpd.log
LOG_TO_GLFTPD=0
#
############################[ END OPTIONS ]##############################

BASEDIR=`dirname $0`
[ -f "$BASEDIR/config" ] && . $BASEDIR/config

ban_user() {	

	[ -z "$1" ] && return 0	
	[ -z "$4" ] && return 0	
	if [ $2 -eq 0 ]; then
		[ -z "$5" ] && return 0	
		[ -z "$6" ] && return 0		
		[ $BANUSER -lt 1 ] && return 0			
		egrep -q '^FLAGS .*6' "$4/$1" && return 0
		[ -n "$LOG" ] && echo "[`date "+%T %D"`] DISABLE USER: $1 for $BANUSER seconds.." >> $LOG
		sed -r 's/^FLAGS .*$/&6/' "$4/$1" > /tmp/ks.$1.$$.dtm && {
			cp "/tmp/ks.$1.$$.dtm" "$4/$1"
			$5 --fork "$6 unban $1 0 $4"	
		}
		
		rm -f /tmp/ks.$1.$$.dtm
		
		return 0
	elif [ $2 -eq 1 ]; then
		egrep -q '^FLAGS .*6' "$4/$1" || return 0
		[ -n "$LOG" ] && echo "[`date "+%T %D"`] ENABLE USER: $1" >> $LOG
		g_FLAGS=`cat "$4/$1" | egrep '^FLAGS ' | tr -d '6'`
		[ -z "$g_FLAGS" ] && return 1
		sed -r "s/^FLAGS .*/$g_FLAGS/" "$4/$1" > /tmp/ks.$1.$$.dtm &&
			cp "/tmp/ks.$1.$$.dtm" "$4/$1" &&
				rm -f /tmp/ks.$1.$$.dtm	
		
	fi
	return 0
}

if [[ "$1" == "ban" ]]; then	
	ban_user $2 0 $3 $4 $5 $0 && exit 1
elif [[ "$1" == "unban" ]]; then
	trap "rm -f /tmp/du-ks/$4; ban_user $2 1 $3 $4; exit 2" 2 15 9 6
	sleep $BANUSER
	ban_user $2 1 $3 $4 && exit 1
fi

echo "$6" | egrep -q '^STOR' || exit 1

[ -n "$FILES_ENFORCED" ] && {
	echo "$6" | egrep -q "${FILES_ENFORCED}" || exit 1
}
[ -n "$PATHS_FILTERED" ] && echo "$9" | egrep -qi "${PATHS_FILTERED}" && exit 1

[ -n "$EXEMPTUSERS" ] && echo "$3" | egrep -q "^(${EXEMPTUSERS})\$" && {
	[ -f "/tmp/du-ks/$4" ] && rm -f /tmp/du-ks/$4
	exit 1
}
[ $EXEMPTSITEOPS -eq 1 ] && echo "$8" | grep -q 1 && exit 1

! [ -d "/tmp/du-ks" ] && mkdir -p /tmp/du-ks

BXFER=$1

if [ $BXFER -lt 1 ]; then
	[ -f "/tmp/du-ks/$4" ] && rm -f /tmp/du-ks/$4 
	exit 1
fi

[ $IGNORE_LONE_RANGER -eq 1 ] && {
	$7 -w --batch match "user,${3}" or iregex status,"^STOR\ " and ilom "bxfer" and imatch "ndir,${14}" | egrep -q "^ONLINE" || {
		#echo "NOTICE: ignoring lone ranger '${3}'" >> "$LOG"
		[ -f /tmp/du-ks/$4 ] && rm -f /tmp/du-ks/$4
		exit 1
	}
}

trap "rm -f /tmp/du-ks/$4; exit 2" 2 15 9 6

DRATE=$5
LUPDT=$2
GLUSER=$3
CT=`date +%s`

DIFFT=`expr $CT - $LUPDT`;

#[ $DIFFT -lt 1 ] && exit 1

#echo "$GLUSER @ $DRATE B/s for $DIFFT seconds"

SLOW=0
SHOULDKILL=0
KILLED=0

[ $DIFFT -gt $WAIT ] && [ $DRATE -lt $MINRATE ] && SLOW=1 && [ $VERBOSE -gt 0 ] && 
        echo "[`date "+%T %D"`] WARNING: Too slow (running $DIFFT secs): $GLUSER [PID: $4] [Rate: $DRATE/$MINRATE B/s]" >> $LOG

if [ $SLOW -eq 1 ] && [ -f "/tmp/du-ks/$4" ]; then
	MT1=`stat -c %Y "/tmp/du-ks/$4"` 
	UNDERTIME=`expr $CT - $MT1`
	[ $UNDERTIME -gt $MAXSLOWTIME ] && {		
		echo "[`date "+%T %D"`] KILLING: [PID: $4]: Below speed limit for too long ($UNDERTIME secs): $GLUSER [Rate: $DRATE/$MINRATE B/s]" >> $LOG
		ban_user $GLUSER 0 $8 ${10} $7 $0
		SHOULDKILL=1
		kill $4 && {
			[ $WIPE_FILE -eq 1 ] && [ -f "${15}${9}" ] && rm -f "${15}${9}"			
			KILLED=1 && rm -f /tmp/du-ks/$4
		}
	}
	if [ $KILLED -eq 1 ]; then 
		FORCEKILL=0		
		i=0
		while [ -n "`ps -p $4 -o comm=`" ] && [ $i -lt 4 ]; do
			i=`expr $i + 1`
			sleep 1
		done

		[ -n "`ps -p $4 -o comm=`" ] && { 
				echo "[`date "+%T %D"`] WARNING: process still running after $i seconds, killing by force" >> $LOG
				kill -9 $4 && FORCEKILL=1
		}

		[ $LOG_TO_GLFTPD -gt 0 ] && {
			gllog="${11}/glftpd.log"
			[ -f "$gllog" ] && echo "`date "+%a %b %e %T %Y"` KILLSLOW: \"$GLUSER\" \"$4\" \"$DRATE\" \"$MINRATE\" \"$UNDERTIME\" \"$FORCEKILL\" \"$9\" \"$1\" \"$(echo "$6" | sed 's/^STOR //')\" \"$1\" \"${12}\" \"${13}\"" >> $gllog || 
				echo "[`date "+%T %D"`] ERROR: could not log to glftpd.log" >> $LOG
		}   	
		[ $WIPE_FROM_DUPELOG -eq 1 ] && {
			g_FILE=`echo "${6}" | cut -f 2- -d " "`
			[ -n "$g_FILE" ]  && $7 -e dupefile match "file,$g_FILE" and match "user,${3}" --loglevel=6 -vvv -ff
		}
    fi    	
elif [ $SLOW -eq 1 ]; then
	touch "/tmp/du-ks/$4"
elif [ $SLOW -eq 0 ] && [ -f "/tmp/du-ks/$4" ]; then
 	rm -f "/tmp/du-ks/$4"
fi

[ $SHOULDKILL -eq 1 ] && [ $KILLED -eq 0 ] && echo "[`date "+%T %D"`] ERROR: Sending SIGTERM to PID $4 ($GLUSER) failed!" >> $LOG && 
	[ -f "/tmp/du-ks/$4" ] && rm -f "/tmp/du-ks/$4"

exit 1

