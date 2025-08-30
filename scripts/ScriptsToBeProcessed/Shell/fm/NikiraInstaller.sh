#!/bin/bash
. ~/.bashrc
#*******************************************************************************
#* The Script may be used and/or copied with the written permission from
#* Subex Ltd. or in accordance with the terms and conditions stipulated in
#* the agreement/contract (if any) under which the program(s) have been supplied
#*
#* AUTHOR:Saktheesh & Vijayakumar & Mahendra Prasad S 
#* Modified: 10-Jul-2015

#* Description:
#*	This script will install(porting,not compilation) Nikira 6.1 on production box

#*******************************************************************************
. ./InstallerFunctions.sh

#----------------------------------------------------- Functions Start ----------------------------------------------------------------------------#

function Help
{
        echo_log "Usage: $PROG_NAME [-d Partitioned][-m ModuleName]";
        echo_log "or    $PROG_NAME -h ";
        echo_log "-m Module_Name [FMSROOT/FMSTOOLS/FMSCLIENT] Default ALL";
        echo_log "-e Module_Name [FMSROOT/FMSTOOLS/FMSCLIENT]";
        echo_log "-d Partitioned_DB [Partitioned/NO] Default NO";
        echo_log "-h Help.";
}


#----------------------------------------------------- Functions End ----------------------------------------------------------------------------#

# Main Starts Here..............................................

ModuleName="ALL";
Exclude=0
PROG_NAME=$0;
while getopts m:d:e:h OPTION
do
        case ${OPTION} in
                m) ModuleName="${OPTARG}" ;;
                e) ModuleName="${OPTARG}";
		   Exclude=1;;
                d) PartitionedDB="${OPTARG}" ;;
                h) Help;exit 0;;
                \?) Help;exit 2;;
        esac
done
PartitionedDB=`perl -e "print uc($PartitionedDB)"`
if [ "$PartitionedDB" == "" -o "$PartitionedDB" != "PARTITIONED" ]
then 
     PartitionedDB="NO"
fi



CurrentDirectoryCheck;
Validations;
InstallDirCheck_And_CopySource;
EnvironmentCheck;
ExportSRCPaths;
DoInstall;
GenerateNewConf;



# Main Ends Here..............................................
