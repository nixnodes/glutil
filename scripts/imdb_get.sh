#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:imdb:{m:exe} -x {m:arg1} --silent --dir --exec `{m:spec1} "`basename '{arg}'`" '{exe}' '{imdbfile}' '{glroot}' '{siterootn}' '{arg}'` {m:arg2}
#@MACRO:imdb-c:{m:exe} -x {m:arg1} --cdir --exec "{m:spec1} `basename {arg}` '{exe}' '{imdbfile}' '{glroot}' '{siterootn}' '{arg}'" {m:arg2}
#@MACRO:imdb-d:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:exe} -v --backup imdb" -exec "{m:spec1} '{basedir}' '{exe}' '{imdbfile}' '{glroot}' '{siterootn}' '{dir}'" --iregexi "{m:arg1}" 
#
## Gets movie info using iMDB native API and omdbapi (XML)
#
## Requires: -glutil-1.6 or above
##			 -libxml2 v2.7.7 or above 
#
## Tries to find ID using iMDB native API first - in case of failure, omdbapi search is used
#
## Usage (macro): ./glutil -m imdb --arg1=/path/to/movies [--arg2=<path filter>]        (filesystem based)
##                ./glutil -m imdb-d --arg1 '\/(x264|xvid)\/.*\-[a-zA-Z0-9\-_]+$'       (dirlog based)
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
###########################[ BEGIN OPTIONS ]#############################
#
# omdbapi base url
URL="http://www.omdbapi.com/"
#
# iMDB base url
IMDBURL="http://www.imdb.com/"
#
INPUT_SKIP="^(.* complete .*|sample|subs|no-nfo|incomplete|covers|cover|proof|cd[0-9]{1,3}|dvd[0-9]{1,3}|nuked\-.*|.* incomplete .*|.* no-nfo .*)$"
#
INPUT_CLEAN_REGEX="([._-\(\)][1-2][0-9]{3,3}|)([._-\(\)](VOBSUBS|SUBPACK|BOXSET|FESTIVAL|(720|1080)[ip]|RERIP|UNRATED|DVDSCR|TC|TS|CAM|EXTENDED|TELESYNC|DVDR|X264|HDTV|SDTV|PDTV|XXX|WORKPRINT|SUBBED|DUBBED|DOCU|THEATRICAL|RETAIL|SUBFIX|NFOFIX|DVDRIP|HDRIP|BRRIP|BDRIP|LIMITED|PROPER|REPACK|XVID)([._-\(\)]|$).*)|-([A-Z0-9a-z_-]*$)"
#
## If set to 1, might cause mis-matches 
## Only runs if exact match fails
LOOSE_SEARCH=0
#
## Updates imdblog
UPDATE_IMDBLOG=1
#
## Set to 0, imdblog directory path fields are set to glroot  
##  relative path
## Set to 1, imdblog directory path fields are set to exact 
##  query that was made
## Existing records are always overwritten, except if DENY_IMDBID_DUPE=1
DATABASE_TYPE=1
#
## If set to 1, do not import records with same
## iMDB ID already in the database
DENY_IMDBID_DUPE=1
#
## Overwrite existing matched record, when it's atleast 
##  this old (days) (when DENY_IMDBID_DUPE=1)
RECORD_MAX_AGE=14
#
## Work with unique database for each type
TYPE_SPECIFIC_DB=0
#
## Verbose output
VERBOSE=1
#
############################[ END OPTIONS ]##############################

CURL="/usr/bin/curl"
CURL_FLAGS="--silent"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

! [ -f "$CURL" ] && CURL=`whereis curl | awk '{print $2}'`
! [ -f "$XMLLINT" ] && XMLLINT=`whereis xmllint | awk '{print $2}'`

[ -z "$XMLLINT" ] && echo "Could not find command line XML tool" && exit 1
[ -z "$CURL" ] && echo "Could not find curl" && exit 1

BASEDIR=`dirname $0`

[ $TYPE_SPECIFIC_DB -eq 1 ] && [ $DATABASE_TYPE -gt 0 ] && LAPPEND="$DATABASE_TYPE"

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

echo "$1" | egrep -q -i "$INPUT_SKIP" && exit 1

QUERY=`echo "$1" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r 's/[._-\(\)]/+/g' | sed -r 's/(^[+ ]+)|([+ ]+$)//g'`

[ -z "$QUERY" ] && exit 1

imdb_search()
{
	$CURL $CURL_FLAGS "$IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$1" | $XMLLINT --xpath "((/IMDbResults//ImdbEntity)[1]/@id)" - 2> /dev/null | sed -r 's/(id\=)|( )|["]//g'
}

cad() {
	RTIME=`$1 --imdblog "$4$LAPPEND" -a $2 "$3" --imatchq -exec "echo {time}" --silent`
	
	CTIME=`date +"%s"`
	[ -n "$RTIME" ] && DIFF1=`expr $CTIME - $RTIME` && DIFF=`expr $DIFF1 / 86400`
	if [ $RECORD_MAX_AGE -gt 0 ] && [ -n "$DIFF" ] && [ $DIFF -ge $RECORD_MAX_AGE ]; then
	 	echo "NOTICE: $QUERY: $SHOWID: Record too old ($DIFF days) updating.."
	else
		if [ -n "$RTIME" ]; then
			[ $VERBOSE -gt 0 ] && echo "WARNING: $QUERY: [$2 $3]: already exists in database (`expr $DIFF1 / 60` min old)"
			exit 1
		fi
	fi
}

