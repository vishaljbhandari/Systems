#! /bin/bash

error_inc()
{
	ERROR=`expr $ERROR + 1`
}

on_exit()
{
	echo -e "CONFIG FILE : [$CON_FILE] PROCESSED BACKUP & [$ERROR] ERRORS\nPLEASE CHECK LOG FILE $LOG_FILE"
	printf "\nCONFIG FILE : [$CON_FILE] PROCESSED BACKUP & [$ERROR] ERRORS\n\n\n" >> $LOG_FILE
}

ValidateConfigFile(){
	local CON_F=$1
	if [ ! -f $CON_F ]
	then
		printf "\nCONFIG FILE : [$CON_F] DOESNT EXISTS\n"  >> $LOG_FILE
		error_inc
		exit 3
	fi
	sed '/^[ \t]*$/d' < $CON_F > $CON_F.tmp
	mv $CON_F.tmp $CON_F
	echo "$CON_F" | grep "\." > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
  		printf "\nBAD CONFIG FILE [SPACES]: [$CON_F]\n"  >> $LOG_FILE 
	fi
	linecount=`cat $CON_F | wc -l`
	numberofnoslash=`grep '^.*[^/]$' $CON_F | wc -l`
	if [ $linecount -eq 0 -o $linecount -ne $numberofnoslash ]
	then
  		printf \n"BAD CONFIG FILE [INVALID ENTRIES] & LINES [$linecount] IN CONFIG FILE: [$CON_F]\n"  >> $LOG_FILE
		error_inc
		exit 5
	fi
	printf "\nCONFIG FILE: [$CON_F] IS A VALID FILE"  >> $LOG_FILE
}

CreateSubDirectories(){
	local CDIR=$1
	cd $CDIR$SUFFIX
	for dirs in `find . -type d | grep "./"`
	do
		cd $CDIR$SUFFIX
		if [ -d $CDIR/$dirs ]
		then
			printf "\nDIRECTORY: [$CDIR/$dirs] ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
			rm -rf $CDIR/$dirs
		fi
		mkdir $CDIR/$dirs
		if [ $? -ne 0 ]
        	then
        		printf "\nDIRECTORY [$CDIR/$dirs] CANT BE CREATED"  >> $LOG_FILE
			error_inc
		else
			printf "\nDIRECTORY [$CDIR/$dirs] IS CREATED"  >> $LOG_FILE	
		fi
	done
}

CopySubLinks(){
        local CDIR=$1
        cd $CDIR$SUFFIX
        for dirs in `find . -type l | grep "./"`
        do
                cd $CDIR$SUFFIX
                ln -s $(readlink $dirs) $CDIR/$dirs
                if [ $? -ne 0 ]
                then
                        printf "\nLINK [$CDIR$SUFFIX/$dirs] CANT BE CREATED FOR [$CDIR/$dirs]"  >> $LOG_FILE
                        error_inc
                else
			printf "\nLINK [$CDIR$SUFFIX/$dirs] CREATED FOR [$CDIR/$dirs]"  >> $LOG_FILE
                fi
        done
}

ProcessConfigFileInMemoryDirectory(){
        local DIR=$1
        if [ -d $DIR$SUFFIX ]
        then
                printf "\nDIRECTORY: [$DIR$SUFFIX] ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
                rm -rf $DIR$SUFFIX
        fi
        mv $DIR $DIR$SUFFIX
        if [ $? -eq 0 ]
        then
                printf "\nDIRECTORY: [$DIR] MOVED TO [$DIR$SUFFIX]"  >> $LOG_FILE
                mkdir $DIR
                if [ $? -eq 0 ]
                then
                        printf "\nDIRECTORY: [$DIR] CREATED"  >> $LOG_FILE
                        CreateSubDirectories $DIR
                        CopySubLinks $DIR
                        CopyArchiveDirectories $DIR 
                else
                        printf "\nDIRECTORY: [$DIR] CANT BE CREATED"  >> $LOG_FILE
                        error_inc
                fi
        else
                printf "\nDIRECTORY: [$DIR] CANT BE MOVED"  >> $LOG_FILE
                error_inc
        fi
}

