#! /bin/bash

RuleReplicateTT="CRMIReplicateTroubleTicket"
RuleUpdateAction="CRMICreateAlarm"

SUBSCRIBER_REFERENCE_TYPE=1
PHONE_NUMBER_AGG_TYPE=2
SUBSCRIBER_FC_ID=16
DEFAULT_NORMAL_SUBSCRIBER_ID=4

. rangerenv.sh

on_exit ()
{
	if [ -f $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID ]; then
		PID=`cat $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID`
		if [ $PID -eq $$ ]; then
			rm -f $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID
		fi
	fi
}
trap on_exit EXIT

Status=$1
if [ -z $Status ] 
then
	echo >&2 "Usage dugeneratealarmcrmi.sh <start/stop>" && exit 1
fi

HandleRejection()
{
MAILID=`sqlplus -s /nolog << EOF 2> /dev/null
	CONNECT_TO_SQL
	set heading off ;
	set linesize 1000 ;
	set pagesize 1000 ;
	set feedback off ;
	select value from configurations where config_id = 'RANGER.CRM.NOTIFICATION' ;
EOF`
	uuencode $RANGER5_4HOME/RangerData/CRMIData/$file $file | mailx -s "Rejected CRM Request" "$MAILID"
	mv $RANGER5_4HOME/RangerData/CRMIData/$file $RANGERHOME/RangerData/CRMIData/Rejected/$file
	rm $RANGER5_4HOME/RangerData/CRMIData/success/$file
}

GetDataFromFile ()
{
	for file in `ls $RANGER5_4HOME/RangerData/CRMIData/success`
	do
		AccountName=`cat $RANGER5_4HOME/RangerData/CRMIData/$file |  sed 's/.*<\(.*\)>\(.*\)<\/\(.*\)>.*/\1=\2/g' |grep ^CUSTOMER_CODE | cut -d"=" -f2`
		if [ -z "$AccountName" ]; then
			AccountName=`cat $RANGER5_4HOME/RangerData/CRMIData/$file |  sed 's/.*<\(.*\)>\(.*\)<\/\(.*\)>.*/\1=\2/g' |grep ^ACCT_NUMBER | cut -d"=" -f2`
		fi

		ActivityID=`cat $RANGER5_4HOME/RangerData/CRMIData/$file | grep "ACTIVITY_UID" | tail -1 | sed 's/.*<\(.*\)>\(.*\)<\/\(.*\)>.*/\2/g'`

		if [ -z $ActivityID ]; then
			RuleName="$RuleReplicateTT"
		else
			RuleName="$RuleUpdateAction"
		fi

PhoneNumber=`sqlplus -s /nolog << EOF 2> /dev/null
	CONNECT_TO_SQL
	set heading off ;
	set linesize 1000 ;
	set pagesize 1000 ;
	set feedback off ;
	select phone_number from subscriber where account_id = (select id from account where account_name = '$AccountName' and rownum < 2) ;
EOF`
		PhoneNumber=`echo $PhoneNumber| cut -d. -f2`

		LogTime=`date +"%F %R:%S"`
		PhoneNumber=`echo $PhoneNumber | perl -pi -e "s#\n##g"`
		if [ -z  "$PhoneNumber" ]; then
			echo -n "$LogTime - [File : $file] - " >>$RANGERHOME/LOG/dugeneratealarmcrmi.log
			echo "Invalid Account Name : [$AccountName]." >>$RANGERHOME/LOG/dugeneratealarmcrmi.log
			HandleRejection
		else
			CheckForRuleName
			AlarmID=`CreateAlarm`
			echo -n "$LogTime - [File : $file] - " >>$RANGERHOME/LOG/dugeneratealarmcrmi.log
			echo "Alarm created for Phonenumber : $PhoneNumber of Account Name : $AccountName for the rule: $RuleName" >>$RANGERHOME/LOG/dugeneratealarmcrmi.log
			cp $RANGER5_4HOME/RangerData/CRMIData/$file $RANGER5_4HOME/RangerData/CRMIData/Backup/$file
			Timestamp=`date +%F_%H_%M_%S`
			FileName=CRMI_$Timestamp.xml
			mkdir -p $RAILS_ROOT/public/Attachments/Alarm/$AlarmID
			mv $RANGER5_4HOME/RangerData/CRMIData/$file $RAILS_ROOT/public/Attachments/Alarm/$AlarmID/$FileName
			rm $RANGER5_4HOME/RangerData/CRMIData/success/$file
			sqlplus -s /nolog << EOF > /dev/null 2>&1
			CONNECT_TO_SQL
			whenever sqlerror exit 5;
			whenever oserror exit 5;
			set heading off ;
			set feedback off ;
			insert into alarm_attachments(id, alarm_id, file_name) values(alarm_attachments_seq.nextval, $AlarmID, '$FileName') ;
EOF
		fi
	done
}

