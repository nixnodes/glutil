#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:imdb:{m:exe} -x {m:arg1} --silent --dir --exec "{m:spec1} $(basename {arg}) score"
#
## Retrieves iMDB info using omdbapi (XML)
#
## Usage (macro): ./dirupdate -m imdb --arg1=/path/to/movies
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
URL="http://www.omdbapi.com/"
#
INPUT_CLEAN_REGEX="([._-\(\)](MACOSX|NUKED|EUR|Creators[._-\(\)]Edition|PATCH|DATAPACK|GAMEFIX|READ[._-\(\)]NFO|MULTI[0-9]{1,2}|HD|PL|POLISH|RU|RUSSIAN|JAPANESE|SWEDISH|DANISH|GERMAN|ITALIAN|KOREAN|LINUX|ISO|MAC|NFOFIX|DEVELOPERS[._-\(\)]CUT|READNFO|DLC|INCL[._-\(\)]+|v[0-9]|INSTALL|FIX|UPDATE|PROPER|REPACK|GOTY|MULTI|Crack|DOX)[._-\(\)].*)|(-[A-Z0-9a-z_-]*)$"
#
############################[ END OPTIONS ]##############################

QUERY=$(echo $1 | sed -r "s/($INPUT_CLEAN_REGEX)//gi" | sed -r "s/[._-\(\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")

[ -z "$QUERY" ] && exit 1

YEAR=$2

iid=$($CURL $CURL_FLAGS "$URL/?r=xml&s=$QUERY" | xmllint --xpath "((/root/Movie)[1]/@imdbID)" - | sed -r 's/(imdbID\=)|(\s)|[\"]//g')

[ -z "$iid" ] && echo "ERROR: $QUERY: unable to get iMDB ID [$URL/?r=xml&s=$QUERY]"

get_field()
{
	echo $DDT | xmllint --xpath "((/root/movie)[1]/@$1)" - | sed -r "s/($1\=)|([ ])|[\"]//g"
}

DDT=$($CURL $CURL_FLAGS "$URL/?r=XML&i=$iid")

[ -z "$DDT" ] && echo "ERROR: $QUERY: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1

echo "IMDB: $QUERY: $(get_field imdbRating) $(get_field imdbVotes) $(get_field genre)"


exit 0
