#!/bin/env bash

. testFunctions.sh

UTLogFile=$1

if [ "$UTLogFile" = "" ];then
	UTLogFile="Server_UTs.log"
fi

if [ "$SummaryLog" = "" ];then
	SummaryLog=Res.log
	> Res.log
fi

#(cd ../Server && nohup ./runtestapp.sh $* 2>&1 |tee  $UTLogFile)
cat $UTLogFile | tr [:cntrl:] '\n' |grep -v '^$' | grep -v -e '\.\.\. Done$' -e 'un all Tests' -e '\<Suite\>' -e 'Running.*sql$' > Res.log
Log "****************************"
grep -e "Error" -e "Failed" -e '\<core\>' -e "Segm" -e "Aborted" $SummaryLog

LogFile=$UTLogFile

CollectNormalResult
