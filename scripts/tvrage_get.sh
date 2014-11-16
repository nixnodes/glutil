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
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:3
#@REVISION:37
#@MACRO:tvrage|TVRage lookups based on folder names (filesystem) [-arg1=<path>] [-arg2=<path regex>]:{m:exe} -x {m:arg1} -lom "depth>0" --silent --dir --preexec "{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage" -execv `{m:spec1} {basepath} {exe} {tvragefile} {glroot} {siterootn} {path} 0 0 '' {m:arg3}` {m:arg2}
#@MACRO:tvrage-d|TVRage lookups based on folder names (dirlog) [-arg1=<regex filter>]:{m:exe} -d --silent --loglevel=1 --preexec "{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage" -execv `{m:spec1} {basedir} {exe} {tvragefile} {glroot} {siterootn} {dir} 0 0 '' {m:arg3}` regexi "dir,{m:arg1}"  {m:arg2} 
#@MACRO:tvrage-su|Update existing tvlog records, pass query/dir name through the search engine:{m:exe} -h --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec "{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage" -execv `{m:spec1} {basedir} {exe} {tvragefile} {glroot} {siterootn} {dir} 1`
#@MACRO:tvrage-su-id|Rebuild entire tvlog based on showid fields:{m:exe} -h --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec "{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage" -execv `{m:spec1} {basedir} {exe} {tvragefile} {glroot} {siterootn} {dir} 2 {showid}`
#@MACRO:tvrage-e|Lookup/process using a specific TVRage query (by show name) [-arg1=<showname>]:{m:exe} noop --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec "{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage; {m:spec1} '{m:arg1}' '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' '{m:arg2}' 0 '' '' {m:arg3}"
#@MACRO:tvrage-e-id|Lookup/process using a specific TVRage query (by show id) [-arg1=<showid>]:{m:exe} noop --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec "{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage; {m:spec1} '-' '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' 0 2 '{m:arg1}'"
#@MACRO:tvrage-e-full|Retrieve a full show list from TVRage and build/update log based on returned showids:{m:exe} noop --tvlog={m:q:tvrage@file} --silent --loglevel=1 --preexec `{m:exe} --tvlog={m:q:tvrage@file} --backup tvrage; for i in $(curl http://services.tvrage.com/feeds/show_list.php | sed -r 's/<show><id>|<\/id><name>.*|<(\/|())shows>|<\?xml.*>//g' | sed -r '/^$/d'); do if echo "$i" | egrep -q '^[0-9]+$'; then {m:spec1} '-' '{exe}' '{tvragefile}' '{glroot}' '{siterootn}' 0 2 "$i"; else echo "ERROR: invalid id: '$i'"; fi; done`
#
## Install script dependencies + libs into glftpd root, preserving library paths (requires mlocate)
#
#@MACRO:tvrage-installch|Install required libraries into glFTPd root:{m:exe} noop --preexec `! updatedb -e "{glroot}" -o /tmp/glutil.mlocate.db && echo "updatedb failed" && exit 1 ; li="/bin/curl /bin/xmllint /bin/date /bin/egrep /bin/sed /bin/expr /bin/rev /bin/cut /bin/recode"; for lli in $li; do lf=$(locate -d /tmp/glutil.mlocate.db "$lli" | head -1) && l=$(ldd "$lf" | awk '{print $3}' | grep -v ')' | sed '/^$/d' ) && for f in $l ; do [ -f "$f" ] && dn="/glftpd$(dirname $f)" && ! [ -d $dn ] && mkdir -p "$dn"; [ -f "{glroot}$f" ] || if cp --preserve=all "$f" "{glroot}$f"; then echo "$lf: {glroot}$f"; fi; done; [ -f "{glroot}/bin/$(basename "$lf")" ] || if cp --preserve=all "$lf" "{glroot}/bin/$(basename "$lf")"; then echo "{glroot}/bin/$(basename "$lf")"; fi; done; rm -f /tmp/glutil.mlocate.db`
#
## Gets show info using TVRAGE API (XML)
#
## Requires: - glutil-2.4.13-2 or above
##			 - libxml2 v2.7.7 or above
##           - curl, date, egrep, sed, expr, rev, cut, recode (optional)
#
## Usage (macro): -m tvrage --arg1=/path/to/shows [--arg2=<path filter>]                 					(filesystem based)
##                -m tvrage-d --arg1 '\/tv\-((sd|hd|)x264|xvid|bluray|dvdr(ip|))\/.*\-[a-zA-Z0-9\-_]+$'   	(dirlog based)
##                -m tvrage-su-id                                                                           (rebuild entire log based on showid fields)
##                -m tvrage-e -arg1="<showname>"                                                            (lookup/process using a specific query (by show name))
##                -m tvrage-e-id -arg1=<showid>                                                             (lookup/process using a specific query (by show id))
##                -m tvrage-e-full                                                                          (retrieve a full show list and build/update log based on returned showids)
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
##  Keeping a local cache not only provides much faster lookups for you, but helps keep the public (free) 
##  TVRage services API unencumbered.. Performing web lookups each time you need the info, then discarding
##  that data the moment you're done with it, is extremely wastefull. 
##  In this repository you can find a pre-built (full) gzipped database, avoid building one from scratch
##  yourself - only keep the pre-built one up to date.
##
##  For example, set RECORD_MAX_AGE=30 and run periodic updates (<tvrage-e-full> recommended) every so often. 
##  This would lookup and update records older than 30 days and only add content that doesn't already exist in 
##  the local db. Moreover, your glutil based database query mechanism should only perform a web query, when it 
##  can not find something in the local database. Note when using an up-to-date full local db cache, not getting 
##  results from a query probably means, that the show doesn't exist, or your query string was malformed.
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
RECORD_MAX_AGE=14
#
## Work with unique database for each type
TYPE_SPECIFIC_DB=0
#
## Extract year from release string and apply to searches
TVRAGE_SEARCH_BY_YEAR=1
#
## TVRage services api key (leave blank if you don't know
## what this is)
TVRAGE_API_KEY=""
#
## Log compression level
#
## Setting this to 0 or removing it disables
## compression
#
TVLOG_COMPRESSION=0
#
## Sort shows by date started after
## updating log
#
TVLOG_SORT=0
#
## (Re)load the log into shared memory segment
## after an update occurs
#
TVLOG_SHARED_MEM=0
#
VERBOSE=1
#
## Wipes given characters out from show name, before
## writing the log (regex)
## To disable this, comment out the below line
TITLE_WIPE_CHARS="\'\´\:\""
#
## Array of sed substitute options, each applied
## in the respective order to show name before it
## gets written
#
TITLE_REPLACE=("s/ \& / and /")
#
############################[ END OPTIONS ]##############################
############################[ BEGIN PATHS ]##############################