CheckForRuleName()
{
sqlplus -s /nolog << EOF > /dev/null 2>&1
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off

spool generateManualAlarms1.dat
select 1 from dual
where 1 = (select count(*) from rules where name = '$RuleName' 
		   and version = (select max(version) from rules where name = '$RuleName')) ;
spool off

spool generateManualAlarms2.dat
select 1 from dual
where 0 = (select count(*) from rules where is_enabled = 1 and name = '$RuleName' 
		   and version = (select max(version) from rules where name = '$RuleName')) ;
spool off

EOF

if test $? -eq 5 || grep "no rows" generateManualAlarms1.dat > /dev/null 2>&1
then
	rm -f generateManualAlarms1.dat
	rm -f generateManualAlarms2.dat
	echo "Rule :$RuleName does not exist"
	exit 1
fi

if test $? -eq 5 || grep "no rows" generateManualAlarms2.dat > /dev/null 2>&1
then
	rm -f generateManualAlarms1.dat
	rm -f generateManualAlarms2.dat
	echo "Rule :$RuleName should be in disabled state"
	exit 2
fi

rm -f generateManualAlarms1.dat
rm -f generateManualAlarms2.dat
}

CreateAlarm ()
{
	sqlplus -s /nolog << EOF > /dev/null 2>&1
		CONNECT_TO_SQL
		spool $RANGERHOME/tmp/spooleddata.dat
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		set serveroutput on ;
		set feedback off ;

	declare
		day_of_year             number;
		subscriberID            number(20,0) := 0;

	begin
		begin
			select id into subscriberID
			from subscriber
			where id > 1024
			and status in (1,2)
			and phone_number = '$PhoneNumber' ;

		exception
			when no_data_found then
				select subscriber_id.nextval into subscriberID from dual;
				insert into subscriber(ID,ACCOUNT_ID,PHONE_NUMBER,CONNECTION_TYPE,STATUS,PRODUCT_TYPE) values (subscriberID,4,'$PhoneNumber',1,1,0);
		end ;

		select to_char(sysdate,'ddd') into day_of_year from dual;
		dbms_output.put_line (subscriberID) ;
		dbms_output.put_line (day_of_year) ;

	end ;
/
	spool off ;
EOF

	SubscriberID=`head -1 $RANGERHOME/tmp/spooleddata.dat | awk {'print $1'}`
	DayOfYear=`tail -1 $RANGERHOME/tmp/spooleddata.dat | awk {'print $1'}`

	ruby <<EOF
		require ENV['RAILS_ROOT'] + '/lib/framework/manual_alarm_wrapper'
		alarm_details = Hash.new
		alarm_details[:network_id] = 999
		alarm_details[:score] = 50
		alarm_details[:user_id] = ''
		alarm_details[:reference_type] = $SUBSCRIBER_REFERENCE_TYPE
		alarm_details[:reference_id] = $SubscriberID
		alarm_details[:reference_value] = '$PhoneNumber'

		alert_details = Hash.new
		alert_details[:score] = 50
		alert_details[:aggregation_type] = $PHONE_NUMBER_AGG_TYPE
		alert_details[:aggregation_value] = '$PhoneNumber'
		alert_details[:start_day] = $DayOfYear
		alert_details[:end_day] = $DayOfYear
		alarm_id = ManualAlarmWrapper.new.create_manual_alarm(alarm_details, alert_details, $DEFAULT_NORMAL_SUBSCRIBER_ID, '$RuleName', $SUBSCRIBER_FC_ID)
		puts alarm_id.to_s
EOF
}
			
main ()
{
	while true
	do
		GetDataFromFile
	done

	exit 0
}


if [ "$Status" == "stop" ]; then
	if [ -f $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID ]; then
		kill -9 `cat $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID`
		rm -f $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID
		echo "dugeneratealarmcrmi Stopped Successfuly ..."
		exit 0 
	else
		echo "No Such Process ... "
		exit 1
	fi
elif [ "$Status" == "start" ]; then
	if [ ! -f $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID ]; then
		echo "dugeneratealarmcrmi Started Successfuly ..."
		echo $$ > $RANGERHOME/share/Ranger/dugeneratealarmcrmiPID
		while true
		do
			main $*
		done
	else
		echo "dugeneratealarmcrmi Already Running ..."
		exit 2
	fi
else
	echo >&2 "Usage dugeneratealarmcrmi.sh <-r start/stop>" && exit 1 
	exit 3
fi
