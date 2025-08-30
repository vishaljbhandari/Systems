#! /bin/bash
                                                                                                                 
#/*******************************************************************************                                 
#*  Copyright (c) Subex Limited 2014. All rights reserved.                      *                                 
#*  The copyright to the computer program(s) herein is the property of Subex    *                                 
#*  Limited. The program(s) may be used and/or copied with the written          *                                 
#*  permission from Subex Limited or in accordance with the terms and           *                                 
#*  conditions stipulated in the agreement/contract under which the program(s)  *                                 
#*  have been supplied.                                                         *                                 
#********************************************************************************/ 

if [ -z "$RANGERHOME" ]; then
    echo "RANGERHOME is not set"
    exit 1
fi

if [ $# -lt 2 ]
then
        echo "Usage: scriptlauncher $0 [LOAD|APPEND] CompleteFilePath"
        exit 2
fi

LoadStatus=$1
FilePath=$2

if [ "$LoadStatus" == "LOAD" -o "$LoadStatus" == "load" ]
then
        LOADTYPE=truncate
elif [ "$LoadStatus" == "APPEND" -o "$LoadStatus" == "append" ]
then
        LOADTYPE=append
else
        echo "Wrong Load Type : Use LOAD or APPEND"
        exit 3
fi

if [ ! -f $FilePath ]
then
        echo "File Does not Exist: $FilePath"
        exit 4
else
        sed '/^[ \t]*$/d' < $FilePath > $FilePath.tmp
        mv $FilePath.tmp  $FilePath
        echo "$FilePath" | grep "\." > /dev/null 2>&1
        if [ $? -ne 0 ]
        then
                mv $FilePath $FilePath.dat
        fi
fi

#Replace Load type with Actual Loadtype [Append or Truncate ]
sed -e 's/\$LOADTYPE/'$LOADTYPE'/g' load_control_file.ctl.ctl > load_control_file.ctl.tmp
CONNECT_TO_SQLLDR_START
                control=$RANGERHOME/tmp/loadVPMNData.ctl.tmp \
                log=temp.log \
                data=$FilePath \
                bad=$RANGERHOME/tmp/loadVPMNData.bad \
                discard=$RANGERHOME/tmp/loadVPMNData.discard \
                skip=1 \
		silent="header" \
		rows=64
CONNECT_TO_SQLLDR_END
         retcode=`echo $?`
    case "$retcode" in
        0) returnmessg="SQL*Loader execution successful"
            ;;
        1) returnmessg="SQL*Loader execution exited with EX_FAIL"
            ;;
        2) returnmessg="SQL*Loader execution exited with EX_WARN"
            ;;
        3) returnmessg="SQL*Loader execution encountered a fatal error"
            ;;
        *) returnmessg="SQL*Loader: unknown return code"
            ;;
    esac
echo "Return message : $returnmessg"
cat temp.log >> $RANGERHOME/LOG/loadVPMNData.log
rm temp.log
echo "Please Check Log File : $RANGERHOME/LOG/loadVPMNData.log"
