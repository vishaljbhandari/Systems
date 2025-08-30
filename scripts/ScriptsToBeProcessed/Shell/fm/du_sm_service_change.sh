#!/bin/bash

SUBSCRIBER_REFERENCE_TYPE=1

Action=$1
ServiceCode=$2
Reason=$3
AlarmID=$4
ReferenceType=$6

Reason=`echo $Reason | perl -pi -e "s#'#''#g"`

. rangerenv.sh

PrepareMessageForContractForSubscriberBasedAlarm ()
{
echo " dbms_output.put_line ('MESSAGE_TYPE=[SERVICE]|ns0:REQUEST_ID=' || req_id || '|CONTRACT_CODE=' || co_id || '|STATUS=$Action|SERVICE_CODE=$ServiceCode|REASON_FOR_STATUS_CHANGE=' || '$Reason' || '|CUSTOMER_CODE=' || acc_name) ; " >> /tmp/log.log
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
			acc_name	varchar2(256)   := null ;
			sub_id		number(20) 		:= null ;
			req_id		number(20)		:= null ;
		begin
			select request_id.nextval into req_id from dual ;
			select reference_id into sub_id from alarms where id = $AlarmID ;
			select optional_field1, account_id into co_id, acc_id from subscriber where id = sub_id ;
			select account_name into acc_name from account where id = acc_id ;
			dbms_output.put_line ('MESSAGE_TYPE=[SERVICE]|ns0:REQUEST_ID=' || req_id || '|CONTRACT_CODE=' || co_id || '|STATUS=$Action|SERVICE_CODE=$ServiceCode|REASON_FOR_STATUS_CHANGE=' || '$Reason' || '|CUSTOMER_CODE=' || acc_name) ;
		end ;
/
        spool off ;
EOF
	return $?
}

if [ $ReferenceType -eq $SUBSCRIBER_REFERENCE_TYPE ]; then
	PrepareMessageForContractForSubscriberBasedAlarm
	if [ $? -ne 0 ]; then
		logger "ERROR - du_sm_service_change.sh : Unable To Prepare Contract Level $ServiceCode Change Message For Alarm ID : $AlarmID"
		exit 0
	fi
else
	logger "ERROR - du_sm_service_change.sh : The given Alarm ID: $Alarm is not a subscriber based alarm"
	exit 0 
fi

TimeStamp=`date +%F_%H_%M_%S`
mv $RANGERHOME/tmp/spooleddata.dat $RANGER5_4HOME/RangerData/DataSourcePIData/PISERVICECHANGEDATA_$TimeStamp
touch $RANGERHOME/tmp/spooleddata.dat $RANGER5_4HOME/RangerData/DataSourcePIData/success/PISERVICECHANGEDATA_$TimeStamp

exit 0
