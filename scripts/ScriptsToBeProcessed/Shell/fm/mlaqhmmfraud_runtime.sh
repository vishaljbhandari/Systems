#! /bin/bash

if [ $# -ne 2 ]
then
	echo "Usage: ./mlaqhmmfraud_runtime.sh <Start date in dd/mm/yyyy> <End date in dd/mm/yyyy>"
	exit 1
fi

StartDate="to_date('$1', 'dd/mm/yyyy')"
EndDate="to_date('$2', 'dd/mm/yyyy')"

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

create or replace view hmm_iaq_alarm_v as
	select a.id, trunc(a.created_date) created_date, a.status, trunc(c.comment_date) comment_date
	from 
		alarms a,
		(
		 	select alarm_id, comment_date
			from
				(
					select alarm_id, comment_date,
					count(*) over (partition by alarm_id order by comment_date rows unbounded preceding) cnt
					from alarm_comments
				)
			where
				cnt = 1 and
				comment_date >= $StartDate and
				comment_date <= $EndDate
		) c,
		(
		 	select distinct id 
			from ml_aq_alarm
		) m
	where 
		a.status in (2,4) and
		c.alarm_id = a.id  and 
		m.id = a.id ;

set lines 300 ;
set serveroutput on ;

exec ml_hmm_iaq_stats.CompareFraudRunTimes ;

quit ;

EOF
