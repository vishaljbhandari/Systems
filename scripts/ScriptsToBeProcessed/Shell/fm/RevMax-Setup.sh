#! /bin/bash

echo "Running revmaxdb.sql"
sqlplus -l -s $DB_USER/$DB_PASSWORD < revmax/revmaxdb.sql
if [ $? -ne 0 ]
then
	exit 1 ;
fi

echo "Running revmax_exec.sql"
sqlplus -l -s $DB_USER/$DB_PASSWORD < revmax/revmax_exec.sql
if [ $? -ne 0 ]
then
	exit 1 ;
fi
