#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:tvrage:{m:exe} -x {m:arg1} --silent --dir --exec `{m:spec1} "$(basename {arg})" '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' '{arg}'` {m:arg2}"
#@MACRO:tvrage-c:{m:exe} -x {m:arg1} --cdir --exec "{m:spec1} $(basename {arg}) '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' '{arg}'" {m:arg2}"
#@MACRO:tvrage-d:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:exe} -v --backup tvrage" -exec `{m:spec1} "{basedir}" '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' '{dir}'` --iregex "{m:arg1}" 
#
## Gets show info using TVRAGE API (XML)
#
## Requires glutil-1.7-6 or greater
#
## Usage (macro): ./glutil -m tvrage --arg1=/path/to/shows [--arg2=<path filter>]                 (filesystem based)
##                ./glutil -m tvrage-d --arg1 '\/tv\-((sd|hd|)x264|xvid)\/.*\-[a-zA-Z0-9\-_]+$'   (dirlog based)
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
###########################[ BEGIN OPTIONS ]#############################
#
# tvrage services base url
URL="http://services.tvrage.com"
#
#INPUT_SKIP="^(.* complete .*|sample|subs|no-nfo|incomplete|covers|cover|proof|cd[0-9]{1,3}|dvd[0-9]{1,3}|nuked\-.*|.* incomplete .*|.* no-nfo .*)$"
#
#INPUT_CLEAN_REGEX="([._-\(\)][1-2][0-9]{3,3}|)([._-\(\)](S[0-9]{1,3}E[0-9]{1,3}|XVID|X264|REPACK|DVDRIP|(H|P)DTV|BRRIP)([._-\(\)]|$).*)|-([A-Z0-9a-z_-]*$)"
#
## Updates tvlog
UPDATE_TVLOG=1
#
## Set to 0, tvlog directory path fields are set to glroot  
##  relative path
## Set to 1, tvlog directory path fields are set to exact 
##  query that was made
## Existing records are always overwritten, except if DENY_IMDBID_DUPE=1
DATABASE_TYPE=1
#
## If set to 1, do not import records with same
## showID already in the database
DENY_TVID_DUPE=1
#
## If set to 1, do not import records with same
## name already in the database
DENY_QUERY_DUPE=1
#
## Overwrite existing matched record, when it's atleast 
##  this old (days) (when DENY_TVID_DUPE=1 or DENY_QUERY_DUPE=1)
RECORD_MAX_AGE=14
#
## Work with unique database for each type
TYPE_SPECIFIC_DB=1
#
VERBOSE=0
############################[ END OPTIONS ]##############################

CURL="/usr/bin/curl"
CURL_FLAGS="--silent"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

! [ -f "$CURL" ] && CURL=$(whereis curl | awk '{print $2}')
! [ -f "$XMLLINT" ] && XMLLINT=$(whereis xmllint | awk '{print $2}')

[ -z "$XMLLINT" ] && echo "Could not find command line XML tool" && exit 1
[ -z "$CURL" ] && echo "Could not find curl" && exit 1

BASEDIR=$(dirname $0)

[ $TYPE_SPECIFIC_DB -eq 1 ] && [ $DATABASE_TYPE -gt 0 ] && LAPPEND="$DATABASE_TYPE"

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

echo "$1" | grep -P -i "$INPUT_SKIP" > /dev/null && exit 1

QUERY=$(echo "$1" | tr ' ' '+' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r "s/[._-\(\)]/+/g" | sed -r "s/^[+ ]+//"| sed -r "s/[+ ]+$//")

[ -z "$QUERY" ] && exit 1

cad() {
	RTIME=$($1 --tvlog "$4$LAPPEND" -h $2 "$3" --imatchq -exec "echo {time}" --silent)
	CTIME=$(date +"%s")
	[ -n "$RTIME" ] && DIFF1=$(expr $CTIME - $RTIME) && DIFF=$(expr $DIFF1 / 86400)
	if [ $RECORD_MAX_AGE -gt 0 ] && [ -n "$DIFF" ] && [ $DIFF -ge $RECORD_MAX_AGE ]; then
	 	echo "NOTICE: $QUERY: $SHOWID: Record too old ($DIFF days) updating.."
	else
		if [ -n "$RTIME" ]; then
			[ $VERBOSE -gt 0 ] && echo "WARNING: $QUERY: [$2 $3]: already exists in database ($(expr $DIFF1 / 60) min old)"
			exit 1
		fi
	fi
}

if [ $UPDATE_TVLOG -eq 1 ] && [ $DENY_QUERY_DUPE -eq 1 ]; then
	s_q=$(echo $QUERY | sed 's/\+/\\\0/g')
	cad $2 "--iregexi" "dir,$s_q" "$3"
fi

[ $VERBOSE -gt 1 ] && echo "NOTICE: query: $QUERY: $1"

