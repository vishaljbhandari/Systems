#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#!/bin/bash

on_exit ()
{
	rm -f $RANGERHOME/RangerData/Scheduler/DUOldestProcessedFileByDSStreamPID$NetworkID
}
trap on_exit EXIT 


 ## CONFIGURE THE FOLLOWING ###
 . rangerenv.sh
 DIAMOND_DB='nikira_spark' ;
 ## CONFIGURATION END ###

NetworkID=$1
TimeStamp=$2

LoadFileToTempDB ()
{
	InputFileName=$1
	CONNECT_TO_SQLLDR
		SILENT=(all) \
		CONTROL="$RANGERHOME/share/Ranger/DUOldestProcessedFileByDSStream.ctl" \
		DATA=$InputFileName \
		LOG="$RANGERHOME/tmp/DUOldestProcessedFileByDSStream.log" \
		BAD="$RANGERHOME/tmp/DUOldestProcessedFileByDSStream.bad" \
		DISCARD=$RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dsc ERRORS=999999 > $RANGERHOME/tmp/DUOldestProcessedFileByDSStreamMain.log 2>&1
	Ret=$?
	if [ $Ret -ne 0 ]; then
		return 1
	fi
}

SelectProcessedFilesLogByStreamID ()
{
	DS_ID=$1
	if [ $DS_ID -gt 0 ]; then
		sqlplus -s /nolog << EOF > $RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dat 2>&1
		CONNECT_TO_SQL
			whenever sqlerror exit 5 ;
			whenever oserror exit 6 ;
			set heading off ;
			set pagesize 30000 ;
			select file_name || ',' || file_size || ',' || to_char (file_timestamp, 'yyyy/mm/ddhh24:mi:ss') || ',' || source
				from rip_processed_file_info
			where trunc(file_processing_end_time) = to_date (nvl('$TimeStamp', to_char(sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy')
				and source = $DS_ID ;
EOF
	else
		sqlplus -s /nolog << EOF > $RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dat 2>&1
		CONNECT_TO_SQL
			whenever sqlerror exit 5 ;
			whenever oserror exit 6 ;
			set heading off ;
			set pagesize 30000 ;
			select file_name || ',' || file_size || ',' || to_char (file_timestamp, 'yyyy/mm/ddhh24:mi:ss') || ',' || source
				from du_data_file_processed
			where trunc(file_timestamp) = to_date (nvl('$TimeStamp', to_char(sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy')
				and source = $DS_ID ;
EOF
	fi

	if [ $? -ne 0 ]; then
		logger "DUOldestProcessedFileByDSStream : [E] Processed Files Details Summarising Failed!"
		return 1
	fi

	grep "no rows selected" $RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dat > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		logger "DUOldestProcessedFileByDSStream : [W] No Data Avilable"
		> $RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dat
		return 2
	fi
	grep -v "^$" $RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dat > $RANGERHOME/tmp/DUOldestProcessedFileByDSStreamTemp.dat
	grep -v "rows selected" $RANGERHOME/tmp/DUOldestProcessedFileByDSStreamTemp.dat > $RANGERHOME/tmp/DUOldestProcessedFileByDSStreamClean.dat
	rm -f $RANGERHOME/tmp/DUOldestProcessedFileByDSStreamTemp.dat
	rm -f $RANGERHOME/tmp/DUOldestProcessedFileByDSStream.dat
	return 0
}

ProcessRIPLogTable ()
{
	sqlplus -s /nolog << EOF > $RANGERHOME/tmp/all_ds_streams 2>&1
	CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		set heading off ;
		select ds_id || ',' || regex from du_file_name_sequance_regex ;
EOF
	if [ $? -ne 0 ]; then
		logger "DUOldestProcessedFileByDSStream : [E] Unable To Gat Datastream ID's"
		return 1
	fi

	grep "no rows selected" $RANGERHOME/tmp/all_ds_streams > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		logger "DUOldestProcessedFileByDSStream : [W] No RegEx Configuration Avilable"
		return 0
	fi

	grep -v "^$" $RANGERHOME/tmp/all_ds_streams > $RANGERHOME/tmp/ds_streams
	rm -f $RANGERHOME/tmp/all_ds_streams

	for IDAndRegEX in `cat $RANGERHOME/tmp/ds_streams`
	do
		ID=`echo $IDAndRegEX | cut -d',' -f1`
		RegEx=`echo $IDAndRegEX | cut -d',' -f2`
		> $RANGERHOME/tmp/RecoedWithSequanceNumbers.dat
		SelectProcessedFilesLogByStreamID $ID
		if [ $? -eq 0 ]; then
			for Record in `cat $RANGERHOME/tmp/DUOldestProcessedFileByDSStreamClean.dat`
			do
				FileRecordSequance=`echo $Record | cut -d',' -f1 | sed "$RegEx"`
				if [ ["$FileRecordSequance" != ""] -a ["$FileRecordSequance" != "$FileRecordSequance"] ]; then
					echo "$Record,$FileRecordSequance" >> $RANGERHOME/tmp/RecoedWithSequanceNumbers.dat
				fi
			done
			LoadFileToTempDB $RANGERHOME/tmp/RecoedWithSequanceNumbers.dat
			sqlplus -s /nolog << EOF
			CONNECT_TO_SQL
				insert into du_oldest_file_processed
					select to_date (nvl('$TimeStamp', to_char(sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy'),
							'$ID', file_name, sequance_number, file_timestamp
						from temp_du_processed_files 
						where sequance_number = (select min(sequance_number) from temp_du_processed_files) ;
				delete from du_data_file_processed where file_timestamp < sysdate - 2 ;
EOF
			rm -f $RANGERHOME/tmp/RecoedWithSequanceNumbers.dat
		else
			logger "Summarization Failed For ID [$ID] - Unknown Reasons ..."
		fi
	done
}

ProcessFixedLineSummary()
{
	sqlplus -s /nolog << EOF
	CONNECT_TO_SQL
	insert into du_oldest_file_processed(process_date, ds_id, file_name, file_timestamp, sequance_number)
	(select to_date (nvl('$TimeStamp', to_char(sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy'),
	(select source from DU_SOURCE_DESCRIPTION where description = 'ss7ds'), a.fil_filename,  to_date (to_char
	(z.time, 'YYYY-MON-DD HH24.MI.SS'), 'YYYY-MON-DD HH24.MI.SS'), a.fil_id from $DIAMOND_DB.FILE_TBL a,
	(select distinct(b.tsk_id) tsk_id, row_number() over (order by c.tsk_created_dttm desc) rank,  c.tsk_created_dttm time ,
	b.fil_id
	from $DIAMOND_DB.TASK_OUTPUT_FILE b, $DIAMOND_DB.task c
	WHERE b.tsk_id = c.tsk_id
	and trunc(c.tsk_created_dttm) = to_date(nvl('$TimeStamp', to_char(sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy')
	order by c.tsk_created_dttm desc) z
	where a.fil_id = z.fil_id and z.rank = 1) ;

	commit ;
EOF

}

ProcessRIPLogTable
if [ $? -ne 0 ]; then
	echo "Summary Run Failed ..."
fi

ProcessFixedLineSummary