if [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
	s_q=`echo "$QUERY" | sed 's/\+/\\\\\0/g'`
	cad $2 "--iregexi" "dir,$s_q" "$3"
fi

iid=`imdb_search "$QUERY""&ex=1"`
[ $LOOSE_SEARCH -eq 1 ] &&[ -z "$iid" ] && echo "WARNING: $QUERY: $1: exact match failed, performing loose search.." && iid=`imdb_search "$QUERY"`
[ -z "$iid" ] && echo "WARNING: $QUERY: $1: $IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$QUERY search failed, falling back to secondary" && iid=`$CURL $CURL_FLAGS "$URL?r=xml&s=$QUERY" | $XMLLINT --xpath "((/root/Movie)[1]/@imdbID)" - 2> /dev/null | sed -r 's/(imdbID\=)|(\s)|[\"]//g'`
[ -z "$iid" ] && echo "ERROR: $QUERY: $1: cannot find record [$URL?r=xml&s=$QUERY]" && exit 1

if [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
	cad $2 "--iregex" "imdbid,^$iid$" "$3"	
fi

DDT=`$CURL $CURL_FLAGS "$URL""?r=XML&i=$iid"`

[ -z "$DDT" ] && echo "ERROR: $QUERY: $1: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1

get_field()
{
	echo $DDT | $XMLLINT --xpath "((/root/movie)[1]/@$1)" - 2> /dev/null | sed -r "s/($1\=)|(^[ ]+)|([ ]+$)|[\"]//g" 
}

TYPE=`get_field type`

! echo $TYPE | grep "movie" > /dev/null && echo "ERROR: $QUERY: $1: invalid match (type is $TYPE)" && exit 1

RATING=`get_field imdbRating`
GENRE=`get_field genre`
VOTES=`echo $(get_field imdbVotes) | tr -d ','`
YEAR=`get_field year | tr -d ' '`
TITLE=`get_field title`
RATED=`get_field rated`
ACTORS=`get_field actors`
DIRECTOR=`get_field director`
RELEASED=`date --date="$(D_g=$(get_field released); [ "$D_g" != "N/A" ] && echo "$D_g" || echo "")" +"%s"`
RUNTIME=`get_field runtime`
RUNTIME_h=`echo $RUNTIME | awk '{print $1}' | sed -r 's/[^0-9]+//g'`
RUNTIME_m=`echo $RUNTIME | awk '{print $3}' | sed -r 's/[^0-9]+//g'`
[ -z "$RUNTIME_m" ] && RUNTIME=$RUNTIME_h || RUNTIME=$[RUNTIME_h*RUNTIME_m]

[ -z "$RATING" ] && [ -z "$VOTES" ] && [ -z "$GENRE" ] && echo "ERROR: $QUERY: $1: could not extract movie data" && exit 1

if [ $UPDATE_IMDBLOG -eq 1 ]; then
	trap "rm /tmp/glutil.img.$$.tmp; exit 2" 2 15 9 6
	if [ $DATABASE_TYPE -eq 0 ]; then
		GLR_E=`echo $4 | sed 's/\//\\\//g'`	
		DIR_E=`echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//"`  
		$2 --imdblog="$3$LAPPEND" -a --iregex "$DIR_E" --imatchq -v > /dev/null || $2 -f --imdblog="$3$LAPPEND" -e imdb --regex "$DIR_E" > /dev/null || { 
			echo "ERROR: $DIR_E: Failed removing old record" && exit 1 
		}
	elif [ $DATABASE_TYPE -eq 1 ]; then
		#[ -z "$TITLE" ] && echo "ERROR: $QUERY: $1: failed extracting movie title" && exit 1
		DIR_E=$QUERY		
		$2 --imdblog="$3$LAPPEND" -a --iregex imdbid,"^$iid$" --imatchq > /dev/null || $2 -f --imdblog="$3$LAPPEND" -e imdb --regex imdbid,"^$iid$" > /dev/null || {
			echo "ERROR: $iid: Failed removing old record" && exit 1 
		}
	fi	
	
	echo -en "dir $DIR_E\ntime `date +%s`\nimdbid $iid\nscore $RATING\ngenre $GENRE\nvotes $VOTES\ntitle $TITLE\nactors $ACTORS\nrated $RATED\nyear $YEAR\nreleased $RELEASED\nruntime $RUNTIME\ndirector $DIRECTOR\n\n" > /tmp/glutil.img.$$.tmp
	$2 --imdblog="$3$LAPPEND" -z imdb --nobackup --silent < /tmp/glutil.img.$$.tmp || echo "ERROR: $QUERY: $1: failed writing to imdblog!!"
	rm /tmp/glutil.img.$$.tmp
fi

echo "IMDB: `echo "Q:'$QUERY' | A:'$TITLE'" | tr '+' ' '` : $IMDBURL""title/$iid : $RATING $VOTES $GENRE"

exit 0
