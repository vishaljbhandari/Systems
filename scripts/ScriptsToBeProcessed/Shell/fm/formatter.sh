#################################################################################
#  Copyright (c) Subex Systems Limited 2008. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#! /bin/bash

MappingFile=$3
MappingFileBaseName=`basename $MappingFile`

on_exit ()
{
	if [ -f $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID ]; then
		PID=`cat $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID`
		if [ $PID -eq $$ ]; then
			rm -f $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID
		fi
	fi
}
trap on_exit EXIT

while getopts r: o
do
	case "$o" in
	r)	Status="$2" 
		;;
	*)	echo >&2 "Usage formatter.sh <-r start/stop> <Mapping file>" && exit 1 
		;;
	esac
	shift
done

function main
{
	Logfile=$RANGERHOME/LOG/"$MappingFileBaseName"formatter.log
	for DataFile in `ls -rt $InputPath/success`
	do
		echo "------------------------Formatter Log-----------------------------" >> $Logfile
		echo "File :$DataFile" >> $Logfile
		echo "Start Time:`date +"%D %T"`" >> $Logfile
		OutputFile="$OutputPath/$DataFile"
		gawk -v MappingFile="$MappingFile" -v OutputFile="$OutputFile" -v RecordType="$RecordType" -v FieldSeperator=$FieldSeperator -v Logfile="$Logfile" -f format.awk "$InputPath/$DataFile"
		if [ $? -ne 0 ]; then
			exit 6
		else
			touch $OutputPath/success/$DataFile
		fi
		rm -f "$InputPath/success/$DataFile"
		mv "$InputPath/$DataFile" "$InputPath/Processed"
		echo "End Time:`date +"%D %T"`" >> $Logfile
		echo "------------------------------------------------------------------" >> $Logfile
	done
}


ReadConfigfile()
{
	if [ -f $MappingFile ] ; then
		InputPath=`grep "InputPath" $MappingFile | cut -d"=" -f2 | sed "s:\\$RANGERHOME:$RANGERHOME:"`
	    OutputPath=`grep "OutputPath" $MappingFile | cut -d"=" -f2 | sed "s:\\$RANGERHOME:$RANGERHOME:"`
		RecordType=`grep "RecordType" $MappingFile | cut -d"=" -f2`
		FieldSeperator=`grep "FieldSeperator" $MappingFile | cut -d"=" -f2` 
	else
		echo >&2 "Mapping file $MappingFile is not present"
		exit 4
	fi
}

CheckConfigValues()
{
	if [ -z "$InputPath" -o -z "$OutputPath" -o -z "$RecordType" -o -z "$FieldSeperator" ] ; then
		echo >&2 "Entry is missing for InputPath/OutputPath/RecordType/FieldSeperator in Mapping file"
		exit 5
	fi
}

CreateDirectories()
{
	if [ ! -d "$InputPath/success" ]; then
			mkdir -p "$InputPath/success"
    fi

	if [ ! -d "$OutputPath/success" ]; then
			mkdir -p "$OutputPath/success"
    fi
}


if [ "$Status" == "stop" ]; then
	if [ -f $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID ]; then
		kill -9 `cat $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID`
		rm -f $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID
		echo "Dataformatter Script Stopped Successfuly ..."
		exit 0 
	else
		echo "No Such Process ... "
		exit 1
	fi
elif [ "$Status" == "start" ]; then
	if [ ! -f $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID ]; then
		echo "Dataformatter Script Started Successfuly ..."
		echo $$ > $RANGERHOME/share/Ranger/"$MappingFileBaseName"formatterPID
		ReadConfigfile
		CheckConfigValues
		CreateDirectories
		while true
		do
			main $*
			if [ "$RUN_ONCE" == 1 ]; then
				exit
			fi
		done
	else
		echo "Dataformatter Script Already Running ..."
		exit 2
	fi
else
	echo >&2 "Usage : formatter.sh <-r start/stop> <Mapping file>" && exit 1 
	exit 3
fi
