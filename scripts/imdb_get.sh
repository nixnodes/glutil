#!/bin/bash
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:2
#@REVISION:4
#@MACRO:imdb:{m:exe} -x {m:arg1} --silent --dir --execv `{m:spec1} {basepath} {exe} {imdbfile} {glroot} {siterootn} {path} 0` {m:arg2}
#@MACRO:imdb-d:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:exe} -v --backup imdb" -execv "{m:spec1} {basedir} {exe} {imdbfile} {glroot} {siterootn} {dir} 0" --iregexi "dir,{m:arg1}" 
#@MACRO:imdb-su:{m:exe} -a --silent -v --loglevel=5 --preexec "{m:exe} -v --backup imdb" -execv "{m:spec1} {dir} {exe} {imdbfile} {glroot} {siterootn} {dir} 1 {year}" 
#@MACRO:imdb-su-id:{m:exe} -a --silent -v --loglevel=5 --preexec "{m:exe} -v --backup imdb" -execv "{m:spec1} {imdbid} {exe} {imdbfile} {glroot} {siterootn} {dir} 2 {basedir} {year}" 
#@MACRO:imdb-su-f1:{m:exe} -a --silent -v --loglevel=5 --preexec "{m:exe} -v --backup imdb" -execv "{m:spec1} {dir} {exe} {imdbfile} {glroot} {siterootn} {dir} 1" iregex "dir,\/"
#@MACRO:imdb-e:{m:exe} -d --silent -v --loglevel=5 --preexec "{m:spec1} '{m:arg1}' '{exe}' '{imdbfile}' '{glroot}' '{siterootn}' 0 0"
#
## Gets movie info using iMDB native API and omdbapi (XML)
#
## Requires: - glutil-1.6 or above
##           - libxml2 v2.7.7 or above 
##           - curl, date, egrep, sed, expr
#
## Tries to find ID using iMDB native API first - in case of failure, omdbapi search is used
#
## Usage (macro): glutil -m imdb --arg1=/path/to/movies [--arg2=<path filter>]        		(filesystem based)
##                glutil -m imdb-d --arg1 '\/(x264|xvid|movies)\/.*\-[a-zA-Z0-9\-_]*$'      (dirlog based)
##                glutil -m imdb-e -arg1=<query>                                            (single release)
##                glutil -m imdb-su                                                         (update existing records, pass query/dir name through the search engine)
##                glutil -m imdb-su-id                                                      (update records using existing imdbID's, no searching is done)
#
##  To use these macros, place script in the same directory (or any subdirectory) where glutil is located
#
###########################[ BEGIN OPTIONS ]#############################
#
# omdbapi base url
IMDB_URL="http://www.omdbapi.com/"
#
# iMDB base url
IMDBURL="http://www.imdb.com/"
#
## Define here if not using config file
#INPUT_SKIP="^(.* complete .*|sample|subs|no-nfo|incomplete|covers|cover|proof|cd[0-9]{1,3}|dvd[0-9]{1,3}|nuked\-.*|.* incomplete .*|.* no-nfo .*)$"
#INPUT_CLEAN_REGEX="([._-\(\)][1-2][0-9]{3,3}|())([._-\(\)](VOBSUBS|SUBPACK|BOXSET|FESTIVAL|(720|1080)[ip]|RERIP|UNRATED|DVDSCR|TC|TS|CAM|EXTENDED|TELESYNC|DVDR|X264|HDTV|SDTV|PDTV|XXX|WORKPRINT|SUBBED|DUBBED|DOCU|THEATRICAL|RETAIL|SUBFIX|NFOFIX|DVDRIP|HDRIP|BRRIP|BDRIP|LIMITED|PROPER|REPACK|XVID)([._-\(\)]|$).*)|-([A-Z0-9a-z_-]*$)"
#
## If set to 1, might cause mis-matches 
## Only runs if exact/omdbapi matches fails
LOOSE_SEARCH=1
#
## Updates imdblog
UPDATE_IMDBLOG=1
#
## Set to 0, imdblog directory path fields are set to glroot  
##  relative path
## Set to 1, imdblog directory path fields are set to exact 
##  query that was made
## Existing records are always overwritten, except if DENY_IMDBID_DUPE=1
IMDB_DATABASE_TYPE=1
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
## Allowed types regular expression (per omdbapi)
OMDB_ALLOWED_TYPES="movie|N\/A"
#
## Extract year from release string and apply to searches
IMDB_SEARCH_BY_YEAR=1
#
############################[ END OPTIONS ]##############################

