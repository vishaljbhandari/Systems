. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. ~/.bashrc

if [ -z "$1" ]
then
    filename=$COMMON_MOUNT_POINT/FMSData/RatingLoaderData/rater_special_numbers.csv
else
    filename="$1"
fi

if [ -z "$2" ]
then
    loaderControlFileName=rater_special_numbers_tmp.ctl
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

logfileName="$COMMON_MOUNT_POINT/LOG/rater_special_numbers_sqlldr.log"

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
	truncate table rater_special_numbers_tmp;
	quit ;
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
        dos2unix  $filename > $RANGERHOME/bin/special_numbers_tmp_unix.txt
        mv $RANGERHOME/bin/special_numbers_tmp_unix.txt $filename
    fi

    if [ "$os" == "AIX" ]
    then
        tr -d "\015" < $filename > $RANGERHOME/bin/special_numbers_tmp_unix.txt
        mv $RANGERHOME/bin/special_numbers_tmp_unix.txt $filename
    fi

    if [ "$os" == "HP-UX" ]
    then
        dos2ux  $filename > $RANGERHOME/bin/special_numbers_tmp_unix.txt
        mv $RANGERHOME/bin/special_numbers_tmp_unix.txt $filename
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
		exit $?
    fi
}


insertSpecialNumbers()
{
	$SQL_COMMAND -s /nolog << EOF > $COMMON_MOUNT_POINT/LOG/rater_special_numbers.log
	CONNECT_TO_SQL
    set serveroutput on ;
    set feedback off ;
	DECLARE
		commit_counter number(20) := 0 ;
		is_present number(20) := 0 ;
		net_id number(20) := 0 ;
    BEGIN
        for rater_special_numbers_tmp in (select * from rater_special_numbers_tmp)
        loop
			begin
				select  count(*) into is_present from networks where description = rater_special_numbers_tmp.network ;
				if is_present > 0
				then
				
				is_present := 0 ;
				select id into net_id from networks where description = rater_special_numbers_tmp.network ;
				
				select count(*) into is_present from rater_special_numbers RS,  networks_rater_special_numbers RSN 
				where RS.special_number = rater_special_numbers_tmp.special_number and RSN.network_id = net_id
				and RS.id = RSN.rater_special_number_id ;

				if is_present > 0
				then
					dbms_output.put_line('Error inserting duplicate values for Rater Special Number:'||rater_special_numbers_tmp.special_number) ;
				else
					begin
						insert into rater_special_numbers values(rater_special_numbers_seq.nextval, rater_special_numbers_tmp.description, rater_special_numbers_tmp.special_number, rater_special_numbers_tmp.service_category, rater_special_numbers_tmp.min_chargeable_unit, rater_special_numbers_tmp.min_charge, rater_special_numbers_tmp.pulse, rater_special_numbers_tmp.rate, rater_special_numbers_tmp.pstn_min_chargeable_unit, rater_special_numbers_tmp.PSTN_MIN_CHARGE, rater_special_numbers_tmp.PSTN_PULSE, rater_special_numbers_tmp.PSTN_RATE, rater_special_numbers_tmp.EXTRA_CHARGE, rater_special_numbers_tmp.call_type, rater_special_numbers_tmp.pmn, rater_special_numbers_tmp.match_type) ;
						insert into networks_rater_special_numbers values(rater_special_numbers_seq.currval, net_id) ;
					exception when others then
						dbms_output.put_line('error inserting rater_special number:'||rater_special_numbers_tmp.special_number) ;
					end;
				end if;
				is_present := 0 ;
				else
					dbms_output.put_line(rater_special_numbers_tmp.description||'|'||rater_special_numbers_tmp.special_number||'|'||rater_special_numbers_tmp.service_category||'|'||rater_special_numbers_tmp.min_chargeable_unit||'|'||rater_special_numbers_tmp.min_charge||'|'||rater_special_numbers_tmp.pulse||'|'|| rater_special_numbers_tmp.rate||'|'|| rater_special_numbers_tmp.pstn_min_chargeable_unit||'|'||rater_special_numbers_tmp.PSTN_MIN_CHARGE||'|'||rater_special_numbers_tmp.PSTN_PULSE||'|'||rater_special_numbers_tmp.PSTN_RATE||'|'||rater_special_numbers_tmp.EXTRA_CHARGE||'|'||rater_special_numbers_tmp.call_type||'|'||rater_special_numbers_tmp.pmn||'|'||rater_special_numbers_tmp.match_type||'|'||rater_special_numbers_tmp.network) ;
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
	
	cp $COMMON_MOUNT_POINT/LOG/rater_special_numbers.log $tmpLoaderBadFile
	
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
		insertSpecialNumbers
}

main $* 

