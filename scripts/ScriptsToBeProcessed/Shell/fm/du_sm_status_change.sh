#!/bin/bash

SUBSCRIBER_REFERENCE_TYPE=1

Status=$1
ActionLevel=$2
Reason=$3
AlarmID=$4
ReferenceType=$6

Reason=`echo $Reason | perl -pi -e "s#'#''#g"`

. rangerenv.sh

PrepareMessageForContract ()
{
	sqlplus -s /nolog << EOF > /dev/null 2>&1
	CONNECT_TO_SQL
		spool $RANGERHOME/tmp/spooleddata.dat
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		set serveroutput on ;
		set feedback off ;
		set linesize 4000 ;
		declare
			co_id 		varchar2(256) 	:= null ;
			acc_id		number(20)		:= null ;
			sub_id		number(20) 		:= null ;
			req_id		number(20)		:= null ;
			acc_name	varchar2(256)   := null ;
		begin
			select request_id.nextval into req_id from dual ;
			select reference_id into sub_id from alarms where id = $AlarmID ;
			select optional_field1, account_id into co_id, acc_id from subscriber where id = sub_id ;
			select account_name into acc_name from account where id = acc_id ;
			dbms_output.put_line ('MESSAGE_TYPE=[STATUS]|ns0:REQUEST_ID=' || req_id || '|CONTRACT_CODE=' || co_id || '|STATUS=$Status|REASON_FOR_STATUS_CHANGE=' || '$Reason' || '|CUSTOMER_CODE=' || acc_name) ;
		end ;
/
EOF
	return $?
}

PrepareMessageForAccount ()
{
	sqlplus -s /nolog << EOF > /dev/null 2>&1
	CONNECT_TO_SQL
		spool $RANGERHOME/tmp/spooleddata.dat
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		set serveroutput on ;
		set feedback off ;
		set linesize 4000 ;
		declare
			type co_id is table of subscriber.optional_field1%type ; --index by binary integer ;
			co_id_list			co_id 			:= null ;
			contract_id 		varchar2(256) 	:= null ;
			subscriber_id		varchar2(256) 	:= null ;
			req_id				number(20)		:= null ;
			acc_id				number(20) 		:= null ;
			acc_name			varchar2(256)   := null ;
		begin
			dbms_output.enable(NULL) ;
			select reference_id into subscriber_id from alarms where id = $AlarmID ;
			select account_id into acc_id from subscriber where id = subscriber_id ;
			select account_name into acc_name from account where id = acc_id ;
			select optional_field1 bulk collect into co_id_list from subscriber where account_id = acc_id  and status not in (0,3) ;

			for cid in co_id_list.first..co_id_list.last loop
				select request_id.nextval into req_id from dual ;
				dbms_output.put_line ('MESSAGE_TYPE=[STATUS]|ns0:REQUEST_ID=' || req_id || '|CONTRACT_CODE=' || co_id_list(cid) || '|STATUS=Suspend|REASON_FOR_STATUS_CHANGE=' || '$Reason' || '|CUSTOMER_CODE=' || acc_name) ;
			end loop ;
		end ;
/
EOF
	return $?
}

if [ $ReferenceType -ne $SUBSCRIBER_REFERENCE_TYPE ]; then
	logger "ERROR - du_sm_status_change.sh : The given Alarm ID: $Alarm is not a subscriber based alarm"
	exit 0 
fi

if [ "x$ActionLevel" == "xContract" ]; then
	PrepareMessageForContract
	if [ $? -ne 0 ]; then
		logger "ERROR - du_sm_status_change.sh : Unable To Prepare Contract Level $Status Change Message For Alarm ID : $AlarmID"
		exit 0
	fi
else
	PrepareMessageForAccount
	if [ $? -ne 0 ]; then
		logger "ERROR - du_sm_status_change.sh : Unable To Prepare Account Level $Status Change Message For Alarm ID : $AlarmID"
		exit 0
	fi
fi

TimeStamp=`date +%F_%H_%M_%S`
mv $RANGERHOME/tmp/spooleddata.dat $RANGER5_4HOME/RangerData/DataSourcePIData/PISTATUSCHANGEDATA_$TimeStamp
touch $RANGERHOME/tmp/spooleddata.dat $RANGER5_4HOME/RangerData/DataSourcePIData/success/PISTATUSCHANGEDATA_$TimeStamp

exit 0