CURL="/usr/bin/curl"
CURL_FLAGS="--silent"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

BASEDIR=`dirname $0`

[ $TYPE_SPECIFIC_DB -eq 1 ] && [ $IMDB_DATABASE_TYPE -gt 0 ] && LAPPEND="$IMDB_DATABASE_TYPE"

[ -f "$BASEDIR/config" ] && . $BASEDIR/config

[[ $7 -eq 1 ]] && [[ $IMDB_DATABASE_TYPE -eq 1 ]] && TD=`basename "$1"` || TD="$1"

imdb_do_query() {
	$CURL $CURL_FLAGS "$IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$1"
}

imdb_search()
{
	echo "$1" | $XMLLINT --xpath "((/IMDbResults//ImdbEntity)[1]/@id)" - 2> /dev/null | sed -r 's/(id\=)|( )|[\"]//g'
}

omdb_search()
{
	$CURL $CURL_FLAGS "$IMDB_URL?r=xml&s=$1""$YQ_O" | $XMLLINT --xpath "((/root/Movie)[1]/@imdbID)" - 2> /dev/null | sed -r 's/(imdbID\=)|(\s)|[\"]//g'
}

get_omdbapi_data() {
	$CURL $CURL_FLAGS "$IMDB_URL""?r=XML&i=$1"
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

get_field()
{
	echo $DDT | $XMLLINT --xpath "((/root/movie)[1]/@$1)" - 2> /dev/null | sed -r "s/($1\=)|(^[ ]+)|([ ]+$)|[\"]//g" 
}


if ! [[ $7 -eq 2 ]]; then	
	echo "$TD" | egrep -q -i "$INPUT_SKIP" && exit 1
	
	QUERY=`echo "$TD" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r "s/[\.\_\-\(\)]/+/g" | sed -r "s/(^[+ ]+)|([+ ]+\$)//g"`
	
	[ -z "$QUERY" ] && exit 1
	
	extract_year() {
		echo "$1" | egrep -o "[_\-\(\)\.\+\ ]([1][98][0-9]{2,2}|[2][0][0-9]{2,2})([_\-\(\)\.\+\ ]|())" | tail -1 | sed -r "s/[_\-\(\)\.\+\ ]//g"
	}

	[ $IMDB_SEARCH_BY_YEAR -eq 1 ] && {
		if [[ $7 -eq 1 ]] && [[ $IMDB_DATABASE_TYPE -eq 1 ]]; then
			DENY_IMDBID_DUPE=0
			YEAR_q="$8"
		else
			YEAR_q=`extract_year "$TD"`
			[ -n "$YEAR_q" ] && YQ_O='&y='$YEAR_q
		fi
	}

	if [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
		s_q=`echo "$QUERY" | sed 's/\+/\\\\\0/g'`
		cad $2 "--iregexi" "dir,^$s_q\$" "$3"
	fi
	
	DTMP=`imdb_do_query "$QUERY""&ex=1"` 
	
	imdb_get_by_year() {
		echo "$1" | xmllint --xpath "(((/IMDbResults//ImdbEntity)))" - 2> /dev/null | sed -r "s/<\/ImdbEntity>/\0\\n/g" | egrep "<Description>$2" | tr -d '\n'
	}
	
	unset iid
	YR_F=0
	
	[ -n "$YEAR_q" ] && {
		DTMP_t=`imdb_get_by_year "$DTMP" $YEAR_q`	
		[ -n "$DTMP_t" ] && DTMP="<IMDbResults>""$DTMP_t""</IMDbResults>" || YR_F=1
	}
	
	if [ $YR_F -eq 1 ] && [ $LOOSE_SEARCH -eq 0 ]; then
		echo "WARNING: $QUERY ($YEAR_q): $TD: could not find by year, ignoring match.."
	else
		iid=`imdb_search "$DTMP"`
	fi
	
	
	SMODE=0
	S_OMDB=0
	if [ -n "$iid" ] ; then
		IS_NAME=`echo "$DTMP" | $XMLLINT --xpath "((/IMDbResults//ImdbEntity)[1])" - 2> /dev/null |  sed -r 's/<[^>]+>//'| sed -r 's/<[^>]+>.*//' | sed -r 's/(^[ ]+)|([ ]+$)//g'`
		SMODE=1
	else
		echo "WARNING: $QUERY ($YEAR_q): $TD: $IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$QUERY : iMDB search failed, falling back to omdbapi.." 
		iid=`omdb_search "$QUERY"`
		S_OMDB=1
	fi
	
	S_LOOSE=0
	
	[ $LOOSE_SEARCH -eq 1 ] && [ -z "$iid" ] && {
		echo "WARNING: $QUERY ($YEAR_q): $TD: omdbapi query failed, performing loose iMDB search.."
		DTMP=`imdb_do_query "$QUERY"`
		
		[ -n "$YEAR_q" ] && {
				DTMP_t=`imdb_get_by_year "$DTMP" $YEAR_q`			
				[ -n "$DTMP_t" ] && DTMP="<IMDbResults>$DTMP_t</IMDbResults>" || {
					echo "ERROR: $QUERY: $TD: could not find any object released in $YEAR_q, aborting.." 
					exit 1
				}
		}		
		iid=`imdb_search "$DTMP"`
		SMODE=1
		S_LOOSE=1
	}
	
	[ -z "$iid" ] && echo "ERROR: $QUERY ($YEAR_q): $TD: cannot find record [$IMDB_URL?r=xml&s=$QUERY]" && exit 1
	
	if [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
		cad $2 "--iregex" "imdbid,^$iid$" "$3"	
	fi
	
	DDT=`get_omdbapi_data "$iid"`
	
	[ -z "$DDT" ] && echo "ERROR: $QUERY ($YEAR_q): $TD: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1
	
		
	TYPE=`get_field type`
	
	if ! echo $TYPE | egrep -q "$OMDB_ALLOWED_TYPES"; then
		if [ $S_OMDB -eq 0 ]; then
			echo "WARNING: $QUERY ($YEAR_q): $TD: invalid match (type is $TYPE), trying omdbapi.."
			iid=`omdb_search "$QUERY"`
			[ -z "$iid" ] && 	
				echo "WARNING: $QUERY ($YEAR_q): $TD: cannot find record using omdbapi search [$IMDB_URL?r=xml&s=$QUERY""$YQ_O""]" && exit 1		
			DDT=`get_omdbapi_data "$iid"`
			[ -z "$DDT" ] && echo "ERROR: $QUERY ($YEAR_q): $TD: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1
			TITLE=`get_field title`
			TYPE=`get_field type`
		elif [ $LOOSE_SEARCH -eq 1 ] && [ $S_LOOSE -eq 0 ] ; then
			echo "WARNING: $QUERY ($YEAR_q): $TD: invalid match (type is $TYPE), trying loose search.."
			DTMP=`imdb_do_query "$QUERY"`
			
			[ -n "$YEAR_q" ] && {
					DTMP_t=`imdb_get_by_year "$DTMP" $YEAR_q`			
					[ -n "$DTMP_t" ] && DTMP="<IMDbResults>$DTMP_t</IMDbResults>" || {
						echo "ERROR: $QUERY: $TD: could not find any object released in $YEAR_q, aborting.." 
						exit 1
					}
			}		
			iid=`imdb_search "$DTMP"`
			[ -z "$iid" ] && 	
				echo "WARNING: $QUERY ($YEAR_q): $TD: cannot find record using iMDB loose search [$IMDB_URL?r=xml&s=$QUERY""$YQ_O""]" && exit 1		
			DDT=`get_omdbapi_data "$iid"`
			[ -z "$DDT" ] && echo "ERROR: $QUERY ($YEAR_q): $TD: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1
			TITLE=`get_field title`
			TYPE=`get_field type`
		fi
	fi
	
	TITLE=`get_field title`
	
else
	iid="$1"
	QUERY="$8"
	YEAR_q="$9"
	
	DDT=`get_omdbapi_data "$iid"`
	
	[ -z "$DDT" ] && echo "ERROR: $1: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1
	
	TYPE=`get_field type`		
	TITLE=`get_field title`
fi

! echo $TYPE | egrep -q "$OMDB_ALLOWED_TYPES" && echo "ERROR: $QUERY: $TD: invalid match (type is $TYPE): $IMDB_URL""?r=XML&i=$iid" && exit 1

RATING=`get_field imdbRating`
GENRE=`get_field genre`
VOTES=`echo $(get_field imdbVotes) | tr -d ','`
YEAR=`get_field year | tr -d ' '`
RATED=`get_field rated`
ACTORS=`get_field actors`
DIRECTOR=`get_field director`
RELEASED=`date --date="$(D_g=$(get_field released); [ "$D_g" != "N/A" ] && echo "$D_g" || echo "")" +"%s"`
RUNTIME=`get_field runtime`
RUNTIME_h=`echo $RUNTIME | awk '{print $1}' | sed -r 's/[^0-9]+//g'`
RUNTIME_m=`echo $RUNTIME | awk '{print $3}' | sed -r 's/[^0-9]+//g'`
[ -z "$RUNTIME_m" ] && RUNTIME=$RUNTIME_h || RUNTIME=`expr $RUNTIME_h \* 60 + $RUNTIME_m`
[ -z "$TITLE" ] && [ -z "$RATING" ] && [ -z "$GENRE" ] && echo "ERROR: $QUERY: $TD: could not extract movie data" && exit 1

if [ $UPDATE_IMDBLOG -eq 1 ]; then
	trap "rm /tmp/glutil.img.$$.tmp; exit 2" 2 15 9 6
	if [ $IMDB_DATABASE_TYPE -eq 0 ]; then
		GLR_E=`echo $4 | sed 's/\//\\\//g'`	
		DIR_E=`echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//"`  
		$2 --imdblog="$3$LAPPEND" -a --iregex "$DIR_E" --imatchq -v > /dev/null || $2 -f --imdblog="$3$LAPPEND" --nobackup -e imdb --regex "$DIR_E" > /dev/null || { 
			echo "ERROR: $DIR_E: Failed removing old record" && exit 1 
		}
	elif [ $IMDB_DATABASE_TYPE -eq 1 ]; then
		#[ -z "$TITLE" ] && echo "ERROR: $QUERY: $TD: failed extracting movie title" && exit 1
		DIR_E=$QUERY		
		$2 --imdblog="$3$LAPPEND" -a --iregex imdbid,"^$iid$" --imatchq > /dev/null || $2 -f --imdblog="$3$LAPPEND" --nobackup -e imdb --regex imdbid,"^$iid$" > /dev/null || {
			echo "ERROR: $iid: Failed removing old record" && exit 1 
		}
	fi	

	echo -en "dir $DIR_E\ntime `date +%s`\nimdbid $iid\nscore $RATING\ngenre $GENRE\nvotes $VOTES\ntitle $TITLE\nactors $ACTORS\nrated $RATED\nyear $YEAR\nreleased $RELEASED\nruntime $RUNTIME\ndirector $DIRECTOR\n\n" > /tmp/glutil.img.$$.tmp
	$2 --imdblog="$3$LAPPEND" -z imdb --nobackup --silent < /tmp/glutil.img.$$.tmp || echo "ERROR: $QUERY: $TD: failed writing to imdblog!!"
	rm /tmp/glutil.img.$$.tmp
fi

echo "IMDB: `echo "Q:'$QUERY ($YEAR_q)' | A:'$TITLE ($YEAR)'" | tr '+' ' '` : $IMDBURL""title/$iid : $RATING $VOTES $GENRE"

exit 0
