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
#@REVISION:11
#@MACRO:imdb|iMDB lookups based on folder names (filesystem) [-arg1=<path>] [-arg2=<path regex>]:{exe} -x {arg1} -lom "depth>0 && mode=4" --silent --sort asc,mtime --dir --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb" --execv `{spec1} \{basepath\} \{exe\} \{imdbfile\} \{glroot\} \{siterootn\} \{path\} 0 '' '' 3` {arg2}
#@MACRO:imdb-d|iMDB lookups based on folder names (dirlog) [-arg1=<regex filter>]:{exe} -d --silent --loglevel=1 --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb" -execv "{spec1} \{basedir\} \{exe\} \{imdbfile\} \{glroot\} \{siterootn\} \{dir\} 0 '' '' {arg3}" -l: dir -regexi "{arg1}" 
#@MACRO:imdb-su|Update existing imdblog records, pass query/dir name through the search engine:{exe} -a --imdblog={?q:imdb@file} --silent --loglevel=1 --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb" -execv "{spec1} \{dir\} \{exe\} \{imdbfile\} \{glroot\} \{siterootn\} \{dir\} 1 \{year\}" 
#@MACRO:imdb-su-id|Update imdblog records using existing imdbID's, no searching is done:{exe} -a --imdblog={?q:imdb@file} --silent --loglevel=1 --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb" -execv "{spec1} \{imdbid\} \{exe\} \{imdbfile\} \{glroot\} \{siterootn\} \{dir\} 2 \{basedir\} \{year\}" {arg2}
#@MACRO:imdb-su-f1:{exe} -a --imdblog={?q:imdb@file} --silent --loglevel=1 --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb" -execv "{spec1} \{dir\} \{exe\} \{imdbfile\} \{glroot\} \{siterootn\} \{dir\} 1" -l: dir -regex "\/"
#@MACRO:imdb-e-id|iMDB lookups based on -arg1 input (imdbID) [-arg1=<imdbid>]:{exe} -a --imdblog={?q:imdb@file} --silent --loglevel=1 --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb; {spec1} {arg1} \{exe\} \{imdbfile\} \{glroot\} \{siterootn\} '-' 2 '-' 0" 
#@MACRO:imdb-e|iMDB lookups based on -arg1 input [-arg1=<query>]:{exe} -noop  --imdblog={?q:imdb@file} --silent --loglevel=1 --preexec "{exe} --imdblog={?q:imdb@file} --backup imdb; {spec1} '{arg1}' '\{exe\}' '\{imdbfile\}' '\{glroot\}' '\{siterootn\}' '{arg2}' 0 '' '' {arg3}"
#@MACRO:imdb-purge-dead-paths|..:{exe} -e imdb --imdblog {?q:imdb@file} --nofq -l: "(?X:mode:(?Q:({glroot}\{dir\})))" -match 4 -vvvv
#
## Install script dependencies + libs into glftpd root (requires mlocate)
#
#@MACRO:imdb-installch|Install required libraries into glFTPd root:{exe} -noop  --preexec `! updatedb -e "\{glroot\}" -o /tmp/glutil.mlocate.db && echo "updatedb failed" && exit 1 ; li="/bin/perl /bin/curl /bin/xmllint /bin/date /bin/egrep /bin/sed /bin/expr /bin/recode /bin/awk"; for lli in $li; do lf=$(locate -d /tmp/glutil.mlocate.db "$lli" | head -1) && l=$(ldd "$lf" | awk '{print $3}' | grep -v ')' | sed '/^$/d' ) && for f in $l ; do [ -f "$f" ] && dn="/glftpd$(dirname $f)" && ! [ -d $dn ] && mkdir -p "$dn"; [ -f "\{glroot\}$f" ] || if cp --preserve=all "$f" "\{glroot\}$f"; then echo "$lf: \{glroot\}$f"; fi; done; [ -f "\{glroot\}/bin/$(basename "$lf")" ] || if cp --preserve=all "$lf" "\{glroot\}/bin/$(basename "$lf")"; then echo "\{glroot\}/bin/$(basename "$lf")"; fi; done; rm -f /tmp/glutil.mlocate.db`
#
## Gets movie info using iMDB native API and omdbapi (XML)
#
## Requires: - glutil-2.6.2 or above
##           - libxml2 v2.7.7 or above 
##           - curl, date, egrep, sed, expr, perl (optional), awk
#
## Tries to find ID using iMDB native API first - in case of failure, omdbapi search is used
#
## Usage (macro): glutil -m imdb -arg1=/path/to/movies [--arg2=<path filter>]              			(filesystem based)
##                glutil -m imdb-d -arg1 '\\/(x264|xvid|movies|dvdr|bluray)\\/.*\\-[a-zA-Z0-9\\-_]*$'	(dirlog based)
##                glutil -m imdb-e -arg1=<query>                                            		(single release)
##                glutil -m imdb-su                                                         		(update existing records, pass query/dir name through the search engine)
##                glutil -m imdb-su-id                                                      		(update records using existing imdbID's, no searching is done)
##                glutil -m imdb-e-id -arg1=<imdbID>												(update records by imdbID)
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
#INPUT_SKIP=""
#INPUT_CLEAN_REGEX=""
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
## Log compression level
#
## Setting this to 0 or removing it disables
## compression
#
IMDBLOG_COMPRESSION=0
#
## (Re)load the log into shared memory segment
## after an update occurs
#
IMDB_SHARED_MEM=0
#
## Verbose output
VERBOSE=1
#
## Allowed types regular expression (per omdbapi)
OMDB_ALLOWED_TYPES="movie|series|N\/A"
#
## Extract year from release string and apply to searches
IMDB_SEARCH_BY_YEAR=1
#
## Wipes given characters out from title, before
## writing the log (regex)
## To disable this, comment out the below line
IMDB_TITLE_WIPE_CHARS="\'\´\:\"\’"
#
############################[ END OPTIONS ]##############################