CURL="/usr/bin/curl"
CURL_FLAGS="--silent --max-time 30"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

# recode binary (optional), gets rid of HTML entities
RECODE="recode"

############################[ BEGIN PATHS ]##############################

title_regsub_string() {
	r0_out="${@}"
	
	for r0_i in "${TITLE_REPLACE[@]}"; do
		r0_out=`echo ${r0_out} | sed -r "${r0_i}"`
	done
	
	echo ${r0_out}
}

BASEDIR=`dirname $0`

GLROOT="${4}"

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

echo "${10}" | grep -q "3" && TVRAGE_DATABASE_TYPE=0

[ $TYPE_SPECIFIC_DB -eq 1 ] && [ $TVRAGE_DATABASE_TYPE -gt 0 ] && LAPPEND="$TVRAGE_DATABASE_TYPE"

[ -f "${BASEDIR}/common" ] || { 
	echo "ERROR: ${BASEDIR}/common missing"
	exit 2
}

. ${BASEDIR}/common

op_id=0

echo "${10}" | grep -q "2" && { 
	OUT_PRINT=0
	op_id=1
}

try_lock_r 12 tvr_lk "`echo "${3}${LAPPEND}" | md5sum | cut -d' ' -f1`" 120 "ERROR: could not obtain lock"

