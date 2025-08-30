#! /bin/bash
if [ $# -lt 3 ]
then
        echo "Usage: scriptlauncher $0 DIR FileName start end-today"
	echo "Example: scriptlauncher $0 Dir file 10 5"
        exit 1
fi
dir=$1
filename=$2
start=$3
if [ $# -lt 4 ]
then
	end=0
else
	end=$4
fi
if [ ! -d  $dir ]
then
    	echo "Specified dir $1 is not valid "
	exit 5
fi
counter=$end
while [ $counter -le $start ]
do
    	touch -d $counter" days ago" $dir/$filename"_"$counter".dat"
	counter=`expr $counter + 1`
done