CURL="/usr/bin/curl"
CURL_FLAGS="--silent"

# libxml2 version 2.7.7 or above required
XMLLINT="/usr/bin/xmllint"

# recode binary (optional), gets rid of HTML entities
RECODE="perl"

BASEDIR=`dirname $0`

GLROOT="${4}"

[ -f "${BASEDIR}/config" ] && . ${BASEDIR}/config

echo "${10}" | grep -q "3" && IMDB_DATABASE_TYPE=0

[ $TYPE_SPECIFIC_DB -eq 1 ] && [ $IMDB_DATABASE_TYPE -gt 0 ] && LAPPEND="$IMDB_DATABASE_TYPE"

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

IS_COMP=`${2} --preexec "echo -n {?q:imdblog@comp}" -noop  --imdblog="${3}${LAPPEND}"`

[ -n "${IS_COMP}" ] && [ ${IS_COMP} -eq 1 ] && {	
	[ -z "${IMDBLOG_COMPRESSION}" ] && {
		IMDBLOG_COMPRESSION=2
	} || {
		[ ${IMDBLOG_COMPRESSION} -gt 0  ] || {
			IMDBLOG_COMPRESSION=2
		}
	}
}

[ -n "${IMDBLOG_COMPRESSION}" ] && [ ${IMDBLOG_COMPRESSION} -gt 0 ] && 
	[ ${IMDBLOG_COMPRESSION} -lt 10 ]  && {
		EXTRA_ARGS="--gz ${IMDBLOG_COMPRESSION}"
	}

[[ ${7} -eq 1 ]] && [[ $IMDB_DATABASE_TYPE -eq 1 ]] && TD=`basename "$1"` || TD="$1"

imdb_do_query() {
        $CURL $CURL_FLAGS "$IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$1"
}

imdb_search()
{
        echo "${1}" | ${XMLLINT} --xpath "((/IMDbResults//ImdbEntity)[1]/@id)" - 2> /dev/null | sed -r 's/(id\=)|[ ]*|[\"]//g'
}