IS_COMP=`${2} --preexec "echo -n {?q:tvrage@comp}" noop --tvlog="${3}${LAPPEND}"`

[ -n "${IS_COMP}" ] && [ ${IS_COMP} -eq 1 ] && {	
	[ -z "${TVLOG_COMPRESSION}" ] && {
		TVLOG_COMPRESSION=2
	} || {
		[ ${TVLOG_COMPRESSION} -gt 0  ] || {
			TVLOG_COMPRESSION=2
		}
	}
}

[ -n "${TVLOG_COMPRESSION}" ] && [ ${TVLOG_COMPRESSION} -gt 0 ] && 
	[ ${TVLOG_COMPRESSION} -lt 10 ]  && {
		EXTRA_ARGS="--gz ${TVLOG_COMPRESSION}"
	}

[ $7 -eq 1 ] && [ $TVRAGE_DATABASE_TYPE -eq 1 ] && TD=`basename "$1"` || TD="$1"

echo "$TD" | egrep -q -i "$INPUT_SKIP" && exit 1

if ! [ $7 -eq 2 ]; then
	QUERY=`echo "$TD" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r 's/[\.\_\-\(\)]/+/g' | sed -r 's/(^[+ ]+)|([+ ]+$)//g'`
else
	QUERY=$TD	
fi

[ -z "$QUERY" ] && exit 1

