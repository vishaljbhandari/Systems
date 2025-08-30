#! /bin/bash

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from SCHEDULDED_JOB_EXEC_STATUS ;
delete from reload_configurations ;
delete from FP_PROFILES_16_1 ;
delete from FP_PROFILES_16_2 ;
delete from FP_PROFILES_221_1 ;
delete from FP_PROFILES_221_2 ;
delete from FP_SUSPECT_PROFILES_16 ;
delete from FP_SUSPECT_PROFILES_221 ;
commit;
 
exit;
EOF
