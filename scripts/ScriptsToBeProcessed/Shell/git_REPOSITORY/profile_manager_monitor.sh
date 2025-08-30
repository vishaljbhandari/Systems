#!/bin/bash
#################################################################################
#  Copyright (c) Subex Limited. All rights reserved.                            #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################
# PROFILE MANAGER VERSION 8.02
# PROFILE MANAGER MONITOR SCRIPT VERSION 1.0
# AUTHOR  : VISHAL BHANDARI | DST TEAM
# CUSTOMER: MCI IRAN
#
#
#
#################################################################################
PWD=`pwd`
NumericReg='^[0-9]+$'
IP_ADDRESS=`ip addr show bond0 | grep 'inet ' | awk -F" " '{print $2}'`
HOST_NAME=`hostname --long`
CURRENTTIME=`date +"%Y%m%d%H%M%S"`
SELFPID=`ps -eaf | grep {$0} | awk -F" " '{ print $2}'`
SLEEP_INT=20
SINGLE_LINE='------------------------------------------------------------------'
DOUBLE_LINE='=================================================================='
# Mailing list, seperated by comma
MAILLIST="vishal.bhandari@subex.com"
if [ $# -ne 1 ] ; then
       exit 1
fi
ENTITY_TYPE=$1
if ! [[ $ENTITY_TYPE =~ $NumericReg ]] ; then
        exit 2
fi
LSPattern="No such file or directory"
SUBJECT_HEADER_PREFIX="MCI IRAN"
#MONITOR_LOG_DIRECTORY=$NIKIRAROOT/LOG
MONITOR_LOG_DIRECTORY=/rocfm/subex_working_area/VISHAL/logs
LOG_FILE=$MONITOR_LOG_DIRECTORY/profilemanager.monitor.log.$ENTITY_TYPE.$CURRENTTIME.txt
TMP_FILE=$MONITOR_LOG_DIRECTORY/.profilemanager.monitor.tmp.$ENTITY_TYPE.$CURRENTTIME.txt
FILE_LIST=$MONITOR_LOG_DIRECTORY/profilemanager.monitor.files.$ENTITY_TYPE.$CURRENTTIME.txt
PSTACK_FILE=$MONITOR_LOG_DIRECTORY/profilemanager.monitor.pstack.$ENTITY_TYPE.$CURRENTTIME.txt
LOG_DIRECTORY=$NIKIRACLIENT"/../notificationmanager/app_logs/server/"
NUMBER_OF_PSTACK=10
PROFILEMANAGER_STATUS="RUNNING"
if ! [[ $NUMBER_OF_PSTACK =~ $NumericReg ]] ; then
        NUMBER_OF_PSTACK=10
fi
CURRENTTIME=`date +"%Y-%m-%d %H:%M:%S"`
SCRIPT_NAME=$0
function SignalHandlerSIGHUP(){
	C_T=`date +"%Y-%m-%d %H:%M:%S"`
        echo -e "[INFO] Script($SCRIPT_NAME) Recieved Signal SIGHUP at $C_T" >> $LOG_FILE
}
function SignalHandlerSIGTERM(){
        C_T=`date +"%Y-%m-%d %H:%M:%S"`
        echo -e "[INFO] Script($SCRIPT_NAME) Recieved Signal SIGTERM at $C_T" >> $LOG_FILE
}
function SignalHandlerSIGINT(){
        C_T=`date +"%Y-%m-%d %H:%M:%S"`
        echo -e "[INFO] Script($SCRIPT_NAME) Recieved Signal SIGINT at $C_T" >> $LOG_FILE
}
CID=
function SignalHandlerEXIT(){
	# MAILING REPORTS
	CURRENTTIME=`date +"%Y%m%d%H%M%S"`
	MAIL_HEADER="$SUBJECT_HEADER_PREFIX PROFILE MANAGER RUN($ENTITY_TYPE|$CID|$CURRENTTIME|$PROFILEMANAGER_STATUS)"
	#tar -zcvf $MONITOR_LOG_DIRECTORY/profilemanager.$CURRENTTIME.tar.gz $LOG_FILE $PSTACK_FILE $FILE_LIST
	#`uuencode $MONITOR_LOG_DIRECTORY/profilemanager.$CURRENTTIME.tar.gz | mailx -s $MAIL_HEADER $MAILLIST < $LOG_FILE > /dev/null`
	echo -e "[INFO] MAIL HEADER: $SUBJECT_HEADER_PREFIX PROFILE MANAGER RUN($ENTITY_TYPE|$CID|$CURRENTTIME|$PROFILEMANAGER_STATUS)" >> $LOG_FILE
	echo -e "[INFO] MAIL sent to $MAILLIST" >> $LOG_FILE
	#rm $TMP_FILE

}
trap SignalHandlerSIGHUP SIGHUP
trap SignalHandlerSIGTERM SIGTERM
trap SignalHandlerSIGINT SIGINT
trap SignalHandlerEXIT EXIT

# =====================================================================================

if [ ! -f $FILE_LIST ]
then
	touch $FILE_LIST
fi
>$FILE_LIST
touch $LOG_FILE
if [ ! -f $LOG_FILE ]
then
	exit 10
fi
> $LOG_FILE
echo -e $DOUBLE_LINE >> $PSTACK_FILE
touch $TMP_FILE
if [ ! -f $TMP_FILE ]
then
        exit 11
fi
> $TMP_FILE
touch $PSTACK_FILE
if [ ! -f $PSTACK_FILE ]
then
        exit 12
fi
> $PSTACK_FILE

CID=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
set feedback off
set heading off
select OFFLINE_COUNTER_ID from PROFILE_MANAGER_ENTITY_CONFIGS where ENTITY_TYPE = '$ENTITY_TYPE' ;
EOF`
CID=`echo $CID | sed 's/[\t \n]*//g'`
if ! [[ $CID =~ $NumericReg ]] ; then
        echo -e "[ERROR] Invalid ENTITY_TYPE[$ENTITY_TYPE], Failed to get Offline Counter ID" >> $LOG_FILE
        exit 204
fi

OPID=
CheckStateGathingCounterRunning()
{
	pd=$1
	if [[ $pd =~ $NumericReg ]] ; then
		OPID=`ps -eaf | grep inmemorycountermanager | grep " $pd " | awk -F" " '{ print $2}'`
	fi
}

GetPstackFrame()
{
	pd=$1
	if [[ $pd =~ $NumericReg ]] ; then
		CURR_TIME=`date +"%Y-%m-%d %H:%M:%S"`
		process=`ps -eaf | grep $pd | grep -v grep | awk -F" " ' { print $8 }'`
		loc=`pwdx $pd | awk -F": " '{ print$2 }'`
		echo -e $SINGLE_LINE >> $PSTACK_FILE
		echo -e "# PROCESS($process) INFO FRAME AS ON TIME($CURR_TIME)" >> $PSTACK_FILE
		echo -e "RUNNING LOCATION $loc" >> $PSTACK_FILE
		ps -p $pd -wo lstart,pid,ppid,uid,stime,cmd >> $PSTACK_FILE 
		echo -e $SINGLE_LINE >> $PSTACK_FILE
		top -n 1 | head -7 >> $PSTACK_FILE
		top | grep $pd >> $PSTACK_FILE
		echo -e $SINGLE_LINE >> $PSTACK_FILE
		pstack $pd > $TMP_FILE
		cat $TMP_FILE >> $PSTACK_FILE
		threads=`grep 'Thread ' $TMP_FILE | wc -l`
		main=`grep 'main ' $TMP_FILE | wc -l`
		oracle=`grep Oracle $TMP_FILE | wc -l`
		slaves=`grep CProfileSlave $TMP_FILE | wc -l`
		master=`grep CProfileMaster $TMP_FILE | wc -l`
		echo -e $SINGLE_LINE >> $PSTACK_FILE
		echo -e "THREADS - TOTAL: $threads, MAIN: $main, MASTER: $master, SLAVES: $slaves, IN_ORACLE: $oracle" >> $PSTACK_FILE
		rm $TMP_FILE
		echo -e $DOUBLE_LINE >> $PSTACK_FILE	
	fi
}

echo -e "[INFO] Profile Manager Monitor Script [$SELFPID]" >> $LOG_FILE
echo -e "[INFO] Profile Manager Monitor Script LOG_FILE ["$LOG_FILE"]\n[INFO] Running as on $CURRENTTIME ENTITY_TYPE($ENTITY_TYPE), Host($HOST_NAME), IP($IP_ADDRESS)" >> $LOG_FILE
if [ ! -d $LOG_DIRECTORY ]
then
	echo "[ERROR] NOTIFICATION MANAGER LOG_DIRECTORY : ["$LOG_DIRECTORY"] DOESNT EXIST" >> $LOG_FILE
        exit 2
else
	echo "[INFO] NOTIFICATION MANAGER LOG DIRECTORY["$LOG_DIRECTORY"]" >> $LOG_FILE
fi

OCID=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
set feedback off
set heading off
select ONLINE_COUNTER_ID from PROFILE_MANAGER_ENTITY_CONFIGS where ENTITY_TYPE = '$ENTITY_TYPE' ;

EOF`
OCID=`echo $OCID | sed 's/[\t \n]*//g'`
if ! [[ $OCID =~ $NumericReg ]] ; then
	echo -e "[ERROR] Invalid ONLINE_COUNTER_ID for ENTITY_TYPE [$ENTITY_TYPE]" >> $LOG_FILE
	exit 223
fi
SSPID=`ps -eaf | grep profile_manager | grep rbin | grep PID | grep runscheduledjob | awk -F" " '{ print $2}'`
#SSPID=1
if ! [[ $SSPID =~ $NumericReg ]] ; then
	PROFILEMANAGER_STATUS="NOT RUNNING"
        echo -e "[ERROR] Profile Manager Schedule Script is not running for ENTITY_TYPE[$ENTITY_TYPE]" >> $LOG_FILE
        exit 229
fi

PPSID=`ps -eaf | grep profile_manager | grep rbin | grep ProfilemanagerPID | grep ruby | grep $SSPID | awk -F" " '{ print $2}'`
#PPSID="2"
if ! [[ $PPSID =~ $NumericReg ]] ; then
	PROFILEMANAGER_STATUS="RUBY SCRIPT DOWN"
        echo -e "[ERROR] Profile Manager Ruby Script is not running for ENTITY_TYPE[$ENTITY_TYPE]" >> $LOG_FILE
        exit 230
fi
PID=`ps -eaf | grep profilemanager| grep $PPSID | awk -F" " '{ print $2}'`
#PID="3"
if ! [[ $PID =~ $NumericReg ]] ; then
	PROFILEMANAGER_STATUS="ONLY SCRIPT RUNNING"
        echo -e "[ERROR] Profile Manager Executable not Running for ENTITY_TYPE[$ENTITY_TYPE]" >> $LOG_FILE
        exit 230
fi
echo -e "[INFO] Running Info Schedule Script("$SSPID"), Profile Manager Script("$PPSID")\n[INFO] Profile Manager ("$PID"), Offline Counter($CID)" >> $LOG_FILE

MAX_THREADS=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
set feedback off
set heading off
select MAX_THREADS from profile_manager_entity_configs where OFFLINE_COUNTER_ID = '$CID';
EOF`
MAX_THREADS=`echo $MAX_THREADS | sed 's/[\t \n]*//g'`
if ! [[ $MAX_THREADS =~ $NumericReg ]] ; then
        echo -e "[ERROR] Unable to get MAX_THREADS from DB" >> $LOG_FILE
    exit 205
fi
LAST_RUN_INFO=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
set feedback off
set heading off
ALTER SESSION SET NLS_DATE_FORMAT = 'DD-MON-YYYY HH24:MI:SS';
SELECT (LAST_RUN_REF_TIME - TO_DATE('01-01-1970 00:00:00', 'DD-MM-YYYY HH24:MI:SS')) * 24 * 60 * 60 FROM profile_manager_entity_configs where OFFLINE_COUNTER_ID = '$CID';
EOF`
LAST_RUN_INFO=`echo $LAST_RUN_INFO | sed 's/[\t \n]*//g'`
if ! [[ $LAST_RUN_INFO =~ $NumericReg ]] ; then
        echo -e "[ERROR] Unable to get LAST_RUN_REF_TIME[$LAST_RUN_REF_TIME] for OFFLINE_COUNTER_ID[$CID] from DB" >> $LOG_FILE
    exit 245
fi
echo -e "[INFO] MAX_THREADS($MAX_THREADS), LAST_RUN_REF_TIME($LAST_RUN_INFO | $(date -d @$LAST_RUN_INFO))" >> $LOG_FILE

echo -e $DOUBLE_LINE >> $LOG_FILE
echo -e "[INFO] INFORMATION ABOUT PROFILE COUNTER(ID:$CID)" >> $LOG_FILE

COUNTER_DETAILS=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
set feedback off
set heading off
select COUNTER_INSTANCE_ID||'|'||COUNTER_CATEGORY||'|'||NUMBER_OF_SLAVES||'|'||NUMBER_OF_PARTITIONS_PER_SLAVE||'|'||CACHE_DUMP_DESTINATION||'|'||DELTA_FILE_PATH||'|'||PR_FILE_PATH||'|'||PR_HASH_BUCKET_SIZE FROM IN_MEMORY_COUNTER_DETAILS WHERE ID= '$CID';
EOF`
COUNTER_DETAILS=`echo $COUNTER_DETAILS | sed 's/[\t \n]*//g'`
NUMBER_OF_SLAVES=`echo $COUNTER_DETAILS | awk -F"|" '{print $3 }'`
NUMBER_OF_PARTITIONS_PER_SLAVE=`echo $COUNTER_DETAILS | awk -F"|" '{print $4 }'`
CACHE_DUMP_DESTINATION=`echo $COUNTER_DETAILS | awk -F"|" '{print $5 }'`
DELTA_FILE_PATH=`echo $COUNTER_DETAILS | awk -F"|" '{print $6 }'`
PR_FILE_PATH=`echo $COUNTER_DETAILS | awk -F"|" '{print $7 }'`
PR_HASH_BUCKET_SIZE=`echo $COUNTER_DETAILS | awk -F"|" '{print $8 }'`

if ! [[ $NUMBER_OF_SLAVES =~ $NumericReg ]] ; then
    echo -e "[ERROR] Unable to get NUMBER_OF_SLAVES[$NUMBER_OF_SLAVES] for Profile Counter $CID from DB" >> $LOG_FILE
    exit 207
else
	echo -e "[INFO] Profile Counter NUMBER_OF_SLAVES($NUMBER_OF_SLAVES)" >> $LOG_FILE
fi
if ! [[ $NUMBER_OF_PARTITIONS_PER_SLAVE =~ $NumericReg ]] ; then
    echo -e "[ERROR] Unable to get NUMBER_OF_PARTITIONS_PER_SLAVE[$NUMBER_OF_PARTITIONS_PER_SLAVE] for Profile Counter $CID from DB" >> $LOG_FILE
    exit 208
else
	echo -e "[INFO] Profile Counter NUMBER_OF_PARTITIONS_PER_SLAVE($NUMBER_OF_PARTITIONS_PER_SLAVE)" >> $LOG_FILE
fi
if ! [[ $PR_HASH_BUCKET_SIZE =~ $NumericReg ]] ; then
    echo -e "[ERROR] Unable to get PR_HASH_BUCKET_SIZE[$PR_HASH_BUCKET_SIZE] for Profile Counter $CID from DB" >> $LOG_FILE
    exit 209
else
	echo -e "[INFO] Profile Counter PR_HASH_BUCKET_SIZE($PR_HASH_BUCKET_SIZE)" >> $LOG_FILE
fi
if [ ! -d $CACHE_DUMP_DESTINATION ]; then
    echo -e "[ERROR] Invalid Directory CACHE_DUMP_DESTINATION[$CACHE_DUMP_DESTINATION] for Profile Counter $CID from DB" >> $LOG_FILE
    exit 210
else
	echo -e "[INFO] Profile Counter CACHE_DUMP_DESTINATION($CACHE_DUMP_DESTINATION)" >> $LOG_FILE
fi
if [ ! -d $DELTA_FILE_PATH ]; then
    echo -e "[ERROR] Invalid Directory DELTA_FILE_PATH[$DELTA_FILE_PATH] for Profile Counter $CID from DB" >> $LOG_FILE
    exit 211
else
	echo -e "[INFO] Profile Counter DELTA_FILE_PATH($DELTA_FILE_PATH)" >> $LOG_FILE
fi

if [ ! -d $PR_FILE_PATH ]; then
    	echo -e "[ERROR] Invalid Directory PR_FILE_PATH[$PR_FILE_PATH] for Profile Counter $CID from DB" >> $LOG_FILE
    	exit 212
else
	echo -e "[INFO] Profile Counter PR_FILE_PATH($PR_FILE_PATH)" >> $LOG_FILE
fi

echo -e $DOUBLE_LINE >> $LOG_FILE
echo -e "[INFO] INFORMATION ABOUT STATS GATHERING COUNTER($OCID)" >> $LOG_FILE
CheckStateGathingCounterRunning $OCID
if ! [[ $OPID =~ $NumericReg ]] ; then
    	echo -e "[ERROR] STATS GATHERING COUNTER($OCID) is not running" >> $LOG_FILE
else
	echo -e "[INFO] STATS GATHERING COUNTER($OCID) is running with pid($OPID)" >> $LOG_FILE
fi

ODELTA_FILE_PATH=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
set feedback off
set heading off
select DELTA_FILE_PATH FROM IN_MEMORY_COUNTER_DETAILS WHERE ID= '$OCID';
EOF`
ODELTA_FILE_PATH=`echo $ODELTA_FILE_PATH | sed 's/[\t \n]*//g'`

if [ ! -d $ODELTA_FILE_PATH ]; then
    	echo -e "[ERROR] Invalid Directory Online DELTA_FILE_PATH[$ODELTA_FILE_PATH] for Online Counter($OCID)" >> $LOG_FILE
    	exit 111
else
	echo -e "[INFO] Directory Online DELTA_FILE_PATH[$ODELTA_FILE_PATH] for Online Counter($OCID)" >> $LOG_FILE	
fi

echo -e $DOUBLE_LINE >> $LOG_FILE
echo -e "[INFO] Files Related Statistics for Stats Gathering Counter Manager" >> $LOG_FILE
echo -e $DOUBLE_LINE >> $FILE_LIST
echo -e "[INFO] DELTA FILES UNDER $ODELTA_FILE_PATH" >> $FILE_LIST
echo -e $SINGLE_LINE >> $FILE_LIST
cd $ODELTA_FILE_PATH
ls -ltr  >> $FILE_LIST
cd $PWD
echo -e $DOUBLE_LINE >> $FILE_LIST
oldtime=`ls -ltr $ODELTA_FILE_PATH/cache_* | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
newtime=`ls -ltr $ODELTA_FILE_PATH/cache_* | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
echo -e "[INFO] DELTA PATH[$ODELTA_FILE_PATH] Total Files: $(ls -ltr $ODELTA_FILE_PATH/cache_* | wc -l)\n[INFO] Oldest File Timestamp: $oldtime, Newest File Timestamp:$newtime" >> $LOG_FILE

echo -e $DOUBLE_LINE >> $LOG_FILE
echo -e "[INFO] Files Related Statistics for Profile Manager\n" >> $LOG_FILE

# Cache Delta Directory
cd $DELTA_FILE_PATH
cache_detla_total=$(ls cache_* | wc -l)
echo -e "[INFO] CACHE DELTA[$DELTA_FILE_PATH] Total Files: $cache_detla_total" >> $LOG_FILE
if [[ $cache_detla_total =~ $NumericReg ]] ; then
	if [ $cache_detla_total -gt 0 ]; then 
		echo -e "[INFO] FILES UNDER $DELTA_FILE_PATH" >> $FILE_LIST
		echo -e $SINGLE_LINE >> $FILE_LIST
		ls -ltr >> $FILE_LIST
		echo -e $DOUBLE_LINE >> $FILE_LIST
		for index in $(seq -w 00 $(($NUMBER_OF_PARTITIONS_PER_SLAVE-1)));
		do
			echo -e `pwd`
			echo -e "[INFO] DELTA Statistics For Partition($index)" >> $LOG_FILE
			flcnt=`ls cache_000${index}_* | wc -l`
			if [[ $flcnt =~ $NumericReg ]] ; then
        		if [ $flcnt -gt 0 ]; then
				oldst=`ls -ltr cache_000${index}_* | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
				newst=`ls -ltr cache_000${index}_* | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
				echo -e "[INFO] Partition [$index], Total Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
			fi
			fi
			ls -ltr cache_000${index}* >> $LOG_FILE
		done
	fi
fi
echo -e $SINGLE_LINE >> $LOG_FILE
cd $DELTA_FILE_PATH/archive
if [ ! -d $DELTA_FILE_PATH/archive ]; then
	echo -e "[ERROR] Cache Delta Archive Directory($DELTA_FILE_PATH/archive) is Missing" >> $LOG_FILE
else
	cnt=`ls cache_* | wc -l`
	echo -e "[INFO] Cache Delta Archive Directory($DELTA_FILE_PATH/archive), Total Files: $cnt" >> $LOG_FILE
	if [ $cnt -gt 3000 ]
	then
		echo -e "[WARN] Cleanup For Cache Delta Archive($DELTA_FILE_PATH/archive) is not Happening, HIGH_FILE_COUNT" >> $LOG_FILE
	fi
	ls -ltr cache_* | tail -10 >> $LOG_FILE
fi
cd $PWD

# PR Directory
echo -e $SINGLE_LINE >> $LOG_FILE
cd $PR_FILE_PATH
echo -e "[INFO] ParticipatedRecords[$PR_FILE_PATH] Total Files[C|I]: $(ls cache_* | wc -l)|$(ls index_* | wc -l)" >> $LOG_FILE
echo -e "[INFO] FILES UNDER $PR_FILE_PATH" >> $FILE_LIST
echo -e $SINGLE_LINE >> $FILE_LIST
cd $PR_FILE_PATH 
ls -ltr >> $FILE_LIST
echo -e $DOUBLE_LINE >> $FILE_LIST
for index in $(seq -w 00 $(($NUMBER_OF_PARTITIONS_PER_SLAVE-1)));
do
	echo -e `pwd`
	echo -e "[INFO] ParticipatedRecords Statistics For Partition($index)" >> $LOG_FILE
        flcnt=`ls *_000${index}_* | wc -l`
	if [[ $flcnt =~ $NumericReg ]] ; then
        if [ $flcnt -gt 0 ]; then
        	oldst=`ls -ltr *_000${index}_* | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	newst=`ls -ltr *_000${index}_* | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	echo -e "[INFO] Partition [$index] Total Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
	fi
	fi
	flcnt=`ls cache_000${index}_* | wc -l`
	if [[ $flcnt =~ $NumericReg ]] ; then
        if [ $flcnt -gt 0 ]; then
	        oldst=`ls -ltr cache_000${index}_* | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	newst=`ls -ltr cache_000${index}_* | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
		echo -e "[INFO] Partition [$index] Total Cache Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
	fi
	fi
	flcnt=`ls cache_000${index}_*.TMP | wc -l`
        if [[ $flcnt =~ $NumericReg ]] ; then
        if [ $flcnt -gt 0 ]; then
		oldst=`ls -ltr cache_000${index}_*.TMP | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	newst=`ls -ltr cache_000${index}_*.TMP | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
	        echo -e "[INFO] Partition [$index] Total TMP Cache Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
	fi
	fi
	flcnt=`ls index_000${index}* | wc -l`
        if [[ $flcnt =~ $NumericReg ]] ; then
        if [ $flcnt -gt 0 ]; then
		oldst=`ls -ltr index_000${index}_* | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	newst=`ls -ltr index_000${index}_* | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
		echo -e "[INFO] Partition [$index] Total Index Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
	fi
	fi
	flcnt=`ls -ltr index_000${index}*.TMP | wc -l`
	if [[ $flcnt =~ $NumericReg ]] ; then
        if [ $flcnt -gt 0 ]; then
	        oldst=`ls -ltr index_000${index}_*.TMP | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	newst=`ls -ltr index_000${index}_*.TMP | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
	        echo -e "[INFO] Partition [$index] Total TMP Index Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
	fi
	fi
	ls -ltr cache_000${index}_* >> $LOG_FILE
	ls -ltr index_000${index}_* >> $LOG_FILE
done
cd $PWD

# DUMP Directory
cd $CACHE_DUMP_DESTINATION
echo -e "[INFO] DUMP[$CACHE_DUMP_DESTINATION] Total Files: $(ls cache_* | wc -l)" >> $LOG_FILE
echo -e "[INFO] FILES UNDER $CACHE_DUMP_DESTINATION" >> $FILE_LIST
echo -e $SINGLE_LINE >> $FILE_LIST
ls -ltr >> $FILE_LIST
echo -e $DOUBLE_LINE >> $FILE_LIST
for index in $(seq -w 00 $(($NUMBER_OF_PARTITIONS_PER_SLAVE-1)));
do
        echo -e "[INFO] DUMP Statistics For Partition($index)" >> $LOG_FILE
	flcnt=`ls cache_000${index}_* | wc -l`
	if [[ $flcnt =~ $NumericReg ]] ; then
        if [ $flcnt -gt 0 ]; then
	        oldst=`ls -ltr cache_000${index}_* | head -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	newst=`ls -ltr cache_000${index}_* | tail -1 | awk -F" " '{ print $6" "$7" "$8 }'`
        	echo -e "[INFO] Partition [$index], Total Files: $flcnt, Oldest File Time: $oldst, Newest File Time: $newst" >> $LOG_FILE
	fi
	fi
	ls -ltr cache_000${index}* >> $LOG_FILE
done
cd $PWD
echo -e "[INFO] Collecting PSTACK of Profile Manager($PID) into PSTACK_FILE[$PSTACK_FILE]" >> $LOG_FILE
GetPstackFrame $PID
# Analysing NM Logs
echo -e $DOUBLE_LINE >> $LOG_FILE
echo -e "[INFO] Profile Manager($PID) Logs in Notification Manager Logs" >> $LOG_FILE
echo -e $SINGLE_LINE >> $LOG_FILE
> $TMP_FILE
cd $LOG_DIRECTORY
for index in 10 9 8 7 6 5 4 3 2 1 0
do
	if [ $index -ne 0 ]; then
		grep profilemanager all_modules_all_users_all_ips.$index.log | grep $PID >> $TMP_FILE
	else
		grep profilemanager all_modules_all_users_all_ips.log | grep $PID  >> $TMP_FILE
        fi
done
cd $PWD

cat $TMP_FILE >> $LOG_FILE
echo -e $DOUBLE_LINE >> $LOG_FILE
errors=`grep ERROR $TMP_FILE | wc -l`
echo -e "[INFO] Total Errors : $errors" >> $LOG_FILE
grep ERROR $TMP_FILE >> $LOG_FILE

echo -e $DOUBLE_LINE >> $LOG_FILE
echo -e "[INFO] Observation Based on Logs : Profile Manager Slaves & Partitions" >> $LOG_FILE
alarmgenerator=`grep "Client socket successfully connected to server" $TMP_FILE | awk -F"server" '{ print $2}' | awk -F", FD" ' {print $1 }' | sort -u | head -1`
noofthreads=`grep "Client socket successfully connected to server" $TMP_FILE | awk -F", FD:" '{ print $2}' | awk -F"]]" ' {print $1 }' | sort -u | wc -l`
sockets=`grep "Client socket successfully connected to server" $TMP_FILE | awk -F", FD:" '{ print $2}' | awk -F"]]" ' {print $1 }' | sort -u | tr '\n' ', '`
echo -e "[INFO] Profile Manager [$noofthreads] Threads Connected to Alarm Generator on $alarmgenerator] on Sockets [$sockets]" >> $LOG_FILE
echo -e "[Slave Thread]\tPartition\t[IM-PR Cache Building]\t[PR Store Building]\t[Slave Evaluation]\t[Slave Released]\tSlave Id" >> $LOG_FILE
for index in $(seq 0 $(($NUMBER_OF_PARTITIONS_PER_SLAVE-1)));
do
	slave_thread=`grep "IM and PR Cache building completed for partition $index]" $TMP_FILE | awk -F"[" '{ print $10}' | awk -F"]" ' {print $1 }'`
	cache_building=`grep "IM and PR Cache building completed for partition $index]" $TMP_FILE | awk -F"[" '{ print $3}' | awk -F"]" ' {print $1 }' | awk -F" " '{ print $1" "$2" "$3" "$4}'`
	store_building=`grep "PR Store building completed for partition $index]" $TMP_FILE | awk -F"[" '{ print $3}' | awk -F"]" ' {print $1 }' | awk -F" " '{ print $1" "$2" "$3" "$4}'`
	slave_evaluation=`grep "Profile slave completed evaluation for partition $index]" $TMP_FILE | awk -F"[" '{ print $3}' | awk -F"]" ' {print $1 }' | awk -F" " '{ print $1" "$2" "$3" "$4}'`
	slave_released=`grep "Profileslave about to release for slave" $TMP_FILE | grep "and partition $index]" | awk -F"[" '{ print $3}' | awk -F"]" ' {print $1 }' | awk -F" " '{ print $1" "$2" "$3" "$4}'`
	sliave_id=`grep "Profileslave about to release for slave index " $TMP_FILE | grep "partition $index]" | awk -F"for slave index " '{print $2}' | awk -F" and partition " '{ print $1}'`
        echo -e "[$slave_thread]\t$index\t[$cache_building]\t[$store_building]\t[$slave_evaluation]\t[$slave_released]\t[$sliave_id]" >> $LOG_FILE
done
masterthreadid=`grep "Profile slave got releases for slave index" $TMP_FILE | awk -F"[" '{ print $10}' | awk -F"]" '{ print $1}' | sort -u | head`
releasedslaves=`grep "Profile slave got releases for slave index" $TMP_FILE | awk -F"slave index " '{ print $2}' | awk -F")" ' {print $1")" }' | tr '\n' ', '`
echo -e "\n[INFO] MASTER THREAD[$masterthreadid] Released Slave Indexes $releasedslaves" >> $LOG_FILE
#oldreftime=
#newreftime=
#echo -e "\n[INFO] Last Run Info BEFORE[$oldreftime], NOW[$newreftime]" >> $LOG_FILE 
echo -e $DOUBLE_LINE >> $LOG_FILE
function GetPstacks(){
	# Collecting Pstacks
	echo -e "[INFO] Collecting PSTACKS of Profile Manager($PID) into PSTACK_FILE[$PSTACK_FILE]" >> $LOG_FILE
	for index in $(seq 1 $NUMBER_OF_PSTACK);
	do
		GetPstackFrame $PID
		sleep $SLEEP_INT
	done
	#cat $PSTACK_FILE >> $LOG_FILE
	echo -e $DOUBLE_LINE >> $PSTACK_FILE
	#echo -e $DOUBLE_LINE >> $LOG_FILE
}
GetPstacks

#[    INFO] <LogData> [Mon Feb 13 02:22:22 2017] [Server:/rocfm/NIKIRAROOT//sbin/profilemanager] [profilemaster.cc] [172.17.10.11] [27053] [profilemanager] [] [3647308896] [(UnInitialize) profilemaster.cc:493 - Updating Last Run Info Time - (2017/02/12 17:59:59) for Entity Type (2)]
#[    INFO] <LogData> [Sun Feb 12 23:40:42 2017] [Server:/rocfm/NIKIRAROOT//sbin/profilemanager] [statisticalrulefunctionif.cc] [172.17.10.11] [27053] [profilemanager] [] [2999679296] [(Initialise) statisticalrulefunctionif.cc:209 - Stats Rule Evaluator uses (12/02/2017 17:59:59) as reference time]
