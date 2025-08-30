#! /bin/bash
#################################################################################
#  Copyright (c) Subex Azure Limited 2006. All rights reserved.                 #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

# This Script is used to setup nikira with various options
# to use default options you can use ./setup.sh < .defaults

FilesPath="."
FileList=`find $FilesPath -name "*.in"`
ParseFileList=`find $FilesPath -name "*.parse.in"`

DashBoardUserExists=0

REMOVE_PATTERN='*&%#~!^%';
ESC_REMOVE_PATTERN='\*\&\%\#\~\!\^\%';

GetInput()
{
	display=$1
	Input=""
	while [ "$Input" != "Y" ] && [ "$Input" != "N" ] &&  [ "$Input" != "y" ] && [ "$Input" != "n" ];
	do
		read -e -d';' -p"$display [Y/N] ?" -n 1 Input;
	done
	return $([ "$Input" == "Y" ] || [ "$Input" == "y" ]);
}

GetModuleOption()
{
	display="Which module do you want to install? Enter your choice (FP - 1, Internal Affairs - 2, Alias - 3): "
	Input=""
	while [ "$Input" != 1 ] && [ "$Input" != 2 ] &&  [ "$Input" != 3 ];
	do
		read -e -d';' -p"$display" -n 1 Input;
	done
	return $Input
}

Substitute()
{
	VAR=$1
	VALUE=$(eval "echo \$$VAR")

	SubstituteFileList="$2"
	if [ "$SubstituteFileList" = "" ];then
		SubstituteFileList=$FileList
	fi;

	for file in $SubstituteFileList
	do
		perl -pi -e "s/\@$VAR\@/$VALUE/g" ${file%'.in'}
	done
}

get_ip() {
    os=`uname`
    ip=""
    if [ "$os" == "Linux" ]
    then
        ip=`/sbin/ifconfig  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`
    else
        if [ "$os" == "SunOS" ]
        then
            ip=`ifconfig -a | grep inet | grep -v '127.0.0.1' | awk '{ print $2} '`
        fi
    fi
    echo $ip
}

printDashboardErrorMessage() {
	echo
	echo "Before setting up Dashboard for Nikira, you need to do certain pre-configurations..."
	echo "I am unable to proceed since, these configurations have not been done"
	echo "Refer 'Dashboard.txt' in AppNotes folder for the configuration and installation notes"
	echo
	exit
}

isDashBoardUserExists() 
{
dashboard_log="dashboard.log"
sqlplus ${DASHBOARD_DB_USER}/${DASHBOARD_DB_PASSWORD} << EOF > $dashboard_log 2>&1
WHENEVER OSERROR  EXIT 6 ;
WHENEVER SQLERROR EXIT 5 ;
var exit_val number ;
var number_of_tables number ;

declare
	resource_busy EXCEPTION ;
	pragma exception_init (resource_busy,-54) ;
begin
	:exit_val := 0 ;
	:number_of_tables := 0 ;
exception
	when resource_busy then
		:exit_val := 99 ;
	when others then
		:exit_val := 6 ;
end ;
/
exit :exit_val ;
EOF
  retVal=$?
  rm -f $dashboard_log

  if [ $retVal -ne 0 ]
  then
    printDashboardErrorMessage
  fi

  rm -rf $dashboard_log
  DashBoardUserExists=1
  return 0 
}


SetAdditionalVariables()
{
	NETWORKID=0;
	EQUIPMENTIDVISIBLE=1
	IMSIVISIBLE=1
	prefix=$RANGERHOME
	prefix=$(echo $prefix | sed -e 's/\//\\\//g')
	ipaddress=`get_ip`
	prevea_prefix=$PREVEAHOME
	TABLESPACEPREFIX=$TABLESPACE_PREFIX
	prevea_prefix=$(echo $prevea_prefix | sed -e 's/\//\\\//g')
	GPRS_FEATURE=1;
	dashboard_user=$DASHBOARD_DB_PASSWORD
	INSTALLATION_TYPE=$selectedModule
	DB_USER=$DB_USER

	Substitute "prefix"
	Substitute "ipaddress"
	Substitute "prevea_prefix"
	Substitute "NETWORKID"
	Substitute "EQUIPMENTIDVISIBLE"
	Substitute "IMSIVISIBLE"
	Substitute "GPRS_FEATURE"
	Substitute "INSTALLATION_TYPE"
	Substitute "DB_USER"
	Substitute "DB_USER"
	Substitute "TABLESPACEPREFIX"
}

