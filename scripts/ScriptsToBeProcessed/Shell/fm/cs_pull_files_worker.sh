#! /bin/bash
. $RANGERHOME/sbin/rangerenv.sh

# Configuration variable to specify the number of attempts the script has to retry FTPing the incomplete files.
NumberOfAttempts=2;

GetListOfFileNamesFromRemoteMachine ()
{
	>$RANGERHOME/tmp/ListofAllFiles_$InstanceKey.tmp
	>$RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
	CheckRemoteDir > $RANGERHOME/tmp/remoteDirInfo.tmp 2>&1
	grep "No such file or directory" $RANGERHOME/tmp/remoteDirInfo.tmp > /dev/null
	if [ $? -ne 0 ]
	then
		GetFileList > $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.tmp 
		cat $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.tmp | tr -s ' ' | grep -v "^$" | sed 's/sftp> //g' | grep -v "^d" | cut -d' ' -f5,9 | grep -v "^$" > $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
		cp $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.tmp
		FilterOutOldFiles
		LastFile=`cat $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt | tail -1 | cut -d' ' -f2`

		if [ x$LastFile != "x" ]
		then
			updateLastFile 
		fi
	fi
}

updateLastFile ()
{
sqlplus -s /nolog << EOF > /dev/null
CONNECT_TO_SQL
set feedback off

update configurations set value = '$LastFile' where config_key = 'LAST_CS_FILE';
commit ;

quit;
EOF
}

FilterOutOldFiles ()
{
	if [ $LastFile == "NO_LAST_FILE" ]
	then
		LastFileSequenceNumber=-1
		LastFileDate=-1
	else	
		LastFileSequence=`echo "$LastFile" | cut -d "." -f2`
		LastFileSequenceNumber=${LastFileSequence:9:3}
		LastFileDate=${LastFileSequence:17:6} #yymmdd 
	fi

	>$RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
	while [ 1 ]
	do
		read line 
		if [ "$line" == "" ]
		then 
			break
		fi
		curFileSequence=`echo "$line" |cut -d " " -f2 | cut -d "." -f2`
		curFileSequenceNumber=${curFileSequence:9:3}
		curFileDate=${curFileSequence:17:6}

		if [ $curFileDate -eq $LastFileDate ]
		then
			if [ $curFileSequenceNumber -gt $LastFileSequenceNumber ]
			then
				echo "$line" >> $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
			fi
		elif [ $curFileDate -gt $LastFileDate ]
		then
			echo "$line" >> $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
		fi
	done < $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.tmp

		
}

GetFileList ()
{	
	ftp -n "$MachineIP"<<EOF 
		quote USER $UserName
		quote PASS $UserPass
		cd $Remote_Source/
		ls -l
		bye
EOF
}

CheckRemoteDir ()
{	
	ftp -n "$MachineIP"<<EOF 
		quote USER $UserName
		quote PASS $UserPass
		ls $Remote_Source/
		bye
EOF
}

DoFTP ()
{
	rm -f $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	echo "ftp -n "$MachineIP" <<EOF" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	echo "quote USER $UserName" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	echo "quote PASS $UserPass" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	echo "cd $Remote_Source" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	echo "binary" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	if [ -s $1 ]
	then
		for f in `cat $1 | cut -d ' ' -f2`
		do
			echo "get $f" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
		done
	fi

	echo "bye" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	echo "EOF" >> $RANGERHOME/tmp/docsftp_$InstanceKey.sh
	bash $RANGERHOME/tmp/docsftp_$InstanceKey.sh >/dev/null
}

