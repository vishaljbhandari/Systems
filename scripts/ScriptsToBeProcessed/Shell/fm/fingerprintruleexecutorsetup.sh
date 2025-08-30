#! /bin/bash

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from SCHEDULDED_JOB_EXEC_STATUS ;
delete from reload_configurations ;

commit;
 
exit;
EOF
