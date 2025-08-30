#! /bin/bash

if [ $# -ne 3 ] 
then
       echo " USAGE :  $0 DIRECTORY_1 DIRECTORY_2" 
       exit 1
fi

A=$1
B=$2
C=$3

EXCEPT=".svn|.lib|.deps|.cache"
SEARCH=".lcf|.log|.png|.gif|.jpg|.rtf|.tar|.jar|.jpeg|.zip|.yml|.bmp|.swf|.imp|.awk|.dh|.cc|.h|.dame|.rb|.sh|.am|.x|.cpp|.css|.js|.in"
DIFFSEARCH=".dh|.cc|.h|.dame|.rb|.sh|.am|.x|.cpp|.css|.js|.in"
CURR=`pwd`
LOG_FILE=$CURR/DeepDiff_$C.log
> $LOG_FILE
printf "OPERATION A-B, WHERE\nA = ["$A"]\nB = ["$B"]\n" >> $LOG_FILE

if [ ! -d $A ]
then
        printf "DIRECTORY A : ["$A"] DOESNT EXISTS\n" >> $LOG_FILE
        echo -e "DIRECTORY A : ["$A"] DOESNT EXISTS\n"
        exit 2
fi

if [ ! -d $B ]
then
        printf "DIRECTORY B : ["$B"] DOESNT EXISTS\n" >> $LOG_FILE
        echo -e "DIRECTORY B : ["$B"] DOESNT EXISTS\n"
        exit 3
fi

DirectoryMissing=0
FileMissing=0
FileMissmatch=0

Directory_LOG=$CURR/DirMissing_$C.log
> $Directory_LOG
FileMissing_LOG=$CURR/FileMissing_$C.log
> $FileMissing_LOG
FileMissmatch_LOG=$CURR/FileMissmatch_$C.log
> $FileMissmatch_LOG
temp=$CURR/temp_$C.log
> $temp
tmp=$CURR/tmp_$C.log
> $tmp

cd $B

#printf "========DIRECTORIES IN ["$B"]=========\n" >> $Directory_LOG
#find . -type d | egrep -v $EXCEPT >> $Directory_LOG
printf "=====================================\n" >> $Directory_LOG
cd $A
#printf "========DIRECTORIES IN ["$A"]=========\n" >> $Directory_LOG

> $temp
find . -type d | egrep -v $EXCEPT > $temp
#find . -type d | egrep -v $EXCEPT >> $Directory_LOG
printf "\n========DIRECTORY REPORT=========\n" >> $Directory_LOG

while read DirectoryList
do
	if [ ! -d "$B/$DirectoryList" ]
	then
        	printf "DIRECTORY : ["$B"/"$DirectoryList"] Expected, But DOESNT EXISTS\n" >> $Directory_LOG
		DirectoryMissing=`expr $((DirectoryMissing + 1))`	
	fi
done < $temp

> $temp
find . -type f | egrep -v $EXCEPT | egrep $SEARCH > $temp
#printf "\n========FILES IN ["$A"]=========\n" >> $FileMissing_LOG
#find . -type f | egrep -v $EXCEPT | egrep $SEARCH >> $FileMissing_LOG
printf "\n========FILE REPORT=========\n" >> $FileMissing_LOG

while read FilesList
do
        if [ ! -f "$B/$FilesList" ]
        then
                printf "FILE : ["$B"/"$FilesList"] Expected, But DOESNT EXISTS\n" >> $FileMissing_LOG
                FileMissing=`expr $((FileMissing + 1))`
        else 
		> $tmp
		egrep $DIFFSEARCH $FilesList > $tmp
		sed '/^[ \t]*$/d' < $tmp > $tmp.tmp
                mv $tmp.tmp  $tmp
                echo "$tmp" | grep "\." > /dev/null 2>&1
		charcount=`cat $tmp | wc -c`
                if [ $charcount -gt 0 ]
		then
			> $tmp
			diff -u $A/$FilesList $B/$FilesList > $tmp
			sed '/^[ \t]*$/d' < $tmp > $tmp.tmp
			mv $tmp.tmp  $tmp
			echo "$tmp" | grep "\." > /dev/null 2>&1		
			wordcount=`cat $tmp | wc -w`
			if [ $wordcount -gt 0 ]
			then
				FileMissmatch=`expr $((FileMissmatch + 1))`
				printf "=====================================================================\n" >> $FileMissmatch_LOG
				printf "FILE : $B/$FilesList\n" >> $FileMissmatch_LOG
				ls -l $B/$FilesList >> $FileMissmatch_LOG
				printf "\n" >> $FileMissmatch_LOG
				diff -u $A/$FilesList $B/$FilesList >> $FileMissmatch_LOG
				printf "=====================================================================\n\n\n" >> $FileMissmatch_LOG
			fi
		fi
	fi
done < $temp

printf "OPERATION COMPLETE WITH\nTOTAL DIRECTORIES MISSING IN ["$B"] = ["$DirectoryMissing"]\nLogs can be found at["$Directory_LOG"]\n" >> $LOG_FILE
printf "TOTAL FILES MISSING IN ["$B"] = ["$FileMissing"]\nLogs can be found at["$FileMissing_LOG"]\n" >> $LOG_FILE
printf "TOTAL MISSMATCHING FILES IN ["$B"] = ["$FileMissmatch"]\nLogs can be found at["$FileMissmatch_LOG"]\n" >> $LOG_FILE
printf "===================== COMPLETED =============================" >> $LOG_FILE

echo "Comparison complete. Please find the Logs in $LOG_FILE"

