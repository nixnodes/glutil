#!/bin/bash
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:2
#@REVISION:8
#@MACRO:tvrage:{m:exe} -x {m:arg1} --silent --dir -execv `{m:spec1} {basepath} {exe} {tvragefile} {glroot} {siterootn} {path} 0` {m:arg2}
#@MACRO:tvrage-d:{m:exe} -d --silent --loglevel=1 --preexec "{m:exe} -v --backup tvrage" -execv `{m:spec1} {basedir} {exe} {tvragefile} {glroot} {siterootn} {dir} 0` --iregexi "dir,{m:arg1}"  {m:arg2} 
#@MACRO:tvrage-su:{m:exe} -h --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec "{m:exe} -v --backup tvrage" -execv `{m:spec1} {basedir} {exe} {tvragefile} {glroot} {siterootn} {dir} 1`
#@MACRO:tvrage-su-id:{m:exe} -h --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec "{m:exe} -v --backup tvrage" -execv `{m:spec1} {basedir} {exe} {tvragefile} {glroot} {siterootn} {dir} 2 {showid}`
#@MACRO:tvrage-e:{m:exe} -d --silent --loglevel=1 --preexec "{m:spec1} '{m:arg1}' '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' 0 0"
#
## Install script dependencies + libs into glftpd root, preserving library paths (requires mlocate)
#
#@MACRO:tvrage-installch:{m:exe} noop --preexec `! updatedb -e "{glroot}" -o /tmp/glutil.mlocate.db && echo "updatedb failed" && exit 1 ; li="/bin/curl /bin/xmllint /bin/date /bin/egrep /bin/sed /bin/expr"; for lli in $li; do lf=$(locate -d /tmp/glutil.mlocate.db "$lli" | head -1) && l=$(ldd "$lf" | awk '{print $3}' | grep -v ')' | sed '/^$/d' ) && for f in $l ; do [ -f "$f" ] && dn="/glftpd$(dirname $f)" && ! [ -d $dn ] && mkdir -p "$dn"; [ -f "{glroot}$f" ] || if cp --preserve=all "$f" "{glroot}$f"; then echo "$lf: {glroot}$f"; fi; done; [ -f "{glroot}/bin/$(basename "$lf")" ] || if cp --preserve=all "$lf" "{glroot}/bin/$(basename "$lf")"; then echo "{glroot}/bin/$(basename "$lf")"; fi; done; rm -f /tmp/glutil.mlocate.db`
#
## Gets show info using TVRAGE API (XML)
#
## Requires: - glutil-1.9-34 or greater
##			 - libxml2 v2.7.7 or above
##           - curl, date, egrep, sed, expr, recode (optional)
#
## Usage (macro): ./glutil -m tvrage --arg1=/path/to/shows [--arg2=<path filter>]                 					(filesystem based)
##                ./glutil -m tvrage-d --arg1 '\/tv\-((sd|hd|)x264|xvid|bluray|dvdr(ip|))\/.*\-[a-zA-Z0-9\-_]+$'   	(dirlog based)
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
###########################[ BEGIN OPTIONS ]#############################
#
# tvrage services base url
TVRAGE_URL="http://services.tvrage.com"
#
#INPUT_SKIP=""
#INPUT_CLEAN_REGEX=""
#
## Updates tvlog
UPDATE_TVLOG=1
#
## Set to 0, tvlog directory path fields are set to glroot  
##  relative path
## Set to 1, tvlog directory path fields are set to exact 
##  query that was made
## Existing records are always overwritten, except if DENY_IMDBID_DUPE=1
TVRAGE_DATABASE_TYPE=1
#
## If set to 1, do not import records with same
## showID already in the database
DENY_TVID_DUPE=1
#
## Overwrite existing matched record, when it's atleast 
##  this old (days) (when DENY_TVID_DUPE=1)
RECORD_MAX_AGE=1
#
## Work with unique database for each type
TYPE_SPECIFIC_DB=0
#
## Extract year from release string and apply to searches
TVRAGE_SEARCH_BY_YEAR=1
#
VERBOSE=0
############################[ END OPTIONS ]##############################

CURL="/usr/bin/curl"
CURL_FLAGS="--silent --max-time 30"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

