#! /bin/bash

ReadOptions(){
	if [ "$OPTION" == "BACKUP" ]
	then
 		printf "\nBACKUP OPTION SELECTED [$OPTION]" >> $LOG_FILE
	elif [ "$OPTION" == "REVERT" ]
	then
  		printf "\nREVERT OPTION SELECTED [$OPTION]"  >> $LOG_FILE
	else
  		printf "\nINVALID OPTION PARAMETER [$OPTION]"  >> $LOG_FILE
		error_inc
		exit 2
	fi
}

error_inc(){
	ERROR=`expr $ERROR + 1`
}

on_exit(){
	echo -e "CONFIG FILE : [$CON_FILE] PROCESSED WITH [$OPTION] OPTION & [$ERROR] ERRORS\nPLEASE CHECK LOG FILE $LOG_FILE"
	printf "\nCONFIG FILE : [$CON_FILE] PROCESSED WITH [$OPTION] OPTION & [$ERROR] ERRORS\n\n\n" >> $LOG_FILE
}

ValidateConfigFile(){
	if [ ! -f $CON_FILE ]
	then
        	printf "\nCONFIG FILE : $LOG_FILE DOESNT EXISTS\n"  >> $LOG_FILE
		error_inc
		exit 3
	fi
	sed '/^[ \t]*$/d' < $CON_FILE > $CON_FILE.tmp
	cat $CON_FILE.tmp >> $LOG_FILE
	echo "$CON_FILE" | grep "\." > /dev/null 2>&1
	if [ $? -ne 0 ]
	then
  		printf "\nBAD CONFIG FILE [SPACES]: $CON_FILE\n"  >> $LOG_FILE 
		error_inc
		#exit 4
	fi
	linecount=`cat $CON_FILE | wc -l`
	numberofpipes=`grep '^.*[^/]|.*$' $CON_FILE | wc -l`
	if [ $linecount -ne $numberofpipes ]
	then
  		printf \n"BAD CONFIG FILE [INVALID ENTRIES]: $CON_FILE\n"  >> $LOG_FILE
		error_inc
		exit 5
	fi
}

CreateSubDirectories(){
	CDIR=$1
	CSUFF=$2
	cd $CDIR$CSUFF
	#for dirs in `find . -type d | grep "./" | grep -v cache_delta`
	for dirs in `find . -type d | grep "./"`
	do
        	mkdir $CDIR/$dirs
		if [ $? -ne 0 ]
        	then
               		printf "\nDIRECTORY $CDIR/$dirs CANT BE CREATED"  >> $LOG_FILE
			error_inc
		else
			printf "\nDIRECTORY $CDIR/$dirs IS CREATED"  >> $LOG_FILE	
		fi
	done
}

CopyArchiveDirectories(){
	ADIR=$1
	ASUFF=$2
	cd $ADIR$ASUFF
        for DirList in `find . -type d | grep "./" | grep "cache_delta/archive"`
	do
                AREQUIRED_DIR=`echo $DirList | awk -F"cache_delta" '{print $1F}'`
                printf "\n[$AREQUIRED_DIR] | [$DirList] " >> $LOG_FILE
		if [ "$AREQUIRED_DIR" != "$DirList" ]
                then
                        cp -r $ADIR$ASUFF/$DirList $ADIR/$AREQUIRED_DIR/delta
                	printf "\nDIRECTORY $ADIR$ASUFF/$ADirList COPIED TO $ADIR/$AREQUIRED_DIR/delta"  >> $LOG_FILE
        	fi
	done

}

