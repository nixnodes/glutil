#!/bin/bash
#
# Usage: ./dirupdate -w -exec "scripts/killslow.sh {bxfer} {lupdtime} {user} {pid}"
#

# Minimum allowed transfer rate (bytes per second)
MINRATE=3145728

# Enforce only after transfer is atleast this amount of seconds old
WAIT=15

# File to log to
LOG="/var/log/killslow.log"

#########################################################################

[ -z "$1" ] && exit 1
[ -z "$2" ] && exit 1
[ -z "$3" ] && exit 1
[ -z "$4" ] && exit 1

BXFRD=$1

[ $BXFRD -lt 1 ] && exit 1

LUPDT=$2
GLUSER=$3
CT=$(date +%s)

DIFFT=$[CT-LUPDT];

[ $DIFFT -lt 1 ] && exit 1

DRATE=$[BXFRD/DIFFT]

echo "$GLUSER @ $DRATE B/s for $DIFFT seconds"

KILLED=0

[ $DIFFT -gt $WAIT ] && [ $DRATE -lt $MINRATE ] && 
        O="Too slow (running $DIFFT secs): $GLUSER [$4] ($DRATE B/s)" && echo $O && kill $4 && KILLED=1

[ $KILLED -eq 1 ] && [ -n "$LOG" ] && echo $O >> $LOG

exit 1
