#!/bin/bash
. $HOME/subex_working_area/EAM/export.sh

if [ $# -lt 2 ]
then
	echo "Usage $0 <logfileList> <RetentionInterval>"
	exit 1
fi

cleanupDays=$2

date +%j > /dev/null 2>/dev/null
if [ $? -ne 0 ]
then
	echo "GNU date is required to use this script"
	echo ""
fi

fileList=$1

if [ ! -s $fileList ];then 
	echo "The file $fileList is empty" 
	exit -1
fi

filesuffix=`date '+%j'`

curYear=`date '+%Y'`
prevYear=`expr $curYear - 1`
leapyearCheck1=`expr $prevYear % 4`
leapyearCheck2=`expr $prevYear % 100`
leapyearCheck3=`expr $prevYear % 400`

if [ $leapyearCheck1 -eq 0 -a $leapyearCheck2 -ne 0 -o $leapyearCheck3 -eq 0 ];then
	prev_last_day_of_year=366
else
	prev_last_day_of_year=365
fi

for logfile in `cat $fileList`
do
	if [ -f $logfile ];then

		filedir=`dirname $logfile`
		historicaldir=$filedir/historical
		targetDir=$historicaldir/$filesuffix	
		mkdir -p $targetDir

		for i in `ls -r $historicaldir`
		do

			if [ -d $historicaldir/$i ];then

				if [ $filesuffix -gt $cleanupDays ];then

					diff=`expr $filesuffix - $cleanupDays`			

					if [ $i -le $diff -o $i -gt $filesuffix ];then

						rm -rf $historicaldir/$i
					fi
				else

					diff=`expr $cleanupDays - $filesuffix`	
					diff=`expr $prev_last_day_of_year - $diff`

					if [ $i -le $diff -a $i -gt $filesuffix ];then

						rm -rf $historicaldir/$i
					fi

				fi

			fi

		done

		cp $logfile $targetDir
		>$logfile

	fi
done
