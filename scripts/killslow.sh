#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:killslow:{m:exe} -w --loop=2 --silent --daemon --loglevel=3 -exec "{m:spec1} '{bxfer}' '{lupdtime}' '{user}' '{pid}' '{rate}' '{status}' '{exe}' '{FLAGS}' '{dir}' '{usroot}'"
#
## Kills any matched transfer that is under $MINRATE bytes/s for a minimum duration of $MAXSLOWTIME
#
## Usage (manual): /glroot/bin/glutil -w --loop=2 --silent --daemon --loglevel=3 -exec "/glroot/bin/scripts/killslow.sh '{bxfer}' '{lupdtime}' '{user}' '{pid}' '{rate}' '{status}' '{exe}' '{c:FLAGS}' '{dir}' '{usroot}'"
#
## Usage (macro): ./glutil -m killslow
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
##  NOTE: this macro forks glutil into background, executing this every --loop=<x> seconds - if necessary, 
##        change interval on the third line (one starting with "#@MACRO:..")
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
BANUSER=15 
#
## Exempt users list
EXEMPTUSERS="user1|user2"
#
## Do not enforce limit on siteops
EXEMPTSITEOPS=1
#
## Enforce only on files matching this expression
#
FILES_ENFORCED="\.(r[0-9]{1,3}|rar|mkv|avi)$"
#
## Do NOT enforce paths matching this expression
#
PATHS_FILTERED="\/(sample|cover(s|)|proof)(($)|\/)"
#
############################[ END OPTIONS ]##############################

ban_user() {	

	[ -z "$1" ] && return 0	
	[ -z "$4" ] && return 0	
	if [ $2 -eq 0 ]; then
		[ -z "$5" ] && return 0	
		[ -z "$6" ] && return 0		
		[ $BANUSER -lt 1 ] && return 0			
		cat $4/$1 | grep -P "^FLAGS " | grep 6 > /dev/null && return 0	
		[ -n "$LOG" ] && echo "[$(date "+%T %D")] DISABLE USER: $1 for $BANUSER seconds.." >> $LOG
		cat $4/$1 | sed -r 's/^FLAGS .*$/&6/' > /tmp/ks.$1.$$.dtm &&	
			cat /tmp/ks.$1.$$.dtm > $4/$1 &&			
			$5 --sleep $BANUSER --fork "$6 unban $1 0 $4"
		rm /tmp/ks.$1.$$.dtm
		
		return 0
	elif [ $2 -eq 1 ]; then
		! cat $4/$1 | grep -P "^FLAGS " | grep 6 > /dev/null && return 0
		[ -n "$LOG" ] && echo "[$(date "+%T %D")] ENABLE USER: $1" >> $LOG
		g_FLAGS=$(cat $4/$1 | grep -P "^FLAGS " | tr -d '6') && 
			cat $4/$1 | sed  "s/^FLAGS .*$/$g_FLAGS/" > /tmp/ks.$1.$$.dtm &&
			cat /tmp/ks.$1.$$.dtm > $4/$1 &&
			rm /tmp/ks.$1.$$.dtm	
		
	fi
	return 0
}
if [[ "$1" == "ban" ]];then
	ban_user $2 0 $3 $4 $5 $0 && exit 1
elif [[ "$1" == "unban" ]];then
	ban_user $2 1 $3 $4 && exit 1
fi

! echo $6 | grep -P "^STOR" > /dev/null && exit 1

! echo $6 | grep -P $FILES_ENFORCED > /dev/null && exit 1
echo $9 | grep -P $PATHS_FILTERED > /dev/null && exit 1

[ -n "$EXEMPTUSERS" ] && echo $3 | grep -P "^($EXEMPTUSERS)\$" > /dev/null && exit 1
[ $EXEMPTSITEOPS -eq 1 ] && echo $8 | grep 1 > /dev/null && exit 1

! [ -d "/tmp/du-ks" ] && mkdir -p /tmp/du-ks

BXFER=$1

if [ $BXFER -lt 1 ]; then
	[ -f /tmp/du-ks/$4 ] && rm /tmp/du-ks/$4 
	exit 1
fi

DRATE=$5
LUPDT=$2
GLUSER=$3
CT=$(date +%s)

DIFFT=$(expr $CT - $LUPDT);

#[ $DIFFT -lt 1 ] && exit 1

#echo "$GLUSER @ $DRATE B/s for $DIFFT seconds"

SLOW=0
SHOULDKILL=0
KILLED=0


[ $DIFFT -gt $WAIT ] && [ $DRATE -lt $MINRATE ] && SLOW=1 && [ $VERBOSE -gt 0 ] && 
        O="[$(date "+%T %D")] WARNING: Too slow (running $DIFFT secs): $GLUSER [PID: $4] [Rate: $DRATE/$MINRATE B/s]\n"  

if [ $SLOW -eq 1 ] && [ -f /tmp/du-ks/$4 ]; then
	MT1=$(stat -c %Y /tmp/du-ks/$4) 
	UNDERTIME=$(expr $CT - $MT1)
	[ $UNDERTIME -gt $MAXSLOWTIME ] && 
		O="[$(date "+%T %D")] KILLING: [PID: $4]: Below speed limit for too long ($UNDERTIME secs): $GLUSER [Rate: $DRATE/$MINRATE B/s]\n" &&  
		SHOULDKILL=1 && ban_user $GLUSER 0 $8 ${10} $7 $0  && kill $4 && KILLED=1 &&  rm /tmp/du-ks/$4
	if [ $KILLED -eq 1 ]; then 
		FORCEKILL=0		
		i=0
		while [ -n "$(ps -p $4 -o comm=)" ] && [ $i -lt 4 ]; do
			i=$(expr $i + 1)
			sleep 1
		done

		[ -n "$(ps -p $4 -o comm=)" ] && O="$O[$(date "+%T %D")] WARNING: process still running after $i seconds, killing by force\n" &&
				kill -9 $4 && FORCEKILL=1

    	g_FILE=$(echo $6 | cut -f 2- -d " ")
    	[ -n "$g_FILE" ]  && $7 -e dupefile --match "$g_FILE" --loglevel=6 -vvv
    fi    	
elif [ $SLOW -eq 1 ]; then
	touch /tmp/du-ks/$4
elif [ $SLOW -eq 0 ] && [ -f /tmp/du-ks/$4 ]; then
 	rm /tmp/du-ks/$4
fi

[ -n "$O" ] && echo -en $O && [ -n "$LOG" ] && echo -en $O >> $LOG

[ $SHOULDKILL -eq 1 ] && [ $KILLED -eq 0 ] && echo "[$(date "+%T %D")] ERROR: Sending SIGTERM to PID $4 ($GLUSER) failed!" && 
	[ -f /tmp/du-ks/$4 ] && rm /tmp/du-ks/$4

exit 1

