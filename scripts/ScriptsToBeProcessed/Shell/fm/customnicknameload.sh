#! /bin/bash

source $RANGERHOME/bin/db2utility.sh

TMP_LD_PRELOAD_64=$LD_PRELOAD_64

checkParameters()
{
	if [ $#  -ne 3 ]
	then
		echo "Usage: [`basename $0` <NickName> <Field CategoryID> <NickName Type>]"
		echo "Nickname type Should be DAILYADDITION/REPLACEALL"
		exit 2
	fi

	if ! [ "$3" == "DAILYADDITION" -o "$3" == "REPLACEALL" ] 
	then 
		echo "\nInvalid Nickname type: $3 [Should be DAILYADDITION/REPLACEALL]"
		exit 2
	fi
}

removeCtlFile()
{
	test -e $NicknameControlFile 
	retcode=`echo $?`
	if [ $retcode -eq 0 ]
	then
		rm -f $NicknameControlFile
	fi 


	test -e $NicknameDB2Command
	retcode=`echo $?`
	if [ $retcode -eq 0 ]
	then
		rm -f $NicknameDB2Command
	fi
}

checkForInputFiles()
{
	NoFiles=`ls -l $NicknameInputDirectory | tail -n +2 | wc -l`
	if [ $NoFiles -le 0 ]
	then
		echo "No input files are present in $NicknameInputDirectory Directory. Exiting..."
		exit 1
	fi
}

cleanupCustomNicknameTables()
{
	cleanupStatus=0

	case $NicknameType in 
	"REPLACEALL")

			Query="DELETE FROM custom_nickname_values 
					WHERE custom_nickname_id = (SELECT id FROM custom_nicknames 
												          WHERE LOWER(nickname) = LOWER('$CustomNickname')) ; "
					Category="commit"
					OutputFilename=$TempDir/sqloutput.lst
					getResult 
					Category=""
		;;
	esac

	cleanupStatus=$?
		
	if [ $cleanupStatus -ne 0 ]
	then
		echo "Error: [Failed while cleaning Custom Nickname tables] Exiting..."
		exit 1 
	fi
}





loadNewNumbers()
{
	echo "Nickname: $Nickname"
	for file in `ls -rt $NicknameInputDirectory/`
	do       
		FileBaseName=`basename $file`
		dataFile=$NicknameInputDirectory/$FileBaseName
		tmpdataFile=$NicknameInputDirectory/tmp$FileBaseName

		sort -u $dataFile > $tmpdataFile

		awk '$0 = $0 "',$CustomNicknameID'"' $tmpdataFile > $dataFileForDB2

		echo "Loading data for Custom Nickname \"$Nickname\" from file \"$FileBaseName\"..."
		bad=$COMMON_MOUNT_POINT/LOG/$FileBaseName.sqlldr.bad

		CONNECT_TO_SQLLDR_START
			control=$NicknameControlFile \
			data=$tmpdataFile \
			bad=$COMMON_MOUNT_POINT/LOG/$FileBaseName.sqlldr.bad \
			silent=\(header,errors,feedback\) errors=666666 
		CONNECT_TO_SQLLDR_END

		CONNECT_TO_DB2LOAD_START

			command=`db2load $NicknameDB2Command $dataFileForDB2 $bad`
			$command > /dev/null

		CONNECT_TO_DB2LOAD_END
						
		
		Status=$?
		rm -f $COMMON_MOUNT_POINT/LOG/$FileBaseName.sqlldr.bad

		if test $Status -eq 0
		then
			cp $dataFile $NicknameOutputDirectory/
			rm -f $dataFile $file $tmpdataFile
			if [ ! -z $dataFileForDB2 ]
			then
				rm -f $dataFileForDB2
			fi
		else
			echo "Error: [Failed to load data for Custom Nickname \"$Nickname\" from file \"$FileBaseName\" "
		fi
	done
	echo "Done with loading data files"
}

generateNicknameControlFile()
{
	echo "LOAD DATA" >> $NicknameControlFile
	if [ "$NicknameType" == "DAILYADDITION" ]; then
		echo "INTO TABLE TEMP_CUSTOM_NICKNAME_VALUES APPEND" >> $NicknameControlFile
	else
		echo "INTO TABLE CUSTOM_NICKNAME_VALUES APPEND" >> $NicknameControlFile
	fi		
	echo "TRAILING NULLCOLS" >> $NicknameControlFile
	echo "(" >> $NicknameControlFile
	echo " id \"custom_nickname_values_seq.nextval\", " >> $NicknameControlFile 
	echo " custom_nickname_id constant \"$CustomNicknameID\", " >> $NicknameControlFile
	echo " value position(01:40) char(40)," >> $NicknameControlFile
	echo " modification_date \"sysdate\"" >> $NicknameControlFile
	echo ")" >> $NicknameControlFile
}

