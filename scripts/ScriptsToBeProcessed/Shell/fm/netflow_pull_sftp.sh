#/*******************************************************************************
# *	Copyright (c) Subex Systems Limited 2006. All rights reserved.	            *
# *	The copyright to the computer program(s) herein is the property of Subex    *
# *	Systems Limited. The program(s) may be used and/or copied with the written  *
# *	permission from Subex Systems Limited or in accordance with the terms and   *
# *	conditions stipulated in the agreement/contract under which the program(s)  *
# *	have been supplied.                                                         *
# *******************************************************************************/
#! /bin/bash

. /nikira/NIKIRAROOT/sbin/rangerenv.sh

# Configuration variable to specify the number of attempts the script has to retry FTPing the incomplete files.
NumberOfAttempts=2;

GetListOfFileNamesFromRemoteMachine ()
{
	>$RANGERHOME/tmp/ListofNFFiles_$InstanceKey.tmp
	>$RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
	CheckRemoteDir > $RANGERHOME/tmp/remoteDirInfo.tmp 2>&1
	grep "No such file or directory" $RANGERHOME/tmp/remoteDirInfo.tmp > /dev/null
	if [ $? -ne 0 ]
	then
		GetFileList > $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.tmp 
		cat $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.tmp | tr -s ' ' | grep -v "^$" | sed 's/sftp> //g' | grep -v "^d" | cut -d' ' -f5,9 | grep -v "^$" > $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
		cp $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.tmp
		FilterOutOldFiles
		CurrLastFile=$LastFile
		LastFile=`cat $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt | tail -1 | cut -d " " -f2`

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

update configurations set value = '$LastFile' where config_key = 'LAST_NF_FILE';
commit ;

quit;
EOF
}

FilterOutOldFiles ()
{
	if [ $LastFile == "NO_LastFile" ]
	then
		LastFileDate=`date +"%y%m%d"`
	else	
		LastFileDate=`echo "$LastFile" | cut -d "." -f1 | cut -d "_" -f3`
	fi

	>$RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
	while [ 1 ]
	do
		read line
		if [ "$line" == "" ]
		then 
			break
		fi
		curFileDate=`echo "$line" | cut -d " " -f2 | cut -d "." -f1 | cut -d "_" -f3`

        if [ $curFileDate -eq $LastFileDate ]
        then
            if [ $LastFile == "NO_LastFile" ]
            then
                echo "$line" >> $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
            fi
        elif [ $curFileDate -gt $LastFileDate ]
        then
            echo "$line" >> $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
        fi
	done < $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.tmp
}

GetFileList ()
{	
#		ls -l ip_trafic_*
	ftp -n "$MachineIP"<<EOF 
		quote USER $UserName
		quote PASS $UserPass
		cd $Remote_Source/
		dir ip_trafic_*
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
	rm -f $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	echo "ftp -n "$MachineIP" <<EOF" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	echo "quote USER $UserName" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	echo "quote PASS $UserPass" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	echo "cd $Remote_Source" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	echo "binary" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	if [ -s $1 ]
	then
		for f in `cat $1 | cut -d ' ' -f2`
		do
			echo "get $f" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
		done
	fi

	echo "bye" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	echo "EOF" >> $RANGERHOME/tmp/donfftp_$InstanceKey.sh
	bash $RANGERHOME/tmp/donfftp_$InstanceKey.sh >/dev/null
}

CheckForConsistency ()
{

	>$RANGERHOME/tmp/nfcompletedfiles_$InstanceKey.txt
	>$RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt
	>$RANGERHOME/tmp/nfmissingfiles_$InstanceKey.txt

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
					echo $line >>$RANGERHOME/tmp/nfcompletedfiles_$InstanceKey.txt
				else
					echo $line >>$RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt
				fi
			else
				echo $line >>$RANGERHOME/tmp/nfmissingfiles_$InstanceKey.txt
			fi		
		
		done < $1

	fi

}

