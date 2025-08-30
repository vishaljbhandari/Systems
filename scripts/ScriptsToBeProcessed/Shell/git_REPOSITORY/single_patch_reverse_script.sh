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
MODULE_NAME=patching
CWD=`pwd`
LOG_DIRECTORY=
function InitializeLogs()
{
        if [ ! -d $LOG_DIRECTORY ]
        then
                        echo -e "[ERROR] LOG_DIRECTORY : ["$LOG_DIRECTORY"] DOESNT EXIST"
                        LOG_DIRECTORY=$CWD
                        echo -e "[INFO] TAKING CURRENT WORKING DIRECTORY AS LOG DIRECTORY["$LOG_DIRECTORY"]"
                        #exit 11
        else
                        echo -e "[INFO] LOG DIRECTORY["$LOG_DIRECTORY"]"
        fi
        LOG_FILE=$LOG_DIRECTORY/$LOG_PREFIX.$MODULE_NAME.$CURRENTTIME.$LOG_SUFFIX.$LOG_FILE_FORMATE
        touch $LOG_FILE
        if [ ! -f $LOG_FILE ]
        then
                        echo -e "[ERROR] LOG_FILE : [$LOG_FILE] CREATION FAILED"
                        exit 12
        else
                        echo -e "[INFO] LOG_FILE : [$LOG_FILE]" >> $LOG_FILE
        fi

}
InitializeLogs
#################################################################################
BASE_DIRECTORY=$1
if [ $# -ne 1 ] ; then
        echo -e "[ERROR] BASE_DIRECTORY : ["$1"] DOESNT EXIST, ENTER VALID DIRECTORY" >> $LOG_FILE
	BASE_DIRECTORY=$CWD
        exit 1
fi

if [ ! -d $BASE_DIRECTORY ]
then
        echo -e "[ERROR] BASE_DIRECTORY : [$BASE_DIRECTORY] DOESNT EXIST" >> $LOG_FILE
	exit 2
else
        echo -e "[INFO] BASE DIRECTORY[$BASE_DIRECTORY]" >> $LOG_FILE
fi
PATCH_DIR=`pwd`
PATCH_VERSION=$2
echo -e "[INFO] [$CURRENTTIME] APPLYING REVERSE PATCH $PATCH_VERSION" >> $LOG_FILE
cd $BASE_DIRECTORY

patch -p0 -R -i $PATCH_DIR/file.patch

cd $CWD
