#! /bin/bash

. $REVMAXHOME/bin/revmaxenv.sh

echo "Running revmax-db.sql"
sqlplus -l -s $DB_USER/$DB_PASSWORD < revmax/revmax-db.sql
if [ $? -ne 0 ]
then
	exit 1 ;
fi

echo "Running revmax-exec.sql"
sqlplus -l -s $DB_USER/$DB_PASSWORD < revmax/revmax-exec.sql
if [ $? -ne 0 ]
then
	exit 1 ;
fi