BackupProcessOfDirectories(){
        printf "BACKUP PROCESS STARTS"  >> $LOG_FILE
        while read DirectoryList
        do
                printf "\nENTRY : [$DirectoryList]"  >> $LOG_FILE
                ENTRY=$DirectoryList
                if [ -d $ENTRY ]
                then
                        printf "\nPROCESSING ENTRY: [$ENTRY], WHICH IS A DIRECTORY"  >> $LOG_FILE
                        COPYOPTION=NO
			ProcessConfigFileInMemoryDirectory $ENTRY
                else
                        printf "\nINVALID ENTRY: [$ENTRY], NOT A DIRECTORY"  >> $LOG_FILE
                        error_inc
                fi

        done < $CON_FILE
}

GetConfigFileOfLinks(){
	local DCDIR=$1
	cd $DCDIR
	printf "\nINSIDE BACK UP DIRECTORY: [$DCDIR]"  >> $LOG_FILE
	printf "\nNEW CONFIG FILE IS [$NEW_CONFIG_FILE], THAT WILL BE USED FOR FURTHER PROCESS"  >> $LOG_FILE
	echo -e "NEW CONFIG FILE IS [$NEW_CONFIG_FILE], THAT WILL BE USED FOR FURTHER PROCESS"
	for dcdirs in `find . -type l | grep "./"`
        do
        	printf "$DCDIR/$dcdirs\n" >> $NEW_CONFIG_FILE
	done
}

ReformatConfigFile(){
	local CFFILE=$1
	if [ ! -f $CFFILE ]
	then
		printf "\nFILE MISSING: [$CFFILE]"  >> $LOG_FILE
		return
	fi
        sed 's/\/.\//\//' $CFFILE > $CFFILE.tmp
	mv $CFFILE.tmp $CFFILE	
	printf "\nFILE: [$CFFILE] FORMATTED"  >> $LOG_FILE
}

CreateSubLinks(){
        local CDIR=$1
        cd $CDIR$SUFFIX
        for links in `find . -type l`
        do
                cd $CDIR$SUFFIX
                if [ ! -L $links ]
                then
                        printf "\nLINK: [$links] IS NOT VALID"  >> $LOG_FILE
                        error_inc
                        continue
                fi
                local LN_DIR=`echo $(readlink $links)`
                if [ ! -d $LN_DIR ]
                then
                        printf "\nDIRECTORY: [$LN_DIR] IS NOT VALID"  >> $LOG_FILE
                        error_inc
                        continue
                fi
                if [ -d $LN_DIR$SUFFIX ]
                then
                        printf "\nDIRECTORY: [$LN_DIR$SUFFIX] WAS ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
                        rm -rf $LN_DIR$SUFFIX
                fi
                mv $LN_DIR $LN_DIR$SUFFIX
                if [ $? -ne 0 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR] CANT BE MOVED TO [$LN_DIR$SUFFIX]"  >> $LOG_FILE
                        error_inc
                        continue
                else
                        printf "\nDIRECTORY: [$LN_DIR] MOVED TO [$LN_DIR$SUFFIX]"  >> $LOG_FILE
                fi
                rm $links
                ln -s $LN_DIR$SUFFIX $links
                if [ $? -ne 0 ]
                then
                        printf "\nLINK: [$links] CANT BE CREATED FOR [$LN_DIR$SUFFIX]"  >> $LOG_FILE
                        error_inc
                        continue
                else
                        printf "\nLINK: [$links] CREATED FOR [$LN_DIR$SUFFIX]"  >> $LOG_FILE
                fi
                mkdir $LN_DIR
                if [ $? -ne 0 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR] CANT BE CREATED"  >> $LOG_FILE
                        error_inc
                        continue
                else    
                        printf "\nDIRECTORY: [$LN_DIR] CREATED"  >> $LOG_FILE
                fi
                CreateSubDirectories $LN_DIR
                CopyArchiveDirectories $LN_DIR
                CreateSubLinks $LN_DIR
        done
}

