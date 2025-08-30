#!/bin/env bash


. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. ~/.bashrc

if [ -z "$1" ]
then
	echo -e "Unable to Find the Input File.\n Exiting loader ...."
	exit 1
else
    filename="$1"
fi

if [ -z "$2" ]
then
	echo -e "Unable to Find the control file.\n Exiting loader ...."
	exit 2
else
    loaderControlFileName="$2"
fi

if [ -z "$3" ]
then
	echo -e "Unable to Find the control file directory.\n Exiting loader ...."
	exit 3
else
    controlFileDir="$3"
fi

if [ -z "$4" ]
then
    tmpLoaderBadFile="tmp_alarm_seq.bad"
else
    tmpLoaderBadFile="$4"
fi

if [ -z "$5" ]
then
    tmpLoaderDscFile="tmp_alarm_seq.dsc"
else
    tmpLoaderDscFile="$5"
fi

if [ -z "$6" -o -z "$7" ]
then
	echo -e "Invalid DB Username / password"
	exit 4
else
	db_user_name=$6
	db_password=$7
fi

if [ -z "$8" ]
then
	echo -e "Database SID not specified"
else
	db_sid="@$8"
fi

logfileName="$COMMON_MOUNT_POINT/LOG/tmp_alarm_seq_sqlldr.log"

if [ ! -e $filename ]
then
	echo -e "Unable to Find the Input File $filename.\n Exiting loader ...."
	exit 2
fi

loadToDB()
{

	os=`uname`

    if [ "$os" == "Linux" ]
    then
        dos2unix $filename
    fi

    if [ "$os" == "SunOS" ]
    then
        dos2unix  $filename > $RANGERHOME/bin/tmp_alarm_seq_unix.txt
        mv $RANGERHOME/bin/tmp_alarm_seq_unix.txt $filename
    fi

    if [ "$os" == "AIX" ]
    then
        tr -d "\015" < $filename > $RANGERHOME/bin/tmp_alarm_seq_unix.txt
        mv $RANGERHOME/bin/tmp_alarm_seq_unix.txt $filename
    fi

    if [ "$os" == "HP-UX" ]
    then
        dos2ux  $filename > $RANGERHOME/bin/tmp_alarm_seq_unix.txt
        mv $RANGERHOME/bin/tmp_alarm_seq_unix.txt $filename
    fi

	export logFileName="$logfileName"
	export loaderControlFileName
	export loaderDataFileName=$filename
	export controlFileDir
	export tmpLoaderBadFile="$filename.bad"

	sqlldr $db_user_name/$db_password$db_sid \
        silent=header,feedback \
        control=$controlFileDir/$loaderControlFileName \
        data=$loaderDataFileName \
        log=$logFileName \
        bad=$tmpLoaderBadFile \
        discard=$tmpLoaderDscFile \
        ERRORS=999999 \
		bindsize=20971520 \
		readsize=20971520 \
		rows=200000

	if [ $? -ne 0 ]
    then
        echo "SQLLoader execution failed" >> $COMMON_MOUNT_POINT/LOG/tmp.log
		exit $?
    fi
}


loadToDB $*