ReadTargetSubLinks(){
	DIR=$1
	SUFF=$2
	cd $DIR$SUFF
	printf "\nINSIDE : [$DIR$SUFF]"  >> $LOG_FILE
	CURR=`pwd`
	find . -maxdepth 1 -type l | grep "./" >> $LOG_FILE
	for links in `find . -maxdepth 1 -type l | grep "./"`
        do
		if [ -L $links ]
                then
			SUB_DIR=`echo $(readlink $links)`
			if [ -d $SUB_DIR ]
                        then
				mv $SUB_DIR $SUB_DIR$SUFF
                                if [ $? -eq 0 ]
                                then
                                        rm $links
					printf "\nLINK REMOVED : [$links]"  >> $LOG_FILE
					ln -s $SUB_DIR$SUFF $links$SUFF
					printf "\nLINK RECREATED : [$links$SUFF] FOR [$SUB_DIR$SUFF]"  >> $LOG_FILE
					mkdir $SUB_DIR
					printf "\nSUB DIRECTORY CREATED : [$SUB_DIR]"  >> $LOG_FILE
					ln -s $SUB_DIR $DIR/$links
					printf "\nSUB LINK CREATED : [$DIR/$links] FOR [$SUB_DIR]"  >> $LOG_FILE
					CreateSubDirectories $SUB_DIR $SUFF
					cd $CURR
					CopyArchiveDirectories $SUB_DIR $SUFF
					cd $CURR
				else
                                        printf "\nCOULD NOT MOVE LINKED DIRECOTRY : [$SUB_DIR]"  >> $LOG_FILE
					error_inc
				fi	
			else
				printf "\nBROKEN LINK FOUND, SUB DIRECOTRY : [$SUB_DIR] NOT EXIST"  >> $LOG_FILE
				error_inc
			fi
		else
			printf "\nBROKEN LINK FOUND : [$links] NOT EXIST"  >> $LOG_FILE
			error_inc
		fi
        done
}

ParentDirectoryBackupProcess(){
	PDIR=$1
	PSUFF=$2
	PLINK=$3
	mv $PDIR $PDIR$PSUFF
        if [ $? -eq 0 ]
        then
        	ln -s $PDIR$PSUFF $PLINK$PSUFF
           	if [ $? -eq 0 ]
                then
                	mkdir $PDIR
                        cd $PDIR$PSUFF
                        CreateSubDirectories $PDIR $PSUFF
			cd $PDIR$PSUFF
			CopyArchiveDirectories $PDIR $PSUFF
			cd $PDIR$PSUFF
			ReadTargetSubLinks $PDIR $PSUFF
			cd $PDIR$PSUFF
           	else
                	printf "\nCOULD NOT CREATE SOFT LINK FOR : [$PDIR$PSUFF]"  >> $LOG_FILE
                    	error_inc
                fi
    	else
        	printf "\nCOULD NOT MOVE LINKED DIRECOTRY : [$PDIR]"  >> $LOG_FILE
                error_inc	
     	fi
}


BackupProcess(){
        printf "BACKUP STARTS"  >> $LOG_FILE
        while read DirectoryList
        do
               	printf "\nENTRY : [$DirectoryList]"  >> $LOG_FILE
                TARGET_LINK=`echo $DirectoryList | awk -F"|" '{print $1F}'`
                if [ -L $TARGET_LINK ]
                then
                        TARGET_DIR=`echo $(readlink $TARGET_LINK)`
                        SUFFIX=`echo $DirectoryList | awk -F"|" '{print $2F}'`
        		printf "\nBACKUP STARTED WITH [$TARGET_DIR]"  >> $LOG_FILE
			if [ -d $TARGET_DIR ]
                        then
				ParentDirectoryBackupProcess $TARGET_DIR $SUFFIX $TARGET_LINK
			else
				printf "\nBROKEN LINK, DIRECOTRY : [$TARGET_DIR] NOT EXIST"  >> $LOG_FILE 
				error_inc
			fi
		else
			printf "\nINVALID LINK, LINK [$TARGET_LINK] DOES NOT LINK ANYTHING"  >> $LOG_FILE
			error_inc
		fi
	done < $CON_FILE
}

