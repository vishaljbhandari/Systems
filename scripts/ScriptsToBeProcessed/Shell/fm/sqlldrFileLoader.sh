#! /bin/bash

export	bDeleteFiles='false'
ret_status=0
function checkVars ()
{
	if [ x"$logFileName" = "x" ]
	then
		echo -e "Error :: \$logFileName not set...\t\texiting "
		exit 1 ;
	fi

	if [ x"$loaderControlFileName" = "x" ]
	then
		echo -e "Error :: \$loaderControlFileName not set...\t\texiting "
		exit 2 ;
	fi

	if [ x"$loaderDataFileName" = "x" ]
	then
		echo -e "Error :: \$loaderDataFileName not set...\t\texiting "
		exit 3 ;
	fi
}

function checkOptions ()
{
	if [ x"$useDirectLoading" = "x" ]
	then
		echo "Info :: Using conventional loading ..." >>$logFileName
		directOption="FALSE"
	else
		directOption="TRUE"
		echo "Info :: Using Direct loading option for sqlldr ..." >> $logFileName
	fi

	if [ x"$controlFileDir" = "x" ]
	then
		echo "Warn :: controlFileDir var Not Set, Considering default Default as $RANGERHOME/share/Ranger" >>$logFileName
		controlFileDir="$RANGERHOME/share/Ranger"
	fi

	echo $baseFileName |grep "\." >/dev/null
	if [ $? -ne 0 ]
	then
		echo "Error :: FileName is not in expected format ... Changing the file Name to $baseFileName.dat for loading" >>$logFileName
		mv $loaderDataFileName $loaderDataFileName.dat
		baseFileName=`basename $loaderDataFileName`
	fi	

}

function checkForBadData ()
{
	if [ -s $tmpLoaderBadFile ]
	then
		ret_status=1
		badFileName=`basename $tmpLoaderBadFile`
		if [ "$bDeleteFiles" == 'true' ]
		then
			echo -e "WARN :: Bad Records found during File Loading ... \n\tMoving the bad records to $COMMON_MOUNT_POINT/tmp/$badFileName" >>$logFileName
			mv $tmpLoaderBadFile $COMMON_MOUNT_POINT/tmp/
		fi
	fi

	if [ -s $tmpLoaderDiscFile ]
	then
		ret_status=1
		discardFileName=`basename $tmpLoaderDiscFile`
		if [ "$bDeleteFiles" == 'true' ]
		then
			echo -e "WARN :: Discarded Records found during File Loading ... \n\tMoving the bad records to $COMMON_MOUNT_POINT/tmp/$discardFileName" >>$logFileName
			mv $tmpLoaderDiscFile $COMMON_MOUNT_POINT/tmp/
		fi
	fi
}

function logLoaderSummaryLog ()
{
	echo -e "\n********************** Loader Summary Log **********************" >>$logFileName
    echo -e "\nData File:      $loaderDataFileName" >> $logFileName
	grep -e "Rejected" -e "ORA" $tmpLoaderLogFile  >>$logFileName 
    recread=`cat $tmpLoaderLogFile | grep "^Total logical records read:"`
    recrejected=`cat $tmpLoaderLogFile | grep "^Total logical records rejected:"`
    recdiscarded=`cat $tmpLoaderLogFile | grep "^Total logical records discarded:"`
    runtime=`cat $tmpLoaderLogFile | grep "^Run began on"`
    elapsedtime=`cat $tmpLoaderLogFile | grep "^Elapsed time was:"`
    echo -e "$runtime \n\n$recread \n$recrejected \n$recdiscarded \n\n$elapsedtime" >> $logFileName
    recrejectedno=`cat $tmpLoaderLogFile | grep "^Total logical records rejected:" | tr -s " " | cut -d" " -f5`
    recdiscardedno=`cat $tmpLoaderLogFile | grep "^Total logical records discarded:" | tr -s " " | cut -d" " -f5`

    echo -e "\n$returnmessg" >> $logFileName

}

function loadData ()
{
	checkVars ;
	baseFileName=`basename $loaderDataFileName`
	echo "Info :: Loading dataFile $loaderDataFileName ... ">> $logFileName 
	checkOptions ;
	
	if [ -z "$tmpLoaderBadFile" ]
	then
		tmpLoaderBadFile="/tmp/$baseFileName$$.bad"
		bDeleteFiles='true'
	fi
	tmpLoaderLogFile="/tmp/$baseFileName$$.log"
	tmpLoaderDiscFile="/tmp/$baseFileName$$.dsc"

	echo -e "Info :: Options Used for Loading the data... \n\t\tcontrolFile = $controlFileDir/$loaderControlFileName" >> $logFileName
	echo -e "\t\tdataFile = $loaderDataFileName \n\t\tlogFile = $tmpLoaderLogFile" >> $logFileName
	echo -e "\t\tbadFile = $tmpLoaderBadFile \n\t\tdirect = $directOption" >> $logFileName

	if [ x"$additionalOptions" != "x" ]
	then
		 echo "\t\t $additionalOptions ">>$logFileName
	fi

	CONNECT_TO_DB2LOAD_START
		command=`db2load $controlfile $loaderDataFileName $tmpLoaderBadFile`
		$command > $tmpLoaderLogFile
	CONNECT_TO_DB2LOAD_END


    CONNECT_TO_SQLLDR_START
        silent=header,feedback \
        control=$controlFileDir/$loaderControlFileName \
        data=$loaderDataFileName \
        log=$tmpLoaderLogFile \
        bad=$tmpLoaderBadFile \
        discard=$tmpLoaderDiscFile \
        direct=$directOption \
        ERRORS=999999 $additionalOptions
    CONNECT_TO_SQLLDR_END

	if [ x"$useVerboseLogging" = "x" ] || [ x"$useVerboseLogging" = "xN" ]
	then
		logLoaderSummaryLog
		checkForBadData
	else
		cat $tmpLoaderLogFile >>$logFileName
		checkForBadData
	fi
	if [ "$bDeleteFiles" == 'true' ] 
	then
		rm $tmpLoaderBadFile
	fi
    echo -e "\n---------------------------------------------" >> $logFileName
	return $ret_status
}

loadData ;
