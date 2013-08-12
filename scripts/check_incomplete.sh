#!/bin/bash
#
# Usage: /glroot/bin/dirupdate -d -exec "/glftpd/bin/scripts/check_incomplete.sh '{dir}' '{exe}' '{glroot}'"
#
#
## Verbose output
VERBOSE=0
#
#########################################################################


[ -z "$1" ] && exit 1
[ -z "$2" ] && exit 1

GLROOT=$3

DIR=$GLROOT$1
EXE=$2

#echo $DIR $EXE

#sleep 1

! [ -d "$DIR" ] && exit 1

proc_dir() {
	for i in $1/*; do
		echo $i
		pdbn=$(basename "$i")
		[ -d "$i" ] && [[ "$pdbn" != ".." ]] && [[ "$pdbn" != "." ]] && proc_dir $i
		if [ -f "$i" ] && echo $i | grep -P "\.sfv$" > /dev/null; then
			while read l; do
				arr=($(echo $l | tr " " "\n")) ; nm="${#arr[@]}"
				FCRC="${arr[$nm-1]}"; FFL=""
				for ((ii=0; ii<nm-1; ii++)); do
					FFL="$FFL${arr[$ii]}"
				done
	
				FFT=$(dirname $i)/$FFL
				! [ -f "$FFT" ] && echo "WARNING: $DIR: incomplete, missing file: $FFL" && continue
				CRC32=$($EXE --crc32 $FFT)				
				[ $CRC32 != $FCRC ] && echo "WARNING: $DIR: corrupted: $FFL, CRC32: $CRC32, should be: $FCRC" && continue
					
				[ $VERBOSE -gt 0 ] && echo "OK: $FFL: $CRC32"
			done < "$i"
		fi	
	done
}

proc_dir $DIR

exit 1

