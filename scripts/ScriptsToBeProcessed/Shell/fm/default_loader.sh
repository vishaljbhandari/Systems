#!/bin/env bash


. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

validateParameters()
{
	if [ $# -lt 4 ]
	then
		echo "Insufficient parameters...exiting" >> $logfileName
		exit 1
	fi

	if [ ! -f "$1" ]
	then
		echo "Specified input file not present...exiting" >> $logfileName
		exit 2
	fi

	if [ ! -f "$3/$2" ]
	then
		echo "Specified control file not present...exiting" >> $logfileName
		exit 3
	fi

	if [ -z "$4" ]
	then
    	echo "Bad file name not specified...exiting" >> $logfileName
		exit 4
	fi

	filename="$1"
	loaderControlFileName="$2"
	controlFileDir="$3"
	tmpLoaderBadFile="$4"
}

loadToDB()
{
	os=`uname`

    if [ "$os" == "Linux" ]
    then
        dos2unix $filename
    fi

    if [ "$os" == "SunOS" ]
    then
        dos2unix  $filename > $RANGERHOME/bin/default_tmp_unix.txt
        mv $RANGERHOME/bin/default_tmp_unix.txt $filename
    fi

    if [ "$os" == "AIX" ]
    then
        tr -d "\015" < $filename > $RANGERHOME/bin/default_tmp_unix.txt
        mv $RANGERHOME/bin/default_tmp_unix.txt $filename
    fi

    if [ "$os" == "HP-UX" ]
    then
        dos2ux  $filename > $RANGERHOME/bin/default_tmp_unix.txt
        mv $RANGERHOME/bin/default_tmp_unix.txt $filename
    fi

	dos2unix $filename
	export logFileName="$logfileName"
	export loaderControlFileName
	export loaderDataFileName=$filename
	export controlFileDir
	export tmpLoaderBadFile
	
	cat $tmpLoaderBadFile >> $COMMON_MOUNT_POINT/LOG/backup.log
	rm $tmpLoaderBadFile

	scriptlauncher $RANGERHOME/bin/sqlldrFileLoader.sh
	ret_status=$?
	if [ $ret_status -ne 0 ]
    then
        echo "SQLLoader execution failed"
		exit $ret_status
    fi
}

main ()
{
		logfileName="$COMMON_MOUNT_POINT/LOG/default_loader.log"
		echo "**************************************Start*************************************" >> $logfileName
		validateParameters $*
		loadToDB
		echo "***************************************End**************************************" >> $logfileName
}

main $* 
