#! /bin/bash




dropconfFile=$1

>alltablespace
>tablespaces.sql

echo "spool drop-nikira-DDL-tablespace.sql.log; " >> tablespaces.sql

echo "set feedback off ;" >> tablespaces.sql
echo "set serveroutput off ;">> tablespaces.sql

cat $dropconfFile | while read LINE
do

>alltablespace
#echo "Reading file $LINE for droppping tablespaces"

    if [ ! -f $LINE ]
        then
        	echo "$LINE path mention does not exist"    
	        exit 1;
    fi;
grep "TableSpaceName" $LINE | awk '{ if(match($0,"TableSpace")>0) print $2}'|cut -d "=" -f2|sed 's/"//g' >> alltablespace



cat alltablespace | while read LINE

do

echo "DROP TABLESPACE $LINE INCLUDING CONTENTS AND DATAFILES;" >> tablespaces.sql 

done


done

echo "commit;" >> tablespaces.sql
echo "exit;" >> tablespaces.sql

