#! /bin/bash

CreateNicknameScript()
{

	rm -f $nickname_file
	file_name=`basename $nickname_file`

	echo "#! /bin/bash" > $nickname_file
	echo '	scriptlauncher $WEBSERVER_RANGER_HOME/bin/customnicknameload.sh ' "\"$Nickname\" $FieldCategoryId $NicknameType" >> $nickname_file

	chmod u+x $nickname_file
}

CreateNickname()
{
	$SQL_COMMAND -s /nolog << EOF
	CONNECT_TO_SQL
	whenever oserror exit 5;
	whenever sqlerror exit 5;
	--set feedback off;
	--set heading off;
	set serveroutput on ;
		exec NICKNAME_SETUP('$Nickname', '$NicknameType', $FieldCategoryId, $ValidityPeriod, $ValueType, $ListType) ;
		delete from scheduler_command_maps where job_name = 'Custom Nickname - New Customer' ;
		delete from scheduler_command_maps where job_name like 'Custom Nickname - ' || '$Nickname' ;
		insert into scheduler_command_maps (job_name, command, is_parameter_required, is_network_based, report_file_name)
			values
		('Custom Nickname - ' || '$Nickname', '$nickname_file_with_relative_path,' || '$Nickname'|| 'Pid' ||'$FieldCategoryId', 0, 0, '' );
EOF
	if test $? -ne 0
	then
		echo "Error: Failed to create Nickname: $Nickname - SQL error!!!"
		exit 1
	fi
}


CreateDirectories()
{
	mkdir -p $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/	
	mkdir -p $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/"$Nickname"_"$FieldCategoryId"
	mkdir -p $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/"$Nickname"_"$FieldCategoryId"/new
	mkdir -p $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/"$Nickname"_"$FieldCategoryId"/loaded
}


ValidityPeriod=90
Nickname=""
NicknameType=""
ValueType=2
ListType=0

if test $# -lt 3 
then
	echo "Usage: scriptlauncher customnicknamesetup.sh <Nickname> <Type> <Field CategoryID> [<Validity Period = $ValidityPeriod>]"
	echo "Type can be either DAILYADDITION or REPLACEALL"
	exit 1;
fi

if ! test -d $WEBSERVER_RANGER_HOME/ || ! test -d $COMMON_MOUNT_POINT/FMSData/
then
	echo "Error: Ranger not installed!!!"
	exit 1
fi

Nickname=$1
NicknameType=$2
FieldCategoryId=$3

if [ $NicknameType != "DAILYADDITION" ] && [ $NicknameType != "REPLACEALL" ]
then
	echo "Error: Invalid nickname type!!! [should be DAILYADDITION/REPLACEALL]"
	exit 1
fi

if test $# -eq 4
then
	ValidityPeriod=$4
fi

nickname_file="$WEBSERVER_RANGER_HOME/bin/customnicknameload_$Nickname$NicknameType$FieldCategoryId.sh"
nickname_file_with_relative_path="customnicknameload_$Nickname$NicknameType$FieldCategoryId.sh"

ValueType=2
CreateNicknameScript
CreateNickname
CreateDirectories
