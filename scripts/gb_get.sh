#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:getscore:{m:exe} -x {m:arg1} --silent --dir --exec "{m:spec1} $(basename {arg}) score"
#
## Retrieves game info using giantbomb API (XML)
#
## Usage (macro): ./dirupdate -m getscore --arg1=/path/to/games
#
##  To use this macro, place script in the same directory (or any subdirectory) where dirupdate is located
#
CURL="/usr/bin/curl"
CURL_FLAGS="--silent"
XMLLINT="/usr/bin/xmllint"

! [ -f "$CURL" ] && CURL=$(whereis curl | awk '{print $2}')
! [ -f "$XMLLINT" ] && XMLLINT=$(whereis xmllint | awk '{print $2}')

[ -z "$XMLLINT" ] && echo "Could not find command line XML tool" && exit 1
[ -z "$CURL" ] && echo "Could not find curl" && exit 1

###########################[ BEGIN OPTIONS ]#############################
#
URL="http://www.giantbomb.com/api"
#
## Get it from giantbomb website (registration required)
API_KEY="e0c8aa999e45d61f9ada46be9d983f24fdd5e288"
#
INPUT_CLEAN_REGEX="([._-\(\)]{,1}(MULTI|Crack|DOX).*)|(-[A-Z0-9a-z_-]*)$"
#
############################[ END OPTIONS ]##############################

QUERY=$(echo $1 | sed -r "s/($INPUT_CLEAN_REGEX)//gi" | sed -r "s/[._-\(\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")

WHAT=$2

[ "$WHAT" = "score" ] && FIELD="reviews"

[ -z "$FIELD" ] && exit 1
[ -z "$QUERY" ] && exit 2

APIKEY_STR="?api_key=$API_KEY"

API_DETAIL_URL=$($CURL $CURL_FLAGS "$URL/search/$APIKEY_STR&query=$QUERY&limit=1" | $XMLLINT --xpath "string((/response/results/game/api_detail_url)[1])" -)

[ -z "$API_DETAIL_URL" ] && echo "PARSE: '$QUERY': could not find [@$URL/search/?api_key=$API_KEY&query=$QUERY&limit=1]" && exit 1

API_DETAIL_URL2=$($CURL $CURL_FLAGS "$API_DETAIL_URL$APIKEY_STR&field_list=$FIELD" | $XMLLINT --xpath "string((/response/results/$FIELD//api_detail_url[1]))" -)

[ -z "$API_DETAIL_URL2" ] && echo "PARSE: '$QUERY': could not parse $API_DETAIL_URL$APIKEY_STR&field_list=$FIELD" && exit 1

RES=$($CURL $CURL_FLAGS "$API_DETAIL_URL2$APIKEY_STR&field_list=$WHAT" | $XMLLINT --xpath "string((/response/results/$WHAT)[1])" -)

[ -z "$RES" ] && echo "PARSE: '$QUERY': could not get result '$WHAT' from @$API_DETAIL_URL2" && exit 1

[ "$WHAT" = "score" ] && { 
 	[ $RES -gt -1 ] || exit 1
    [ $RES -lt 11 ] || exit 1
    echo "SCORE: '$QUERY': $RES"
}


