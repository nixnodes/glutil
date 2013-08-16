#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:killslow:{m:exe} -w --loop=3 --silent --daemon --loglevel=3 -exec "{m:spec1} '{bxfer}' '{lupdtime}' '{user}' '{pid}' '{rate}' '{status}' '{exe}' '{FLAGS}' '{dir}'"
#
## Kills any transfer that is under $MINRATE bytes/s for a minimum duration of $MAXSLOWTIME
#
## Usage (manual): /glroot/bin/dirupdate -w --loop=3 --silent --daemon --loglevel=3 -exec "/glroot/bin/scripts/killslow.sh '{bxfer}' '{lupdtime}' '{user}' '{pid}' '{rate}' '{status}' '{exe}' '{FLAGS}' '{dir}'"
#
## Usage (macro): ./dirupdate -m killslow
#
##  To use this macro, place script in the same directory (or any subdirectory) where dirupdate is located
#
## See ./dirupdate --help for more info about options
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
############################[ END OPTIONS ]##############################

! echo $6 | grep -P "STOR|RETR" > /dev/null && exit 1

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

DIFFT=$[CT-LUPDT];

#[ $DIFFT -lt 1 ] && exit 1

echo "$GLUSER @ $DRATE B/s for $DIFFT seconds"

SLOW=0
SHOULDKILL=0
KILLED=0

[ $DIFFT -gt $WAIT ] && [ $DRATE -lt $MINRATE ] && SLOW=1 && [ $VERBOSE -gt 0 ] && 
        O="[$(date "+%T %D")] WARNING: Too slow (running $DIFFT secs): $GLUSER [PID: $4] [Rate: $DRATE/$MINRATE B/s]\n"  

if [ $SLOW -eq 1 ] && [ -f /tmp/du-ks/$4 ]; then
	MT1=$(stat -c %Y /tmp/du-ks/$4) 
	UNDERTIME=$[CT-MT1]
	[ $UNDERTIME -gt $MAXSLOWTIME ] && 
		O="[$(date "+%T %D")] KILLING: [PID: $4]: Below speed limit for too long ($UNDERTIME secs): $GLUSER [Rate: $DRATE/$MINRATE B/s]\n" &&  
		SHOULDKILL=1 && kill $4 && KILLED=1 &&  rm /tmp/du-ks/$4
	if [ $KILLED -eq 1 ]; then 
		FORCEKILL=0

		i=0
		while [ -n "$(ps -p $4 -o comm=)" ] && [ $i -lt 4 ]; do
			i=$[i+1]
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
