SummaryFileName=$COMMON_MOUNT_POINT/LOG/SchedulerLogs/Summary/scheduler_summarize_
MonitorFileName=$COMMON_MOUNT_POINT/LOG/SchedulerLogs/Monitor/scheduler_monitoring_
SpecificFileName=$COMMON_MOUNT_POINT/LOG/SchedulerLogs/SpecificScript/
LogStartTimeStamp=""
ModuleName=""
Pad=""

unamestr=`uname`
if [[ "$unamestr" == 'SunOS' ]]; then
   datec='gdate'
else
   datec='date'
fi




LogSchedulerSummaryStart()
{
	ModuleName=$1

	ModuleLength=`echo $ModuleName | wc -c | awk {'print $1'}`
	PadLength=$((50 - ModuleLength))
	Pad=""
	i=0
	while [ $i -lt $PadLength ]
	do
		Pad=$Pad" "
		i=$((i + 1))
	done

	LogStartTimeStamp=`$datec +"%m/%d/%Y %H:%M:%S"`

	File=$SummaryFileName`$datec --date="$LogStartTimeStamp" +%Y%m%d`".log"
	echo -e  "`$datec --date="$LogStartTimeStamp" +%Y%m%d`\t`$datec --date="$LogStartTimeStamp" +%H:%M:%S`\t        \t        \t$ModuleName$Pad Running"  >> $File
}

LogSchedulerSummaryEnd()
{
	Status="Failure"
	if [ $1 -eq 0 ] 
	then
		Status="Success"
	fi

	LogEndTimeStamp=`$datec +"%m/%d/%Y %H:%M:%S"`

	StartSecs=`$datec --date="$LogStartTimeStamp" +%s`
	EndSecs=`$datec --date="$LogEndTimeStamp" +%s`
	DiffSecs=$((EndSecs - StartSecs))
	DiffHours=$((DiffSecs / 3600))
	DiffSecs=$((DiffSecs - (DiffHours * 3600)))
	DiffMins=$((DiffSecs / 60))
	DiffSecs=$((DiffSecs - (DiffMins * 60)))
	
	File=$SummaryFileName`$datec --date="$LogEndTimeStamp" +%Y%m%d`".log"
	echo -e "`$datec --date="$LogEndTimeStamp" +%Y%m%d`\t`$datec --date="$LogStartTimeStamp" +%H:%M:%S`\t`$datec --date="$LogEndTimeStamp" +%H:%M:%S`\t$DiffHours:$DiffMins:$DiffSecs   \t$ModuleName$Pad $Status" >> $File
}

LogSchedulerMonitor()
{
	Status="Failure"
	if [ $1 -eq 0 ] 
	then
		Status="Success"
	fi

	File=$MonitorFileName`$datec --date="$LogEndTimeStamp" +%Y%m%d`".log"
	
	if [ -f $File ]
    then
		grep -v "^$ModuleName" $File > $File".tmp"
		mv $File.tmp $File
	fi
	
	echo -e "$ModuleName$Pad `$datec --date="$LogEndTimeStamp" +%Y%m%d`\t`$datec --date="$LogStartTimeStamp" +%H:%M:%S`\t`$datec --date="$LogEndTimeStamp" +%H:%M:%S`\t$Status" >> $File
	sort $File > $File".tmp"
	mv $File.tmp $File
		
}

LogScriptSpecific()
{
	SummaryInputFile=$1
	
	File="$SpecificFileName$ModuleName"_`$datec --date="$LogEndTimeStamp" +%Y%m%d_%H%M%S`".log"
	echo '******************************* Start Log ***************************' >> $File
	echo "Start Time: " `$datec --date="$LogStartTimeStamp"` >> $File
	echo " " >> $File
	cat $SummaryInputFile >> $File
	echo " " >> $File
	echo "End Time: " `$datec --date="$LogEndTimeStamp"` >> $File
	echo '*******************************  End Log  ****************************' >> $File
	rm $SummaryInputFile
}
