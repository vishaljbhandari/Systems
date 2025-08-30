#! /bin/bash
TARGS=$#

if [ $TARGS -ne 2 ] ; then
	echo " USAGE : $0 [Full data directory Path] [No of Days file retention] "
       exit 2
fi
DIR=$1
NUM_DAY=$2
x=0
while [ $x -lt 1 ]
do
        find  $DIR -mtime +$NUM_DAY  -exec rm {} \; 2> /dev/null
    	x=`expr $x + 1`
done
