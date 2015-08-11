#!/usr/local/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:getscore-f:{m:exe} -x {m:arg1} --silent --dir -exec "{m:spec1} $(basename {arg}) score"
#
## Retrieves game info using giantbomb API (XML)
#
## Usage (macro): ./dirupdate -m getscore-f --arg1=/path/to/games
#
##  To use this macro, place script in the same directory (or any subdirectory) where dirupdate is
 located
#
CURL="/usr/local/bin/curl"
CURL_FLAGS="--silent"
XMLLINT="/usr/local/bin/xmllint"

###########################[ BEGIN OPTIONS ]#############################
#
BURL="http://www.giantbomb.com/"
URL="$BURL""api"
#
## Get it from giantbomb website (registration required)
API_KEY=""
#
INPUT_CLEAN_REGEX="([\\.\\_\\-\\(\\)](MACOSX|NUKED|EUR|Creators[\\.\\_\\-\\(\\)]Edition|PATCH|DATAPACK|GAMEFIX|READ[\\.\\_\\-\\(\\)]NFO|MULTI[0-9]{1,2}|HD|PL|POLISH|RU|RUSSIAN|JAPANESE|SWEDISH|DANISH|GERMAN|ITALIAN|KOREAN|LINUX|ISO|MAC|NFOFIX|DEVELOPERS[\\.\\_\\-\\(\\)]CUT|READNFO|DLC|INCL[\\.\\_\\-\\(\\)]+|v[0-9]|INSTALL|FIX|UPDATE|PROPER|REPACK|GOTY|MULTI|Crack|DOX)[\\.\\_\\-\\(\\)].*)|(-[A-Z0-9a-z_-]*)$"
#
############################[ END OPTIONS ]##############################

QUERY=$(echo $1 | sed -r "s/($INPUT_CLEAN_REGEX)//gI" | sed -r "s/[\\.\\_\\-\\(\\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")

WHAT=$2

[ "$WHAT" = "score" ] && FIELD="reviews"

[ -z "$FIELD" ] && exit 1
[ -z "$QUERY" ] && exit 2

APIKEY_STR="?api_key=$API_KEY"

G_ID=$($CURL $CURL_FLAGS "$URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY" | $XMLLINT --xpath "string((/response/results//id)[1])" -)

#echo "$URL/search/$APIKEY_STR&limit=1&resources=game&query=$QUERY"

[ -z "$G_ID" ] && echo "ERROR: '$QUERY': Failed getting game ID" && exit 1

RES=$($CURL $CURL_FLAGS $BURL""game/3030-$G_ID/user-reviews/ | grep "<span class=\"average-score\">" | head -1 | sed 's/.*<span class="average-score">//' | sed 's/[ ]*stars.*//')

[ -z "$RES" ] && echo "ERROR: '$QUERY': could not get result '$WHAT' from $BURL""game/3030-$G_ID/user-reviews/" && exit 1

[ "$WHAT" = "score" ] && {
    echo "SCORE: '$QUERY': $RES"
}

exit 0