generateNicknameControlFileForDB2()
{
	echo "db2 IMPORT" >> $NicknameDB2Command
	echo "FROM :DATA" >> $NicknameDB2Command
	echo "OF DEL" >>  $NicknameDB2Command
	echo "METHOD P(1, 2)" >> $NicknameDB2Command
	if [ "$NicknameType" == "DAILYADDITION" ]; then
		echo "INSERT INTO TEMP_CUSTOM_NICKNAME_VALUES" >> $NicknameDB2Command
	else
		echo "INSERT INTO CUSTOM_NICKNAME_VALUES" >> $NicknameDB2Command
	fi	
	echo "(value,custom_nickname_id)" >> $NicknameDB2Command
}

getFieldCategoryName()
{
	$SQL_COMMAND -s /nolog << EOF > $TempDir/fieldcategoryname.lst  2>&1
	CONNECT_TO_SQL
			whenever sqlerror exit 5;
			set heading off;
			set feedback off ;
				select name from field_categories where field_categories.id = $FieldCategoryId ;
			quit;
EOF
	status=`grep -ic "no rows selected" $TempDir/fieldcategoryname.lst`
	if [ $status -ne 0 ]
	then 
		echo "\nInvalid FieldCategory ID : $1"
		exit 1
	fi

	grep -v "Connected." $TempDir/fieldcategoryname.lst |tr -d "\n" > $TempDir/temp 
	FieldCategoryName=`cat $TempDir/temp`
	CustomNickname="$Nickname""_""$FieldCategoryName"
}

getnickNameID()
{
	$SQL_COMMAND -s /nolog << EOF > $TempDir/nicknameid.lst  2>&1
	CONNECT_TO_SQL
			whenever sqlerror exit 5;
			set heading off;
			set feedback off ;
				SELECT id FROM custom_nicknames 
						  WHERE LOWER(nickname) = LOWER('$CustomNickname') 
						  and LOWER(nickname_type) = LOWER('$NicknameType') ;
			quit;
EOF
	status=`grep -ic "no rows selected" $TempDir/nicknameid.lst`
	if [ $status -ne 0 ]
	then 
		echo "Nicknames not found going for insertion..........."
		insertNickname
	fi

	grep -v "Connected." $TempDir/nicknameid.lst |tr -d "\n" | tr -d " " > $TempDir/temp 
	CustomNicknameID=`cat $TempDir/temp`
}

insertNickname()
{
	$SQL_COMMAND -s /nolog << EOF > $TempDir/customnickname.lst  2>&1
	CONNECT_TO_SQL
		whenever sqlerror exit 5;
		whenever oserror exit 6;
		set heading off;
		set feedback off ;
		insert into custom_nicknames (id, nickname, nickname_type, no_of_days, field_category_id)
		select custom_nicknames_seq.nextval, '$Nickname', '$NicknameType', $ValidityPeriod, $FieldCategoryId from dual ;
			commit ; 
		quit;
EOF
	sqlretcode=`echo $?`
	if [ $sqlretcode -ne 0 ]
	then
		errorMessage="Statement failed:\n insert into custom_nicknames (id, nickname, nickname_type, no_of_days, field_category_id)
				select custom_nicknames_seq.nextval, '$Nickname', '$NicknameType', $ValidityPeriod,
						 $FieldCategoryId from dual ;
"
		echo $errorMessage
		exit 1;
	fi
}

