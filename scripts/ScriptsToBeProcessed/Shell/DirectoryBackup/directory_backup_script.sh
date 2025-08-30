#! /bin/bash

if [ $# -ne 2 ] 
then
       echo " USAGE :  $0 CONFIG_FILE OPTION"
       exit 1
fi

CON_FILE=$1
OPTION=$2
OPTION=`echo $OPTION | tr 'a-z' 'A-Z'`
LOG_FILE=$RANGERHOME/LOG/directory_backup_script.log
BACKUP=BACKUP
REVERT=REVERT
printf "==================================================================================\n" >> $LOG_FILE
if [ "$OPTION" == "BACKUP" ]
then
        printf "BACKUP OPTION SELECTED [$OPTION]\n" >> $LOG_FILE
        echo -e "BACKUP OPTION SELECTED [$OPTION]\n"
elif [ "$OPTION" == "REVERT" ]
then
        printf "REVERT OPTION SELECTED [$OPTION]\n" >> $LOG_FILE
        echo -e "REVERT OPTION SELECTED [$OPTION]\n"
else
        printf "INVALID OPTION PARAMETER [$OPTION]\n" >> $LOG_FILE
        echo -e "INVALID OPTION PARAMETER [$OPTION]\n"
        exit 2
fi

if [ ! -f $CON_FILE ]
then
	printf "CONFIG FILE : $LOG_FILE DOESNT EXISTS\n" >> $LOG_FILE
        echo -e "CONFIG FILE : $LOG_FILE DOESNT EXISTS\n"
	exit 3
fi

sed '/^[ \t]*$/d' < $CON_FILE > $CON_FILE.tmp
mv $CON_FILE.tmp  $CON_FILE
echo "$CON_FILE" | grep "\." > /dev/null 2>&1
if [ $? -ne 0 ]
then
        echo -e "BAD CONFIG FILE [SPACES]: $CON_FILE"
        printf "BAD CONFIG FILE [SPACES]: $CON_FILE\n" >> $LOG_FILE
	exit 4
fi

linecount=`cat $CON_FILE | wc -l`
numberofpipes=`grep '^.*[^/]|.*/$' $CON_FILE | wc -l`
if [ $linecount -ne $numberofpipes ]
then
        echo -e "BAD CONFIG FILE [INVALID ENTRIES]: $CON_FILE"
        printf "BAD CONFIG FILE [INVALID ENTRIES]: $CON_FILE\n" >> $LOG_FILE
    exit 5
fi

ERROR=0

if [ "$OPTION" == "BACKUP" ]
then
        printf "BACKUP STARTS\n" >> $LOG_FILE
        echo -e "BACKUP STARTS\n"
	while read DirectoryList
	do
		echo "ENTRY : $DirectoryList"
        	printf "ENTRY : $DirectoryList\n" >> $LOG_FILE
                TARGET_DIR=`echo $DirectoryList | awk -F"|" '{print $1F}'`
                T_DIR=`echo $(basename $TARGET_DIR)`
                ONLY_BACKUP_DIR=`echo $DirectoryList | awk -F"|" '{print $2F}'`
                BACKUP_DIR=$ONLY_BACKUP_DIR/$T_DIR
		#echo "ONLY_BACKUP_DIR - $ONLY_BACKUP_DIR || BACKUP_DIR - $BACKUP_DIR"
		if [ -d $TARGET_DIR ]
		then
			if [ -d $ONLY_BACKUP_DIR ]
			then
				if [ -d $BACKUP_DIR ]
				then
					rm -rf $BACKUP_DIR
				fi
				BACKUP_DIR_EMPTY_SPACE="$(df -v $ONLY_BACKUP_DIR |  awk '{ print $4 }' | tail -1)"
				TARGET_DIR_SPACE="$(du -sk $TARGET_DIR | awk '{ print $1}' | head -1)"
				#echo "BACKUP_DIR_EMPTY_SPACE : $BACKUP_DIR_EMPTY_SPACE || TARGET_DIR_SPACE - $TARGET_DIR_SPACE"
				SIZE_DIFF=$(expr $BACKUP_DIR_EMPTY_SPACE - $TARGET_DIR_SPACE)
				if [ $SIZE_DIFF -gt 0 ]
				then
					#echo -e "LOCATION OF BACKUP WOULD BE : [$BACKUP_DIR]"
					#printf "LOCATION OF BACKUP WOULD BE : [$BACKUP_DIR]" >> $LOG_FILE
					cp --preserve=all -r $TARGET_DIR $BACKUP_DIR
					if [ $? -ne 0 ]
					then	
						printf "COPY OF BACKUP : [$TARGET_DIR] IS FAILED\n" >> $LOG_FILE
						echo -e "COPY OF BACKUP : [$TARGET_DIR] IS FAILED\n"
						ERROR=`expr $ERROR + 1`
					else
						printf "BACKUP OF [$TARGET_DIR] IS COPIED TO DIRECTORY [$ONLY_BACKUP_DIR]\n" >> $LOG_FILE
                                        	echo -e "BACKUP OF [$TARGET_DIR] IS COPIED TO DIRECTORY [$ONLY_BACKUP_DIR]\n"
					fi
				else
					printf "NO ENOUGH SPACE IN BACKUP DIRECTORY : $ONLY_BACKUP_DIR\n" >> $LOG_FILE
					echo -e "NO ENOUGH SPACE IN BACKUP DIRECTORY : $ONLY_BACKUP_DIR\n\n"
					ERROR=`expr $ERROR + 1`
				fi
			else
				printf "BACKUP DIRECTORY : $ONLY_BACKUP_DIR IS NOT VALID\n" >> $LOG_FILE
				echo -e "BACKUP DIRECTORY : $ONLY_BACKUP_DIR IS NOT VALID\n"
				ERROR=`expr $ERROR + 1`
			fi	
		else
	        	printf "TARGET DIRECTORY : $TARGET_DIR IS NOT VALID\n" >> $LOG_FILE
        		echo -e "TARGET DIRECTORY : $TARGET_DIR IS NOT VALID\n"
			ERROR=`expr $ERROR + 1`
		fi
	done < $CON_FILE
elif [ "$OPTION" == "REVERT" ]
then
        printf "RESTORE STARTS\n" >> $LOG_FILE
        echo -e "RESTORE STARTS\n"
        while read DirectoryList
        do
                echo -e "ENTRY : $DirectoryList"
                printf "ENTRY : $DirectoryList\n" >> $LOG_FILE
                TARGET_DIR=`echo $DirectoryList | awk -F"|" '{print $2F}'`
                BACKUP_DIR=`echo $DirectoryList | awk -F"|" '{print $1F}'`
		B_DIR=`echo $(basename $BACKUP_DIR)`
		TARGET_DIR=$TARGET_DIR/$B_DIR
                ONLY_BACKUP_DIR=`echo $(dirname $BACKUP_DIR)`
		if [ -d $TARGET_DIR ]
                then
                        if [ -d $ONLY_BACKUP_DIR ]
                        then
                                if [ -d $BACKUP_DIR ]
                                then
                                        rm -rf $BACKUP_DIR
                                fi
                                BACKUP_DIR_EMPTY_SPACE="$(df -v $ONLY_BACKUP_DIR |  awk '{ print $4 }' | tail -1)"
                                TARGET_DIR_SPACE="$(du -sk $TARGET_DIR | awk '{ print $1}' | head -1)"
                                #echo "BACKUP_DIR_EMPTY_SPACE : $BACKUP_DIR_EMPTY_SPACE || TARGET_DIR_SPACE - $TARGET_DIR_SPACE"
                                SIZE_DIFF=$(expr $BACKUP_DIR_EMPTY_SPACE - $TARGET_DIR_SPACE)
                                if [ $SIZE_DIFF -gt 0 ]
                                then
                                        #echo -e "TARGET TO BE COPIED IS : [$BACKUP_DIR]"
                                        #printf "TARGET TO BE COPIED IS : [$BACKUP_DIR]" >> $LOG_FILE
                                        cp  --preserve=all -r $TARGET_DIR $BACKUP_DIR
                                        if [ $? -ne 0 ]
                                        then
                                        	printf "COPY OF BACKUP [$TARGET_DIR] IS FAILED\n" >> $LOG_FILE
                                                echo -e "COPY OF BACKUP [$TARGET_DIR] IS FAILED\n"
                                                ERROR=`expr $ERROR + 1`
                                        else
                                                printf "BACKUP OF [$TARGET_DIR] IS RESTORED TO DIRECTORY [$ONLY_BACKUP_DIR]\n" >> $LOG_FILE
                                                echo -e "BACKUP OF [$TARGET_DIR] IS RESTORED TO DIRECTORY [$ONLY_BACKUP_DIR]\n"
                                        fi
                                else
                                        printf "NO ENOUGH SPACE IN BACKUP DIRECTORY : $ONLY_BACKUP_DIR\n" >> $LOG_FILE
                                        echo -e "NO ENOUGH SPACE IN BACKUP DIRECTORY : $ONLY_BACKUP_DIR\n\n"
                                        ERROR=`expr $ERROR + 1`
                                fi
                        else
                                printf "BACKUP DIRECTORY : $ONLY_BACKUP_DIR IS NOT VALID\n" >> $LOG_FILE
                                echo -e "BACKUP DIRECTORY : $ONLY_BACKUP_DIR IS NOT VALID\n"
                                ERROR=`expr $ERROR + 1`
                        fi
                else
                        printf "DIRECTORY : $TARGET_DIR IS NOT VALID\n" >> $LOG_FILE
                        echo -e "DIRECTORY : $TARGET_DIR IS NOT VALID\n"
                        ERROR=`expr $ERROR + 1`
                fi
        done < $CON_FILE
else
        printf "INVALID OPTION PARAMETER [$OPTION]\n" >> $LOG_FILE
        echo -e "INVALID OPTION PARAMETER [$OPTION]\n"
        exit 2
fi




echo -e "CONFIG FILE : [$CON_FILE] PROCESSED WITH $ERROR ERRORS\nPLEASE CHECK LOG FILE $LOG_FILE\n---------------------------------------------"
printf "CONFIG FILE : [$CON_FILE] PROCESSED WITH $ERROR ERRORS\nPLEASE CHECK LOG FILE $LOG_FILE\n---------------------------------------------" >> $LOG_FILE
