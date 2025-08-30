#! /bin/bash
OutputFile=$1
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
set newpage none ;
set termout off ;
set feedback off ;
set heading off ;
set null "*NULLVALUE*" ;
set trimspool on ;
set linesize 3000 ;

spool $OutputFile ;
select ID from networks ;
spool off ;

set heading on ;
set termout on ;
set feedback on ;

exit ;
/
EOF
