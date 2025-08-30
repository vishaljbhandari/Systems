#!/bin/bash
. ~/.bashrc
#*******************************************************************************
#* The Script may be used and/or copied with the written permission from
#* Subex Ltd. or in accordance with the terms and conditions stipulated in
#* the agreement/contract (if any) under which the program(s) have been supplied
#*
#* AUTHOR: Saktheesh & Vijayakumar  
#* Modified: 15-APR-2008

#* Description:
#*	This script will Do changes Required for password Encryption .
#*  This should be run if u face password problem after installing using ./NikiraInstaller.sh.

#*******************************************************************************
. ./InstallerFunctions.sh
#----------------------------------------------------- Functions Start ----------------------------------------------------------------------------#
function Help
{
	echo_log "Usage: $PROG_NAME";
	echo_log "-h Help.";
}

function DoEncryptChange
{
	Get_Encrypted_Password;
	Changes_NikiraRoot ;
	Changes_ApacheRoot ;
	ClearLogs;

	Changes_NikiraClient ;
	
	Start_Apache ;
	exit 0;
}

#----------------------------------------------------- Functions End ----------------------------------------------------------------------------#



# Main Starts Here..............................................

ModuleName="ALL";
PROG_NAME=$0;
while getopts h OPTION
do
	case ${OPTION} in
		h) Help;exit 0;;
		\?) Help;exit 2;;
	esac
done
CurrentDirectoryCheck;
EnvironmentCheck;
ExportSRCPaths;
DoEncryptChange;
# Main Ends Here..............................................
