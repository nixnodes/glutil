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
# DO NOT EDIT THESE LINES
#@VERSION:1
#@REVISION:10
#@MACRO:gamescore|Game info lookup based on folder names (filesystem) [-arg1=<path>]:{m:exe} -x {m:arg1} -lom "depth>0" --silent -v --loglevel=5 --preexec "{m:exe} -v --backup game" --dir -execv `{m:spec1} {basepath} {exe} {gamefile} {glroot} {siterootn} {path}`
#@MACRO:gamescore-d|Game info lookup based on folder names (dirlog) [-arg1=<regex>]:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:exe} -v --backup game" -execv "{m:spec1} {basedir} {exe} {gamefile} {glroot} {siterootn} {dir}" regexi "dir,{m:arg1}" 
#
## Retrieves game info using giantbomb API (XML)
#
## Requires: - glutil-2.4.11 or above
##			 - libxml2 v2.7.7 or above 
##           - curl, date, grep, egrep, sed
#
## Usage (macro): ./glutil -m gamescore --arg1=/path/to/games
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
CURL="/usr/bin/curl"
CURL_FLAGS="--silent"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

###########################[ BEGIN OPTIONS ]#############################
#
GIANTBOMB_BURL="http://www.giantbomb.com/"
GIANTBOMB_URL="$GIANTBOMB_BURL""api"
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

BASEDIR=`dirname $0`

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

echo "$1" | egrep -q -i "$INPUT_SKIP" && exit 1

[ -z "$API_KEY" ] && echo "ERROR: set API_KEY first" && exit 1

QUERY=`echo "$1" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r 's/[\.\_\-\(\)]/+/g' | sed -r 's/(^[+ ]+)|([+ ]+$)//g'`


FIELD="reviews"

[ -z "$QUERY" ] && exit 2

APIKEY_STR="?api_key=$API_KEY"

G_ID=`$CURL $CURL_FLAGS "$GIANTBOMB_URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY" | $XMLLINT --xpath "string((/response/results//id)[1])" -`

#echo "$GIANTBOMB_URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY"

[ -z "$G_ID" ] && echo "ERROR: '$QUERY': Failed getting game ID" && exit 1

RES=`$CURL $CURL_FLAGS $GIANTBOMB_BURL""game/3030-$G_ID/user-reviews/ | grep "<span class=\"average-score\">" | head -1 | sed 's/.*<span class="average-score">//' | sed 's/[ ]*stars.*//'`

[ -z "$RES" ] && echo "ERROR: '$QUERY': could not get result score from $GIANTBOMB_BURL""game/3030-$G_ID/user-reviews/" && exit 1


echo "SCORE: '$QUERY': $RES"


if [ $UPDATE_GAMELOG -eq 1 ]; then
	trap "rm /tmp/glutil.gg.$$.tmp; exit 2" 2 15 9 6
	GLR_E=`echo $4 | sed 's/\//\\\\\//g'`	   
	DIR_E=`echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//"`
	$2 -k regex "$DIR_E" --imatchq > /dev/null || $2 -f -e game ! match "$DIR_E" > /dev/null
	echo -en "dir $DIR_E\ntime `date +%s`\nscore $RES\n\n" > "/tmp/glutil.gg.$$.tmp"
	$2 -z game --nobackup --silent < "/tmp/glutil.gg.$$.tmp" || echo "ERROR: failed writing to gamelog!!"
	rm /tmp/glutil.gg.$$.tmp
fi

exit 0
