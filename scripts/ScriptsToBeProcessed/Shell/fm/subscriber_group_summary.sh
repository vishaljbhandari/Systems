#! /bin/bash

$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL

WHENEVER SQLERROR EXIT 5;
SET SERVEROUTPUT ON;

BEGIN
	GroupSummary.PopulateSummary ;
    COMMIT ;
EXCEPTION
    WHEN no_data_found THEN
    DBMS_OUTPUT.PUT_LINE('Account Subscriber Group Summary Population Failed') ;
END;
/
COMMIT;
EOF