WriteLog ()
{
	nooffilesftped=`cat $RANGERHOME/tmp/nfftpedfiles_$InstanceKey.txt | wc -l`
	Date=`date "+%Y/%m/%d %T"`
	echo -e "----- Files pulled from ($Remote_Source) to ($Local_Destination) directory -----" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log 
	
	echo "" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo -e "Timestamp         : $Date" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log 
	echo -e "No of files FTPed : $nooffilesftped" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo ""	>> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo -e "Size		File Name" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo ""	>> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	awk '{print $1"\t"$2}' $RANGERHOME/tmp/nfftpedfiles_$InstanceKey.txt >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo ""	>> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	if [ -s $RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt ] || [ -s $RANGERHOME/tmp/nfmissingfiles_$InstanceKey.txt ]
	then
		echo -e "FTP Failed for Files : " >>$RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
		echo ""	>> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
		cat $RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt  | cut -d ' ' -f2 >>$RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
		cat $RANGERHOME/tmp/nfmissingfiles_$InstanceKey.txt | cut -d ' ' -f2 >>$RANGERHOME/LOG/nf_pull_files_$InstanceKey.log 
	fi
	
	echo "" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo -e "---------------------------------END-------------------------------" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo "" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log
	echo "" >> $RANGERHOME/LOG/nf_pull_files_$InstanceKey.log

}

RemoveInCompleteFiles ()
{

	if [ -s $RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt ]
	then
		for f in `cat $RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt | cut -d ' ' -f2`
		do
			rm -f $f
		done
	fi

}

FTPRawfiles ()
{
	cd $Local_Destination

	GetListOfFileNamesFromRemoteMachine

	>$RANGERHOME/tmp/nfftpedfiles_$InstanceKey.txt
	if [ -s $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt ]
	then
		DoFTP $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
		CheckForConsistency $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
		cat $RANGERHOME/tmp/nfcompletedfiles_$InstanceKey.txt >> $RANGERHOME/tmp/nfftpedfiles_$InstanceKey.txt
		for (( i=0 ; i<$NumberOfAttempts ; i++ ))
		do	
			cp $RANGERHOME/tmp/nfincompletefiles_$InstanceKey.txt $RANGERHOME/tmp/retryNFFTP_$InstanceKey.tmp
			cat $RANGERHOME/tmp/nfmissingfiles_$InstanceKey.txt >>$RANGERHOME/tmp/retryNFFTP_$InstanceKey.tmp   
			DoFTP $RANGERHOME/tmp/retryNFFTP_$InstanceKey.tmp
			CheckForConsistency $RANGERHOME/tmp/retryNFFTP_$InstanceKey.tmp
			cat $RANGERHOME/tmp/nfcompletedfiles_$InstanceKey.txt >> $RANGERHOME/tmp/nfftpedfiles_$InstanceKey.txt
		done
		RemoveInCompleteFiles		
	fi

	if [ -s $RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt ]
	then
		WriteLog
	fi
}

GetLastFile ()
{
sqlplus -s /nolog << EOF > nflastFileTime.txt
CONNECT_TO_SQL
set feedback off

select value from configurations where config_key='LAST_NF_FILE' ;

quit;
EOF

LastFile=`cat nflastFileTime.txt | grep -v Connected | tail -1`
rm -f nflastFileTime.txt

if [ x$LastFile == "x" ] 
then 
	LastFile="NO_LastFile"; 
fi

}

GettheFTPDetailsAndStartBGProcess ()
{
#	if [ -s $RANGERHOME/bin/netflow_pull_sftp.cfg ]
	if [ -s /nikira/subex_working_area/bin/netflow_pull_sftp/netflow_pull_sftp.cfg ]
	then
		grep -v "##" /nikira/subex_working_area/bin/netflow_pull_sftp/netflow_pull_sftp.cfg | grep -v "^$" >$RANGERHOME/tmp/nf_sftp.conf
		
		while read line
		do
			MachineIP=`echo "$line" | cut -d ',' -f1`
			UserName=`echo "$line" | cut -d ',' -f2`
			UserPass=`echo "$line" | cut -d ',' -f3`
			Remote_Source=`echo "$line" | cut -d ',' -f4`
			Local_Destination=`echo "$line" | cut -d ',' -f5`

			if [ -z "$MachineIP" -o  -z "$Remote_Source" -o -z "$Local_Destination" -o -z "$UserName" -o -z "$UserPass" ]
			then
				echo "Invalid configuration in netflow_pull_sftp.cfg"
				echo " -- Please refer netflow_pull_sftp.cfg file for more information" 
				exit 1
			fi

			InstanceKey=`basename $Remote_Source`

			FTPRawfiles

		done < $RANGERHOME/tmp/nf_sftp.conf

	else 
		echo "Unable to start the script..."
		echo "Configuration file: netflow_pull_sftp.cfg in path ($RANGERHOME/bin) may be missing or an empty file."
		exit 1 ;
	fi
}


main()
{
	>$RANGERHOME/tmp/ListofNFFiles_$InstanceKey.txt
#	while true
#	do
		GetLastFile
		GettheFTPDetailsAndStartBGProcess
#		sleep 86400 #1 Day
#	done
    exit 0
}

main $*
