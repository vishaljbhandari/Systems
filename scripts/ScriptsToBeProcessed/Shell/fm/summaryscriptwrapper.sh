#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

normalyear=( 0 31 28 31 30 31 30 31 31 30 31 30 31 )
leapyear=( 0 31 29 31 30 31 30 31 31 30 31 30 31 )

script=$1
year=$2
month=$3
day=$4
date="$4-$3-$2"
hourofday=0


updatehourofday()
{
if [ $DB_TYPE == "1" ]
then
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
set termout off
set heading off
set echo off
update cdr set HOUR_OF_DAY=0;
quit
EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF
set termout off
set heading off
set echo off
update cdr set HOUR_OF_DAY=0;
quit
EOF

fi
}

getdayofyear()
{	

leapyearcondn1=`expr $year % 4`
leapyearcondn2=`expr $year % 100`
leapyearcondn3=`expr $year % 400`

if [ $leapyearcondn1 = 0 -a $leapyearcondn2 != 0 ] || [ $leapyearcondn3 = 0 ]   
then
	leap=1
else
	leap=0
fi	

case $leap in
		1) for ((i=1; i < $month; i++))
			do
				day=`expr $day + ${leapyear[$i]}`
			done
			;;
		0) for ((i=1; i < $month; i++))
			do
				day=`expr $day + ${normalyear[$i]}`
			done
			;;
esac
}

runsummaryscript()
{
	bash $RANGERHOME/bin/$script $date $day $hourofday
}
	
main()
{
	updatehourofday;
	getdayofyear;
	runsummaryscript;
}

main
