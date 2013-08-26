#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:gamescore:{m:exe} -x {m:arg1} --silent -v --loglevel=5 --preexec "{m:exe} -v --backup game" --dir -exec "{m:spec1} $(basename {arg}) '{exe}' '{gamefile}' '{glroot}' '{siterootn}' '{dir}'"
#@MACRO:gamescore-d:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:exe} -v --backup game" -exec "{m:spec1} '{basedir}' '{exe}' '{gamefile}' '{glroot}' '{siterootn}' '{dir}'" --iregex "{m:arg1}" 
#
## Retrieves game info using giantbomb API (XML)
#
## Usage (macro): ./glutil -m getscore --arg1=/path/to/games
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
CURL="/usr/bin/curl"
CURL_FLAGS="--silent"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

! [ -f "$CURL" ] && CURL=$(whereis curl | awk '{print $2}')
! [ -f "$XMLLINT" ] && XMLLINT=$(whereis xmllint | awk '{print $2}')

[ -z "$XMLLINT" ] && echo "Could not find command line XML tool" && exit 1
[ -z "$CURL" ] && echo "Could not find curl" && exit 1

###########################[ BEGIN OPTIONS ]#############################
#
BURL="http://www.giantbomb.com/"
URL="$BURL""api"
#
## Get it from giantbomb website (registration required)
API_KEY=""
#
INPUT_SKIP="^(.* complete .*|sample|subs|no-nfo|incomplete|covers|cover|proof|cd[0-9]{1,3}|dvd[0-9]{1,3}|nuked\-.*|.* incomplete .*|.* no-nfo .*)$"
#
## Updates gamelog
UPDATE_GAMELOG=1
#
INPUT_CLEAN_REGEX="([._-\(\)](MACOSX|EUR|Creators[._-\(\)]Edition|PATCH|DATAPACK|GAMEFIX|READ[._-\(\)]NFO|MULTI[0-9]{1,2}|HD|PL|POLISH|RU|RUSSIAN|JAPANESE|SWEDISH|DANISH|GERMAN|ITALIAN|KOREAN|LINUX|ISO|MAC|NFOFIX|DEVELOPERS[._-\(\)]CUT|READNFO|DLC|INCL[._-\(\)]+|v[0-9]|INSTALL|FIX|UPDATE|PROPER|REPACK|GOTY|MULTI|Crack|DOX)([._-\(\)]|$).*)|(-[A-Z0-9a-z_-]*)$"
#
############################[ END OPTIONS ]##############################

BASEDIR=$(dirname $0)

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

echo "$1" | grep -P -i "$INPUT_SKIP" > /dev/null && exit 1

[ -z "$API_KEY" ] && echo "ERROR: set API_KEY first" && exit 1

QUERY=$(echo $1 | sed -r "s/($INPUT_CLEAN_REGEX)//gi" | sed -r "s/[._-\(\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")


FIELD="reviews"

[ -z "$QUERY" ] && exit 2

APIKEY_STR="?api_key=$API_KEY"

G_ID=$($CURL $CURL_FLAGS "$URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY" | $XMLLINT --xpath "string((/response/results//id)[1])" -)

#echo "$URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY"

[ -z "$G_ID" ] && echo "ERROR: '$QUERY': Failed getting game ID" && exit 1

RES=$($CURL $CURL_FLAGS $BURL""game/3030-$G_ID/user-reviews/ | grep "<span class=\"average-score\">" | head -1 | sed 's/.*<span class="average-score">//' | sed 's/[ ]*stars.*//')

[ -z "$RES" ] && echo "ERROR: '$QUERY': could not get result score from $BURL""game/3030-$G_ID/user-reviews/" && exit 1


echo "SCORE: '$QUERY': $RES"


if [ $UPDATE_GAMELOG -eq 1 ]; then
	trap "rm /tmp/glutil.gg.$$.tmp; exit 2" SIGINT SIGTERM SIGKILL SIGABRT
	GLR_E=$(echo $4 | sed 's/\//\\\//g')	    
	DIR_E=$(echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//")
	$2 -k --iregex "$DIR_E" --imatchq > /dev/null || $2 -e game --match "$DIR_E" > /dev/null
	echo -en "dir $DIR_E\ntime $(date +%s)\nscore $RES\n\n" > /tmp/glutil.gg.$$.tmp
	$2 -z game --nobackup --silent < /tmp/glutil.gg.$$.tmp || echo "ERROR: failed writing to gamelog!!"
	rm /tmp/glutil.gg.$$.tmp
fi

exit 0