CopyArchiveDirectories(){
        if [ "$COPYOPTION" = "NO" ]
        then
                return
        fi
	local ADIR=$1
        cd $ADIR$SUFFIX
        for DirList in `find . -type d | grep "./" | grep "cache_delta/archive$"`
        do
                AREQUIRED_DIR=`echo $DirList | awk -F"cache_delta" '{print $1F}'`
                printf "\n[$AREQUIRED_DIR] | [$DirList] " >> $LOG_FILE
                if [ "$AREQUIRED_DIR" != "$DirList" ]
                then
                        local NUMBEROFDELTAFILES=`ls $ADIR$SUFFIX/$DirList | wc -l`
                        if [ $NUMBEROFDELTAFILES -eq 0 ]
                        then
                               continue
                        fi
                        cp $ADIR$SUFFIX/$DirList/* $ADIR/$AREQUIRED_DIR/delta/
                        printf "\nDIRECTORY $ADIR$SUFFIX/$DirList/* COPIED TO $ADIR/$AREQUIRED_DIR/delta/"  >> $LOG_FILE
                fi
        done
        cd $ADIR$SUFFIX
        for DirList2 in `find . -type d | grep "./" | grep "/delta$"`
        do
                local NUMBEROFDELTAFILES=`ls $ADIR$SUFFIX/$DirList2 | wc -l`
                if [ $NUMBEROFDELTAFILES -eq 0 ]
                then
                        continue
                fi
                cp $ADIR$SUFFIX/$DirList2/* $ADIR/$DirList2/
                printf "\nDIRECTORY $ADIR$SUFFIX/$DirList2/*  COPIED TO $ADIR/$DirList2/"  >> $LOG_FILE
        done
}

ProcessConfigFileLinks(){
        local ENTRY=$1
        local LN_DIR=`echo $(readlink $ENTRY)`
        if [ -d $LN_DIR ]
        then
                if [ -d $LN_DIR$SUFFIX ]
                then
                        printf "\nDIRECTORY: [$LN_DIR$SUFFIX] ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
                        rm -rf $LN_DIR$SUFFIX
                fi
                mv $LN_DIR $LN_DIR$SUFFIX
                if [ $? -eq 0 ]
                then
                        printf "\nDIRECTORY: [$LN_DIR] MOVED TO [$LN_DIR$SUFFIX]"  >> $LOG_FILE
                        rm $ENTRY
                        ln -s $LN_DIR$SUFFIX $ENTRY
                        if [ $? -eq 0 ]
                        then
                                printf "\nDIRECTORY: [$LN_DIR$SUFFIX] IS LINKED TO LINK [$ENTRY]"  >> $LOG_FILE
                                mkdir $LN_DIR
                                if [ $? -eq 0 ]
                                then
                                        printf "\nDIRECTORY: [$LN_DIR] CREATED"  >> $LOG_FILE
                                        CreateSubDirectories $LN_DIR
                                        CopyArchiveDirectories $LN_DIR
                                        CreateSubLinks $LN_DIR
                                else
                                        printf "\nDIRECTORY: [$LN_DIR] CANT NOT BE CREATED"  >> $LOG_FILE
                                        error_inc
                                fi
                        else
                                printf "\nDIRECTORY: [$LN_DIR$SUFFIX] CANT NOT BE LINKED"  >> $LOG_FILE
                                error_inc
                        fi
                else
                        printf "\nDIRECTORY: [$LN_DIR] CANT NOT BE MOVED"  >> $LOG_FILE
                        error_inc
                fi
        else
                printf "\nINVALID LINKED DIRECTORY: [$LN_DIR]" >> $LOG_FILE
                error_inc
        fi
}

ProcessConfigFileDirectories(){
        local DIR=$1
        if [ -d $DIR$SUFFIX ]
        then
                printf "\nDIRECTORY: [$DIR$SUFFIX] ALREADY PRESENT, SO DELETED"  >> $LOG_FILE
                rm -rf $DIR$SUFFIX
        fi
        mv $DIR $DIR$SUFFIX
        if [ $? -eq 0 ]
        then
                printf "\nDIRECTORY: [$DIR] MOVED TO [$DIR$SUFFIX]"  >> $LOG_FILE
                mkdir $DIR
                if [ $? -eq 0 ]
                then
                        printf "\nDIRECTORY: [$DIR] CREATED"  >> $LOG_FILE
                        CreateSubDirectories $DIR
                        CopyArchiveDirectories $DIR 
                        CreateSubLinks $DIR
                else
                        printf "\nDIRECTORY: [$DIR] CANT BE CREATED"  >> $LOG_FILE
                        error_inc
                fi
        else
                printf "\nDIRECTORY: [$DIR] CANT BE MOVED"  >> $LOG_FILE
                error_inc
        fi
}

BackupProcessForInMemorySubLinks(){
	local WIC_CONF=$1
        printf "\nBACKUP PROCESS FOR COUNTERS STARTS"  >> $LOG_FILE
        while read wcDirectoryList
        do
                printf "\nENTRY : [$wcDirectoryList]"  >> $LOG_FILE
                local ENTRY=$wcDirectoryList
                if [ -L $ENTRY ]
                then
                        printf "\nPROCESSING ENTRY: [$ENTRY], WHICH IS A LINK"  >> $LOG_FILE
                        ProcessConfigFileLinks $ENTRY
                elif [ -d $ENTRY ]
                then
                        printf "\nPROCESSING ENTRY: [$ENTRY], WHICH IS A DIRECTORY"  >> $LOG_FILE
                        ProcessConfigFileDirectories $ENTRY
                else
                        printf "\nINVALID ENTRY: [$ENTRY], NOT A LINK OR DIRECTORY"  >> $LOG_FILE
                        error_inc
                fi

        done < $WIC_CONF
}

PromptForUserDecisionForNewConfigFile(){
	NEW_CON_FILE=$1
	cd $CURRDIR

        printf "\nCONTENTS OF NEW CONFIG FILE ARE\n" >> $LOG_FILE
        cat $NEW_CONFIG_FILE >> $LOG_FILE
        echo -e "CONTENTS OF NEW CONFIG FILE ARE"
        cat $NEW_CONFIG_FILE

	cp $NEW_CON_FILE $NEW_CON_FILE.for.counter.links.cnf
	mv $NEW_CON_FILE $NEW_CON_FILE.for.non.counter.links.cnf

	printf "\nNEW CONFIG FILE: [$NEW_CON_FILE] REPLICATTED INTO"  >> $LOG_FILE
	printf "\nCONFIG FILE WITH COUNTER: [$NEW_CON_FILE.for.counter.links.cnf]" >> $LOG_FILE
	printf "\nCONFIG FILE WITHOUT COUNTER: [$NEW_CON_FILE.for.non.counter.links.cnf]" >> $LOG_FILE

	echo -e  "\nNEW CONFIG FILE: [$NEW_CON_FILE] REPLICATTED INTO"
        echo -e "CONFIG FILE WITH COUNTER: [$NEW_CON_FILE.for.counter.links.cnf]" 
        echo -e "CONFIG FILE WITHOUT COUNTER: [$NEW_CON_FILE.for.non.counter.links.cnf]"
	
	printf "\n-------------------------------------------------------------------------" >> $LOG_FILE
	printf "\nSCRIPT IS ON HOLD TILL NEXT ENTER, WHILE," >> $LOG_FILE
	printf "\nPLEASE OPEN RESPECTIVE CONFIG FILES, AND REMOVED IRRELATIVE LINKS, AND CLOSED AFTER SAVE"  >> $LOG_FILE
	echo -e "-------------------------------------------------------------------------"
	echo -e "SCRIPT IS ON HOLD TILL NEXT ENTER, WHILE,\nPLEASE OPEN RESPECTIVE CONFIG FILES, AND REMOVED IRRELATIVE LINKS, AND CLOSED AFTER SAVE"
	
	printf "\nHIT ENTER WHEN YOU ARE DONE WITH CONFIG FILES\n"  >> $LOG_FILE
	echo -e "\nHIT ENTER WHEN YOU ARE DONE WITH CONFIG FILES\n"
	read CHOICE
	
	cd $CURRDIR
	printf "\n------------------------------\nCALLING COUNTER SCRIPT\n------------------------------\n"  >> $LOG_FILE
        echo -e "\n-----------------------------\nCALLING COUNTER SCRIPT\n------------------------------\n"
	ValidateConfigFile $NEW_CON_FILE.for.counter.links.cnf
	COPYOPTION=YES	
	BackupProcessForInMemorySubLinks $NEW_CON_FILE.for.counter.links.cnf		

	cd $CURRDIR	
	printf "\n-------------------------------\nCALLING NON COUNTER SCRIPT\n--------------------------\n"  >> $LOG_FILE
        echo -e "\n------------------------------\nCALLING NON COUNTER SCRIPT\n-------------------------\n"
        ValidateConfigFile $NEW_CON_FILE.for.non.counter.links.cnf
	COPYOPTION=NO
	BackupProcessForInMemorySubLinks $NEW_CON_FILE.for.non.counter.links.cnf
}

DeepCopyProcess(){
	printf "\nDEEP COPY PROCESS STARTS"  >> $LOG_FILE
	cd $CURRDIR
	while read DirectoryListdc
        do
                printf "\nENTRY : [$DirectoryListdc]"  >> $LOG_FILE
                ENTRY=$DirectoryListdc$SUFFIX
                if [ -d $ENTRY ]
                then
                        printf "\nPROCESSING ENTRY: [$ENTRY], WHICH IS A BACKED UP DIRECTORY"  >> $LOG_FILE
                        GetConfigFileOfLinks $ENTRY
                else
                        printf "\nINVALID ENTRY: [$ENTRY], NOT A DIRECTORY"  >> $LOG_FILE
                        error_inc
                fi

        done < $CON_FILE
        ReformatConfigFile $NEW_CONFIG_FILE
        PromptForUserDecisionForNewConfigFile $NEW_CONFIG_FILE
}

main()
{
	CURRDIR=`pwd`
	trap on_exit EXIT
	ERROR=0
	CON_FILE=NONE
	LOG_FILE=~/InMemoryBackupAt$(date +"%d%m%Y%H%M%S").log
	#LOG_FILE=$RANGERHOME/LOG/InMemoryBackupAt$(date +"%d%m%Y%H%M%S").log

	if [ $# -ne 1 ] 
	then
		echo -e "USAGE : $0 CONFIG_FILE"
       		printf "USAGE : $0 CONFIG_FILE"  >> $LOG_FILE
		error_inc
		exit 1
	fi

	CON_FILE=$1
	SUFFIX="_801"
	if [ "$SUFFIX" = "" ]
        then
                printf "\nSUFFIX IS NULL, QUITTING"  >> $LOG_FILE
                error_inc
                exit 55
        fi
	ValidateConfigFile $CON_FILE
	BackupProcessOfDirectories
	NEW_CONFIG_FILE=$CURRDIR/new_config_file
	touch $NEW_CONFIG_FILE
	> $NEW_CONFIG_FILE
	DeepCopyProcess
}

main "$@"
