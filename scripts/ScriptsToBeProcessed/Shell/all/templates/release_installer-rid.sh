#/*******************************************************************************
# *  Copyright (c) Subex Limited 2013. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from Subex Limited or in accordance with the terms and          *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************/
#!/bin/bash

# Display Related Settings Starts
SC_COLUMNS=$(tput cols)
SC_ROWS=$(tput lines)
BACKGROUND_COLOR=3
FOREGROUND_COLOR=5
SET_BACKGROUND=$(tput setaf $BACKGROUND_COLOR)
SETFOREGROUND=$(tput setaf $FOREGROUND_COLOR)
clear
# Display Related Settings Ends

# Global Variable Starts
CURR_DIR=`pwd`
LOG_FILE="$CURR_DIR/release.log"
LOG_DIR=$CURR_DIR
RELEASE_ID="Not Assigned"
RELEASES_DIR=""
WELCOME_STRING="A Subex Release"
HEADER="Not Created"
RUN_STATUS="FAILED/TERMINATED/ABORTED"
# Global Variable Ends

function ProgressBar {
        let _progress=(${1}*100/${2}*100)/100
        let _done=(${_progress}*4)/10
        let _left=40-$_done
        _fill=$(printf "%${_done}s")
        _empty=$(printf "%${_left}s")
        printf "\r        PROGRESS : [${_fill// /#}${_empty// /-}] ${_progress}%%"
}
function FramePrint()
{
	echo -e "~~~~~~~~~~~~~~~~~~~~~~~~~" >> $LOG_FILE	
	clear
	tput sgr0
	tput bold
	echo -n $SET_BACKGROUND$SETFOREGROUND
	printf '%*s\n' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	printf "%*s\n" $(((${#HEADER}+$SC_COLUMNS)/2)) "$HEADER"
	printf '%*s\n' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	tput cup $((SC_ROWS-5)) 0
	printf '\n%*s' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	tput cup $((SC_ROWS-3)) 0
	ProgressBar $1 100
	tput cup $((SC_ROWS-3)) 68
	printf "%s" "^CURRENT SECTION: [$2] $3"
	tput cup $((SC_ROWS-1)) 0
	printf '%*s' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	tput sgr0
}

function SetProgressBar()
{
	tput sgr0
        tput bold
        echo -n $SET_BACKGROUND$SETFOREGROUND
	tput cup $((SC_ROWS-3)) 0
        ProgressBar $1 100
	tput sgr0
}

function CheckIfRunningFromInsideDirectory()
{
	if [ ! -f $CURR_DIR/.inside_lock ]
	then
		printf "\t%s\n" "[ERROR] Run Release Script from Release directory only"
		exit 1001
	fi
}
function getEmptySpace()
{
	if [ ! -d $1 ]
	then
		return 0
	fi
	space=`df -BG $1 | egrep -v Available | awk -F" " '{ print $4}' | awk -F"G" '{ print $1}'`
	return $space
}
function WelcomeText()
{
	printf "%*s\n" $(((${#WELCOME_TEXT}+$SC_COLUMNS)/2)) "$WELCOME_TEXT"
	echo -e $WELCOME_TEXT >> $LOG_FILE
}

function GetReleaseNumber()
{
	if [ "$RELEASE_ID" == "" ];then
		printf "\n\t%s\n" "[ERROR] Release Number is missing"
                exit 1006
	fi
}

function ReleaseRunInfo()
{
	printf ""
}

function CurrentSystemInfo()
{
	printf "\n\t%s\n" "[INFO] Release Install Platform"
	printf "\t\t\t%s\n" "IP ADDRESS [$(ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/')]"
	printf "\t\t\t%s\n" "HOST NAME [$(hostname --long)]"
	printf "\t\t\t%s\n" "Kernel Version - [$(uname -mrs)]"
	printf "\t\n"
}

function CheckIfAlreadyRunning()
{
	PID=`ps -eaf | grep {$0} | awk -F" " '{ print $2}'`
        if [ -f $CURR_DIR/logs/.$PID ]
	then
		printf "\t%s\n" "[ERROR] Release script[$0] is already running, Hence exiting"
		exit 1003	
	fi 
	touch $CURR_DIR/logs/.$PID
}

function CheckIfLastRunCompleted()
{
	if [ -f $CURR_DIR/logs/.lock_script ]
	then
	        printf "\t%s\n" "[ERROR] Last run was not successful, Cant Proceed, Please contact Subex Support"
        	exit 1004
	fi
	touch $CURR_DIR/logs/.lock_script
}
function CheckIfAlreadyInstalled()
{
        if [ -f $CURR_DIR/logs/.lock_already_installed ]
        then
                printf "\t%s\n" "[ERROR] This Release Is Already Installed, Cant Install Again"
                exit 1004
        fi
        touch $CURR_DIR/logs/.lock_already_installed
}
function FormatedPrint()
{
	echo "$2" 
	echo "$((${#$2}))"
	printf "%*s\n" $(((${#$2}+$SC_COLUMNS)/$1)) $2
}

function LoadConfigurations()
{
        if [ ! -f $CURR_DIR/config/configurations.file ]
        then
                printf "\t%s\n" "[ERROR] Unable to load Configurations, Missing Configurations File [$CURR_DIR/config/configurations.file]"
                exit 1005
        fi
	chmod 755 $CURR_DIR/config/configurations.file
        . $CURR_DIR/config/configurations.file
	if [ $? -ne 0 ]; then
		printf "\t%s\n" "[ERROR] Unable to load Configurations"
		exit 1006
	fi	
        touch $CURR_DIR/logs/touchx.log
	rm $CURR_DIR/logs/*.log
	LOG_DIR="$CURR_DIR/logs"
        LOG_FILE=$CURR_DIR/logs/release_installer.log
	> $LOG_FILE
	HEADER=$WELCOME_STRING" [$RELEASE_ID] FOR [$CUSTOMER_NAME] #DATE $(date +%Y-%m-%d:%H:%M:%S) #IP [$(ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/')]"
	echo -e $HR_LINE > $LOG_FILE
	echo -e $HEADER >> $LOG_FILE
	echo -e $HR_LINE >> $LOG_FILE
	echo -e "[INFO] Configurations are loaded from $CURR_DIR/config/configurations.file" >> $LOG_FILE
}

function UnInitialize()
{
	rm $CURR_DIR/logs/.$PID
	rm $CURR_DIR/logs/.lock_script
	RUN_STATUS='COMPLETED SUCCESSFULLY'
}

function SetLogFile()
{
	printf "\t%s\n" "[INFO] Release Log file : [$LOG_FILE]"
	printf "\t%s\n" "[INFO] Release Log Directory : [$CURR_DIR/logs]"
}
function PrintProductRelatedInformation()
{
        printf "\n\t%s\n" "[INFO] PRODUCT NAME : $PRODUCT_NAME"
	printf "\t%s\n" "[INFO] PRODUCT VERSION : $PRODUCT_VERSION"
	printf "\t%s\n" "[INFO] PRODUCT PATCH VERSION= : $PRODUCT_PATCH_VERSION"
	printf "\t\n"
}

function PrintContactDetails()
{
	c="DEVELOPER : $DEVELOPER_CONTACT"
	printf "\n%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="DEVELOPER LEAD : $DEVELOPER_LEAD_CONTACT"
        printf "%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="DEVELOPER MANAGER : $DEVELOPER_MANAGER_CONTACT"
        printf "%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="TESTER : $TESTER_CONTACT"
        printf "\n%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="TESTER LEAD : $TESTER_LEAD_CONTACT"
        printf "%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="TESTER MANAGER : $TESTER_MANAGER_CONTACT"
        printf "%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE

        c="SUPPORT ENGINEER : $SUPPORT_CONTACT"
        printf "\n%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="SUPPORT LEAD : $SUPPORT_LEAD_CONTACT"
        printf "%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
        c="SUPPORT MANAGER : $SUPPORT_MANAGER_CONTACT"
        printf "%*s\n" $(((${#c}+$SC_COLUMNS)/2)) "$c"
	echo -e $c >> $LOG_FILE
	echo -e $HR_LINE >> $LOG_FILE
	printf "\n"
}

function PrintCopyRightInformation()
{
        tput bold
	echo -e $HR_LINE >> $LOG_FILE
#c1="Copyright (c) Subex Limited $COPYRIGHT_YEAR. All rights reserved."
#	printf "\n%*s\n" $(((${#c1}+$SC_COLUMNS)/2)) "$c1"
#	echo -e $c1 >> $LOG_FILE
#c2="The copyright to the computer program(s) herein is the property of Subex Limited."
#	printf "%*s\n" $(((${#c2}+$SC_COLUMNS)/2)) "$c2"
#	echo -e $c2 >> $LOG_FILE
#c3="The program(s) may be used and/or copied with the written permission from Subex"
#	printf "%*s\n" $(((${#c3}+$SC_COLUMNS)/2)) "$c3"
#	echo -e $c3 >> $LOG_FILE
#c4="Limited or in accordance with the terms and conditions stipulated in the"
#	printf "%*s\n" $(((${#c4}+$SC_COLUMNS)/2)) "$c4"
#	echo -e $c4 >> $LOG_FILE
#c5="agreement/contract under which the program(s) have been supplied."
#	printf "%*s\n" $(((${#c5}+$SC_COLUMNS)/2)) "$c5"
#	echo -e $c5 >> $LOG_FILE
        if [ -f $CURR_DIR/config/copyright.text ]
        then
                while read line; do
                c1=$line
		printf "\n%*s" $(((${#c1}+$SC_COLUMNS)/2)) "$c1"
		echo -e $c2 >> $LOG_FILE
		done < $CURR_DIR/config/copyright.text
        fi
	echo -e $HR_LINE >> $LOG_FILE
	tput sgr0
}

function PrintSubexContactInformation()
{
	c5="TO BE ADDED LATER"
        printf "\n\n%*s\n" $(((${#c5}+$SC_COLUMNS)/2)) "$c5"
	echo -e $c5 >> $LOG_FILE
}

GetConfirmYes()
{
	display=$1
        Input=""
	while [ "$Input" != "Y" ] && [ "$Input" != "y" ];
    	do
        	read -e -d';' -p"        $display [Y/N] ?" -n 1 Input;
		echo -e "[INFO] $line : Customer Input[$Input]" >> $LOG_FILE
    	done
        return $([ "$Input" == "Y" ] || [ "$Input" == "y" ]);
}

function PrintPreRequisiteAndGetUserConfirmation()
{
	echo -e "PROMPTING USER CONFIRMATIONS" >> $LOG_FILE
        if [ -f $CURR_DIR/config/prompted_questions ]
        then
		printf "\t%s\n" "[INFO] RELESE PRE REQUISITES, PLEASE CONFIRM [Mandatory] $CURR_DIR/config/prompted_questions"
		echo -e "[INFO] RELESE PRE REQUISITES, PLEASE CONFIRM [Mandatory] $CURR_DIR/config/prompted_questions" >> $LOG_FILE
		index=0
		counter=0
		while read line; do
			(( counter!=0 )) && lines[$index]="$line"
			(( counter!=0 )) && index=$(($index+1))
			counter=$(($counter+1))
		done < $CURR_DIR/config/prompted_questions
		for line in "${lines[@]}"
		do
			GetConfirmYes "$line"
			echo -e "[INFO] $line : Customer Confirmed" >> $LOG_FILE
		done
	fi
}

function CheckingMemoryRequirements()
{
        if [ -f $CURR_DIR/config/memory_limits.conf ]
        then
                printf "\n\t%s\n" "[INFO] Checking Memory Availability, Reading Memory Requirements $CURR_DIR/config/memory_limits.conf"
                echo -e "[INFO] Checking Memory Availability, Reading Memory Requirements $CURR_DIR/config/memory_limits.conf" >> $LOG_FILE
                index=0
		counter=0
                while read line; do
                        (( counter!=0 )) && lines[$index]="$line"
                        (( counter!=0 )) && index=$(($index+1))
                        counter=$(($counter+1))
               	done < $CURR_DIR/config/memory_limits.conf
                for line in "${lines[@]}"
                do
        		dir=$(echo $line | awk -F"#" '{ print $1 }')
			required=$(echo "$line" | awk -F"#" '{ print $2 }')
			NumericReg='^[0-9]+$'
			if ! [[ $required =~ $NumericReg ]] ; then
				printf "\t%s\n\tCan't Proceed, EXITING" "[ERROR] Invalid Space value[$required] for [$dir] entry in [$CURR_DIR/config/memory_limits.conf] File"
   				echo -e "[ERROR] Invalid Space value[$required] for [$dir] entry in [$CURR_DIR/config/memory_limits.conf] File\n\tCan't Proceed, EXITING" >> $LOG_FILE
				exit 202
			fi
        		getEmptySpace $dir
        		space=$?
        		printf "\t%s\n" "[INFO] Memory Required [$required GB] for Mount[$dir]: Available [$space GB]"
			echo -e "[INFO] Memory Required [$required GB] for Mount[$dir]: Available [$space GB]" >> $LOG_FILE
			if [ $required -ge $space ]; then
				printf "\t%s\n" "[ERROR] Insufficient Memory Required, Can't Proceed, EXITING"
				echo -e "[ERROR] Insufficient Memory Required, Can't Proceed, EXITING" >> $LOG_FILE
				exit 201
			fi
                done
        fi
}

function CheckingServerConnectivity()
{
        if [ -f $CURR_DIR/config/host_address.conf ]
        then
                printf "\n\t%s" "[INFO] Checking IP Host Availability, Reading Host Requirements $CURR_DIR/config/host_address.conf"
                echo -e "[INFO] Checking IP Host Availability, Reading Host Requirements $CURR_DIR/config/host_address.conf" >> $LOG_FILE
                index=0
                counter=0
                while read line; do
                        (( counter!=0 )) && alines[$index]="$line"
                        (( counter!=0 )) && index=$(($index+1))
                        counter=$(($counter+1))
                done < $CURR_DIR/config/host_address.conf
		line=""
                for line in "${alines[@]}"
                do
                        ip=$(echo $line | awk -F"#" '{ print $1 }')
			host=$(echo $line | awk -F"#" '{ print $2 }')
			user=$(echo $line | awk -F"#" '{ print $3 }')

                        IPReg='^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$'
                        if ! [[ $ip =~ $IPReg ]] ; then
                                printf "\t%s\n\tCan't Proceed, EXITING" "[ERROR] Invalid IP value[$ip] entry in [$CURR_DIR/config/host_address.conf] File"
                                echo -e "[ERROR] Invalid IP value[$ip] entry in [$CURR_DIR/config/host_address.conf] File\n\tCan't Proceed, EXITING" >> $LOG_FILE
                                exit 202
                        fi
			ping -c 5 $ip &> /dev/null; 
			if [ $? -eq 0 ]; then
				ping_report=`ping -c 5 10.113.58.198 | tail -1 | awk -F" = " '{print $2}'`
				printf "\n\t%s" "[INFO] HOST IP[$ip] is Available, Ping Status [$ping_report]"
				echo -e "[INFO] HOST IP[$ip] is Available, Ping Status [$ping_report]" >> $LOG_FILE
			else
				printf "\n\t%s" "[ERROR] HOST IP[$ip] is Not Available, Ping Status [5/5 Failed]"
				echo -e "[ERROR] HOST IP[$ip] is Not Available, Ping Status [5/5 Failed]" >> $LOG_FILE
                                exit 204
                        fi
                done
        fi
	printf "\n"
}

function CheckEnvironmentVariableValue()
{
	 printf "\n"	
}


function CheckEnvironmentVariables()
{
        if [ -f $CURR_DIR/tools/check_environment.pl ]
        then
		chmod 755 $CURR_DIR/tools/check_environment.pl
		#perl $CURR_DIR/tools/check_environment.pl $CURR_DIR/config/environment_var.list
		if [ $? -ne 0 ]; then	
			printf "\n\t%s" "[ERROR] Module $0 Failed"
			echo -e "[ERROR] Module $0 Failed" >> $LOG_FILE
			exit 205
		fi
	else
		printf "\t%s\n" "[ERROR] Tool Module $0 Missing, Quitting"
		echo -e "[ERROR] Tool Module $0 Missing, Quitting" >> $LOG_FILE
		exit 205
        fi
	# Checking Enviromental Variable
	ev="Environmental Variable"
	printf "\t%s\n" "[INFO] Checking $ev"
	e="[DB_HOSTNAME] $ev is ";
	if [ -z $"DB_HOSTNAME" ]; then echo -e "[ERROR] $e not set" >> $LOG_FILE; printf "\t%s\n" "[ERROR] $e is not set"; exit 77; 
	else echo -e "[INFO] $e set to [$DB_HOSTNAME]" >> $LOG_FILE; printf "\t%s\n" "$e set to [$DB_HOSTNAME]"; fi

	e="[DB_PASSWORD] $ev is ";
        if [ -z $"DB_PASSWORD" ]; then echo -e "[ERROR] $e not set" >> $LOG_FILE; printf "\t%s\n" "[ERROR] $e is not set"; exit 77;
        else echo -e "[INFO] $e set to [$DB_PASSWORD]" >> $LOG_FILE; printf "\t%s\n" "[INFO] $e set to [$DB_PASSWORD]"; fi

	
	#if [ -z $"DB_PASSWORD" ] && { e="[ERROR] $ev [DB_PASSWORD] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"DB_USER" ] && { e="[ERROR] $ev [DB_USER] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"HOME" ] && { e="[ERROR] $ev [HOME] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"ORACLE_HOME" ] && { e="[ERROR] $ev [ORACLE_HOME] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"ORACLE_SID" ] && { e="[ERROR] $ev [ORACLE_SID] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"HOSTNAME" ] && { e="[ERROR] $ev [HOSTNAME] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"IP_ADDRESS" ] && { e="[ERROR] $ev [IP_ADDRESS] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"ENCRYPTED_DB_USER" ] && { e="[ERROR] $ev [ENCRYPTED_DB_USER] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
	#if [ -z $"ENCRYPTED_PASSWORD" ] && { e="[ERROR] $ev [ENCRYPTED_PASSWORD] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;

	#if [ -z $"CLASSPATH" ] && { e="[ERROR] $ev [CLASSPATH] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"JAVA_HOME" ] && { e="[ERROR] $ev [JAVA_HOME] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"PATH" ] && { e="[ERROR] $ev [PATH] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"RUBYROOT" ] && { e="[ERROR] $ev [RUBYROOT] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"RANGERROOT" ] && { e="[ERROR] $ev [RANGERROOT] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"NIKIRAROOT" ] && { e="[ERROR] $ev [NIKIRAROOT] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"SPARK_DB_PASSWORD" ] && { e="[ERROR] $ev [SPARK_DB_PASSWORD] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
        #if [ -z $"SPARK_DB_USER" ] && { e="[ERROR] $ev [SPARK_DB_USER] is not set"; echo -e $e >> $LOG_FILE; printf "\t%s\n" "$e"; } && exit 77;
}

function PrintGetEnvironmentConfirmation()
{
	echo -e $HR_LINE >> $LOG_FILE
printf "\n\t%s\n" "[INFO] Checking System Environment, Storing Environment Backup Internally"
	env > $CURR_DIR/logs/.before_release-254.environment
	echo -e "[INFO] Checking System Environment, Storing Environment Backup Internally to [$CURR_DIR/logs/.before_release-254.environment]" >> $LOG_FILE
	CheckEnvironmentVariables	
	
}

function CheckLinuxConnectivity()
{
	printf "\n"
}

function PromptPressToContinue()
{
	tput bold
	tput cup $((SC_ROWS-2)) 8
	read -p "Press Any Key To Continue " -n 1 -r
	tput sgr0
}

function PrintCheckResourceRequirements()
{
	printf "\n\t%s\n" "[INFO] Checking Hardware & System Requirements are being fullfilled?"
	CheckingMemoryRequirements
	CheckingServerConnectivity	
	printf "\n\t%s\n" "[INFO] CPU Required [$TOTAL_CPU_REQUIRED], Which must to be ensured by Customer."
	

}

function PrintAboutRelease()
{
	printf "\n\t%s\n" "[INFO] Release Information : [$RELEASE_INFO]"
	printf "\n"
}
function VerifyRelease()
{
        if [ ! -f $CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT ]
        then
        	printf "\n\t%s" "[ERROR] Release File [contents/release-$RELEASE_ID.$TAR_FORMAT] is missing"
                echo -e "[ERROR] Release File [contents/release-$RELEASE_ID.$TAR_FORMAT] is missing" >> $LOG_FILE
                exit 105
	fi
	rcksum=`cksum $CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT | awk -F" " '{ print $1}'`
	echo -e "[INFO] Release Tarball Used [$CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT]" >> $LOG_FILE
	if [ "$rcksum" != "$TARBALL_CKSUM" ]; then
		printf "\n\t%s" "[ERROR] CheckSUM Mismatch Expected[$TARBALL_CKSUM"] Actual[$rcksum]"
		echo -e "[ERROR] CheckSUM Mismatch Expected[$TARBALL_CKSUM"] Actual[$rcksum]" >> $LOG_FILE
		exit 104
	fi
	echo -e "[INFO] Release Tarball [$CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT] cksum [$rcksum]" >> $LOG_FILE
	if [ "$TAR_FORMAT" = "tar.gz" ]; then 
		cd $CURR_DIR/contents
		tar -zxf $CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT
	elif [ "$TAR_FORMAT" = "tar" ]; then
		cd $CURR_DIR/contents
		tar -xf $CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT
	else
		printf "\n\t%s" "[ERROR] Invalid TAR_FORMAT[$TAR_FORMAT] Format"
                echo -e "[ERROR] Invalid TAR_FORMAT[$TAR_FORMAT] Format" >> $LOG_FILE
                exit 106
	fi
	result=$?
	cd $CURR_DIR
        if [ $result -ne 0 -o ! -d $CURR_DIR/contents/release-$RELEASE_ID ]; then
                printf "\t%s\n" "[ERROR] Release Extration Failed"
		echo -e "[ERROR] Release Extration Failed" >> $LOG_FILE
                exit 107
        fi		
	echo -e "[INFO] Release Tarball [$CURR_DIR/contents/release-$RELEASE_ID.$TAR_FORMAT] tarred at [$CURR_DIR/contents/release-$RELEASE_ID/]" >> $LOG_FILE
	RELEASES_DIR=$CURR_DIR/contents/release-$RELEASE_ID
}
function PrintReleaseCaution()
{
        echo -e $HR_LINE >> $LOG_FILE
        if [ -f $CURR_DIR/config/caution.text ]
        then
                while read line; do
                c1=$line
                printf "\n\t%s" "$c1 "
                echo -e $c2 >> $LOG_FILE
		sleep 1
                done < $CURR_DIR/config/caution.text
        fi
        echo -e $HR_LINE >> $LOG_FILE	
}
function Init()
{
	_start=1
	_end=100
	printf "\n\t1. Performing Initial Checks..\n"
	CheckIfRunningFromInsideDirectory
	CheckIfAlreadyRunning
	CheckIfAlreadyInstalled
	CheckIfLastRunCompleted
	for number in $(seq ${_start} ${_end}); do sleep 0.03; ProgressBar ${number} ${_end}; done
	printf "\n\t2. Loading Configurations....\n"
	LoadConfigurations
	for number in $(seq ${_start} ${_end}); do sleep 0.03; ProgressBar ${number} ${_end}; done
        printf "\n\t3. Preparing Subex Release.\n"
	VerifyRelease
        for number in $(seq ${_start} ${_end}); do sleep 0.03; ProgressBar ${number} ${_end}; done
	printf "\n"
	PrintReleaseCaution
	printf "\n\n\t"
	read -p "All SET.!!! Press Any Key To Continue Installing Release" -n 1 -r	
}
function Main()
{
	#------------------------
	Init
	FramePrint 0 "1" "INITIAL VALIDATION"
        tput cup 4 0
	WelcomeText
	PrintCopyRightInformation
	PrintSubexContactInformation
	PrintContactDetails
	PromptPressToContinue
	#-------------------------
	
	FramePrint 20 "1" "INITIAL VALIDATION"
        tput cup 4 0
	SetLogFile
	GetReleaseNumber
        ReleaseRunInfo
	CurrentSystemInfo
	PrintProductRelatedInformation
	PromptPressToContinue
        #-------------------------

        FramePrint 40 "2" "INITIAL VALIDATION"
        tput cup 4 0
	PrintAboutRelease
	PrintPreRequisiteAndGetUserConfirmation

	PromptPressToContinue
	#-------------------------	
	
	FramePrint 60 "3" "INITIAL VALIDATION"
	tput cup 4 0
	PrintGetEnvironmentConfirmation

	PromptPressToContinue
	#-------------------------

	FramePrint 80 "4" "INITIAL VALIDATION"
        tput cup 4 0
        PrintCheckResourceRequirements

        SetProgressBar 100
	PromptPressToContinue
        #-------------------------
	


	### ---------------------------------------------------------------------------------------------
	### EXAMPLE FRAME
	### DEVELOPERS INPUT BELLOW YOUR VALUES	
	### DEVELOPERS CAN EXTRAPOLATE THIS
	FramePrint 100 "0" "DUMMY FRAME" 	# FramePrint <PERCENTAGE_COMPLTED> "<STEP>" "<STEP_NAME>"
	tput cup 4 0				# Dont Change this
	
	### DO your work here


	PromptPressToContinue			# Dont Change this
	### ---------------------------------------------------------------------------------------------
 	







	UnInitialize				# Dont Change this
}
function FinishBye()
{
        tput bold
        e1="RELEASE INSTALLATION $1"
        printf "\n\n\n%*s\n" $(((${#e1}+$SC_COLUMNS)/2)) "$e1"
	echo -e "[INFO] $e1" >> $LOG_FILE
        e1="Release Log file : [$LOG_FILE]"
        printf "%*s\n" $(((${#e1}+$SC_COLUMNS)/2)) "$e1"
	echo -e "[INFO] $e1" >> $LOG_FILE
        e1="Release Log Directory : [$LOG_DIR]"
        printf "%*s\n\n\n" $(((${#e1}+$SC_COLUMNS)/2)) "$e1"
	echo -e "[INFO] $e1" >> $LOG_FILE
	tput sgr0
}
function Finish() {
	FinishBye $RUN_STATUS
}

trap Finish EXIT

Main
