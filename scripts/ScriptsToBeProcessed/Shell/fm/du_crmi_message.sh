#!/bin/bash

SUBSCRIBER_REFERENCE_TYPE=1

Flag=$1
Action=$2
AlarmID=$3
ReferenceType=$5

. rangerenv.sh

file=CRMI

ActivityID=`cat $RAILS_ROOT/public/Attachments/Alarm/$AlarmID/$file"_"*".xml" | grep "ACTIVITY_UID" | tail -1 | sed 's/.*<\(.*\)>\(.*\)<\/\(.*\)>.*/\2/g'`

PrepareMessageCreateCommentAndInvestigate ()
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
			acc_id		number(20)		:= null ;
			acc_name	varchar2(256)   := null ;
			sub_id		number(20) 		:= null ;
			req_id		number(20)		:= null ;
			reason		varchar2(256)	:= null ;
		begin
			select request_id.nextval into req_id from dual ;
			select reference_id into sub_id from alarms where id = $AlarmID ;
			select account_id into acc_id from subscriber where id = sub_id ;
			select account_name into acc_name from account where id = acc_id ;
			select replace(user_comment, chr(10), ' ') into reason from alarm_comments where alarm_id = $AlarmID and comment_date >= (select max(comment_date) from alarm_comments where alarm_id = $AlarmID) ;
			dbms_output.put_line ('MESSAGE_TYPE=[FLAG]|ns0:REQUEST_ID=' || req_id || '|UNDER_INVESTIGATION_FLAG=$Flag|CUSTOMER_CODE=' || acc_name ) ;
			select request_id.nextval into req_id from dual ;
			dbms_output.put_line ('MESSAGE_TYPE=[FLAG]|ns0:REQUEST_ID=' || req_id || '|CUSTOMER_CODE=' || acc_name || '|NOTE=' || reason || '|TYPE=FMS Comments' ) ;
		end ;
/
EOF
	return $?
}

PrepareMessageUpdateAction ()
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
			acc_id		number(20)		:= null ;
			acc_name	varchar2(256)   := null ;
			sub_id		number(20) 		:= null ;
			req_id		number(20)		:= null ;
			reason		varchar2(256)	:= null ;
		begin
			select request_id.nextval into req_id from dual ;
			select reference_id into sub_id from alarms where id = $AlarmID ;
			select account_id into acc_id from subscriber where id = sub_id ;
			select account_name into acc_name from account where id = acc_id ;
			select replace(user_comment, chr(10), ' ') into reason from alarm_comments where alarm_id = $AlarmID and comment_date >= (select max(comment_date) from alarm_comments where alarm_id = $AlarmID) ;
			dbms_output.put_line ('MESSAGE_TYPE=[UPDATE]|ns0:REQUEST_ID=' || req_id || '|STATUS=Done|ACTIVITY_UID=$ActivityID|CUSTOMER_CODE=' || acc_name || '|COMMENT=' || reason ) ;
		end ;
/
EOF
	return $?
}

PrepareMessageTroubleTicket ()
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
			acc_id			number(20)		:= null ;
			acc_name		varchar2(256)   := null ;
			sub_id			number(20) 		:= null ;
			req_id			number(20)		:= null ;
			nationality		varchar2(256)	:= null ;
		begin
			select request_id.nextval into req_id from dual ;
			select reference_id into sub_id from alarms where id = $AlarmID ;
			select account_id,nationality into acc_id,nationality from subscriber where id = sub_id ;
			select account_name into acc_name from account where id = acc_id ;
			dbms_output.put_line ('MESSAGE_TYPE=[TT]|ns0:REQUEST_ID=' || req_id || '|ACCT_NUMBER=' || acc_name || '|NATIONALITY=' || nationality || '|SR_TYPE=$SRType') ;
		end ;
/
EOF
	return $?

}

if [ $ReferenceType -ne $SUBSCRIBER_REFERENCE_TYPE ]; then
	logger "ERROR - du_crmi_message.sh : The given Alarm ID: $Alarm is not a subscriber based alarm"
	exit 0 
fi

case "$Action" in
	'TTE')
		SRType="ENT_Collections"
		PrepareMessageTroubleTicket
		;;
	'TTC')
		SRType="Collections"
		PrepareMessageTroubleTicket
		;;
	'FLAG')
		PrepareMessageCreateCommentAndInvestigate
		;;
	'UPDATE')
		PrepareMessageUpdateAction
		;;
	*)
		logger "ERROR - ducrmimessage.sh : Improper Action Argument."
		exit 0
		;;
esac

if [ $? -ne 0 ]; then
	logger "ERROR - ducrmimessage.sh : Unable To Prepare Message for $Action Action For Alarm ID : $AlarmID"
	exit 0
else
	echo "Created Message for $Action Action For Alarm ID : $AlarmID" >>$RANGERHOME/LOG/du_crmi_message.log
fi

TimeStamp=`date +%F_%H_%M_%S`
mv $RANGERHOME/tmp/spooleddata.dat $RANGER5_4HOME/RangerData/DataSourceCRMIData/CRMI_"$Action"DATA_"$TimeStamp"
touch $RANGERHOME/tmp/spooleddata.dat $RANGER5_4HOME/RangerData/DataSourceCRMIData/success/CRMI_"$Action"DATA_"$TimeStamp"

exit 0
