#!/bin/bash

. $RANGERHOME/sbin/rangerenv.sh

SUBSCRIBER_REFERENCE_TYPE=1

sqlplus -s /nolog << EOF
CONNECT_TO_SQL
set feedback on;
spool $RANGERHOME/LOG/DuDuplicateIMSICleanup.log

declare
	cursor duplicateIMSI is
		select S.id, S.phone_number from subscriber S
			where S.account_id=4 and
					S.status=1 and
					S.phone_number = '+971' || S.imsi and
					S.imsi in (select S1.imsi from subscriber S1
									where S1.imsi=S.imsi and
										S1.account_id > 1024 and
										S1.status=1 and S1.phone_number <> '+971' || S1.imsi);
begin
	for dupIMSI in duplicateIMSI loop
		delete from ignored_entities where aggregation_type = 2 and ignored_value = dupIMSI.phone_number ;
		update alarms set status=4, user_id='ranger'
			where status not in (2,4,8) and reference_type = $SUBSCRIBER_REFERENCE_TYPE and reference_id = dupIMSI.id ;
		update subscriber set status=3 where phone_number = dupIMSI.phone_number and id = dupIMSI.id ;
	end loop ;

	commit;
end ;
/

commit;

spool off ;

quit;
EOF
