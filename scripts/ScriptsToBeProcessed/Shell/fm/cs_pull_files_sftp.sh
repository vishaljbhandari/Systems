#/*******************************************************************************
# *	Copyright (c) Subex Systems Limited 2006. All rights reserved.	            *
# *	The copyright to the computer program(s) herein is the property of Subex    *
# *	Systems Limited. The program(s) may be used and/or copied with the written  *
# *	permission from Subex Systems Limited or in accordance with the terms and   *
# *	conditions stipulated in the agreement/contract under which the program(s)  *
# *	have been supplied.                                                         *
# *******************************************************************************/
#! /bin/bash

. $RANGERHOME/sbin/rangerenv.sh

GetLastFile ()
{
sqlplus -s /nolog << EOF > cslastFileTime.txt
CONNECT_TO_SQL
set feedback off

select value from configurations where config_key='LAST_CS_FILE' ;

quit;
EOF

LAST_FILE=`cat cslastFileTime.txt | grep -v Connected | tail -1`
rm -f cslastFileTime.txt

if [ x$LAST_FILE == "x" ] 
then 
	LAST_FILE="NO_LAST_FILE"; 
fi

}

GettheFTPDetailsAndStartBGProcess ()
{
	if [ -s $RANGERHOME/bin/cs_pull_files_sftp.cfg ]
	then
		grep -v "##" $RANGERHOME/bin/cs_pull_files_sftp.cfg | grep -v "^$" >$RANGERHOME/tmp/cs_sftp.conf
		
		while read line
		do
			MachineIP=`echo "$line" | cut -d ',' -f1`
			UserName=`echo "$line" | cut -d ',' -f2`
			UserPass=`echo "$line" | cut -d ',' -f3`
			SourceDIR=`echo "$line" | cut -d ',' -f4`
			DestDIR=`echo "$line" | cut -d ',' -f5`

			if [ -z "$MachineIP" -o  -z "$SourceDIR" -o -z "$DestDIR" -o -z "$UserName" -o -z "$UserPass" ]
			then
				echo "Invalid configuration in cs_pull_files_sftp.cfg"
				echo " -- Please refer cs_pull_files_sftp.cfg file for more information" 
				exit 1
			fi
			
			if [ $1 == "Today" ]
			then
				DateDir=`date "+%Y%m%d"`
			elif [ $1 == "Yesterday" ]
			then
				DateDir=`date "+%Y%m%d"`
				DateDir=`expr $DateDir - 1`
			fi

			SourceDIR=$SourceDIR/$DateDir/NGN
			$RANGERHOME/bin/scriptlauncher.sh $RANGERHOME/bin/cs_pull_files_worker.sh "$MachineIP $UserName $UserPass $SourceDIR $DestDIR $LAST_FILE" 
		done < $RANGERHOME/tmp/cs_sftp.conf

	else 
		echo "Unable to start the script..."
		echo "Configuration file: cs_pull_files_sftp.cfg in path ($RANGERHOME/bin) may be missing or an empty file."
		exit 1 ;
	fi
}


main()
{
	>$RANGERHOME/tmp/ListofAllFiles_$InstanceKey.txt
	while true
	do
		GetLastFile
		GettheFTPDetailsAndStartBGProcess "Yesterday"
		GettheFTPDetailsAndStartBGProcess "Today"
		sleep 1800 #30 min
	done
    exit 0
}

main $*
