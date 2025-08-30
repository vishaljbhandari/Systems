#! /bin/bash

. $RANGERHOME/sbin/rangerenv.sh

sqlplus -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
set feedback off
declare
    Number_Of_Days number(10,0);

Begin
        select to_number(value) into Number_Of_Days from Configuration where id = 'FMSLog Cleanup-Interval in Days';
        delete from rip_processed_file_info where trunc(FILE_TIMESTAMP) < trunc(sysdate) - Number_Of_Days;
end;
/
commit;

EOF

if [ $? -ne 5 ]
then
    exit 0
else
    exit 1
fi
