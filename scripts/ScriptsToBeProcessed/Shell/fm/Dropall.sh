#!/bin/env bash

RunDropall()
{
	if [ "$1" == "t" ];then
		Negation=''
		Constraints=' cascade constraints'
	else
		Negation="!"
		Constraints=''
	fi
	sqlplus -s -l $User/$Password > /dev/null << EOF
	set heading off ;
	set feedback off ;
	spool dropall1.sql
	select 'drop '|| object_type||' "'||object_name||'" $Constraints;' from user_objects 
	where object_type $Negation= 'TABLE'
	and object_name not like 'BIN$%' 
	and object_name not like  'SYS%$%' 
	order by object_type desc ;
	spool off ;
EOF
	grep "^drop" dropall1.sql > dropall.sql ;
	rm dropall1.sql ;
	echo "quit ;" >> dropall.sql ;
	sqlplus -s -l $User/$Password @dropall | egrep -vi -e 'dropped' -e "^ *$";
}

CleanRecycleBin()
{
	sqlplus -s -l $User/$Password > /dev/null << EOF
	purge recyclebin ;
EOF
}

GetDBVersion()
{
	DBVersion=0	
	DBVersion=$(sqlplus \nolog </dev/null|head -2|tail -1|cut -d' ' -f3|cut -d'.' -f1)
}

GetCount()
{
	Out=$(sqlplus -s -l $User/$Password << EOF
		set heading off ;
		set feedback off ;
		select count(*) from user_objects ;
EOF
	)
	if [ $? -ne 0 ]; then
		echo $Out
		exit 1 ;
	fi
	Count=$(echo $Out | tr -d ' \n\t')
}

DropTablespaceFunction()
{

	sqlplus -s -l $User/$Password @tablespaces.sql > /dev/null << EOF 
EOF

}

User=$1
Password=$2
DropTableSpace=$3

if [ $# -lt 3 ]
then
	echo "Usage  $0 <DB_SUER> <DB_PASSWORD>  <DROP TABLESPACE[1/0]> "
	exit 
fi

if [ $3 -eq "1" ]
then

echo "Enter conf File Path"
read confFile


sh DropTableSpace.sh $confFile


if [ $? -ne 0 ];then
    echo "Drop tablespace file creation failed"
    exit 1 ;
fi

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
	RunDropall 't'
	RunDropall
	GetDBVersion
	if [ $DBVersion -ge 10 ];then
		echo "Cleaning Recyclebin"
		CleanRecycleBin
	fi
	GetCount
done

if [ "$DropTableSpace" = "1" ]; then
	echo "Dropping TableSpace" 
	DropTablespaceFunction
	if [ $? -ne 0 ];then
    exit 1 ;
	fi


	echo "Tablespace Dropped"
fi


if [ $Count -eq 0 ]; then
	echo "Dropall Completed."
else
	echo "Coudn't drop all objects.Gave up (Try closing/commiting all your SQL sessions and try again.)"
fi

rm -rf dropall.sql tablespaces.sql alltablespace