omdb_search()
{
        $CURL $CURL_FLAGS "${IMDB_URL}?r=xml&s=${1}${YQ_O}" | $XMLLINT --xpath "((/root/Movie)[1]/@imdbID)" - 2> /dev/null | sed -r 's/(imdbID\=)|[ ]*|[\"]//g'
}

get_omdbapi_data() {
        $CURL $CURL_FLAGS "${IMDB_URL}?r=XML&i=${1}"
}

get_imdb_screens_count() {
		screens_c=`$CURL $CURL_FLAGS "${IMDBURL}/title/${1}/business" | egrep -o '\([,0-9]+ Screens\)' | sed -r 's/[ ()]|Screens|.$//g' | tr -d ',' | sed ':a;N;$!ba;s/\n/ + /g' | sed -r 's/ \+$//' | egrep  '[+ 0-9]' `
		[ -n "${screens_c}" ] && expr ${screens_c}
}

cad() {
        RTIME=`${1} --imdblog "${4}${LAPPEND}" -a -l: "${5}" ${2} "${3}" --imatchq -printf "{time}" --silent --nobuffer --rev`
        CTIME=`date +"%s"`
        [ -n "$RTIME" ] && DIFF1=`expr $CTIME - $RTIME` && DIFF=`expr $DIFF1 / 86400`
        if [ $RECORD_MAX_AGE -gt 0 ] && [ -n "$DIFF" ] && [ $DIFF -ge $RECORD_MAX_AGE ]; then
                print_str "NOTICE: $QUERY: $SHOWID: Record too old ($DIFF days) updating.."
        else
                if [ -n "$RTIME" ]; then
                        [ $VERBOSE -gt 0 ] && print_str "WARNING: $QUERY: [${2} ${3} (${5})]: already exists in database (`expr ${DIFF1} / 60` min old)"
                        II_ID=`$1 --imdblog "$4$LAPPEND" -a ${2} "${3}" --imatchq -printf "{imdbid}" --silent --nobuffer --rev`
                        [ ${op_id} -eq 1 ] && echo "${II_ID}"
                        exit 0
                fi
        fi
}

get_field()
{
        echo "$DDT" | $XMLLINT --xpath "((/root/movie)[1]/@$1)" - 2> /dev/null | sed -r "s/($1\=)|(^[ ]+)|([ ]+$)|[\"]//g"
}

if ! [ $7 -eq 2 ]; then
        echo "$TD" | egrep -q -i "$INPUT_SKIP" && exit 1

        QUERY=`echo "$TD" | tr ' ' '.' | sed -r "s/$INPUT_CLEAN_REGEX//gi" | sed -r "s/[\.\_\-\(\)]/+/g" | sed -r "s/(^[+ ]+)|([+ ]+\$)//g"`

        [ -z "$QUERY" ] && exit 1

        extract_year() {
                echo "$1" | egrep -o "[_\-\(\)\.\+\ ]([1][98][0-9]{2,2}|[2][0][0-9]{2,2})([_\-\(\)\.\+\ ]|())" | head -1 | sed -r "s/[_\-\(\)\.\+\ ]//g"
        }

        [ $IMDB_SEARCH_BY_YEAR -eq 1 ] && {
                if [ $7 -eq 1 ] && [ $IMDB_DATABASE_TYPE -eq 1 ]; then
                        DENY_IMDBID_DUPE=0
                        YEAR_q="$8"
                else
                        YEAR_q=`extract_year "$TD"`
                        [ -n "$YEAR_q" ] && YQ_O='&y='$YEAR_q
                fi
        }

