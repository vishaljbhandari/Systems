#!/bin/env bash

RunDropall()
{
	#Constraints=' cascade constraints'
	$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING > /dev/null << EOF
set heading off ;
set feedback off ;
set lines 250 ;
spool dropall1.sql
select 'drop '|| rtrim(object_type) ||' "'||object_name||'" ;' from user_objects 
where object_name not like 'BIN$%' 
and object_name not like  'SYS%$%' 
and object_schema not like 'SYS%'
order by object_type desc ;
prompt show user ;
spool off ;
EOF
	grep "^drop" dropall1.sql > dropall.sql ;
	rm dropall1.sql ;
	echo "quit ;" >> dropall.sql ;
	$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @dropall | egrep -vi -e 'dropped' -e "^ *$";
}

GetCount()
{
	$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING << EOF > /dev/null
		set heading off ;
		set feedback off ;
spool tmp_count.lst
		select count(*) from user_objects where object_schema not like 'SYS%' ;
spool off ;
EOF
	Out=$(cat tmp_count.lst)
	rm -f .tmp_count.lst
	if [ $? -ne 0 ]; then
		echo $Out
		exit 1 ;
	fi
	Count=$(echo $Out | tr -d ' \n\t')
}

DropTablespace()
{
	$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @drop-nikira-DDL-tablespace > /dev/null << EOF 
EOF
}

User=$1
Password=$2

if [ $# -lt 1 ]
then
	echo "Usage  $0 user_name [password]"
	exit 
fi


if [ "$Password" = "" ]; then
	Password=$User ;
fi

GetCount
for((i=0;i<3 && Count > 0;i++))
do
	if [ $Count -gt 0 ]; then
		echo "Number of Objects in DB: $Count"
		if [ $i -eq 0 ];then
			echo "Dropping..."
		else
			echo "Trying Again."
		fi
	fi
	RunDropall
	#GetDBVersion
	#if [ $DBVersion -ge 10 ];then
	#	echo "Cleaning Recyclebin"
	#	CleanRecycleBin
	#fi
	GetCount
done

DropTablespace
echo "Tablespace Dropped"

if [ $Count -eq 0 ]; then
	echo "Dropall Completed."
else
	echo "Coudn't drop all objects.Gave up (Try closing/commiting all your SQL sessions and try again.)"
fi

rm -rf dropall.sql
