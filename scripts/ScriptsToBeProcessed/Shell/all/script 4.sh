#! /bin/bash

sqlplus -s /nolog << eof
CONNECT_TO_SQL
set serveroutput off;
set feedback off;
set lines 120;
set colsep ,
set pages 0;
set heading off;
whenever sqlerror exit 5 ;




eof

if [ $? -eq 5 ]
then
	exit 0
else
	exit  1
fi
