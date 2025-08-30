#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#!/bin/bash

InputFileDir=$HOME/mach/
OutputFileDir=$RANGER5_4HOME/RangerData/DataSourceTAPINCDRData/
FileSufix=mac
Backup_Dir="/nikira/subex_working_area/TAP_BAK"

WorkingDirectory=`pwd`

on_exit ()
{
        cd $WorkingDirectory
}
trap on_exit EXIT

UpdateFileDetails ()
{
        FileName=$1
        FileSize=`ls -s "$FileName" | sed 's/^ *//g' | cut -d' ' -f1`
        FileSize=`expr $FileSize \* 1024`
        Source="-10"
        DateString=`date +%F`
        FileName=`basename $FileName`

        sqlplus -s /nolog << EOF
		CONNECT_TO_SQL
                insert into du_data_file_processed (file_timestamp, file_name, file_size, source)
                        values (to_date ('$DateString', 'yyyy-mm-dd'), '$FileName', $FileSize, $Source) ;
EOF
}

if [ ! -d "$InputFileDir" ]; then
        echo "Please Provide A Valid Input Directory !"
        exit 1
fi

if [ ! -d "$OutputFileDir" -o ! -d "$OutputFileDir/success" ]; then
        echo "Please Provide A Valid Output Directory !"
        exit 2
fi

if [ "$FileSufix" == "" ]; then
        echo "No Sufix Provided, Please Provied File Sufix !" 
        exit 3
fi

cd "$InputFileDir"

while true
do
        for File in `ls -rt *.gz 2>/dev/null `
        do
                CKSum1=`cksum $File`
                CKSum2=0
				IsIgnored=0
                while true
                do      
                        CKSum2=$CKSum1
                        sleep 5
                        CKSum1=`cksum $File`
                        if [ "$CKSum2" == "$CKSum1" ]; then
                                UnCompressedFile=`echo $File | sed 's/\.gz$//'`
								while read i
                                do
									ls $File |grep $i 2>&1 > $RANGERHOME/tmp/tapin_mac_temp
									if [ $? == 0 ]
									then
										mv $File $Backup_Dir
										IsIgnored=1
										break
									fi
                                done < $RANGERHOME/bin/NRTRDE_VPMN_LIST

								if [ $IsIgnored == 1 ]
								then
									break
								fi

                                gunzip -c $File > "$OutputFileDir/$UnCompressedFile""_""$FileSufix"

                                UpdateFileDetails "$OutputFileDir/$UnCompressedFile""_""$FileSufix"

                                touch "$OutputFileDir/success/$UnCompressedFile""_""$FileSufix"
                                rm -f $File
                                break
                        fi
                done
        done
        sleep 30
done
