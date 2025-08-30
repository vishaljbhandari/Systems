#!/bin/env bash


. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. ~/.bashrc

if [ -z "$1" ]
then
    filename=$COMMON_MOUNT_POINT/FMSData/RatingLoaderData/sdr_rates.csv
else
    filename="$1"
fi

if [ -z "$2" ]
then
    loaderControlFileName=sdr_rates_tmp.ctl
else
    loaderControlFileName="$2"
fi

if [ -z "$3" ]
then
    controlFileDir="$RANGERHOME/share/Ranger"
else
    controlFileDir="$3"
fi

if [ ! -z "$4" ]
then
    tmpLoaderBadFile="$4"
fi

logfileName="$COMMON_MOUNT_POINT/LOG/sdr_rates_sqlldr.log"

if [ ! -e $filename ]
then
	echo -e "Unable to Find the Input File $filename.\n Exiting loader ...."
	exit 2
fi

tempTableCreation()
{
$SQL_COMMAND -s /nolog << EOF >> $COMMON_MOUNT_POINT/LOG/sqlLogIn.log
	CONNECT_TO_SQL
	set heading off;
	set feedback off ;
	truncate table sdr_rates_tmp;
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
        dos2unix  $filename > $RANGERHOME/bin/sdr_rates_tmp_unix.txt
        mv $RANGERHOME/bin/sdr_rates_tmp_unix.txt $filename
    fi

    if [ "$os" == "AIX" ]
    then
        tr -d "\015" < $filename > $RANGERHOME/bin/sdr_rates_tmp_unix.txt
        mv $RANGERHOME/bin/sdr_rates_tmp_unix.txt $filename
    fi

    if [ "$os" == "HP-UX" ]
    then
        dos2ux  $filename > $RANGERHOME/bin/sdr_rates_tmp_unix.txt
        mv $RANGERHOME/bin/sdr_rates_tmp_unix.txt $filename
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
        echo "SQLLoader execution failed" >> $COMMON_MOUNT_POINT/LOG/tmp.log
		exit $?
    fi
}

insertSdrRates()
{
	$SQL_COMMAND -s /nolog <<EOF > $COMMON_MOUNT_POINT/LOG/sdr_rates.log
	CONNECT_TO_SQL
    set serveroutput on ;
    set feedback off ;
	DECLARE
		commit_counter number(20) := 0 ;
		is_present number(20) := 0 ;
		net_id number(20) := 0 ;
    BEGIN
        for sdr_rates_tmp in (select * from sdr_rates_tmp)
        loop
			begin
				select  count(*) into is_present from networks where description = sdr_rates_tmp.network ;
				if is_present > 0
				then
					select id into net_id from networks where description = sdr_rates_tmp.network ;
											
					begin
						insert into sdr_rates values(sdr_rates_seq.nextval, sdr_rates_tmp.start_date, sdr_rates_tmp.end_date, sdr_rates_tmp.sdr_value) ;
						insert into networks_sdr_rates values(net_id, sdr_rates_seq.currval) ;
						exception when others then
							dbms_output.put_line('Error inserting sdr rates:'||sdr_rates_tmp.sdr_value) ;
					end;
				is_present := 0 ;
				else
					dbms_output.put_line(sdr_rates_tmp.start_date||'|'||sdr_rates_tmp.end_date||'|'||sdr_rates_tmp.sdr_value||'|'||sdr_rates_tmp.network) ;
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
	cp $COMMON_MOUNT_POINT/LOG/sdr_rates.log $tmpLoaderBadFile

	if [ -s $tmpLoaderBadFile ]
	then
		exit 2
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
		insertSdrRates
}

main $* 
