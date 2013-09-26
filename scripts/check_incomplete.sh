#!/bin/bash
# DO NOT EDIT THESE LINES
#@MACRO:incomplete:{m:exe} -d -execv "{m:spec1} {dir} {exe} {glroot} {m:arg1}" --silent
#@MACRO:incomplete-c:{m:exe} -d -execv "{m:spec1} {dir} {exe} {glroot} {m:arg2}" --iregex "{m:arg1}" --silent
#
## Checks a release for incomplete/corrupt files by comparing SFV data with filesystem
#
## Usage (manual): /glroot/bin/glutil -d -exec "/glftpd/bin/scripts/check_incomplete.sh '{dir}' '{exe}' '{glroot}'"
#
## Usage (macro): ./glutil -m incomplete                             (full dirlog)
##                ./glutil -m incomplete-c -arg1="Unforgettable"     (regex filtered dirlog scan)
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
## See ./glutil --help for more info about options
#
###########################[ BEGIN OPTIONS ]#############################
#
## Verbose output
VERBOSE=0
#
## Optional corruption checking (CRC32 calc & match against .sfv)
CHECK_CORRUPT=1
#
############################[ END OPTIONS ]##############################


[ -z "$1" ] && exit 1
[ -z "$2" ] && exit 1

GLROOT=$3

EXE=$2


c_dir() {
	while read l; do
		FFL=$(echo "$l" | sed -r 's/ [A-Fa-f0-9]*([ ]*|$)$//')
#		FCRC=$(echo "$l" | rev | cut -d " " -f -1 | rev)
#		echo $FFL $FCRC
		FFT=$(dirname "$1")/$FFL
		! [ -f "$FFT" ] && echo "WARNING: $DIR: incomplete, missing file: $FFL" && continue
		[ $CHECK_CORRUPT -gt 0 ] && { 
			FCRC=$(echo "$l" | rev | cut -d " " -f -1 | rev) && CRC32=$($EXE --crc32 $FFT) && 
				[ $CRC32 != $FCRC ] && echo "WARNING: $DIR: corrupted: $FFL, CRC32: $CRC32, should be: $FCRC" && continue
		}
		[ $VERBOSE -gt 0 ] && echo "OK: $FFT: $FCRC"
	done < "$1"
}

[ "$4" = "cdir" ] && {
	DIR="$1"
	[ -n "$5" ] && VERBOSE=1
	c_dir "$DIR"
	exit 1
}

DIR=$GLROOT$1
[ -n "$4" ] && VERBOSE=1

! [ -d "$DIR" ] && exit 1

$EXE -x "$DIR" --iregexi "\.sfv$" -execv "$0 {arg} $2 $3 cdir $4" --silent -recursive

exit 1

proc_dir() {
	for i in $1/*; do
		if [ -f "$i" ] && echo $i | egrep -q "\.sfv$"; then
			c_dir "$i"
		elif [ -d "$i" ]; then
			pdbn=$(basename "$i")
			[[ "$pdbn" != ".." ]] && [[ "$pdbn" != "." ]] && proc_dir $i
		fi
	done
}

proc_dir $DIR

exit 1