ProcessLinksForRevert(){
	RDIR=$1
	RSUFF=$2
	cd $RDIR
        printf "\nINSIDE : [$RDIR]" >> $LOG_FILE
	find . -maxdepth 1 -type l | grep "./" >> $LOG_FILE
	for rlinks in `find . -maxdepth 1 -type l | grep "./"`
        do
                if [ -L $rlinks ]
                then
                        TDIR=`echo $(readlink $rlinks)`
                        if [ -d $TDIR ]
                        then
				REV_LINK=`echo $rlink | awk -F"$RSUFF" '{print $1F}'`
				BDIR=`echo $TDIR | awk -F"$RSUFF" '{print $1F}'`
				if [ -d $BDIR ]
				then
					rm $rlinks
					rm -rf $BDIR/*
					mv $TDIR/* $BDIR/
					if [ $? -ne 0 ]
					then
						printf "\nCOULD NOT REVERT FROM [$TDIR] TO [$BDIR]" >> $LOG_FILE
					fi
					rm -rf $TDIR
					ln -s $BDIR $REV_LINK
					printf "\nREVERTED FROM [$TDIR] TO [$BDIR]" >> $LOG_FILE
				else
					printf "\nBROKEN LINK FOUND, DIRECOTRY : [$BDIR]  NOT EXIST - B"  >> $LOG_FILE
                                       	error_inc
				fi
                        else
                                printf "\nBROKEN LINK FOUND, DIRECOTRY : [$TDIR] NOT EXIST - A"  >> $LOG_FILE
                                error_inc
                        fi
                else
                        printf "\nINVALID LINK, LINK [$links] DOES NOT LINK ANYTHING"  >> $LOG_FILE
                        error_inc
                fi
        done
}

RevertProcess(){
        printf "REVERT STARTS"  >> $LOG_FILE
        while read DirectoryList
        do
                printf "\nENTRY : [$DirectoryList]"  >> $LOG_FILE
                TARGET_LINK=`echo $DirectoryList | awk -F"|" '{print $1F}'`
                if [ -L $TARGET_LINK ]
                then
                        TARGET_DIR=`echo $(readlink $TARGET_LINK)`
                        SUFFIX=`echo $DirectoryList | awk -F"|" '{print $2F}'`
                        printf "\nREVERT STARTED WITH [$TARGET_DIR]"  >> $LOG_FILE
                        if [ -d $TARGET_DIR ]
                        then
				if [ -d $TARGET_DIR$SUFFIX ]
				then
					rm -rf $TARGET_DIR/*
					mv $TARGET_DIR$SUFFIX/* $TARGET_DIR/
					rm -rf $TARGET_DIR$SUFFIX
					ProcessLinksForRevert $TARGET_DIR $SUFFIX
					printf "\nBACKUP DIRECTORY : [$TARGET_DIR$SUFFIX] PROCESSED"  >> $LOG_FILE
					rm $TARGET_LINK$SUFFIX
				else
                               		printf "\nBACKUP DIRECTORY : [$TARGET_DIR$SUFFIX] NOT FOUND"  >> $LOG_FILE
				fi
                        else
                                printf "\nBROKEN LINK, DIRECOTRY : [$TARGET_DIR] NOT EXIST"  >> $LOG_FILE
                                error_inc
                        fi
                else
                        printf "\nINVALID LINK, LINK [$TARGET_LINK] DOES NOT LINK ANYTHING"  >> $LOG_FILE
                        error_inc
                fi
        done < $CON_FILE
}



trap on_exit EXIT
ERROR=0
CON_FILE=NONE
OPTION=NONE
LOG_FILE=~/directory_backup_script.counter.log
#LOG_FILE=$RANGERHOME/LOG/directory_backup_script.counter.log

if [ $# -ne 2 ] 
then
	echo -e "USAGE : $0 CONFIG_FILE OPTION"
       	printf "USAGE : $0 CONFIG_FILE OPTION"  >> $LOG_FILE
	error_inc
	exit 1
fi

CON_FILE=$1
OPTION=$2
OPTION=`echo $OPTION | tr 'a-z' 'A-Z'`

ReadOptions
ValidateConfigFile

if [ "$OPTION" == "BACKUP" ]
then
	BackupProcess
else
	RevertProcess
fi
