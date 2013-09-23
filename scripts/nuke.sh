#!/bin/bash
#
## Nukes releases using only glutil (no vanilla or other third-party tools needed)
#
## Requires: -glutil-1.8-8 or greater
##           -bc utility
#
## Searches dirlog using regex matching 
#
## Usage: ./nuke.sh <nuke/unnuke> <release> <reason> [<mult>]
#
##  Requires 'bc' utility
#
###########################[ BEGIN OPTIONS ]#############################
#
## More than one match is possile (all get nuked),
## otherwise just the first is nuked.
MULTIPLE_MATCHES=0
#
## Nuker's username
NUKER="glftpd"
#
############################[ END OPTIONS ]##############################

glutil="/home/reboot/workspace/glutil/glutil"
nuker="/glftpd/bin/nuker"

#########################################################################

mode=$1
release=$2
reason=$3
ratio=$4

glroot=`$glutil noop --silent --preexec "echo '{glroot}'"`
glconf=`$glutil noop --silent --preexec "echo '{glconf}'"`

[ -d "$glroot" ] || echo "ERROR: could not get glroot automatically, set manually"
[ -f "$glconf" ] || echo "ERROR: could not get glconf automatically, set manually"

if [ "$mode" = "unnuke" ]; then 
	status=1
	nstr="UNNUKE"
	nstr1="unnuke"
else
	nstr="NUKE"
	nstr1="nuke"
	status=0
fi

[ -z "$mode" ] && cho "No nuke/unnuke specified" && exit 1
[ -z "$release" ] && echo "Type: !nuke <release> <mult> <reason>" && exit 1
[ -z "$reason" ] && echo "Please enter a reason. !nuke <release> <mult> <reason>" && exit 1
[ $status -eq 0 ] && [ -z "$ratio" ] && echo "Please enter a nuke multiplier. !nuke <release> <mult> <reason>" && exit 1

get_username_from_uid() {
	while read line; do
		echo "$line" | cut -f 3 -d ":" | grep $1 > /dev/null && echo "$line" | cut -f 1 -d ":" && return 0
	done < "$glroot/etc/passwd"
	return 1
}


build_nuke_packet() {
	bnp_ts=`date +"%s"`
	echo -en "dir $1\nstatus $2\ntime $bnp_ts\nnuker $3\nunnuker $4\nnukee $5\nmult $6\nreason $7\nbytes $8\n\n"
}

get_stat_offset() {
    st_bst=`cat "$glroot/etc/glftpd.conf" | grep "stat_section" | grep -v -P '[^\/]DEFAULT[^\/]'`
	st_b=`echo "$st_bst" | grep "stat_section" | awk '{print $3}' | sed -r 's/\[\:and\:\]/|/g' | sed -r 's/[\*]+//g' | sed -r ':a;N;$!ba;s/[^A-Za-z0-9\|\n]/\\\\\0/g'`
	st_i=0
	
	for st_ii in $st_b; do
	echo "$st_ii"
		st_i=`expr $st_i + 1`
		echo "$st_ii" | awk '{print $2}' | grep 'DEFAULT' && continue			
		echo "$1" | grep -P "$st_ii" > /dev/null && echo $st_i && return 0
	done
	return 1
}

perform_nuke() {	
	pf_found=0
	pf_bn=`basename "$2"`	
	[ $1 -eq 0 ] && { 
		findnuke=`$glutil -n --ilom "status = $1" and --imatch dir,"$2" --batch --imatchq` || pf_found=1
	}
	[ $pf_found -eq 1 ] && {		
		pn_r=0
		pf_ts=`echo "$findnuke" | cut -f 8`
		[ $pf_ts -lt $3 ] && {
			echo "WARNING: $pf_bn: [$nstr1:$pf_ts, dir:$3] $nstr1 is older than directory, assumind bad"
			$glutil -e nukelog --silent --lom "status = $1" and --match dir,"$2" -vvv && pn_r=1 || 
				echo "WARNING: could not remove bad nukelog record!"
		}
		[ $pn_r -eq 0 ] && echo "$nstr: $pf_bn: already exists (by: `echo "$findnuke" | cut -f 6`)" && exit 1
		[ $pn_r -eq 1 ] && echo "$nstr: $pf_bn: erased bad record (dirlog timestamp newer than nukelog)"
	}
	
	pf_found=1
	[ $1 -eq 1 ] && { 
		findnuke=`$glutil -n --ilom "status = 0" and --imatch dir,"$2" --batch --imatchq` && {	
			echo "$nstr: $pf_bn: is not nuked" && exit 1
		}
	}	
		
	pf_found=0
	[ $1 -eq 1 ] && { 
		findnuke=`$glutil -n --ilom "status = $1" and --imatch dir,"$2" --batch --imatchq` ||  {	
			$glutil -e nukelog --silent --match dir,"$2" 
		}
	}	
	
	[ $1 -eq 0 ] && UNNUKER=" " || UNNUKER=$NUKER
	
    build_nuke_packet "$2" $1 "$NUKER" "$UNNUKER" "$4" "$6" "$7" "$5" | $glutil -z nukelog --silent || {
    	echo "ERROR: $pf_bn: failed writing nukelog!"; 
    	build_nuke_packet "$2" $1 "$NUKER" "$UNNUKER" "$4" "$6" "$7" "$5"
    	exit 1
    }
    	
    echo "$nstr: $2: $pf_bn: user '$4' $nstr1 $6""x"
    echo ":: `get_stat_offset $2`" 
}


[ $MULTIPLE_MATCHES -eq 0 ] && frap="--imatchq" || frap=""

findrelease=`$glutil -d --batch -v --iregex dir,"$release" $frap` 

[ -z "$findrelease" ] && echo "$release was not found on site"

found=`echo "$findrelease" | wc -l`

echo "DIRLOG: found $found releases matching '$release'"

for i in "$findrelease"; do
	USERID=`echo "$i" | cut -f 6`
	USER="`get_username_from_uid $USERID`"
	[ -z "$USER" ] && echo "ERROR: unable to get username from UID: $USERID" && continue
	SIZE=`echo "$i" | cut -f 3`
	SIZEMB=`echo "$SIZE / 1024 / 1024" | bc -l`
	perform_nuke $status "`echo "$i" | cut -f 2`" `echo "$i" | cut -f 5` "$USER" "$SIZEMB" "$ratio" "$reason"
done

exit 0