# recode binary (optional)
RECODE="recode"

BASEDIR=`dirname $0`

[ $TYPE_SPECIFIC_DB -eq 1 ] && [ $TVRAGE_DATABASE_TYPE -gt 0 ] && LAPPEND="$TVRAGE_DATABASE_TYPE"

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

[ $7 -eq 1 ] && [ $TVRAGE_DATABASE_TYPE -eq 1 ] && TD=`basename "$1"` || TD="$1"

echo "$TD" | egrep -q -i "$INPUT_SKIP" && exit 1

if ! [ $7 -eq 2 ]; then
	QUERY=`echo "$TD" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r 's/[\.\_\-\(\)]/+/g' | sed -r 's/(^[+ ]+)|([+ ]+$)//g'`
else
	QUERY=$TD	
fi

[ -z "$QUERY" ] && exit 1

cad() {
	RTIME=`$1 --tvlog "$4$LAPPEND" -h $2 "$3" --imatchq -exec "echo {time}" --silent`
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

extract_year() {
	echo "$1" | egrep -o "[_\-\(\)\.\+\ ]([1][98][0-9]{2,2}|[2][0][0-9]{2,2})([_\-\(\)\.\+\ ]|())" | head -1 | sed -r "s/[_\-\(\)\.\+\ ]//g"
}

[ $TVRAGE_SEARCH_BY_YEAR -eq 1 ] && {
	YEAR_q=`extract_year "$TD"`
	[ -n "$YEAR_q" ] && YQ_O='+'$YEAR_q
}

[ $VERBOSE -gt 1 ] && echo "NOTICE: query: $QUERY: $TD"

if [ $7 -eq 2 ]; then
	[ -z "$8" ] && {
		echo "ERROR: missing show id"
		exit 1
	}
	if [ $UPDATE_TVLOG -eq 1 ] && [ $DENY_TVID_DUPE -eq 1 ]; then
		cad $2 "--iregex" "showid,^$8$" "$3"	
	fi
	DDT=`$CURL $CURL_FLAGS "$TVRAGE_URL""/feeds/showinfo.php?sid=""$8"`
	SFIELD="/Showinfo"
	SLINK="showlink"
	SCOUNTRY="origin_country"
	SNAME="showname"
	SDATE="startdate"
	
else
	DDT=`$CURL $CURL_FLAGS "$TVRAGE_URL""/feeds/full_search.php?show=""$QUERY""$YQ_O"`
	SFIELD="/Results//show"
	SLINK="link"
	SNAME="name"
	SCOUNTRY="country"
	SDATE="started"
fi

echo "$DDT" | egrep -q "exceeded[a-zA-Z\' ]*max_user_connections" && {
	[ $VERBOSE -gt 0 ] && echo "$DDT - retrying.."
	sleep 2
	$0 "$1" "$2" "$3" "$4" "$5" $6 $7 $8
	exit $?
}

[ -z "$DDT" ] && echo "ERROR: $QUERY: $TD: unable to get show data $TVRAGE_URL""/feeds/full_search.php?show=$QUERY""$YQ_O" && exit 1

get_field()
{
	echo "$DDT" | $XMLLINT --xpath "(($SFIELD)[1]/""$1"")" -  | sed -r "s/<[\/a-zA-Z0-9]+>//g"
}

get_field_t()
{
	echo "$DDT" | $XMLLINT --xpath "(($SFIELD)[1]/""$1"")" - | sed -r "s/<[\/a-zA-Z0-9]+>/,/g" | sed -r "s/(^[,]+)|([,]+$)//g" | sed -r "s/[,]{2,}/,/g"
}

! [ $7 -eq 2 ] &&
	SHOWID=`get_field showid` ||
	SHOWID=$8

if [ -z "$SHOWID" ]; then 
	echo "ERROR: $QUERY: $TD: could not get show id: $TVRAGE_URL""/feeds/full_search.php?show=$QUERY""$YQ_O"
	[ $VERBOSE -gt 0 ] && echo "$DDT"
	exit 1
fi

if ! [ $7 -eq 2 ] && [ $UPDATE_TVLOG -eq 1 ] && [ $DENY_TVID_DUPE -eq 1 ]; then
	cad $2 "--iregex" "showid,^$SHOWID$" "$3"	
fi

adjust_tc() {
	ZZ=`get_field $1`
	tc=`echo "$ZZ" | tr '/' ' ' | wc -w`
	if [ $tc -eq 2 ]; then
		echo "1 $ZZ"
	elif [ $tc -eq 1 ]; then
		echo "1 Jan $ZZ"
	else
		echo "$ZZ"
	fi
}

NAME=`get_field $SNAME`
GENRES=`get_field_t '/genres//genre[.]'`

$RECODE --version 2&> /dev/null && {
	NAME=`echo $NAME | $RECODE -f HTML_4.0`
	GENRES=`echo $GENRES | $RECODE -f HTML_4.0`
	
}

[ -z "$GENRES" ] && GENRES="N/A"

STATUS=`get_field status`
[ -z "$STATUS" ] && STATUS="N/A"
COUNTRY=`get_field "$SCOUNTRY"`
[ -z "$COUNTRY" ] && COUNTRY="N/A"
SEASONS=`get_field seasons`
[ -z "$SEASONS" ] && SEASONS=0
CLASS=`get_field classification`
[ -z "$CLASS" ] && CLASS="N/A"
AIRTIME=`get_field airtime`
[ -z "$AIRTIME" ] && AIRTIME="N/A"
AIRDAY=`get_field airday`
[ -z "$AIRDAY" ] && AIRDAY="N/A"
RUNTIME=`get_field runtime`
[ -z "$RUNTIME" ] && RUNTIME=0
LINK=`get_field "$SLINK"`
[ -z "$LINK" ] && LINK="N/A"
ZZ=`adjust_tc started`
[ -n "$ZZ" ] && SDATE=`date --date="$(echo $ZZ | tr '/' ' ')" +"%s"` || STARTED=0
ZZ=`adjust_tc ended`
[ -n "$ZZ" ] && ENDED=`date --date="$(echo $ZZ | tr '/' ' ')" +"%s"` || ENDED=0

if [ $UPDATE_TVLOG -eq 1 ]; then
	trap "rm /tmp/glutil.img.$$.tmp; exit 2" 2 15 9 6
	if [ $TVRAGE_DATABASE_TYPE -eq 0 ]; then
		GLR_E=`echo $4 | sed 's/\//\\\//g'`	
		DIR_E=`echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//"`  
		$2 --tvlog="$3$LAPPEND" -h --iregex "$DIR_E" --imatchq -v > /dev/null || $2 -f --tvlog="$3$LAPPEND" -e tvrage --regex "$DIR_E" > /dev/null || { 
			echo "ERROR: $DIR_E: Failed removing old record" && exit 1 
		}
	elif [ $TVRAGE_DATABASE_TYPE -eq 1 ]; then		
		DIR_E=$QUERY
		$2 --tvlog="$3$LAPPEND" -h --iregex showid,"^$SHOWID$" --imatchq > /dev/null || $2 -f --tvlog="$3$LAPPEND" -e tvrage --regex showid,"^$SHOWID$" > /dev/null || {
			echo "ERROR: $SHOWID: Failed removing old record" && exit 1 
		}
	fi	
	
	echo -en "dir $DIR_E\ntime `date +%s`\nshowid $SHOWID\nclass $CLASS\nname $NAME\nstatus $STATUS\ncountry $COUNTRY\nseasons $SEASONS\nairtime $AIRTIME\nairday $AIRDAY\nruntime $RUNTIME\nlink $LINK\nstarted $STARTED\nended $ENDED\ngenre $GENRES\n\n" > /tmp/glutil.img.$$.tmp
	$2 --tvlog="$3$LAPPEND" -z tvrage --nobackup --silent < /tmp/glutil.img.$$.tmp || echo "ERROR: $QUERY: $TD: failed writing to tvlog [$3$LAPPEND]"
	rm /tmp/glutil.img.$$.tmp
fi

echo "TVRAGE: `echo "Q:'$QUERY' | A:'$NAME'" | tr '+' ' '` : $TD : $LINK : $CLASS | $GENRES"

exit 0