cad() {
	RTIME=`$1 --tvlog "$4$LAPPEND" -h $2 "$3" --imatchq -printf "{time}" --silent --nobuffer --rev`
	CTIME=`date +"%s"`
	[ -n "$RTIME" ] && DIFF1=`expr $CTIME - $RTIME` && DIFF=`expr $DIFF1 / 86400`
	if [ $RECORD_MAX_AGE -gt 0 ] && [ -n "$DIFF" ] && [ $DIFF -ge $RECORD_MAX_AGE ]; then
	 	print_str "NOTICE: $QUERY: $SHOWID: Record too old ($DIFF days) updating.."
	else
		if [ -n "$RTIME" ]; then
			[ $VERBOSE -gt 0 ] && print_str "WARNING: $QUERY: [$2 $3]: already exists in database (`expr $DIFF1 / 60` min old)"
			II_ID=`${1} --tvlog "$4$LAPPEND" -h $2 "$3" --imatchq -printf "{showid}" --silent --nobuffer --rev`
            [ ${op_id} -eq 1 ] && echo "${II_ID}"
			exit 0
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

if [ -n "$TVRAGE_API_KEY" ]; then 
	q_TVR_KEY="key=""$TVRAGE_API_KEY""&"
	q_FEEDS="myfeeds"
else
	q_FEEDS="feeds"
fi

if [ $7 -eq 2 ]; then
	[ -z "$8" ] && {
		print_str "ERROR: missing show id"
		exit 1
	}
	SHOWID=${8}
	if [ $UPDATE_TVLOG -eq 1 ] && [ $DENY_TVID_DUPE -eq 1 ]; then
		cad $2 "lom" "showid=${8}" "$3"	
	fi
	
	[ $VERBOSE -gt 1 ] && print_str "NOTICE: query: $QUERY: $TD - ""$TVRAGE_URL""/""$q_FEEDS""/showinfo.php?""$q_TVR_KEY""sid=""$8"
	
	DDT=`$CURL $CURL_FLAGS "$TVRAGE_URL""/""$q_FEEDS""/showinfo.php?""$q_TVR_KEY""sid=""$8"`
	SFIELD="/Showinfo"
	SLINK="showlink"
	SCOUNTRY="origin_country"
	SNAME="showname"
	SDATE="startdate"
else
	[ $VERBOSE -gt 1 ] && print_str "NOTICE: query: $QUERY: $TD - ""$TVRAGE_URL""/""$q_FEEDS""/full_search.php?""$q_TVR_KEY""show=""$QUERY""$YQ_O"
	DDT=`$CURL $CURL_FLAGS "$TVRAGE_URL""/feeds/full_search.php?""$q_TVR_KEY""show=""$QUERY""$YQ_O"`
	SFIELD="/Results//show"
	SLINK="link"
	SNAME="name"
	SCOUNTRY="country"
	SDATE="started"
fi

echo "$DDT" | egrep -q "exceeded[a-zA-Z\' ]*max_user_connections" && {
	[ $VERBOSE -gt 0 ] && print_str "$DDT - retrying.."
	sleep 2
	$0 "$1" "$2" "$3" "$4" "$5" $6 $7 $8
	exit $?
}

echo "$DDT" | egrep -q "^Invalid" && {
	print_str "$DDT - aborting.."
	exit 1
}

[ -z "$DDT" ] && print_str "ERROR: $QUERY: $TD: unable to get show data (empty) - $8" && exit 1

get_field()
{
	echo "$DDT" | $XMLLINT --xpath "(($SFIELD)[1]/""$1"")" - 2> /dev/null | sed -r "s/<[\/a-zA-Z0-9\_\ \=\"]+>//g"
}

get_field_t()
{
	echo "$DDT" | $XMLLINT --xpath "(($SFIELD)[1]/""$1"")" - 2> /dev/null | sed -r "s/<[\/a-zA-Z0-9\_\ \=\"]+>/,/g" | sed -r "s/(^[,]+)|([,]+$)//g" | sed -r "s/[,]{2,}/,/g"
}

! [ $7 -eq 2 ] &&
	SHOWID=`get_field showid` ||
	SHOWID=$8

if [ -z "$SHOWID" ]; then 
	print_str "ERROR: $QUERY: $TD: could not get show id"
	[ $VERBOSE -gt 0 ] && print_str "$DDT"
	exit 1
fi

if ! [ $7 -eq 2 ] && [ $UPDATE_TVLOG -eq 1 ] && [ $DENY_TVID_DUPE -eq 1 ]; then
	cad $2 "lom" "showid=${SHOWID}" "$3"	
fi

adjust_tc() {
	ZZtc=`echo "$1" | tr '/' ' '`
	tc=`echo "$ZZtc" | wc -w`
	if [ $tc -eq 2 ]; then
		echo "$ZZtc" | sed -r "s/[ ]+/ 1 /"
	elif [ $tc -eq 1 ]; then
		echo "Jan 1 $ZZtc"
	else
		echo "$ZZtc"
	fi
}

NAME=`get_field $SNAME`
[ -n "$TITLE_WIPE_CHARS" ] && NAME=`echo "$NAME" | sed -r "s/[${TITLE_WIPE_CHARS}]+//g"`
[ -z "$NAME" ] && print_str "ERROR: $QUERY: $TD: could not extract show name, fatal.." && exit 1

GENRES=`get_field_t '/genres//genre[.]'`
COUNTRY=`get_field "$SCOUNTRY"`
[ -z "$COUNTRY" ] && COUNTRY="N/A"
NETWORK=`get_field "network[@country='$COUNTRY']"`

$RECODE --version 2&> /dev/null && {
	NAME=`echo $NAME | $RECODE -f HTML_4.0`
	GENRES=`echo $GENRES | $RECODE -f HTML_4.0`
	NETWORK=`echo $NETWORK | $RECODE -f HTML_4.0`
}

[ -n "${TITLE_REPLACE}" ] && {
	NAME=`title_regsub_string ${NAME}`
}

[ -z "$GENRES" ] && GENRES="N/A"
[ -z "$NETWORK" ] && NETWORK="N/A"

STATUS=`get_field status`
[ -z "$STATUS" ] && STATUS="N/A"
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
ZZ_SD=`get_field started`
ZZ_ED=`get_field ended`
ZZ=`adjust_tc "$ZZ_SD"`
[ -n "$ZZ" ] && STARTED=`date --date="$ZZ" +"%s"` || STARTED=0
ZZ=`adjust_tc "$ZZ_ED"`
[ -n "$ZZ" ] && ENDED=`date --date="$ZZ" +"%s"` || ENDED=0
STARTYEAR=`echo "$ZZ_SD" | rev | cut -d "/" -f1 | rev`
[ -z "$STARTYEAR" ] && STARTYEAR=0
ENDYEAR=`echo "$ZZ_ED" | rev | cut -d "/" -f1 | rev`
[ -z "$ENDYEAR" ] && ENDYEAR=0


if [ $UPDATE_TVLOG -eq 1 ]; then
	trap "rm /tmp/glutil.img.$$.tmp; exit 2" 2 15 9 6
	if [ $TVRAGE_DATABASE_TYPE -eq 0 ]; then
		GLR_E=`echo $4 | sed 's/\//\\\\\//g'`	
		DIR_E=`echo $6 | sed "s/^$GLR_E//" `  

		[ -e "$3$LAPPEND" ] && {
	 		$2 -ff --nobackup --tvlog="$3$LAPPEND" -e tvrage ! regex "dir,${DIR_E}" --nofq --nostats --silent ${EXTRA_ARGS} || { 
				print_str "ERROR: $DIR_E: Failed removing old record"; exit 1 
			}
		}
		
	elif [ $TVRAGE_DATABASE_TYPE -eq 1 ]; then		
		DIR_E=$QUERY
		[ -e "$3$LAPPEND" ] && {
			$2 -ff --nobackup --tvlog="$3$LAPPEND" -e tvrage ! lom "showid=$SHOWID" --nofq --nostats --silent ${EXTRA_ARGS} || {
				print_str "ERROR: $SHOWID: Failed removing old record"; exit 1 
			}
		}
	fi	
	
	echo -en "dir $DIR_E\ntime `date +%s`\nshowid $SHOWID\nclass $CLASS\nname $NAME\nstatus $STATUS\ncountry $COUNTRY\nseasons $SEASONS\nairtime $AIRTIME\nairday $AIRDAY\nruntime $RUNTIME\nlink $LINK\nstarted $STARTED\nended $ENDED\ngenre $GENRES\nstartyear $STARTYEAR\nendyear $ENDYEAR\nnetwork $NETWORK\n\n" > /tmp/glutil.img.$$.tmp
	$2 --tvlog="$3$LAPPEND" -z tvrage --nobackup --nostats --silent ${EXTRA_ARGS} < /tmp/glutil.img.$$.tmp || print_str "ERROR: $QUERY: $TD: failed writing to tvlog [$3$LAPPEND]"
	rm /tmp/glutil.img.$$.tmp
	[ ${TVLOG_SORT} -gt 0 ] && ${2} -e tvrage --tvlog="$3$LAPPEND" --nobackup --nostats --silent --sort num,asc,started
	echo "${10}" | grep -q "1" || [ ${TVLOG_SHARED_MEM} -gt 0 ] && ${2} -q tvrage --tvlog="$3$LAPPEND" --shmem --shmdestroy --shmreload --loadq --silent --shmcflags 666
fi

[ ${VERBOSE} -eq 1 ] && print_str "TVRAGE: `echo "Q:'$QUERY' | A:'$NAME'" | tr '+' ' '` : $TD : $LINK : $CLASS | $GENRES"
[ ${op_id} -eq 1 ] && echo "${SHOWID}"

exit 0
