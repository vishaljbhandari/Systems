#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

killProcess()
{
echo "Killing process $processName for this user..."
		
for i in `ps -fu $LOGNAME | grep "$processName" | grep -v "grep" | grep -v "stopProgram*" |awk {'print $2'}`
do
	kill -15 $i
done



}

killProgramManagerChildProcesses()
{
pkill -KILL "programmana*"
pkill -KILL "recorddispa*"
pkill -KILL "recordproc*"
pkill -KILL "alarmgene*"
pkill -KILL "alarmdenor*"
pkill -KILL "countermanag*"
pkill -KILL "smartpattern*"
pkill -KILL "dbwrit*"
pkill -KILL "datasourc*"
pkill -KILL "scriptlauncher*"
pkill -KILL "licenseenforce*"
pkill -KILL "licenseengine"
pkill -KILL "participationco*"
pkill -KILL "offlinedatabuild*"
pkill -KILL "recordload*"
pkill -KILL "offline_data_build*"
pkill -KILL "offline_data_load*"
pkill -KILL "offline_index_build*"
pkill -KILL "irmagent*"
pkill -KILL "notificationgen*"
}

killRecordloader()
{
	for i in `ps -fu $LOGNAME | grep "offlinedatabuilder.rb" | grep -v "grep" | awk {'print $2'}`
	do
	    kill -9 $i
	done
	for i in `ps -fu $LOGNAME | grep "recordloader.rb" | grep -v "grep" | awk {'print $2'}`
	do
		kill -9 $i
	done
	for i in `ps -fu $LOGNAME | grep "offline_data" | grep -v "grep" | awk {'print $2'}`
	do
		kill -9 $i
	done
	for i in `ps -fu $LOGNAME | grep "offline_index" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "countermanag" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "recorddispa" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "recordproc" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "alarmgene" | grep -v "grep" | awk {'print $2'}`
	do
	    kill -9 $i
	done
	for i in `ps -fu $LOGNAME | grep "alarmdenor" | grep -v "grep" | awk {'print $2'}`
	do
		kill -9 $i
	done
	for i in `ps -fu $LOGNAME | grep "smartpattern" | grep -v "grep" | awk {'print $2'}`
	do
		kill -9 $i
	done
	for i in `ps -fu $LOGNAME | grep "dbwrit" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "datasourc" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "scriptlauncher" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "licenseenforce" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "participationco" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "irmagent" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	for i in `ps -fu $LOGNAME | grep "notificationgen" | grep -v "grep" | awk {'print $2'}`
	do
        kill -9 $i
    done
	
}

echo $1
echo $#
if [ $# -eq 0 ]
then
	processName='programma*'
	killProcess
	killProgramManagerChildProcesses
	killRecordloader
else
	processName=$1
	killProcess
	killRecordloader
fi
