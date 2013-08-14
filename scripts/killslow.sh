#!/bin/bash
#
#@MACRO:killslow:{m:exe} -w --loop=3 --daemon --loglevel 3 --silent -exec "{m:spec1} {bxfer} {lupdtime} {user} {pid} {rate} {status} {exe} {FLAGS}"
#
## Kills any transfer that is under $MINRATE bytes/s for a minimum duration of $MAXSLOWTIME
#
## Usage: /glroot/bin/dirupdate -w --loop=3 --daemon --loglevel 3 --silent -exec "/glroot/bin/scripts/killslow.sh {bxfer} {lupdtime} {user} {pid} {rate} {status} {exe} {FLAGS}"
#
## See ./dirupdate --help for more info
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
#########################################################################

[ -z "$1" ] && exit 1
[ -z "$2" ] && exit 1
[ -z "$3" ] && exit 1
[ -z "$4" ] && exit 1
[ -z "$5" ] && exit 1
[ -z "$6" ] && exit 1

! echo $6 | grep -P "STOR|RETR" > /dev/null && exit 1

! [ -d "/tmp/du-ks" ] && mkdir -p /tmp/du-ks

BXFER=$1

if [ $BXFER -lt 1 ]; then
	[ -d /tmp/du-ks/$4 ] && rmdir /tmp/du-ks/$4 
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

[ $DIFFT -gt $WAIT ] && [ $DRATE -lt $MINRATE ] && 
        O="WARNING: Too slow (running $DIFFT secs): $GLUSER [PID: $4] [Rate: $DRATE/$MINRATE B/s]" && echo $O && SLOW=1

if [ $SLOW -eq 1 ] && [ -d /tmp/du-ks/$4 ]; then
	MT1=$(stat -c %Y /tmp/du-ks/$4) 
	UNDERTIME=$[CT-MT1]
	[ $UNDERTIME -gt $MAXSLOWTIME ] && 
		O="KILLING: Below speed limit for too long ($UNDERTIME secs): $GLUSER [PID: $4] [Rate: $DRATE/$MINRATE B/s]" &&  
		echo $O && SHOULDKILL=1 && kill $4 && KILLED=1 && rmdir /tmp/du-ks/$4
elif [ $SLOW -eq 1 ]; then
	mkdir /tmp/du-ks/$4
elif [ $SLOW -eq 0 ] && [ -d /tmp/du-ks/$4 ]; then
 	rmdir /tmp/du-ks/$4
fi

[ -n "$O" ] && [ -n "$LOG" ] && echo $O >> $LOG

[ $SHOULDKILL -eq 1 ] && [ $KILLED -eq 0 ] && echo "Sending SIGTERM to PID $4 ($GLUSER) failed!" && 
	[ -d /tmp/du-ks/$4 ] && rmdir /tmp/du-ks/$4

exit 1
