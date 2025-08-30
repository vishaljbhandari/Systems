#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#!/bin/bash

TempConfigValue=""

on_exit ()
{
	if [ -f $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID ]; then
		PID=`cat $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID`
		if [ $PID -eq $$ ]; then
			rm -f $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID
		fi
	fi
}
trap on_exit EXIT

if [ $# -ne 1 ]
then
	echo >&2 "Usage DUTerminalUsageInformationLoader.sh <start/stop>" && exit 1
fi

Status="$1" 

LoadFileToTempDB ()
{
	CONNECT_TO_SQLLDR
	 	SILENT=feedback \
		CONTROL="$RANGERHOME/share/Ranger/DUTerminalUsageInformationLoader.ctl" \
		DATA="$RANGERHOME/tmp/DUTerminalUsageInformation.txt" \
		LOG="$RANGERHOME/LOG/DUTerminalUsageInformation.log" \
		BAD="$RANGERHOME/LOG/DUTerminalUsageInformation.bad" \
		DISCARD="$RANGERHOME/LOG/DUTerminalUsageInformation.dsc" ERRORS=999999 > /dev/null 2>&1

	Ret=$?
	if [ $Ret -ne 0 ]; then
		return 1
	fi
}

SummarizeTempTable ()
{
	sqlplus -s /nolog << EOF  > /dev/null 2>&1
	CONNECT_TO_SQL

		set serveroutput off ;
		set feedback off ;
		declare
			is_match			number (2) := 0 ;
			m_tac_code			varchar2(20) ;
			first_day_of_use	date := null ;
			last_day_of_use		date := null ;
			date_diff			number(4) := 0 ;

			days_of_use			varchar2(1024) := null ;
			first_date_of_use	date := null ;
			distinct_days		number := 0 ;
			last_date_of_use	date := null ;

			cursor cdr_rows is select * from temp_du_terminal_usage_info ;

		begin
			for cdr_rec in cdr_rows loop
				m_tac_code := substr(cdr_rec.imei, 1, 8) ;	
				is_match := 0 ;
				begin
					select 
						1,days_used,first_date_of_usage,last_date_of_usage,distinct_days_used
							into is_match, days_of_use,first_date_of_use,last_date_of_use,distinct_days
					from 
						du_terminal_usage_info
					where 
						phone_number = cdr_rec.phone_number and
						imsi = cdr_rec.imsi and
						tac_code = m_tac_code ;
				exception
					when no_data_found then
						is_match := 0 ;
				end ;

				if is_match = 0 then
					insert into du_terminal_usage_info
						(phone_number, imsi, tac_code, distinct_days_used, first_date_of_usage, last_date_of_usage,imei,days_used)
					values
						(cdr_rec.phone_number, cdr_rec.imsi, m_tac_code, 1, cdr_rec.time_stamp, cdr_rec.time_stamp,cdr_rec.imei,to_char(cdr_rec.time_stamp,'ddd')) ;
				else
					date_diff := trunc(cdr_rec.time_stamp) - trunc(last_date_of_use) ;
					
					if ( date_diff < 0 ) then
						if ( days_of_use not like '%,' || to_char(cdr_rec.time_stamp, 'ddd') || ',%' ) then
							if ( trunc(first_date_of_use) > trunc(cdr_rec.time_stamp) ) then
								first_date_of_use := cdr_rec.time_stamp ;
								days_of_use := days_of_use ||','|| to_char(cdr_rec.time_stamp, 'ddd') ;
							end if ;
							distinct_days := distinct_days + 1 ;
						end if ;
					else
						if ( date_diff > 0 ) then
						last_date_of_use := cdr_rec.time_stamp ;
						distinct_days := distinct_days + 1 ;
						days_of_use := days_of_use ||','|| to_char(cdr_rec.time_stamp, 'ddd') ;
						end if ;
					end if ;

					update du_terminal_usage_info
						set last_date_of_usage = last_date_of_use,
							first_date_of_usage = first_date_of_use,
							distinct_days_used = distinct_days,
							days_used = days_of_use
					where
						phone_number = cdr_rec.phone_number and imsi = cdr_rec.imsi and tac_code = m_tac_code ;
				end if ;
			end loop ;
		end ;
/
EOF
}

GetConfigEntry ()
{
sqlplus -s /nolog << EOF > /dev/null 2>&1
	CONNECT_TO_SQL
	set heading off ;
	set feedback off ;
	spool $RANGERHOME/tmp/spooleddata.dat
	select value from configurations where config_key = '$1' ;
EOF
	TempConfigValue=`awk 'NF > 0' $RANGERHOME/tmp/spooleddata.dat | tr -s ' ' | tr -d ' '`
	if [ "x""$TempConfigValue" == "x" ]; then
		return 1
	fi
	return 0
}

CreateOrUpdateLOG ()
{
	LogFile="TerminalUsageInformationLoader.log"
	LogFiles=$1
	OriginalFilename=$2
	grep "^Run" $RANGERHOME/LOG/$LogFiles.log | sed 's/^.\{1,13\}//' > $RANGERHOME/tmp/timestamp.tmp
	echo "                     **************************************************************" >> $RANGERHOME/LOG/$LogFile
	echo "                            Terminal Usage Loader Log" >> $RANGERHOME/LOG/$LogFile
	echo "                     **************************************************************" >> $RANGERHOME/LOG/$LogFile
	echo    "File Name   : $OriginalFilename" >> $RANGERHOME/LOG/$LogFile
	echo -n "Start Time  : " >> $RANGERHOME/LOG/$LogFile 
	head -1 $RANGERHOME/tmp/timestamp.tmp >> $RANGERHOME/LOG/$LogFile
	echo -n "End Time    : " >> $RANGERHOME/LOG/$LogFile 
	head -1 $RANGERHOME/tmp/timestamp.tmp >> $RANGERHOME/LOG/$LogFile
	echo -n "Duration    : " >> $RANGERHOME/LOG/$LogFile
	tail -2 $RANGERHOME/LOG/$LogFiles.log | head -1 | tr -d ' ' | cut -f2- -d':' >> $RANGERHOME/LOG/$LogFile
	echo "                     --------------------------------------------------------------" >> $RANGERHOME/LOG/$LogFile
	grep "^Total logical records " $RANGERHOME/LOG/$LogFiles.log | sed 's/^.\{1,22\}//' | tr -d ' ' | cut -f2 -d':' > $RANGERHOME/tmp/timestamp.tmp
	echo -n "   Total Records Processed : " >> $RANGERHOME/LOG/$LogFile
	head -2 $RANGERHOME/tmp/timestamp.tmp | tail -1 >> $RANGERHOME/LOG/$LogFile
	echo -n "   Total Records Accepted  : " >> $RANGERHOME/LOG/$LogFile
	grep ".*Row* successfully loaded.$" $RANGERHOME/LOG/$LogFiles.log | sed 's/^ *//' | cut -f1 -d' ' >> $RANGERHOME/LOG/$LogFile
	echo -n "   Total Records Rejected  : " >> $RANGERHOME/LOG/$LogFile
	head -3 $RANGERHOME/tmp/timestamp.tmp | tail -1 >> $RANGERHOME/LOG/$LogFile
	echo -n "   Total Records Discarded : " >> $RANGERHOME/LOG/$LogFile
	head -4 $RANGERHOME/tmp/timestamp.tmp | tail -1 >> $RANGERHOME/LOG/$LogFile
	if [ -s $RANGERHOME/LOG/$LogFiles.bad ];then
		RejLines=`cat $RANGERHOME/LOG/$LogFiles.bad | sed -n "$="`
		if [ $RejLines -gt 0 ]; then 
			echo "                     --------------------------------------------------------------" >> $RANGERHOME/LOG/$LogFile
			echo "                                Rejected ..." >> $RANGERHOME/LOG/$LogFile
			echo "                     --------------------------------------------------------------" >> $RANGERHOME/LOG/$LogFile
			cat $RANGERHOME/LOG/$LogFiles.bad >> $RANGERHOME/LOG/$LogFile
		fi
	fi
	if [ -s $RANGERHOME/LOG/$LogFiles.dsc ];then
		RejLines=`cat $RANGERHOME/LOG/$LogFiles.dsc | sed -n "$="`
		if [ $RejLines -gt 0 ]; then 
			echo "                     --------------------------------------------------------------" >> $RANGERHOME/LOG/$LogFile
			echo "                                Discarded ..." >> $RANGERHOME/LOG/$LogFile
			echo "                     --------------------------------------------------------------" >> $RANGERHOME/LOG/$LogFile
			cat $RANGERHOME/LOG/$LogFiles.dsc >> $RANGERHOME/LOG/$LogFile
		fi
	fi
	echo -e "                     **************************************************************\n\n\n\n" >> $RANGERHOME/LOG/$LogFile
	rm -f $RANGERHOME/LOG/$LogFiles.*
	rm -f $RANGERHOME/tmp/timestamp.tmp
}

main ()
{
	ConfigID=$1
	GetConfigEntry "$ConfigID"
	if [ $? -eq 0 ]; then
		INPUT_PATH=$TempConfigValue
	else
		logger "DUTerminalUsageInformationLoader - Unable To Find Config Entry $ConfigID [Terminal Usage Summary Script Exiting] ..."
		exit 1
	fi

	if [ ! -d $INPUT_PATH -o ! -d $INPUT_PATH/success ]; then
		logger "DUTerminalUsageInformationLoader - Unable To Find $INPUT_PATH or $INPUT_PATH/success [Terminal Usage Summary Script Exiting] ..."
		exit 2
	fi

	for File in `ls -rt $INPUT_PATH/success`
	do
		"$RANGERHOME/bin/DUTerminalUsageInformationCDRProcessor.pl" $INPUT_PATH/$File $RANGERHOME/tmp/DUTerminalUsageInformation.txt

		LoadFileToTempDB
		retcode=$?
		
		CreateOrUpdateLOG "DUTerminalUsageInformation" $File

		case "$retcode" in 

		0)
			logger "SQL*Loader execution successful"
			;; 
		1)
			logger "SQL*Loader execution encountered an Oracle error" 
			retcode=0
			;; 
		2) 
			logger "All/some rows rejected/discarded."
			logger "Please check the logfile [$RANGERHOME/LOG/DUTerminalUsageInformationLoader.log] for details"
			retcode=0
			;; 
		3) 
			logger "SQL*Loader execution encountered a fatal OS error" 
			;; 
		*) 
			logger "SQL*Loader encountered an unknown error" 
			;; 
		esac	

		if [ $retcode -ne 0 ]; then
			logger "Unable To Load File ! [$INPUT_PATH/$File] Contact Ranger Admin !"
			exit 1
		fi

		if [ $retcode -eq 0 ]; then
			rm -f $INPUT_PATH/$File
		fi
		rm -f $INPUT_PATH/success/$File
		rm -f $RANGERHOME/tmp/DUTerminalUsageInformation.txt

		SummarizeTempTable
	done
}

if [ "$Status" == "stop" ]; then
	if [ -f $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID ]; then
		kill -9 `cat $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID`
		rm -f $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID
		echo "DUTerminalUsageInformationLoader Stopped Successfuly ..."
		exit 0 
	else
		echo "No Such Process ... "
		exit 1
	fi
elif [ "$Status" == "start" ]; then
	if [ ! -f $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID ]; then
		echo "DUTerminalUsageInformationLoader Started Successfuly ..."
		echo $$ > $RANGERHOME/share/Ranger/DUTerminalUsageInformationLoaderPID
		while true
		do
			main "TERMINAL_USAGE_DATA_INPUT"
		done
	else
		echo "DUTerminalUsageInformationLoader Already Running ..."
		exit 2
	fi
else
	echo >&2 "Usage DUTerminalUsageInformationLoader.sh <start/stop>" && exit 1 
	exit 3
fi