if [ "$RANGERHOME" == "" ];then
	echo "Warning: RANGERHOME is not set"
	echo -en "\nEnter RANGERHOME [press <enter> to quit]: "
	read RANGERHOME
	if [ "$RANGERHOME" == "" ];then
		exit 0
	fi
fi

if [ "$PREVEAHOME" == "" ];then
	echo "Warning: PREVEAHOME is not set"
	echo -en "\nEnter PREVEAHOME [press <enter> to quit]: "
	read PREVEAHOME 
	if [ "$PREVEAHOME" == "" ];then
		exit 0
	fi
fi

TABLESPACE_PREFIX=""
echo -en "\nEnter table space prefix [press <enter> to quit]: "
read TABLESPACE_PREFIX
if [ "x$TABLESPACE_PREFIX" == "x" ];then
	exit 0
fi

if [ "${DASHBOARD_DB_USER}x" == "x" ]
then
	DASHBOARD_DB_USER='dashboard'
fi

if [ "${DASHBOARD_DB_PASSWORD}x" == "x" ]
then
	DASHBOARD_DB_PASSWORD='dashboard'
fi
                              
FULL_FEATURE_LIST="ORACLE DB2"

declare -a FullFeatureString; 
FullFeatureString=($FULL_FEATURE_LIST);

for((index=0;$index<${#FullFeatureString[*]};++index))
do
	eval ${FullFeatureString[$index]}_=1;
done		

clear;
echo -e "FMS V8.0.1 Setup Script \n";
isFull=0

if [ $isFull -eq 0 ]
then
	FEATURE_LIST=$FULL_FEATURE_LIST
	selectedModule=6
	echo
	echo -e "Select the features .....\n";
fi

declare -a FeatureString; 
FeatureString=($FEATURE_LIST);

for((index=0;$index<${#FeatureString[*]};++index))
do
	GetInput ${FeatureString[$index]};
	retVal=$?
	eval ${FeatureString[$index]}_=$retVal;

	if [ ${FeatureString[$index]} == 'INTERNAL_USER' -a  $retVal -eq 0 ]
	then
		selectedModule=4
	fi

	if [ ${FeatureString[$index]} == 'FINGER_PRINT_PROFILE' -a  $retVal -eq 0 ]
	then
		if [ $selectedModule -eq 4 ]
		then
			selectedModule=0
		else
			selectedModule=5
		fi
	fi

	if [ ${FeatureString[$index]} == 'DASHBOARD' -a $retVal -eq 0 ]
	then
		isDashBoardUserExists
	fi

done		

clear;
echo -e "\nNikira: Features Enabled for V7 DB Setup \n";
> .setup_options

for((index=0;$index<${#FeatureString[*]};++index))
do
	FeatureEnabled=$(eval "echo \$${FeatureString[$index]}_")
    if [ $FeatureEnabled -eq 0 ]; then
   		echo ${FeatureString[$index]};
		echo ${FeatureString[$index]} >> .setup_options
    fi
done		

echo 
GetInput "Do you Wish to continue"
CONTINUE=$?;

if [ $CONTINUE -eq 1 ];then
	echo "Exiting ..."
	exit 0 ;
fi

for file in $FileList
do
	cp $file ${file%".in"}
done

for((index=0;$index<${#FullFeatureString[*]};++index))
do
	if [ $(eval "echo \$${FullFeatureString[$index]}_") -eq 0 ];then
		eval NOT${FullFeatureString[$index]}='$REMOVE_PATTERN';
		eval ${FullFeatureString[$index]}='';
	else
		eval ${FullFeatureString[$index]}='$REMOVE_PATTERN';
		eval NOT${FullFeatureString[$index]}='';
	fi
	Substitute "${FullFeatureString[$index]}"
	Substitute "NOT${FullFeatureString[$index]}"
done		

SetAdditionalVariables

for InputFile in $FileList
do
	configure_input=$(echo "Generated from $InputFile by setup.sh at `date`" | sed -e 's/\//\\\//g')
	Substitute "configure_input" $InputFile
done

for InputFile in $ParseFileList
do
	sed /"$ESC_REMOVE_PATTERN"/d ${InputFile%".in"} > ${InputFile%".parse.in"}
done

find $FilesPath -name "*.parse" -exec rm -f {} \;
chmod +x *.sh
echo "Completed Successfully."
