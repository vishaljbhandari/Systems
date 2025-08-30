#! /bin/bash
#/*******************************************************************************                                 
#*  Copyright (c) Subex Limited 2014. All rights reserved.                      *                                 
#*  The copyright to the computer program(s) herein is the property of Subex    *                                 
#*  Limited. The program(s) may be used and/or copied with the written          *                                 
#*  permission from Subex Limited or in accordance with the terms and           *                                 
#*  conditions stipulated in the agreement/contract under which the program(s)  *                                 
#*  have been supplied.                                                         *                                 
#********************************************************************************/ 
if [ $# -lt 2 ]
then
        echo "Usage: scriptlauncher $0 [BACKUP|RESTORE] FileName"
        exit 1
fi

ActionType=$1
InputFile=$2
LogFile=`pwd`/$InputFile.log
extToken=".bck"

touch $LogFile
printf "\n------------ $(date) ----------\n" >> $LogFile
printf "Action Type : $ActionType \n" >> $LogFile
printf "Input File : $InputFile \n" >> $LogFile
printf "Extention Tocken : $extToken \n" >> $LogFile

if [ "$ActionType" == "BACKUP" -o "$ActionType" == "Backup" ]
then
        FirstSuffix=""
	SecondSuffix=$extToken
elif [ "$ActionType" == "RESTORE" -o "$ActionType" == "Restore" ]
then
        FirstSuffix=$extToken
        SecondSuffix=""
else
	printf "Wrong ACTION Type : Use BACKUP or RESTORE\n-----------------\n" >> $LogFile
        echo -e "Wrong ACTION Type : Use BACKUP or RESTORE"
	echo -e "TERMINATED : Please check Log File at $LogFile"
        exit 2
fi

if [ ! -f $InputFile ]
then
        printf "Input File Does not Exist: $InputFile \n-----------------\n" >> $LogFile
        echo -e "Input File Does not Exist: $InputFile"
	echo -e "TERMINATED : Please check Log File at $LogFile"
        exit 3
else
        sed '/^[ \t]*$/d' < $InputFile > $InputFile.tmp
        mv $InputFile.tmp  $InputFile
        echo "$InputFile" | grep "\." > /dev/null 2>&1
        if [ $? -ne 0 ]
        then
                mv $InputFile $InputFile.data
		CurrentDir=`pwd`
		CompleteFilePath=$CurrentDir/$InputFile.data
		echo -e "Datafile is $CompleteFilePath"
		printf "Datafile is $CompleteFilePath \n" >> $LogFile
        else
		printf "Bad Input File: $InputFile \n-----------------\n" >> $LogFile
		echo -e "Bad Input File $InputFile"
		echo -e "TERMINATED : Please check Log File at $LogFile"
		exit 4	
	fi
fi

echo "Please Enter Root Directory for Relative Linkings"
read ROOTDIR

if [ -d  $ROOTDIR ]
then
    	cd $ROOTDIR
   	printf "Root Directory is : $ROOTDIR \n" >> $LogFile
else
	printf "Root Directory Doesnt Exist: $ROOTDIR \n-----------------\n" >> $LogFile
	echo -e "Root Directory Doesnt Exist: $ROOTDIR"
	echo -e "TERMINATED : Please check Log File at $LogFile"
	exit 5
fi

countofrecords=`cat $CompleteFilePath | wc -l`
printf "Number of files for $ActionType are $countofrecords\n" >> $LogFile
echo -e "Number of files for $ActionType are $countofrecords"

while read line
do
	if [ -f  $ROOTDIR/$line$FirstSuffix ]
	then
		cp $ROOTDIR/$line$FirstSuffix $ROOTDIR/$line$SecondSuffix
     		echo -e "File $line$FirstSuffix - $ActionType Done"
		printf "File $line$FirstSuffix - $ActionType Done \n" >> $LogFile
	else
        	printf "File Doesnt Exist: $ROOTDIR/$line$FirstSuffix ---------------- SKIPPED\n" >> $LogFile
        	echo -e "File Doesnt Exist: $ROOTDIR/$line$FirstSuffix ---------------- SKIPPED"
	fi
done < $CompleteFilePath

echo -e "Done, Please check Log File at $LogFile"
