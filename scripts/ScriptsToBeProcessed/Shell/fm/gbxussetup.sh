#! /bin/bash
#################################################################################
#  Copyright (c) Subex Azure Limited 2006. All rights reserved.                 #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

FilesPath="."
FileList="$FilesPath/*.in"
ParseFileList="$FilesPath/*.parse.in"

REMOVE_PATTERN='*&%#~!^%';
ESC_REMOVE_PATTERN='\*\&\%\#\~\!\^\%';
FEATURE_LIST="GSM FIXED STANDALONE AUTOACCOUNTCLOSE"

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

Substitute()
{
	VAR=$1
	VALUE=$(eval "echo \$$VAR")
	for file in $FileList
	do
		perl -pi -e "s/\@$VAR\@/$VALUE/g" ${file%'.in'}
	done
}


SetAdditionalVariables()
{
	NETWORKID=0;
	EQUIPMENTIDVISIBLE=1
	IMSIVISIBLE=1
	prefix=$RANGERHOME
	prefix=$(echo $prefix | sed -e 's/\//\\\//g')
	GPRS_FEATURE=0

	Substitute "prefix"
	Substitute "NETWORKID"
	Substitute "EQUIPMENTIDVISIBLE"
	Substitute "IMSIVISIBLE"
	Substitute "GPRS_FEATURE"
}

if [ "$RANGERHOME" == "" ];then
	echo "Warning: RANGERHOME is not set"
	echo -en "\nEnter RANGERHOME [press <enter> to quit]: "
	read RANGERHOME
	if [ "$RANGERHOME" == "" ];then
		exit 0
	fi
fi

clear;
echo -e "***************NIKIRA V6.0 SETUP SCRIPT FOR GBX USA******************** \n";
echo -e "*********PLEASE SELECT GSM,FIXED,AUTOACCOUNTCLOSE OPTIONS***************\n "
echo -e "---INSTALATION INFORMATION::\n";

declare -a FeatureString; 
FeatureString=($FEATURE_LIST);

for((index=0;$index<${#FeatureString[*]};++index))
do
	eval ${FeatureString[$index]}_=0;
done		

clear;
echo -e "\n*************NIKIRA V6.0 FEATURS ENABLED FOR GBX USA*************** \n";

for((index=0;$index<${#FeatureString[*]};++index))
do
	FeatureEnabled=$(eval "echo \$${FeatureString[$index]}_")
    if [ $FeatureEnabled -eq 0 ]; then
   		echo ${FeatureString[$index]};
    fi
done		

echo 
GetInput "Do You Wish To Continue???"
CONTINUE=$?;

if [ $CONTINUE -eq 1 ];then
	echo "Exiting From Setup ..."
	exit 0 ;
fi

for file in $FileList
do
	cp $file ${file%".in"}
done

for((index=0;$index<${#FeatureString[*]};++index))
do
	if [ $(eval "echo \$${FeatureString[$index]}_") -eq 0 ];then
		eval NOT${FeatureString[$index]}='$REMOVE_PATTERN';
		eval ${FeatureString[$index]}='';
	else
		eval ${FeatureString[$index]}='$REMOVE_PATTERN';
		eval NOT${FeatureString[$index]}='';
	fi
	Substitute "${FeatureString[$index]}"
	Substitute "NOT${FeatureString[$index]}"
done		

SetAdditionalVariables

for InputFile in $ParseFileList
do
	configure_input=$(echo "Generated from $InputFile by setup.sh at `date`" | sed -e 's/\//\\\//g')
	Substitute "configure_input"
	sed /"$ESC_REMOVE_PATTERN"/d ${InputFile%".in"} > ${InputFile%".parse.in"}
done

rm -f $FilesPath/*.parse
chmod +x *.sh
echo -e "****************STARTING NIKIRA V6.0 SETUP*************************\n "

mkdir -p $RANGERHOME/FMSData/Datasource/CDR
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/success
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/Log
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/Rejected

mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber
mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber/success
mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber/Log
mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber/Rejected

if [ $? -ne 0 ]; then
		echo -e "********SOME PROBLEM OCCURED IN SETUP**********************\n"
        exit 5
fi

echo -e "***************NIKIRA V6.0 SETUP COMEPLETED SUCESSFULLY****\n"
echo -e "HIT ENTER TO CONTINUE....."
read dummy
echo -e "***************STARTING NIKIRA v6.0 DATABASE SETUP****************\n"
./gbxusdbsetup.sh
echo -e "***************NIKIRA V6.0 DATABASE SETUP COMEPLETED SUCESSFULLY***\n"
