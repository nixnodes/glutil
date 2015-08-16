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
#@REVISION:12
#@MACRO:gamescore|Game info lookup based on folder names (filesystem) [-arg1=<path>]:{exe} -x {arg1} -lom "depth>0" --silent -v --loglevel=5 --preexec "{exe} -v --backup game" --dir -execv `{spec1} \{basepath\} \{exe\} \{gamefile\} \{glroot\} \{siterootn\} \{path\}`
#@MACRO:gamescore-d|Game info lookup based on folder names (dirlog) [-arg1=<regex>]:{exe} -d --silent -v --loglevel=5 --preexec "{exe} -v --backup game" -execv "{spec1} \{basedir\} \{exe\} \{gamefile\} \{glroot\} \{siterootn\} \{dir\}" -regexi "{arg1}" 
#@MACRO:gamescore-e|Game info lookup:{exe} -noop  --gamelog={?q:game@file} --silent --loglevel=1 --preexec `{exe} --gamelog={?q:game@file} --backup game; {spec1} \{basedir\} \{exe\} \{gamefile\} \{glroot\} \{siterootn\} "{arg1}" 1`
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
GIANTBOMB_URL="${GIANTBOMB_BURL}api"
#
## Get it from giantbomb website (registration required)
GB_API_KEY="e0c8aa999e45d61f9ada46be9d983f24fdd5e288"
#
## Updates gamelog
UPDATE_GAMELOG=1
#
############################[ END OPTIONS ]##############################

BASEDIR=`dirname ${0}`

[ -f "${BASEDIR}/config" ] && . ${BASEDIR}/config

if ! [ "${7}" = "1" ]; then
	echo "${1}" | egrep -q -i "${INPUT_SKIP}" && exit 1
	QUERY=`echo "$1" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r 's/[\.\_\-\(\)]/+/g' | sed -r 's/(^[+ ]+)|([+ ]+$)//g'`
else
	QUERY="${6}"
fi

[ -z "$GB_API_KEY" ] && echo "ERROR: set GB_API_KEY first" && exit 1

FIELD="reviews"

[ -z "$QUERY" ] && exit 2

APIKEY_STR="?api_key=${GB_API_KEY}"

G_ID=`$CURL $CURL_FLAGS "${GIANTBOMB_URL}/search/${APIKEY_STR}&limit=1&resources=game&query=$QUERY" | $XMLLINT --xpath "string((/response/results//id)[1])" -`

#echo "$GIANTBOMB_URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY"

[ -z "${G_ID}" ] && echo "ERROR: '$QUERY': Failed getting game ID" && exit 1

RES=`$CURL $CURL_FLAGS ${GIANTBOMB_BURL}game/3030-${G_ID}/user-reviews/ | grep "<span class=\"average-score\">" | head -1 | sed 's/.*<span class="average-score">//' | sed 's/[ ]*stars.*//'`

[ -z "$RES" ] && echo "ERROR: '$QUERY': could not get result score from $GIANTBOMB_BURL""game/3030-$G_ID/user-reviews/" && exit 1


echo "SCORE: '$QUERY': $RES"


if [ ${UPDATE_GAMELOG} -eq 1 ]; then
	trap "rm /tmp/glutil.gg.$$.tmp; exit 2" 2 15 9 6
	GLR_E=`echo ${4} | sed 's/\//\\\\\//g'`	   
	DIR_E=`echo ${6} | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//"`
	${2} -k -regex "$DIR_E" --imatchq --silent && ${2} -f -e game ! -match "$DIR_E" --silent
	echo -en "dir $DIR_E\ntime `date +%s`\nscore $RES\n\n" > "/tmp/glutil.gg.$$.tmp"
	${2} -z game --nobackup --silent < "/tmp/glutil.gg.$$.tmp" || echo "ERROR: failed writing to gamelog!!"
	rm /tmp/glutil.gg.$$.tmp
fi

exit 0
