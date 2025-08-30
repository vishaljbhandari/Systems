#! /bin/bash

error_inc(){
	ERROR=`expr $ERROR + 1`
}

on_exit(){
	echo -e "CONFIG FILE : [$CON_FILE] PROCESSED WITH REVERT & [$ERROR] ERRORS\nPLEASE CHECK LOG FILE $LOG_FILE"
	printf "\nCONFIG FILE : [$CON_FILE] PROCESSED WITH REVERT & [$ERROR] ERRORS\n\n\n" >> $LOG_FILE
}

ValidateConfigFile(){
	if [ ! -f $CON_FILE ]
	then
		printf "\nCONFIG FILE : [$CON_FILE] DOESNT EXISTS\n"  >> $LOG_FILE
		error_inc
		exit 3
	fi
	sed '/^[ \t]*$/d' < $CON_FILE > $CON_FILE.tmp
	mv $CON_FILE.tmp $CON_FILE
	echo "$CON_FILE" | grep "\." > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
  		printf "\nBAD CONFIG FILE [SPACES]: [$CON_FILE]\n"  >> $LOG_FILE 
	fi
	linecount=`cat $CON_FILE | wc -l`
	numberofnoslash=`grep '^.*[^/]$' $CON_FILE | wc -l`
	if [ $linecount -eq 0 -o $linecount -ne $numberofnoslash ]
	then
  		printf \n"BAD CONFIG FILE [INVALID ENTRIES] & LINES [$linecount] IN CONFIG FILE: [$CON_FILE]\n"  >> $LOG_FILE
		error_inc
		exit 5
	fi
}

ProcessLinks(){
        local RDIR=$1
	cd $RDIR$SUFFIX2
	for rvlinks in `find . -type l`
	do
		cd $RDIR$SUFFIX2
                if [ ! -L $rvlinks ]
                then
                        printf "\nLINK: [$rvlinks] IS NOT VALID"  >> $LOG_FILE
                        error_inc
                        continue
                fi
                local LN_DIR82=`echo $(readlink $rvlinks)`
                if [ ! -d $LN_DIR82 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR82] IS NOT VALID"  >> $LOG_FILE
                        error_inc
                        continue
                fi
		if [ ! -L $RDIR/$rvlinks ]
                then
                        printf "\nLINK: [$RDIR/$rvlinks] IS NOT VALID"  >> $LOG_FILE
                        error_inc
                        continue
                fi
                
		local LN_DIR81=`echo $(readlink $RDIR/$rvlinks)`
                if [ ! -d $LN_DIR81 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR81] IS NOT VALID"  >> $LOG_FILE
                        error_inc
                        continue
                fi
                if [ -d $LN_DIR82$SUFFIX2 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR82$SUFFIX2] WAS ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
                        rm -rf $LN_DIR82$SUFFIX2
                fi
                
		mv $LN_DIR82 $LN_DIR82$SUFFIX2
                if [ $? -ne 0 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR82] CANT BE MOVED TO [$LN_DIR82$SUFFIX2]"  >> $LOG_FILE
                        error_inc
                        continue
                else
                        printf "\nDIRECTORY: [$LN_DIR82] MOVED TO [$LN_DIR82$SUFFIX2]"  >> $LOG_FILE
                fi
		
		rm $rvlinks
                ln -s $LN_DIR82$SUFFIX2 $rvlinks
                if [ $? -ne 0 ]
                then
                        printf "\nLINK: [$RDIR$SUFFIX2/$rvlinks] CANT BE CREATED FOR [$LN_DIR82$SUFFIX2]"  >> $LOG_FILE
                        error_inc
                else
                        printf "\nLINK: [$RDIR$SUFFIX2/$rvlinks] CREATED FOR [$LN_DIR82$SUFFIX2]"  >> $LOG_FILE
                fi
		
		mv $LN_DIR81 $LN_DIR82
                if [ $? -ne 0 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR81] CANT BE MOVED TO [$LN_DIR82]"  >> $LOG_FILE
                        error_inc
                        continue
                else
                        printf "\nDIRECTORY: [$LN_DIR81] MOVED TO [$LN_DIR82]"  >> $LOG_FILE
                fi		
		
		rm $RDIR/$rvlinks
		ln -s $LN_DIR82 $RDIR/$rvlinks
		if [ $? -ne 0 ]
                then
			printf "\nLINK: [$RDIR/$rvlinks] CANT BE CREATED FOR [$LN_DIR82]"  >> $LOG_FILE
                        error_inc
                else
                        printf "\nLINK: [$RDIR/$rvlinks] CREATED FOR [$LN_DIR82]"  >> $LOG_FILE
                fi
		ProcessLinks $LN_DIR82
	done
}

ProcessConfigFileDirectories(){
        local DIR=$1
	if [ ! -d $DIR$SUFFIX ]
        then
		printf "\nBACKUP DIRECOTRY: [$DIR$SUFFIX] NOT PRESENT, SO QUITING"  >> $LOG_FILE
		error_inc
		return
	fi
        if [ -d $DIR$SUFFIX2 ]
       	then
		printf "\nBACKUP DIRECOTRY: [$DIR$SUFFIX2] ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
        	rm -rf $DIR$SUFFIX2
        fi
        mv $DIR $DIR$SUFFIX2
        if [ $? -eq 0 ]
        then
		printf "\nDIRECOTRY: [$DIR] MOVED TO [$DIR$SUFFIX2]"  >> $LOG_FILE
        	mv $DIR$SUFFIX $DIR
                if [ $? -eq 0 ]
                then
                	printf "\nDIRECOTRY: [$DIR$SUFFIX] MOVED TO [$DIR]"  >> $LOG_FILE
			ProcessLinks $DIR
		else
                        printf "\nDIRECOTRY: [$DIR$SUFFIX] CANT BE MOVED"  >> $LOG_FILE
                        error_inc
              	fi
      	else                    
        	printf "\nDIRECOTRY: [$DIR] CANT BE MOVED"  >> $LOG_FILE
                error_inc
        fi
}

RevertProcess(){
        printf "REVERT PROCESS STARTS"  >> $LOG_FILE
        while read DirectoryList
        do
                printf "\nENTRY : [$DirectoryList]"  >> $LOG_FILE
                ENTRY=$DirectoryList
                if [ -d $ENTRY ]
                then
                        printf "\nPROCESSING ENTRY: [$ENTRY], WHICH IS A DIRECTORY"  >> $LOG_FILE
                        ProcessConfigFileDirectories $ENTRY
                else
                        printf "\nINVALID ENTRY: [$ENTRY], NOT A LINK OR DIRECTORY"  >> $LOG_FILE
                        error_inc
                fi

        done < $CON_FILE
}

main()
{
	trap on_exit EXIT
	ERROR=0
	CON_FILE=NONE
        LOG_FILE=~/InMemoryRevertAt$(date +"%d%m%Y%H%M%S").log
        #LOG_FILE=$RANGERHOME/LOG/InMemoryRevertAt$(date +"%d%m%Y%H%M%S").log
	if [ $# -ne 1 ] 
	then
		echo -e "USAGE : $0 CONFIG_FILE"
       		printf "USAGE : $0 CONFIG_FILE"  >> $LOG_FILE
		error_inc
		exit 1
	fi

	CON_FILE=$1
	SUFFIX="_801"
	SUFFIX2="_802"
	if [ "$SUFFIX" = "" -o "$SUFFIX2" = "" ]
        then
                printf "\nSUFFIX IS NULL, QUITTING"  >> $LOG_FILE
                error_inc
                exit 55
        fi
	ValidateConfigFile
	RevertProcess
}

main "$@"
