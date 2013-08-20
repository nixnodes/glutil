#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:imdb:{m:exe} -x {m:arg1} --silent --dir --exec "{m:spec1} $(basename {arg})"
#@MACRO:imdb-d:{m:exe} -d --silent -exec "{m:spec1} {basedir}" --iregex "{m:arg1}" 
#
## Retrieves iMDB info using omdbapi (XML)
#
## Usage (macro): ./glutil -m imdb --arg1=/path/to/movies
##                ./glutil -m imdb-d --arg1 "\/xvid\/[^,]{3,}"
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
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
URL="http://www.omdbapi.com/"
#
IMDBURL="http://www.imdb.com/"
#
INPUT_CLEAN_REGEX="([._-\(\)](OVA|SUBBED|DUBBED|DOCU|THEATRICAL|RETAIL|SUBFIX|NFOFIX|DVDRIP|[1-2][0-9]{3,3}|HDRIP|BRRIP|BDRIP|LIMITED|PROPER|REPACK|XVID)[._-\(\)].*)|(-[A-Z0-9a-z_-]*)$"
#
############################[ END OPTIONS ]##############################

QUERY=$(echo $1 | sed -r "s/($INPUT_CLEAN_REGEX)//gi" | sed -r "s/[._-\(\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")

[ -z "$QUERY" ] && exit 1

YEAR=$2


iid=$($CURL $CURL_FLAGS "$IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$QUERY" | xmllint --xpath "(/IMDbResults//ImdbEntity[1]/@id)" - 2> /dev/null | sed -r 's/(id\=)|(\s)|[\"]//g')

[ -z "$iid" ] && echo "WARNING: $QUERY: $IMDBURLxml/find?xml=1&nr=1&tt=on&q=$QUERY search failed, falling back to secondary" && iid=$($CURL $CURL_FLAGS "$URL?r=xml&s=$QUERY" | xmllint --xpath "((/root/Movie)[1]/@imdbID)" - 2> /dev/null | sed -r 's/(imdbID\=)|(\s)|[\"]//g')
[ -z "$iid" ] && echo "ERROR: $1: $QUERY: cannot find record [$URL?r=xml&s=$QUERY]" && exit 1

get_field()
{
	echo $DDT | xmllint --xpath "((/root/movie)[1]/@$1)" - 2> /dev/null | sed -r "s/($1\=)|([ ])|[\"]//g" 
}

DDT=$($CURL $CURL_FLAGS "$URL/?r=XML&i=$iid")

[ -z "$DDT" ] && echo "ERROR: $1: $QUERY: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1

RATING=$(get_field imdbRating)

[ -z "$RATING" ] && echo "ERROR: $1: $QUERY: could not extract movie data" && exit 1

echo "IMDB: $(echo $QUERY | tr '+' ' ') : $IMDBURL""title/$iid : $(get_field imdbRating) $(echo $(get_field imdbVotes) | tr -d ',') $(get_field genre) "


exit 0
