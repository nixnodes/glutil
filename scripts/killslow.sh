#!/bin/bash
#
# Example usage: /glroot/bin/dirupdate -w -exec '/glroot/bin/scripts/killslow.sh {bxfer} {lupdtime} {user} {pid} {rate}'
#

# Minimum allowed transfer rate (bytes per second)
MINRATE=512000

# Enforce only after transfer is atleast this amount of seconds old
WAIT=15

# File to log to
LOG="/var/log/killslow.log"

#########################################################################

[ -z "$1" ] && exit 1
[ -z "$2" ] && exit 1
[ -z "$3" ] && exit 1
[ -z "$4" ] && exit 1
[ -z "$5" ] && exit 1

BXFER=$1

[ $BXFER -lt 1 ] && exit 1

DRATE=$5
LUPDT=$2
GLUSER=$3
CT=$(date +%s)

DIFFT=$[CT-LUPDT];

[ $DIFFT -lt 1 ] && exit 1

echo "$GLUSER @ $DRATE B/s for $DIFFT seconds"

KILLED=0
SHOULDKILL=0

[ $DIFFT -gt $WAIT ] && [ $DRATE -lt $MINRATE ] && 
        O="Too slow (running $DIFFT secs): $GLUSER [PID: $4] [Rate: $DRATE/$MINRATE B/s]" && echo $O && SHOULDKILL=1 && kill $4 && KILLED=1

[ $KILLED -eq 1 ] && [ -n "$LOG" ] && echo $O >> $LOG

[ $SHOULDKILL -eq 1 ] && [ $KILLED -eq 0 ] && echo "Sending SIGTERM to PID $4 ($GLUSER) failed!" 

exit 1
