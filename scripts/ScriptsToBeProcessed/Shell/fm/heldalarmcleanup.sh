#! /bin/bash

. rangerenv.sh

$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
declare
    NumberOfHours number(10,0);
	AlarmStatusAfterCleanup number(20,0) ;
	ValidSubscriber number(1);
    cursor alarm_cursor is
       select id, reference_id, aggregation_value, alarm_type from alarm 
	   	where status = 7
			  and alarm_type = 2 
			  and aggregation_type = 1
			  and network_id = DECODE($1, 0, network_id, $1) ;
					
    alarm_cursor_val alarm_cursor%ROWTYPE;
Begin
	select to_number(value) into NumberOfHours 
		from Configuration where id = 'Held Alarm Cleanup Interval in Hours';	
	select to_number(value) into AlarmStatusAfterCleanup 
		from Configuration where id = 'Alarm Status after Held Alarm Cleanup';	

    open alarm_cursor;
    loop
        fetch alarm_cursor into alarm_cursor_val;
        exit when alarm_cursor%NOTFOUND;

        ValidSubscriber := 0 ;
        BEGIN
        select 1 into ValidSubscriber from dual where exists
                (select phone_number from subscriber
                        where phone_number = alarm_cursor_val.aggregation_value
							and status = 1) ;
        EXCEPTION
        WHEN NO_DATA_FOUND
        THEN
                ValidSubscriber := 0 ;
        END ;

        update alarm set status = DECODE(ValidSubscriber, 1, AlarmStatusAfterCleanup, 6),
                        modified_date = sysdate
                where alarm.id = alarm_cursor_val.id
				and ((ValidSubscriber = 0 and alarm.created_date < sysdate - (NumberOfHours/24))
                            or ValidSubscriber = 1) ;
		delete from  ignored_phone_number where ignored_phone_number.phone_number = alarm_cursor_val.aggregation_value 
				and ValidSubscriber = 1;
        commit;
    end loop;

    close alarm_cursor;

end;
/
commit;

EOF