DDT=$($CURL $CURL_FLAGS "$URL""/feeds/full_search.php?show=$QUERY")

[ -z "$DDT" ] && echo "ERROR: $QUERY: $1: unable to get show data $URL""/feeds/full_search.php?show=$QUERY" && exit 1

get_field()
{
	echo $DDT | $XMLLINT --xpath "((/Results//show)[1]/$1"")" - | sed -r "s/<[\/a-zA-Z0-9]+>//g"
}

get_field_t()
{
	echo $DDT | $XMLLINT --xpath "((/Results//show)[1]/$1"")" - | sed -r "s/<[\/a-zA-Z0-9]+>/,/g" | sed -r "s/(^[,]+)|([,]+$)//g" | sed -r "s/[,]{2,}/,/g"
}

SHOWID=$(get_field showid)

if [ -z "$SHOWID" ]; then 
	echo "ERROR: $QUERY: $1: could not get show id"
	[ $VERBOSE -gt 0 ] && echo "$DDT"
	exit 1
fi
if [ $UPDATE_TVLOG -eq 1 ] && [ $DENY_TVID_DUPE -eq 1 ]; then
	cad $2 "--iregex" "showid,^$SHOWID$" "$3"	
fi

NAME=$(get_field name)
STATUS=$(get_field status)
[ -z "$STATUS" ] && STATUS="N/A"
COUNTRY=$(get_field country)
[ -z "$COUNTRY" ] && COUNTRY="N/A"
SEASONS=$(get_field seasons)
[ -z "$SEASONS" ] && SEASONS=0
CLASS=$(get_field classification)
AIRTIME=$(get_field airtime)
[ -z "$AIRTIME" ] && AIRTIME="N/A"
AIRDAY=$(get_field airday)
[ -z "$AIRDAY" ] && AIRDAY="N/A"
RUNTIME=$(get_field runtime)
[ -z "$RUNTIME" ] && RUNTIME=0
LINK=$(get_field link)
[ -z "$LINK" ] && LINK="N/A"
ZZ=$(get_field started)
[ $(echo "$ZZ" | tr '/' ' ' | wc -w) -eq 2 ] && ZZ="1 $ZZ"
[ -n "$ZZ" ] && STARTED=$(date --date="$(echo $ZZ | tr '/' ' ')" +"%s") || STARTED=0
ZZ=$(get_field ended)
[ $(echo "$ZZ" | tr '/' ' ' | wc -w) -eq 2 ] && ZZ="1 $ZZ"
[ -n "$ZZ" ] && ENDED=$(date --date="$(echo $ZZ | tr '/' ' ')" +"%s") || ENDED=0
GENRES=$(get_field_t '/genres//genre[.]')
[ -z "$GENRES" ] && GENRES="N/A"

if [ $UPDATE_TVLOG -eq 1 ]; then
	trap "rm /tmp/glutil.img.$$.tmp; exit 2" 2 15 9 6
	if [ $DATABASE_TYPE -eq 0 ]; then
		GLR_E=$(echo $4 | sed 's/\//\\\//g')	
		DIR_E=$(echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//")  
		$2 --tvlog="$3$LAPPEND" -h --iregex "$DIR_E" --imatchq -v > /dev/null || $2 -f --tvlog="$3$LAPPEND" -e tvrage --regex "$DIR_E" > /dev/null || { 
			echo "ERROR: $DIR_E: Failed removing old record" && exit 1 
		}
	elif [ $DATABASE_TYPE -eq 1 ]; then		
		DIR_E=$QUERY
		$2 --tvlog="$3$LAPPEND" -h --iregex showid,"^$SHOWID$" --imatchq > /dev/null || $2 -f --tvlog="$3$LAPPEND" -e tvrage --regex showid,"^$SHOWID$" > /dev/null || {
			echo "ERROR: $SHOWID: Failed removing old record" && exit 1 
		}
	fi	
	
	echo -en "dir $DIR_E\ntime $(date +%s)\nshowid $SHOWID\nclass $CLASS\nname $NAME\nstatus $STATUS\ncountry $COUNTRY\nseasons $SEASONS\nairtime $AIRTIME\nairday $AIRDAY\nruntime $RUNTIME\nlink $LINK\nstarted $STARTED\nended $ENDED\ngenres $GENRES\n\n" > /tmp/glutil.img.$$.tmp
	$2 --tvlog="$3$LAPPEND" -z tvrage --nobackup --silent < /tmp/glutil.img.$$.tmp || echo "ERROR: $QUERY: $1: failed writing to tvlog [$3$LAPPEND]"
	rm /tmp/glutil.img.$$.tmp
fi

echo "TVRAGE: $(echo "Q:'$QUERY' | A:'$NAME'" | tr '+' ' ') : $1 : $LINK : $CLASS | $GENRES"

exit 0
