#! /bin/bash
                                                                                                                 

sqlplus -s /nolog << eof
CONNECT_TO_SQL

set serveroutput off;
set feedback off;
set lines 120;
set colsep ,
set pages 0;
set heading off;

SQL COMMAND


quit;
eof
