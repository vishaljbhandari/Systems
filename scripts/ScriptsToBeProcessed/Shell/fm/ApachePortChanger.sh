#/bin/bash

#*******************************************************************************
#* The Script may be used and/or copied with the written permission from
#* Subex Ltd. or in accordance with the terms and conditions stipulated in
#* the agreement/contract (if any) under which the program(s) have been supplied
#*
#* AUTHOR:Saktheesh & Vijayakumar  
#* Modified: 21-MAY-2008

#* Description:
#*	This script will Change the apache port and will update the port in bashrc and http.conf.
#*	This will restart the apache with the new port.

#*******************************************************************************
Dat=`date +%Dr%H:%M:%S`
logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_ApachePortChanger.log" }'`
echo "logFile = $logFile"
Installation_Time=`date +%Dr#%H:%M:%S`;
NBIT_HOME=`pwd`;
clear

NIKIRAPORT=$1;
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


function CheckInput()
{
		if [ "$NIKIRAPORT" == "" ]
		then
				echo_log "Input Required ....."
                echo_log "Usage: ./ApachePortChanger.sh <port>";
				exit -1;
		fi		

}

function Env_check()
{
		if [ "$APACHEROOT" == "" ]
		then
				echo " APACHEROOT not set.";
		fi;		

}

function CheckPortAvailability()
{
		netstat -a|grep $NIKIRAPORT;
		if [ $? -eq 0 ]
		then
				echo_log "Port $NIKIRAPORT is already in use.";
		else
				echo_log "Port $NIKIRAPORT is Available...";
		fi;		
}

function UpdateFiles()
{
		perl -pi -e "s/NIKIRA_PORT=.*/NIKIRA_PORT=$NIKIRAPORT/g" $HOME/.bashrc
		perl -pi -e "s/Listen .*/Listen $NIKIRAPORT/g" $APACHEROOT/conf/httpd.conf
}

function Start_Apache()
{
                echo_log " "
                echo_log "Trying to stop old apache"
                $APACHEROOT/bin/apachectl stop
                $APACHEROOT/bin/apachectl start
                ApacheStatus=`ps -fu $LOGNAME |grep http|wc -l`;

                if [ $ApacheStatus -gt 1 ]
                then
                                echo_log "APACHE STARTED(port:$NIKIRA_PORT).................................................."
                                echo_log "Client URL: http://$IP_ADDRESS:$NIKIRA_PORT "
                else
                                echo_log "APACHE Not started check the log files for the error...."
                fi;
}

#-----------------------------------------------------Functions End----------------------------------------------------------------------------#

CheckInput
Env_check
CheckPortAvailability
UpdateFiles
Start_Apache

