#! /bin/bash

if [ $# -eq 2 ]
then
	inputFormat="$1" ;
	outputFormat="$2" ;
else
	inputFormat="%d%m%Y%H%M%S" ;
	outputFormat="%d/%m/%Y %H:%M:%S" ;
fi

sqlplus $DB_USER/$DB_PASSWORD << EOF

whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

delete from date_formatter where formatter_name like  'DATE%' ;
insert into date_formatter values ('DATE', '$inputFormat', '$outputFormat') ;
commit ;

EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
