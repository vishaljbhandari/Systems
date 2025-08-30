#!/bin/env bash

ProgName=$0
DateSuffix="$(date "+%Y_%m_%d_%H_%M")"
LogFile="${ProgName%".sh"}.log"
BackupLogFile="${ProgName%".sh"}_${DateSuffix}.log"
BackupDir=TestLog
> $LogFile

Log()
{
	Msg=$1
	echo "#####[$ProgName] [`date`] :: [ $Msg ]" >> $LogFile
}

RunProg()
{
	Prog=$1
	Message=$2
	ErrMsg=$3
	Quit=$4
	echo $Prog
	echo $Message
	echo $ErrMsg
	Log "Running $Prog, $Message"
	eval "$Prog" | tee -a $LogFile;
	Status=$?
	if [ $Status -ne 0 ];then
		Log "'$Prog' Failed, Exited with status $Status [$ErrMsg]" ;
		if [ "$Quit" = "Y" ];then
			exit $Status ;
		fi
	fi
	return $Status

}

Build()
{
	RunProg "(cd ../Server && make -s -k -j2)" "Starting Build" "Build Failed"

	if [ $? -ne 0 ];then
		RunProg "(cd ../Server && make -s)" "Retrying Build" "No more tries, Quiting" 'Y'
	fi

	RunProg "(cd ../Server && make -s install)" "Installing Build" "Installing Failed" 'Y'
}

BackupLog()
{
	if ! [ -d "$BackupDir" ];then
		mkdir -p $BackupDir
	fi
	gzip -c $LogFile > $BackupDir/$BackupLogFile".gz"
}