#        if [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
#                s_q=`echo "$QUERY" | sed 's/\+/\\\\\0/g'`
#                cad ${2} "regexi" "^$s_q\$" "${3}" "dir"
#        fi

        DTMP=`imdb_do_query "$QUERY""&ex=1"`

        imdb_get_by_year() {
                echo "${1}" | xmllint --xpath "(((/IMDbResults//ImdbEntity)))" - 2> /dev/null | sed -r "s/<\/ImdbEntity>/\0\\n/g" | egrep "<Description>${2}" | tr -d '\n'
        }

        unset iid
        YR_F=0

        [ -n "$YEAR_q" ] && {
                DTMP_t=`imdb_get_by_year "$DTMP" $YEAR_q`
                [ -n "$DTMP_t" ] && DTMP="<IMDbResults>""$DTMP_t""</IMDbResults>" || YR_F=1
        }

        if [ $YR_F -eq 1 ] && [ $LOOSE_SEARCH -eq 0 ]; then
                print_str "WARNING: $QUERY ($YEAR_q): $TD: could not find by year, ignoring match.."
        else
                iid=`imdb_search "$DTMP"`
        fi


        SMODE=0
        S_OMDB=0
        if [ -n "$iid" ] ; then
                IS_NAME=`echo "$DTMP" | $XMLLINT --xpath "((/IMDbResults//ImdbEntity)[1])" - 2> /dev/null |  sed -r 's/<[^>]+>//'| sed -r 's/<[^>]+>.*//' | sed -r 's/(^[ ]+)|([ ]+$)//g'`
                SMODE=1
        else
                print_str "WARNING: $QUERY ($YEAR_q): $TD: $IMDBURL""xml/find?xml=1&nr=1&tt=on&q=$QUERY : iMDB search failed, falling back to omdbapi.." 
                iid=`omdb_search "$QUERY"`
                S_OMDB=1
        fi

        S_LOOSE=0

        [ $LOOSE_SEARCH -eq 1 ] && [ -z "$iid" ] && {
                print_str "WARNING: $QUERY ($YEAR_q): $TD: omdbapi query failed, performing loose iMDB search.."
                DTMP=`imdb_do_query "$QUERY"`

                [ -n "$YEAR_q" ] && {
                                DTMP_t=`imdb_get_by_year "$DTMP" $YEAR_q`
                                [ -n "$DTMP_t" ] && DTMP="<IMDbResults>$DTMP_t</IMDbResults>" || {
                                        print_str "ERROR: $QUERY: $TD: could not find any object released in $YEAR_q, aborting.." 
                                        exit 1
                                }
                }
                iid=`imdb_search "$DTMP"`
                SMODE=1
                S_LOOSE=1
        }

        [ -z "$iid" ] && print_str "ERROR: $QUERY ($YEAR_q): $TD: cannot find record [$IMDB_URL?r=xml&s=$QUERY]" && exit 1

        if [ ${IMDB_DATABASE_TYPE} -eq 1 ] && [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
                cad ${2} "-match" "$iid" "${3}" "imdbid"
        fi

        DDT=`get_omdbapi_data "$iid"`

        [ -z "$DDT" ] && print_str "ERROR: $QUERY ($YEAR_q): $TD: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1


        TYPE=`get_field type`

        if ! echo $TYPE | egrep -q "$OMDB_ALLOWED_TYPES"; then
                if [ $S_OMDB -eq 0 ]; then
                        print_str "WARNING: $QUERY ($YEAR_q): $TD: invalid match (type is $TYPE), trying omdbapi.."
                        iid=`omdb_search "$QUERY"`
                        [ -z "$iid" ] &&
                                print_str "WARNING: $QUERY ($YEAR_q): $TD: cannot find record using omdbapi search [$IMDB_URL?r=xml&s=$QUERY""$YQ_O""]" && exit 1         
                        DDT=`get_omdbapi_data "$iid"`
                        [ -z "$DDT" ] && print_str "ERROR: $QUERY ($YEAR_q): $TD: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1
                        if [ ${IMDB_DATABASE_TYPE} -eq 1 ] && [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
                			cad ${2} "-match" "$iid" "${3}" "imdbid"
		        		fi
						TITLE=`get_field title`
                        TYPE=`get_field type`
                elif [ $LOOSE_SEARCH -eq 1 ] && [ $S_LOOSE -eq 0 ] ; then
                        print_str "WARNING: $QUERY ($YEAR_q): $TD: invalid match (type is $TYPE), trying loose search.."
                        DTMP=`imdb_do_query "$QUERY"`

                        [ -n "$YEAR_q" ] && {
                                        DTMP_t=`imdb_get_by_year "$DTMP" $YEAR_q`
                                        [ -n "$DTMP_t" ] && DTMP="<IMDbResults>$DTMP_t</IMDbResults>" || {
                                                print_str "ERROR: $QUERY: $TD: could not find any object released in $YEAR_q, aborting.." 
                                                exit 1
                                        }
                        }
                        iid=`imdb_search "$DTMP"`
                        [ -z "$iid" ] &&
                                print_str "WARNING: $QUERY ($YEAR_q): $TD: cannot find record using iMDB loose search [$IMDB_URL?r=xml&s=$QUERY""$YQ_O""]" && exit 1      
                        DDT=`get_omdbapi_data "$iid"`
                        [ -z "$DDT" ] && print_str "ERROR: $QUERY ($YEAR_q): $TD: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1
                        if [ ${IMDB_DATABASE_TYPE} -eq 1 ] && [ $UPDATE_IMDBLOG -eq 1 ] && [ $DENY_IMDBID_DUPE -eq 1 ]; then
                			cad ${2} "-match" "${iid}" "${3}" "imdbid"
		        		fi
                        TITLE=`get_field title`
                        TYPE=`get_field type`
                fi
        fi

        TITLE=`get_field title`

else
        iid="$1"
        QUERY="$8"
        YEAR_q="$9"

		[ ${IMDB_DATABASE_TYPE} -eq 1 ] && [ ${UPDATE_IMDBLOG} -eq 1 ] && [ ${DENY_IMDBID_DUPE} -eq 1 ] && cad ${2} "-match" "${iid}" "${3}" "imdbid"

        DDT=`get_omdbapi_data "$iid"`

        [ -z "$DDT" ] && print_str "ERROR: $1: unable to get movie data [http://www.omdbapi.com/?r=XML&i=$iid]" && exit 1

        TYPE=`get_field type`
        TITLE=`get_field title`
fi

! echo $TYPE | egrep -q "$OMDB_ALLOWED_TYPES" && print_str "ERROR: $QUERY: $TD: invalid match (type is $TYPE): $IMDB_URL""?r=XML&i=$iid" && exit 1

[ -n "$IMDB_TITLE_WIPE_CHARS" ] && TITLE=`echo "$TITLE" | sed -r "s/[${IMDB_TITLE_WIPE_CHARS}]+//g"`

[ -z "$TITLE" ] && print_str "ERROR: $QUERY: $TD: could not extract movie title, fatal.." && exit 1

PLOT=`get_field plot`

${RECODE} --version 2&> /dev/null && {
	html_decode() {
		dec_t=`echo "${@}" | ${RECODE} -MHTML::Entities -alne 'print decode_entities($_)' | ${RECODE} -MHTML::Entities -alne 'print decode_entities($_)'`	
		[ -n "${dec_t}" ] && echo "${dec_t}" || echo ${@}
	}
	
	TITLE=`html_decode "${TITLE}"`
	PLOT=`html_decode "${PLOT}"`
}

[ -z "$PLOT" ] && PLOT="N/A"

RATING=`get_field imdbRating`
[ -z "$RATING" ] && RATING="0.0"
GENRE=`get_field genre`
[ -z "$GENRE" ] && GENRE="N/A"
VOTES=`echo $(get_field imdbVotes) | tr -d ','`
[ -z "$VOTES" ] && VOTES=0
YEAR=`get_field year | tr -d ' ' | egrep -o '^[0-9]+'`
[ -z "$YEAR" ] && YEAR=0
echo "$YEAR" | egrep -q '^[0-9]{1,4}$' || YEAR=0
RATED=`get_field rated`
[ -z "$RATED" ] && RATED="N/A"
ACTORS=`get_field actors`
[ -z "$ACTORS" ] && ACTORS="N/A"
DIRECTOR=`get_field director`
[ -z "$DIRECTOR" ] && DIRECTOR="N/A"
LANGUAGE=`get_field language`
[ -z "$LANGUAGE" ] && LANGUAGE="N/A"
COUNTRY=`get_field country`
[ -z "$COUNTRY" ] && COUNTRY="N/A"
SCREENS=`get_imdb_screens_count ${iid}`
[ -z "$SCREENS" ] && SCREENS=0

D_g=`get_field released`
[ -n "$D_g" ] && [ "$D_g" != "N/A" ] && RELEASED=`date --date="$D_g" +"%s"` || RELEASED=0
RUNTIME=`get_field runtime`
if [ -n "$RUNTIME" ]; then
	RUNTIME_h=`echo $RUNTIME | awk '{print $1}' | sed -r 's/[^0-9]+//g'`
	RUNTIME_m=`echo $RUNTIME | awk '{print $3}' | sed -r 's/[^0-9]+//g'`
	[ -z "$RUNTIME_m" ] && RUNTIME=$RUNTIME_h || RUNTIME=`expr $RUNTIME_h \* 60 + $RUNTIME_m`
else
	RUNTIME=0
fi
[ -z "${RUNTIME}" ] && RUNTIME=0

if [ $UPDATE_IMDBLOG -eq 1 ]; then
        trap "rm -f /tmp/glutil.img.$$.tmp" EXIT
        try_lock_r 12 imdb_lk "`echo "${3}${LAPPEND}" | md5sum | cut -d' ' -f1`" 120 "ERROR: could not obtain lock"
        
        if [ $IMDB_DATABASE_TYPE -eq 0 ]; then
                GLR_E=`echo $4 | sed 's/\//\\\\\//g'`
                DIR_E=`echo $6 | sed "s/^$GLR_E//" | sed "s/^$GLSR_E//"`
                [ -e "$3$LAPPEND" ] && {
					${2} --imdblog="${3}${LAPPEND}" -ff --nobackup --nofq -e imdb -l: dir ! -match "${DIR_E}" --nostats --silent ${EXTRA_ARGS} || {
                        print_str "ERROR: $DIR_E: Failed removing old record" && exit 1
                	}
                }
        elif [ $IMDB_DATABASE_TYPE -eq 1 ]; then
                #[ -z "$TITLE" ] && print_str "ERROR: $QUERY: $TD: failed extracting movie title" && exit 1
                DIR_E=$QUERY
                [ -e "$3$LAPPEND" ] && {
               		${2} --imdblog="${3}${LAPPEND}" -ff --nobackup --nofq -e imdb -l: imdbid ! -match "${iid}" --nostats --silent ${EXTRA_ARGS} || {
                   	     print_str "ERROR: $iid: Failed removing old record - $iid - $3$LAPPEND"; exit 1
               		}
               	}
        fi

	echo -en "dir $DIR_E\ntime `date +%s`\nimdbid $iid\nscore $RATING\ngenre $GENRE\nvotes $VOTES\ntitle $TITLE\nactors $ACTORS\nrated $RATED\nyear $YEAR\nreleased $RELEASED\nruntime $RUNTIME\ndirector $DIRECTOR\nplot $PLOT\nlanguage $LANGUAGE\ncountry $COUNTRY\ntype $TYPE\nscreens $SCREENS\n\n" > /tmp/glutil.img.$$.tmp
	${2} --imdblog="${3}${LAPPEND}" -z imdb --nobackup --nostats --silent ${EXTRA_ARGS} < /tmp/glutil.img.$$.tmp || print_str "ERROR: $QUERY: $TD: failed writing to imdblog!!"
	rm -f /tmp/glutil.img.$$.tmp
	echo "${10}" | grep -q "1" || [ ${IMDB_SHARED_MEM} -gt 0 ] && ${2} -q imdb --imdblog="${3}${LAPPEND}" --shmem --shmdestroy --shmreload --loadq --silent --shmcflags 666
fi

[ ${VERBOSE} -eq 1 ] && print_str "IMDB: `echo "Q:'$QUERY ($YEAR_q)' | A:'$TITLE ($YEAR)'" | tr '+' ' '` : $IMDBURL""title/$iid : $RATING $VOTES $GENRE"
[ ${op_id} -eq 1 ] && echo "${iid}"

exit 0
