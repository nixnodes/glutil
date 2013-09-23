#!/bin/bash

glftpddir="/glftpd"
glutil="/glftpd/bin/sources/glutil/glutil"
nukeuser="glftpd"

release=$1
ratio=$2
reason=$3
nuke="/glftpd/bin/nuker"
config="-r /glftpd/etc/java.conf"

if [ "$release" == "" ]; then
  echo "Type: !nuke <release> <ratio> <reason>"
  exit 0
fi

if [ "$ratio" == "" ]; then
  echo "Please enter a ratio. !nuke <release> <ratio> <reason>"
  exit 0
fi

if [ "$reason" == "" ]; then
  echo "Please enter a reason. !nuke <release> <ratio> <reason>"
  exit 0
fi

### No need to edit belowe ###
findrelease=`$glutil -d -v | grep $release | awk '{print $2}'`
if [ "$findrelease" ]
then
$nuke $config -N $nukeuser -n {$findrelease} $ratio $reason >/dev/null
echo "NUKED: $release ratio: $ratio reason: $reason"
else
echo "$release was not found on site"
fi