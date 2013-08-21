#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:imdb:{m:exe} -x {m:arg1} --silent --dir --exec "{m:spec1} $(basename {arg}) '{exe}' '{imdbfile}' '{glroot}' '{siterootn}' '{arg}'"
#@MACRO:imdb-d:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:exe} -v --backup imdb" -exec "{m:spec1} {basedir} '{exe}' '{imdbfile}' '{glroot}' '{siterootn}' '{dir}'" --iregex "{m:arg1}" 
#
## Gets movie info using iMDB native API and omdbapi (XML)
#
## Tries to find ID using iMDB native API first - in case of failure, omdbapi search is used
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
# omdbapi base url
URL="http://www.omdbapi.com/"
#
# iMDB base url
IMDBURL="http://www.imdb.com/"
#
## Updates imdblog
UPDATE_IMDBLOG=1
#
INPUT_SKIP="^(.* complete .*|sample|subs|covers|cover|proof|cd[0-9]{1,3}|dvd[0-9]{1,3})$"
#
INPUT_CLEAN_REGEX="([._-\(\)](FESTIVAL|(720|1080)[ip]RERIP|UNRATED|DVDSCR|TC|TS|CAM|TELESYNC|DVDR|X264|HDTV|SDTV|PDTV|XXX|WORKPRINT|OVA|SUBBED|DUBBED|DOCU|THEATRICAL|RETAIL|SUBFIX|NFOFIX|DVDRIP|[1-2][0-9]{3,3}|HDRIP|BRRIP|BDRIP|LIMITED|PROPER|REPACK|XVID)[._-\(\)].*)|(-[A-Z0-9a-z_-]*)$"
#
############################[ END OPTIONS ]##############################

echo "$1" | grep -P -i "$INPUT_SKIP" > /dev/null && exit 1

QUERY=$(echo "$1" | tr ' ' '+' | sed -r "s/($INPUT_CLEAN_REGEX)//gi" | sed -r "s/[._-\(\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")

[ -z "$QUERY" ] && exit 1

imdb_search()
{
	$CURL $CURL_FLAGS "$IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$1" | xmllint --xpath "((/IMDbResults//ImdbEntity)[1]/@id)" - 2> /dev/null | sed -r 's/(id\=)|(\s)|[\"]//g'
}

iid=$(imdb_search "$QUERY""&ex=1")
[ -z "$iid" ] && echo "WARNING: $QUERY: $1: exact match failed, performing loose search.." && iid=$(imdb_search "$QUERY")
[ -z "$iid" ] && echo "WARNING: $QUERY: $1: $IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$QUERY search failed, falling back to secondary" && iid=$($CURL $CURL_FLAGS "$URL?r=xml&s=$QUERY" | xmllint --xpath "((/root/Movie)[1]/@imdbID)" - 2> /dev/null | sed -r 's/(imdbID\=)|(\s)|[\"]//g')
[ -z "$iid" ] && echo "ERROR: $QUERY: $1: cannot find record [$URL?r=xml&s=$QUERY]" && exit 1

DDT=$($CURL $CURL_FLAGS "$URL""?r=XML&i=$iid")

[ -z "$DDT" ] && echo "ERROR: $QUERY: $1: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1

get_field()
{
	echo $DDT | xmllint --xpath "((/root/movie)[1]/@$1)" - 2> /dev/null | sed -r "s/($1\=)|([ ])|[\"]//g" 
}

RATING=$(get_field imdbRating)
GENRE=$(get_field genre)
VOTES=$(echo $(get_field imdbVotes) | tr -d ',')

[ -z "$RATING" ] && [ -z "$VOTES" ] && [ -z "$GENRE" ] && echo "ERROR: $QUERY: $1: could not extract movie data" && exit 1

if [ $UPDATE_IMDBLOG -eq 1 ]; then
	GLR_E=$(echo $4 | sed 's/\//\\\//g')	
    
	DIR_E=$(echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//")
	$2 -a --iregex "$DIR_E" --imatchq > /dev/null || $2 -e imdb --match "$DIR_E" > /dev/null
	echo -en "dir $DIR_E\ntime $(date +%s)\nimdbid $iid\nscore $RATING\ngenre $GENRE\nvotes $VOTES\n\n" > /tmp/glutil.img.$$.tmp
	$2 -z imdb --nobackup --silent < /tmp/glutil.img.$$.tmp || echo "ERROR: failed writing to imdblog!!"
	rm /tmp/glutil.img.$$.tmp
fi

echo "IMDB: $(echo $QUERY | tr '+' ' ') : $IMDBURL""title/$iid : $RATING $VOTES $GENRE"


exit 0