loadListDetails()
{
	if [ "$NicknameType" == "DAILYADDITION" ]
	then
		Query="insert into CUSTOM_NICKNAME_VALUES select * from TEMP_CUSTOM_NICKNAME_VALUES where custom_nickname_id = $CustomNicknameID  and value not in (select value from CUSTOM_NICKNAME_VALUES where custom_nickname_id = $CustomNicknameID ) ; " 
		Category="commit"
		OutputFilename=$TempDir/insertintocustomnickname.lst ;
		getResult 
		Category=""

		Query="delete from temp_custom_nickname_values ; " 
		Category="commit"
		OutputFilename=$TempDir/insertintocustomnickname.lst ;	
		getResult 
		Category=""
	fi
	
	Query="SELECT id FROM list_configs WHERE name = '$CustomNickname' ; " 
	OutputFilename=$TempDir/listid.lst ;	
	getResult 
		
	status=`grep -ic "no rows selected" $TempDir/listid.lst`
	if [ $status -ne 0 ]
	then
		echo "ListConfig Entries not found going for insertion..........."
		insertListConfigEntry
	fi

	listConfigID=`grep "[0-9]*" $TempDir/listid.lst`
	Query="SELECT id FROM list_details WHERE value = 
				'SELECT value FROM custom_nickname_values WHERE custom_nickname_id = $CustomNicknameID' ;" 

	getResult 
	status="$?"
	if [ $? -ne 0 ]
	then
		echo "Statement Failed:  " $Query
		exit 1 	
	fi 
	Query=" INSERT INTO reload_configurations (id, reload_key, reload_code, reload_type, reload_entity_id) VALUES
				(reload_configurations_seq.nextval,'PROGRAM_MANAGER.RELOAD.CODE'||$ReloadCode, $ReloadCode, $ModifyReload,'SELECT value FROM custom_nickname_values WHERE custom_nickname_id = $CustomNicknameID') ; "
	OutputFilename=$TempDir/reloadlist.lst ;	
	getResult 
	status="$?"
	if [ $? -ne 0 ]
	then
		echo "Statement Failed:  " $Query
		exit 1 	
	fi 
}

insertListConfigEntry()
{
	$SQL_COMMAND -s /nolog << EOF > $TempDir/listconfigs.lst  2>&1
	CONNECT_TO_SQL
		whenever sqlerror exit 5;
		whenever oserror exit 6;
		set heading off;
		set feedback off ;
		insert into list_configs(id, name, list_type, data_type, field_category_id)
			select list_configs_seq.nextval, '$Nickname_'||field_categories.name,
				   $ListType, field_categories.data_type, field_categories.id from
			field_categories where field_categories.id = $FieldCategoryId ;
		commit ; 
		quit;
EOF
	sqlretcode=`echo $?`
	if [ $sqlretcode -ne 0 ]
	then
		errorMessage="Statement failed:\n insert into list_configs(id, name, list_type, data_type, field_category_id)
			select list_configs_seq.nextval, '$Nickname_'||field_categories.name,
				   $ListType, field_categories.data_type, field_categories.id from
			field_categories where field_categories.id = $FieldCategoryId"
		echo $errorMessage
		exit 1;
	fi
}
sendSignalToProgramManager()
{
	$RANGERHOME/sbin/scriptlauncher -R
	echo "Sent ReloadSignal to Program Manager."
}

on_sigterm()
{
	rm -f $COMMON_MOUNT_POINT/FMSData/Scheduler/$ProcessID
	exit 0
}

getResult()
{
	$SQL_COMMAND -s /nolog << EOF> temp  2>&1
	CONNECT_TO_SQL
	whenever sqlerror exit 5;
	whenever oserror exit 6;
	set heading off ;
	set feedback off ;
	spool $OutputFilename ;	
	$Query
	quit;
EOF
	`rm -f temp`
}

############### Script Execution starts Here #########
ProcessID=$1'Pid'$2
echo "ProcessID: $ProcessID"
trap on_sigterm SIGINT SIGTERM SIGKILL 

checkParameters $*

ModifyReload=2
ReloadCode=3
SIGHUP=1
TempDir="$COMMON_MOUNT_POINT/tmp"
Nickname="$1"
CustomNickname=""
FieldCategoryId="$2"
NicknameType="$3"
NicknameDirectory="$COMMON_MOUNT_POINT/FMSData/CustomNicknamesData"
NicknameInputDirectory="$NicknameDirectory/$Nickname"_"$FieldCategoryId/new/"
NicknameOutputDirectory="$NicknameDirectory/$Nickname"_"$FieldCategoryId/loaded/"
NicknameControlFile="$COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/customnicknameloader.ctl"
NicknameDB2Command="$COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/customnicknameloader.ctl.db2"
dataFileForDB2="$NicknameInputDirectory/datafile.data.db2"

Category=""
ListType=0
ValidityPeriod=90
ValueType=2
FieldCategoryName=""

echo "`basename $0` started @`date +%D\ %T` for Nickname \"$Nickname\""
removeCtlFile

getFieldCategoryName
checkForInputFiles
getnickNameID
generateNicknameControlFile
generateNicknameControlFileForDB2
cleanupCustomNicknameTables
loadNewNumbers
loadListDetails
sendSignalToProgramManager

removeCtlFile

export LD_PRELOAD_64=$TMP_LD_PRELOAD_64
