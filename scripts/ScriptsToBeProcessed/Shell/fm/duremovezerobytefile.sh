#! /bin/bash

#/*******************************************************************************
# *     Copyright (c) Subex Systems Limited 2006. All rights reserved.              *
# *     The copyright to the computer program(s) herein is the property of Subex    *
# *     Systems Limited. The program(s) may be used and/or copied with the written  *
# *     permission from Subex Systems Limited or in accordance with the terms and   *
# *     conditions stipulated in the agreement/contract under which the program(s)  *
# *     have been supplied.                                                         *
# *******************************************************************************/


AbsoluteFilePath=$1
FileStatus=$2

if [ "$FileStatus" = "ignored" ]
then
        DirectoryName=`dirname $AbsoluteFilePath`
        FileName=`basename $AbsoluteFilePath`

        rm -f $DirectoryName/$FileName  $DirectoryName/success/$FileName
        echo "==============================================" >> "$REVMAXHOME/log/zerobyte$FileName.log"
        echo "File Name     :   $FileName" >> "$REVMAXHOME/log/zerobyte$FileName.log"
        echo "Rejected Reason   :   Zero byte file " >> "$REVMAXHOME/log/zerobyte$FileName.log"
        echo "Directory : Source directory is `basename $DirectoryName` " >> "$REVMAXHOME/log/zerobyte$FileName.log" 
		echo "---------------------------------------------" >> "$REVMAXHOME/log/zerobyte$FileName.log"
		     
fi

