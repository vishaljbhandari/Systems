# /usr/bin/env bash
logErrorMessage()
{
	errorMessage=$1

	echo "Terminating due to SQL Error. `date`" 
	echo -e "\nCheck $errorlog for more information" 
	echo "Statement failed: " >> $errorlog
	echo "$errorMessage" >> $errorlog
	echo -e "Terminated due to SQL Error. `date`" >> $errorlog
}

sqlErrorCheck() 
{
	sqlretcode=$1

	if [ $sqlretcode -eq 5 -o $sqlretcode -eq 6 ]
	then
		echo "Terminating due to SQL Error. `date`" 
		echo -e "\nCheck $errorlog for more information" 
		echo "Statement failed: " >> $errorlog
		cat $tempsqloutput | grep -v "Connected." | tr -d "\t"  | sed 's/ERROR\ at\ line\ [0-9]*/Error/g' >> $errorlog
		echo -e "Terminated due to SQL Error. `date`" >> $errorlog
		rm $tempsqloutput

		exit 1
	fi	
	rm $tempsqloutput
}

logReport()
{
	echo -e "\nData File:      $mainFileName" >> $mainlog
	recread=`cat $logFileName | grep "^Total logical records read:"`
	recrejected=`cat $logFileName | grep "^Total logical records rejected:"`
	recdiscarded=`cat $logFileName | grep "^Total logical records discarded:"`
	runtime=`cat $logFileName | grep "^Run began on"`
	elapsedtime=`cat $logFileName | grep "^Elapsed time was:"` 
	echo -e "$runtime \n\n$recread \n$recrejected \n$recdiscarded \n\n$elapsedtime" >> $mainlog
	recrejectedno=`cat $logFileName | grep "^Total logical records rejected:" | tr -s " " | cut -d" " -f5`
	recdiscardedno=`cat $logFileName | grep "^Total logical records discarded:" | tr -s " " | cut -d" " -f5`

	echo -e "\n$returnmessg" >> $mainlog
	if [ $recrejectedno -gt 0 ]
	then
		echo -e "Bad File:      $badFileName" >> $mainlog
	fi

	if [ $recdiscardedno -gt 0 ]
	then
		echo -e "Discard File:      $discardFileName" >> $mainlog
	fi
	echo -e "\n---------------------------------------------" >> $mainlog

	rm $logFileName
}

errorReport()
{
	echo -e "\n$returnmessg" >> $errorlog
	cat $logFileName | grep "ORA\-" >> $errorlog
	cat $logFileName | grep "SQL\*Loader\-" >> $errorlog
	echo -e "\n\nRecordLoader Terminated: `date`" >> $errorlog
	echo -e "\nRecordLoader Terminated: `date`" 
	
	rm $logFileName
}

unknownErrorReport()
{
	echo -e "\n$returnmessg" >> $errorlog
	echo -e "\n\nRecordLoader Terminated: `date`" >> $errorlog
	echo -e "\nRecordLoader Terminated: `date`" 
}

scriptError()
{
	echo -e "\n$1" >> $errorlog
	echo -e "\n\nRecordLoader Terminated: `date`" >> $errorlog
	echo -e "\nRecordLoader Terminated: `date`" 
	
	exit 1
}
