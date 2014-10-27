#!/bin/bash

lftp_connect()
{
echo "open ${1}:${2};
user ${3} "${4}"
"
}