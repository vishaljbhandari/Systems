#! /usr/local/bin/bash
export PATH=$PATH:/usr/bin
#============== CONFIGURATION SECTION =============
OUTPUT_DIR=/disk2/trmteam/vishal/output
INPUT_DIR=/disk2/trmteam/vishal/input
TEMP_DIR=/disk2/trmteam/vishal/temp
BATCH_SIZE=100000
File_Size=0
FILE_SUFFIX_SIZE=2
PID_FILE=./ITU_error_split.pid
TIME_CHECK=5
#PID_FILE=/moneta_polled01/ITU_error_split.pid
RECIPIENT_MAIL=recipient@somewhere.com
SPLIT_FAIL_MSG="Please review, Split failed for : " 
COPY_FAIL_MSG="Please review, Splitted but copy failed for : "
MAIL_ALERT_SUBJECT=WARNING 
#return file size
#=================================================
fn_File_Size ()
{
        File_Size=`du -s  $1| awk {'print $1'}`
}

on_exit ()
{
        rm -f $PID_FILE 
        exit;
}

main ()
{
        checkForRunningProcess
        trap on_exit EXIT
        ps -u $LOGNAME | grep ITU_error_split | awk {'print $1'} > $PID_FILE 
        echo "ITU_ERROR_SPLITTER STARTED... "
                while [ 1 ]
                do
                        copyRawFiles
                        sleep 300
                done
        echo "ITU_ERROR_SPLITTER TERMINATED... "
}

function checkForRunningProcess()
{
        if [ -s $PID_FILE ]
        then
                echo "ITU_ERROR_SPLITTER Already Running..."
                exit
        fi
}

function copyRawFiles()
{
        cd $INPUT_DIR
        echo "INSIDE : $INPUT_DIR"
        for filename in `ls`
        do
	old_File=$filename
        fn_File_Size $old_File
        while_condition=1
        while [ $while_condition -ne 0 ]
                do
                        sleep $TIME_CHECK 
                        File_Size_old=$File_Size
                        fn_File_Size $old_File
                        File_Size_new=$File_Size
                        if [ $File_Size_new -eq $File_Size_old ]
                        then
                               	#echo "uncompresse finshed $old_File"
                                #file_name_length=${#old_File}
                                #FILE_TO_SPLIT=${old_File:0:$file_name_length-3}
                                #echo "new file name is $FILE_TO_SPLIT"
                                
				FILE_TO_SPLIT=$old_File
                                chmod 777 $FILE_TO_SPLIT
                                echo "SPLIT STARTS FOR $FILE_TO_SPLIT BATCH SIZE $BATCH_SIZE"
                                split -l $BATCH_SIZE -a $FILE_SUFFIX_SIZE  $FILE_TO_SPLIT $TEMP_DIR/$FILE_TO_SPLIT
				if [ $? -eq 0 ]
				then
					rm $FILE_TO_SPLIT
					cp $TEMP_DIR/$FILE_TO_SPLIT* $OUTPUT_DIR
					if [ $? -eq 0 ]
                                	then
						rm $TEMP_DIR/$FILE_TO_SPLIT*
						echo "FILE $FILE_TO_SPLIT SPLITTED, REMOVED FROM $INPUT_DIR & COPIED TO $OUTPUT_DIR"
					else
						rm $OUTPUT_DIR/$FILE_TO_SPLIT*
						echo "FILE $FILE_TO_SPLIT SPLITTED, REMOVED FROM $INPUT_DIR BUT COULD NOT COPIED TO $OUTPUT_DIR"	
						echo COPY_FAIL_MSG$FILE_TO_SPLIT* | mailx -s MAIL_ALERT_SUBJECT $RECIPIENT_MAIL 
					fi
                                else
					echo $SPLIT_FAIL_MSG$FILE_TO_SPLIT | mailx -s MAIL_ALERT_SUBJECT $RECIPIENT_MAIL				
				fi 
                                while_condition=0
                        fi
                done
        done
}

main $*
