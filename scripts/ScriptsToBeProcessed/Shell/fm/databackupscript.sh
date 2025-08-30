#!/bin/bash

if [ $# -ne 3 ]
then
	echo "Usge : $0 <Input Path> <Output Path1> <Output Path2>"
	exit 0
fi

ls $1/success > /dev/null 2>/dev/null
if [ $? -ne 0 ]
then
	echo $1"/success does not exist"
	exit 0
fi

ls $2/success > /dev/null 2>/dev/null
if [ $? -ne 0 ]
then
	echo $2"/success does not exist"
	exit 0
fi

ls $3/success > /dev/null 2>/dev/null
if [ $? -ne 0 ]
then
	echo $3"/success does not exist"
	exit 0
fi

SourceDir=$1
Target1=$2
Target2=$3

ReadAndSplitFiles()
{
        for file in `ls -tr $SourceDir/success`
        do
		cp $SourceDir/$file $Target1/$file
		CheckError $? "Target1 Copy" $file

		if [ $? == 2 ]
		then
			continue
		fi

		cp $SourceDir/$file $Target2/$file
		CheckError $? "Target2 Copy" $file

		if [ $? == 2 ]
		then
			continue
		fi

		touch $Target2/success/$file
		CheckError $? "Target2 -> success Copy" $file

        rm -f $SourceDir/$file
		CheckError $? "Remove Source File" $file

        rm -f $SourceDir/success/$file
		CheckError $? "Remove Source -> success" $file
        done
}

CheckError()
{
	RetCode=$1
	file=$3
	if [ $RetCode -ne 0 ]
	then
       	echo "`date`: Unable to execute command : $2 : [Source --> $SourceDir] - [File --> $file]" >> $RANGERHOME/LOG/databackupscript.log
		if [ -a $SourceDir/$file ]
		then
			exit 0
		else
	    	`rm $SourceDir/success/$file`
       		echo "Problem occured because of File: --> $file <--, which is present in success directory, but not in source directory" >> $RANGERHOME/LOG/databackupscript.log
			return 2
		fi
	fi
}

main()
{
        while true
        do
			ReadAndSplitFiles
			sleep 60
        done
}

main