CheckForConsistency ()
{

	>$RANGERHOME/tmp/cscompletedfiles_$InstanceKey.txt
	>$RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt
	>$RANGERHOME/tmp/csmissingfiles_$InstanceKey.txt

	if [ -s $1 ]
	then
		while [ 1 ]
		do
			read line
		        if [ "$line" == "" ]
        	        then
                	        break
	                fi
			file=`echo "$line" | cut -d " " -f2`
			if [ -f $file ]
			then
				sizeoFLocalFile=`ls -l $file |tr -s ' ' |cut -d ' ' -f5`
				sizeoFRemoteFile=`echo "$line" | cut -d " " -f1`
				if [ $sizeoFLocalFile -eq $sizeoFRemoteFile ]
				then
					echo $line >>$RANGERHOME/tmp/cscompletedfiles_$InstanceKey.txt
				else
					echo $line >>$RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt
				fi
			else
				echo $line >>$RANGERHOME/tmp/csmissingfiles_$InstanceKey.txt
			fi		
		
		done < $1

	fi

}
BackupAndTouchSuccess ()
{
	if [ -s $RANGERHOME/tmp/cscompletedfiles_$InstanceKey.txt ]
	then
		for file in `cat $RANGERHOME/tmp/cscompletedfiles_$InstanceKey.txt | cut -d ' ' -f2`
		do
			if [ -s $file ]
			then
				ActualFileName=`basename $file`
				touch success/$ActualFileName
			fi
		done
	fi

}

WriteLog ()
{
	nooffilesftped=`cat $RANGERHOME/tmp/csftpedfiles_$InstanceKey.txt | wc -l`
	Date=`date "+%Y/%m/%d %T"`
	echo -e "----- Files pulled from ($Remote_Source) to ($Local_Destination) directory -----" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log 
	
	echo "" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo -e "Timestamp         : $Date" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log 
	echo -e "No of files FTPed : $nooffilesftped" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo ""	>> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo -e "Size		File Name" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo ""	>> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	awk -v TS="	" '{print $1TS$2}' $RANGERHOME/tmp/csftpedfiles_$InstanceKey.txt >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo ""	>> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	if [ -s $RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt ] || [ -s $RANGERHOME/tmp/csmissingfiles_$InstanceKey.txt ]
	then
		echo -e "FTP Failed for Files : " >>$RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
		echo ""	>> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
		cat $RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt  | cut -d ' ' -f2 >>$RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
		cat $RANGERHOME/tmp/csmissingfiles_$InstanceKey.txt | cut -d ' ' -f2 >>$RANGERHOME/LOG/cs_pull_files_$InstanceKey.log 
	fi
	
	echo "" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo -e "---------------------------------END-------------------------------" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo "" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log
	echo "" >> $RANGERHOME/LOG/cs_pull_files_$InstanceKey.log

}
RemoveInCompleteFiles ()
{

	if [ -s $RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt ]
	then
		for f in `cat $RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt | cut -d ' ' -f2`
		do
			rm -f $f
		done
	fi

}

FTPRawfiles ()
{
	cd $Local_Destination

	GetListOfFileNamesFromRemoteMachine

	>$RANGERHOME/tmp/csftpedfiles_$InstanceKey.txt
	if [ -s $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt ]
	then
		DoFTP $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
		CheckForConsistency $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
		cat $RANGERHOME/tmp/cscompletedfiles_$InstanceKey.txt >> $RANGERHOME/tmp/csftpedfiles_$InstanceKey.txt
		BackupAndTouchSuccess
		for (( i=0 ; i<$NumberOfAttempts ; i++ ))
		do	
			cp $RANGERHOME/tmp/csincompletefiles_$InstanceKey.txt $RANGERHOME/tmp/retryFTP_$InstanceKey.tmp
			cat $RANGERHOME/tmp/csmissingfiles_$InstanceKey.txt >>$RANGERHOME/tmp/retryFTP_$InstanceKey.tmp   
			DoFTP $RANGERHOME/tmp/retryFTP_$InstanceKey.tmp
			CheckForConsistency $RANGERHOME/tmp/retryFTP_$InstanceKey.tmp
			cat $RANGERHOME/tmp/cscompletedfiles_$InstanceKey.txt >> $RANGERHOME/tmp/csftpedfiles_$InstanceKey.txt
			BackupAndTouchSuccess
		done
		RemoveInCompleteFiles		
	fi

	if [ -s $RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt ]
	then
		WriteLog
	fi
}


main()
{
	MachineIP="$1"
	UserName="$2"
	UserPass="$3"
	Remote_Source="$4"
	Local_Destination="$5"
	LastFile="$6"

	InstanceKey=`basename $Remote_Source`

	Today=`date +%Y%m%d`

	mkdir -p $Local_Destination/success
	FTPRawfiles 
}

main $*
