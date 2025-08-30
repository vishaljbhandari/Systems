#/bin/bash

#*******************************************************************************
#* The Script may be used and/or copied with the written permission from
#* Subex Ltd. or in accordance with the terms and conditions stipulated in
#* the agreement/contract (if any) under which the program(s) have been supplied
#*
#* AUTHOR:Saktheesh & Vijayakumar  
#* Modified: 21-MAY-2008

#* Description:
#*	This script will Clean all .svn folders and unwanted files before Extracting Nikira.

#*******************************************************************************
Dat=`date +%Dr%H:%M:%S`
logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_Nikira_Cleanup.log" }'`
echo "logFile = $logFile"
Installation_Time=`date +%Dr#%H:%M:%S`;
NBIT_HOME=`pwd`;
clear
#-----------------------------------------------------Functions Start----------------------------------------------------------------------------#


function echo_log()
{
    echo "$@" 
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile 
}

function log_only()
{
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile 
}

function WARNING
{

		echo_log "********************************* Acceptance Notice ******************************************"
		echo_log "Based on the options selected"
		echo_log "This Script will remove  mysql and java present inside jasper home.";
		echo_log "This will clear all log files ";
		echo_log "This will remove tar files present in NIKIRAROOT,NIKIRACLIENT,NIKIRATOOLS";
		echo_log "*********************************************************************************************"
		echo_log ""
		echo -n "I Accept [Y/N]"
		read -n 1 Accept
				echo_log ""
				echo_log ""
		
		if [ "$Accept" != "y" -a "$Accept" != "Y" ]
		then
				echo_log "Acceptance Failure Cleaning Stopped......";
				echo_log ""
				exit 999;
		fi;
				echo_log "You Accepted the Agreement.";
				echo_log ""
		
}
function ExportSRCPaths
{
        for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep -v "TAR.SOURCE"|grep -v "PATH.SOURCE"`
        do
            export $i
        done;
}

function ExportConfPath
{
		for i in `cat $NBIT_HOME/CleanBeforeExtractConfig.yaml|grep -v "#"`
		do
			export $i
		done;
}

function Start
{
		echo_log "Starting $1";
}

function Over
{
		echo_log "$1 is Completed";
		echo_log "";
}

# commenting out the below function as a requirement that it should not remove the svn data from source machine.
#function RemoveSVN
#{
#		echo_log "		.svn Cleanup Starts";
#		find $1 -name .svn|xargs rm -rf
#		echo_log "		.svn Cleanup Over";
		
#}

function RemoveTars
{
		echo_log "		 tar Cleanup Starts";
		find $1 -name *.tar|xargs rm -f
		echo_log "		 tar Cleanup Over";
}

function CheckCustReportAvail
{
		CR=0;
		if [ "$REPORT_DIR" == "$CUST_REPORT_DIR" ]
		then
			CR=1;
		fi;	
}

function CleanNikiraClient
{
		if [ ! -d $SOURCE_NIKIRACLIENT ]
		then
				echo_log "SRC Nikira Client folder Path $SOURCE_NIKIRACLIENT mentioned in Extract.conf is wrong !!!!"
				exit 1;
		fi;

		if [ "$config_CleanNikiraClient_RemoveTars" == "y" -o "$config_CleanNikiraClient_RemoveTars" == "Y" ]
		then
				Start "Remove Tar in $SOURCE_NIKIRACLIENT";
				RemoveTars $SOURCE_NIKIRACLIENT
                 Over "Remove Tar";
		fi;

		if [ "$config_CleanNikiraClient_RemoveLogFiles" == "y" -o "$config_CleanNikiraClient_RemoveLogFiles" == "Y" ]
		then        
				Start "Remove logs in $SOURCE_NIKIRACLIENT";
				cd $SOURCE_NIKIRACLIENT/src/log
				rm -f *.log;
				Over "Remove logs";
		fi;

}

function CleanNikiraRoot
{

		if [ ! -d $SOURCE_NIKIRAROOT ]
		then
				echo_log "SRC Nikira Root folder Path $SOURCE_NIKIRAROOT in Extract.conf is wrong !!!!"
				exit 1;
		fi;

		if [ "$config_CleanNikiraRoot_RemoveTars" == "y" -o "$config_CleanNikiraRoot_RemoveTars" == "Y" ]
		then
				Start "RemoveTars in $SOURCE_NIKIRAROOT";
				RemoveTars $SOURCE_NIKIRAROOT
				Over "RemoveTars";
		fi;

		if [ "$config_CleanNikiraRoot_RemoveNikiraRootFiles" == "y" -o "$config_CleanNikiraRoot_RemoveNikiraRootFiles" == "Y" ]
		then
				Start "Remove files(log and temp) in $SOURCE_NIKIRAROOT";
				find $SOURCE_NIKIRAROOT -type f -name '*.log' | xargs rm -f 
				rm -f $SOURCE_NIKIRAROOT/FMSData/DataRecord/Processed/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/DataRecord/Rejected/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/DataRecord/FutureRecordsRejected/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/DataRecord/LicenseRejected/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/temp/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/SubscriberDataRecord/Processed/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/SubscriberDataRecord/Rejected/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/SubscriberDataRecord/FutureRecordsRejected/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/DataRecord/LicenseRejected/*
				rm -f $SOURCE_NIKIRAROOT/FMSData/RecordLoaderData/* 2>/dev/null
				rm -f $SOURCE_NIKIRAROOT/FMSData/Task\ Logs/*
                find $NIKIRAROOT -name dump >txt.txt
				while read p; do
				 	rm -f $p/*
				done <txt.txt

				find $NIKIRAROOT -name delta >txt.txt
                while read p; do
					rm -f $p/*
				done <txt.txt
				
				find $NIKIRAROOT -name ParticipatedRecords >txt.txt
                while read p; do
					rm -f $p/*
				done <txt.txt

				find $NIKIRAROOT -name cache_dump >txt.txt
                while read p; do
					rm -f $p/*
				done <txt.txt

				find $NIKIRAROOT -name cache_delta >txt.txt
				while read p; do
					rm -f $p/*
				done <txt.txt

				rm -f txt.txt
                Over "remove files";

		fi;

}
function CleanDBSetup
{

		if [ ! -d $SOURCE_DBSETUP ]
		then
				echo_log "DBSetup Path $SOURCE_DBSETUP mentioned in Extract.conf is wrong !!!!"
				exit 2;
		fi;
		
		if [ "$config_CleanDBSetup_RemoveTars" == "y" -o "$config_CleanDBSetup_RemoveTars" == "Y" ]
		then
				Start "RemoveTars in $SOURCE_DBSETUP";
				RemoveTars $SOURCE_DBSETUP
				Over "RemoveTars";
		fi;

}

function CleanReports
{
		
		if [ ! -d $SOURCE_REPORTDIR ]
		then
				echo_log "Report Folder $SOURCE_REPORTDIR  mentioned in Extract.conf is wrong !!!!"
				exit 3;
		fi;


		if [ "$config_CleanReports_RemoveTars" == "y" -o "$config_CleanReports_RemoveTars" == "Y" ]
		then
				Start "RemoveTars in $SOURCE_REPORTDIR";
				RemoveTars $SOURCE_REPORTDIR;
                Over "RemoveTars"
		fi;


}

function CleanCustomReports
{
		if [ $CR -eq 0 ]
		then
				if [ ! -d $SOURCE_CUSTREPORTDIR	]
				then
						echo_log "Customer Report Folder $SOURCE_CUSTREPORTDIR  mentioned in Extract.conf is wrong !!!!"
						exit 4;
				fi;		
				Start "CleanCustomReports in $SOURCE_CUSTREPORTDIR";


				if [ "$config_CleanCustomReports_RemoveTars" == "y" -o "$config_CleanCustomReports_RemoveTars" == "Y" ]
				then
					Start "RemoveTars in $SOURCE_CUSTREPORTDIR";
					RemoveTars $SOURCE_CUSTREPORTDIR;
					Over "RemoveTars";
				fi;

		else
			echo_log "No Seperate Customer Report Folder Available...."; 	
			echo_log ""
		fi;
		
}

function CleanJasperHome
{
		
		if [ ! -d $SOURCE_NIKIRACLIENT/jasperserver ]
		then
				echo_log "JasperHome folder $SOURCE_NIKIRACLIENT/jasperserver is Not Available "
				exit 5;
		fi		
		
		cd $SOURCE_NIKIRACLIENT/jasperserver
		if [ "$config_CleanJasperHome_RemoveJava" == "y" -o "$config_CleanJasperHome_RemoveJava" == "Y" ]
		then
				Start "Remove java in $SOURCE_NIKIRACLIENT/jasperserver ";
				rm -rf java
				Over "Remove java";
		fi;

		if [ "$config_CleanJasperHome_RemoveMysql" == "y" -o "$config_CleanJasperHome_RemoveMysql" == "Y" ]
		then
				Start "Remove mysql in $SOURCE_NIKIRACLIENT/jasperserver ";
				rm -rf mysql
				Over "Remove mysql";
		fi;

		if [ "$config_CleanJasperHome_RemoveTars" == "y" -o "$config_CleanJasperHome_RemoveTars" == "Y" ]
		then
				Start "RemoveTars in $SOURCE_NIKIRACLIENT/jasperserver ";
				RemoveTars "$SOURCE_NIKIRACLIENT/jasperserver" ;
				Over "RemoveTars";
		fi;

}

function CleanMySQL
{
		
		if [ ! -d $SOURCE_MYSQL ]
		then
				echo_log "Mysql folder $SOURCE_MYSQL mentioned in Extract.conf is wrong !!!!"
				exit 6;
		fi;		

		cd $SOURCE_MYSQL

		if [ "$config_CleanMySQL_RemovebinFiles" == "y" -o "$config_CleanMySQL_RemovebinFiles" == "Y" ]
		then
				Start "RemovebinFiles in $SOURCE_MYSQL";
				rm -f data/*-bin*
				rm -f bin/mysqld-debug
				Over "CleanMySQL";
		fi;

}

function CleanApache
{

		if [ ! -d $SOURCE_APACHEROOT ]
		then
			 echo_log "Apache folder $SOURCE_APACHEROOT mentioned in Extract.conf is wrong !!!!"
			 exit 6;
		fi;

		cd $SOURCE_APACHEROOT

		if [ "$config_CleanApache_RemoveLogFiles" == "y" -o "$config_CleanApache_RemoveLogFiles" == "Y" ]
		then
				Start "RemoveLogFiles in $SOURCE_APACHEROOT";
				rm -f logs/*log
				Over "CleanApache";
		fi;

}

function CleanToolTars
{

		cd $SOURCE_APACHEROOT
		cd ..
		if [ "$config_CleanToolTars_RemoveTars" == "y" -o "$config_CleanToolTars_RemoveTars" == "Y" ]
		then
				Start "CleanToolTars";
				find . -name *.tar|rm -f
				Over "CleanToolTars";

		fi;

}

function End
{
		echo_log "";
		echo_log "Cleaning is Done !!!!!!!!!!!!!!!!!!!!!!!";
		echo_log "";
}
#-----------------------------------------------------Functions End----------------------------------------------------------------------------#

WARNING	
ExportSRCPaths
ExportConfPath

CleanNikiraClient
CleanNikiraRoot
CleanDBSetup
CleanReports
CleanJasperHome
CleanMySQL
CleanApache
CleanToolTars
CheckCustReportAvail
CleanCustomReports

End

