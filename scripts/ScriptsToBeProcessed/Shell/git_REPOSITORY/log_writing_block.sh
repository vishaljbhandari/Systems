#!/bin/bash
#################################################################################
#
#
#
#################################################################################
# LOG WRITING CODE BLOCK
#
#
#################################################################################

CURRENTTIME=`date +"%Y%m%d%H%M%S"`
LOG_PREFIX=prefix
LOG_SUFFIX=suffix
LOG_FILE_FORMATE=log
MODULE_NAME=$0
CWD=`pwd`
LOG_DIRECTORY=
# If Log Directory is set Inside, Then remove bellow condition
if [ $# -ne 1 ] ; then
        echo "[ERROR] LOG_DIRECTORY : [$1] DOESNT EXIST"
        exit 1
else
        LOG_DIRECTORY=$1
fi
function InitializeLogs()
{
	if [ ! -d $LOG_DIRECTORY ]
	then
			echo "[ERROR] LOG_DIRECTORY : [$LOG_DIRECTORY] DOESNT EXIST"
			LOG_DIRECTORY=$CWD
			echo "[INFO] TAKING CURRENT WORKING DIRECTORY AS LOG DIRECTORY[$LOG_DIRECTORY]"
			#exit 11 
	else
			echo "[INFO] LOG DIRECTORY[$LOG_DIRECTORY]"
	fi
	LOG_FILE=$LOG_DIRECTORY/$LOG_PREFIX.$MODULE_NAME.$CURRENTTIME.$LOG_SUFFIX.$LOG_FILE_FORMATE
	touch $LOG_FILE
	if [ ! -f $LOG_FILE ]
	then
			echo "[ERROR] LOG_FILE : [$LOG_FILE] CREATION FAILED"
			exit 12
	else
			echo "[INFO] LOG_FILE : [$LOG_FILE]"
	fi

}
InitializeLogs
#################################################################################
