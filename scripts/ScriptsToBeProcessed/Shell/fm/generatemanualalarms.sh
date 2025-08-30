#!/bin/bash

if [ $# -ne 2 ]
then
	echo "Usage ./generateManualAlarms.sh <RuleName> <PhoneNumbersFile> "
	exit -1
fi

RuleName=$1
PhoneNumbersFile=$2

SUBSCRIBER_REFERENCE_TYPE=1
PHONE_NUMBER_AGG_TYPE=2
SUBSCRIBER_FC_ID=16
DEFAULT_NORMAL_SUBSCRIBER_ID=4
NO_OWNER=2

checkForRuleName ()
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
		echo "Rule Name :$RuleName does not exist"
		exit 1
	fi

	if test $? -eq 5 || grep "no rows" generateManualAlarms2.dat > /dev/null 2>&1
	then
		rm -f generateManualAlarms1.dat
		rm -f generateManualAlarms2.dat
		echo "Rule Name :$RuleName should be in disabled state"
		exit 2
	fi

	rm -f generateManualAlarms1.dat
	rm -f generateManualAlarms2.dat
}

generateManualAlarm ()
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
	begin
		select to_char(sysdate,'ddd') into day_of_year from dual;
		dbms_output.put_line (day_of_year) ;
	end ;
/
	spool off ;
EOF

	DayOfYear=`head -1 $RANGERHOME/tmp/spooleddata.dat | awk {'print $1'}`

	for PhoneNumber in `cat $PhoneNumbersFile`
	do 
		sqlplus -s /nolog << EOF > /dev/null 2>&1
			CONNECT_TO_SQL
			spool $RANGERHOME/tmp/spooleddata.dat
			whenever sqlerror exit 5 ;
			whenever oserror exit 6 ;
			set serveroutput on ;
			set feedback off ;

		declare
			subscriberID            number(20,0) := 0;

		begin
			begin
				select id into subscriberID from subscriber where id > 1024 and status in (1,2) and phone_number = '$PhoneNumber' ;

			exception
				when no_data_found then
					select subscriber_id.nextval into subscriberID from dual;
					insert into subscriber(ID,ACCOUNT_ID,PHONE_NUMBER,CONNECTION_TYPE,STATUS,PRODUCT_TYPE) values (subscriberID,4,'$PhoneNumber',1,1,0);
			end ;

			dbms_output.put_line (subscriberID) ;
		end ;
/
		spool off ;
EOF
		SubscriberID=`head -1 $RANGERHOME/tmp/spooleddata.dat | awk {'print $1'}`

		ruby <<EOF
			require ENV['RAILS_ROOT'] + '/lib/framework/manual_alarm_wrapper'
			alarm_details = Hash.new
			alarm_details[:network_id] = 999
			alarm_details[:score] = 50
			alarm_details[:user_id] = ''
			alarm_details[:reference_type] = $SUBSCRIBER_REFERENCE_TYPE
			alarm_details[:reference_id] = $SubscriberID
			alarm_details[:reference_value] = '$PhoneNumber'
			alarm_details[:owner_type] = $NO_OWNER

			alert_details = Hash.new
			alert_details[:score] = 50
			alert_details[:aggregation_type] = $PHONE_NUMBER_AGG_TYPE
			alert_details[:aggregation_value] = '$PhoneNumber'
			alert_details[:start_day] = $DayOfYear
			alert_details[:end_day] = $DayOfYear
			ManualAlarmWrapper.new.create_manual_alarm(alarm_details, alert_details, $DEFAULT_NORMAL_SUBSCRIBER_ID, '$RuleName', $SUBSCRIBER_FC_ID)
EOF
	done
}

main ()
{
	checkForRuleName
	generateManualAlarm
}

main
