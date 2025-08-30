#!/bin/env bash

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. ~/.bashrc

if [ -z "$1" ]
then
    filename=$COMMON_MOUNT_POINT/FMSData/RatingLoaderData/free_numbers.csv
else
    filename="$1"
fi

if [ -z "$2" ]
then
    loaderControlFileName=free_numbers_tmp.ctl
else
    loaderControlFileName="$2"
fi

if [ -z "$3" ]
then
    controlFileDir="$COMMON_MOUNT_POINT/share/Ranger"
else
    controlFileDir="$3"
fi

if [ ! -z "$4" ]
then
    tmpLoaderBadFile="$4"
fi

logfileName="$COMMON_MOUNT_POINT/LOG/free_numbers_sqlldr.log"

if [ ! -e $filename ]
then
	echo -e "Unable to Find the Input File $filename.\n Exiting loader ...."
	exit 2
fi

tempTableCreation()
{
$SQL_COMMAND -s /nolog << EOF > $COMMON_MOUNT_POINT/LOG/sqlLogIn.log
	CONNECT_TO_SQL
	set heading off;
	set feedback off ;
	truncate table free_numbers_tmp;
	exit ;
EOF
}

loadToDB()
{

	os=`uname`

    if [ "$os" == "Linux" ]
    then
        dos2unix $filename
    fi

    if [ "$os" == "SunOS" ]
    then
        dos2unix  $filename > $RANGERHOME/bin/free_numbers_tmp_unix.txt
        mv $RANGERHOME/bin/free_numbers_tmp_unix.txt $filename
    fi

    if [ "$os" == "AIX" ]
    then
        tr -d "\015" < $filename > $RANGERHOME/bin/free_numbers_tmp_unix.txt
        mv $RANGERHOME/bin/free_numbers_tmp_unix.txt $filename
    fi

    if [ "$os" == "HP-UX" ]
    then
        dos2ux  $filename > $RANGERHOME/bin/free_numbers_tmp_unix.txt
        mv $RANGERHOME/bin/free_numbers_tmp_unix.txt $filename
    fi


	export logFileName="$logfileName"
	export loaderControlFileName
	export loaderInputDir=$COMMON_MOUNT_POINT/FMSData/RatingLoaderData/
	export loaderDataFileName=$filename
	export controlFileDir
	export tmpLoaderBadFile
	
	cat $tmpLoaderBadFile >> $COMMON_MOUNT_POINT/LOG/backup.log
	rm $tmpLoaderBadFile

	scriptlauncher $RANGERHOME/bin/sqlldrFileLoader.sh

	if [ $? -ne 0 ]
    then
        echo "SQLLoader execution failed"
		exit 1 
    fi
}

insertFreeNumbers()
{
	$SQL_COMMAND -s /nolog <<EOF > $COMMON_MOUNT_POINT/LOG/free_numbers.log
	CONNECT_TO_SQL
    set serveroutput on ;
    set feedback off ;
	DECLARE
		commit_counter number(20) := 0 ;
		is_present number(20) := 0 ;
		net_id number(20) := 0 ;
    BEGIN
        for free_numbers_tmp in (select * from free_numbers_tmp)
        loop
			begin
				select  count(*) into is_present from networks where description = free_numbers_tmp.network ;
				if is_present > 0
				then
				select id into net_id from networks where description = free_numbers_tmp.network ;
				is_present := 0 ;
				
				select count(*) into is_present from free_numbers FN,  free_numbers_networks FNN
				where FN.free_number = free_numbers_tmp.free_number and FNN.network_id = net_id
				and FN.id = FNN.free_number_id ;
				if is_present > 0
				then
					dbms_output.put_line('Error inserting duplicate values for Free Number:'||free_numbers_tmp.free_number) ;
				else
					begin
						insert into free_numbers values(free_numbers_seq.nextval, free_numbers_tmp.description, free_numbers_tmp.free_number) ;
						insert into free_numbers_networks values(free_numbers_seq.currval, net_id) ;
					exception when others then
						dbms_output.put_line('Error inserting free number:'||free_numbers_tmp.free_number) ;
					end;
				end if;
				is_present := 0 ;
				else
					dbms_output.put_line(free_numbers_tmp.free_number||'|'||free_numbers_tmp.description||'|'||free_numbers_tmp.network) ;
				end if;
			end ;
		commit_counter := commit_counter + 1 ;
		if commit_counter = 1000
		then
			commit_counter := 0 ;
			commit ;
		end if ;
		end loop ;
		commit ;
	end ;
/
EOF
	
	cp $COMMON_MOUNT_POINT/LOG/free_numbers.log $tmpLoaderBadFile
	
	if [ -s $tmpLoaderBadFile ]
	then
		exit 1
	fi
	
	if [ $? -ne 0 ]
	then
        exit $?
    fi
}

main ()
{
		tempTableCreation
		loadToDB
		insertFreeNumbers
}

main $* 
