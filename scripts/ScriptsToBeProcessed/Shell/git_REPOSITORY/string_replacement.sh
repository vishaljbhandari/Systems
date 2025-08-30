#!/bin/bash
exclude="swp"
include=""
CURRENTTIME=`date +"%Y%m%d%H%M%S"`
if [ $# -ne 3 ]
then
        echo -e "[$CURRENTTIME] Usage $0 [PATH] [TARGET] [REPLACEMENT]"
        exit 1
fi

path=$1
target=$2
replacement=$3

find .|grep -v $exclude|xargs grep -sl "$path"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|ruby script|script text|bash script|shell script|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$target/$replacement/g" >/dev/null 2>&1
if [ $? -eq 0 ]
then
        echo -e "[$CURRENTTIME] Executed Successfully";
        exit 0
else
        echo -e "[$CURRENTTIME] Failed While Executingy";
        exit 2
fi

