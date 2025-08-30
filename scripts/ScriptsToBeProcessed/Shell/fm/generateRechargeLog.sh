#!/bin/bash

#*******************************************************************************
# *  Copyright (c) SubexAzure Limited 2006. All rights reserved.               *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Azure Limited. The program(s) may be used and/or copied with the written   *
# *  permission from SubexAzure Limited or in accordance with the terms and    *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************
# Author - Vishal M/Ajitesh Srinetra

. $RANGERHOME/sbin/rangerenv.sh
. $RUBY_UNIT_HOME/Scripts/configuration.sh

arg=`expr $# - 2`
lookback=`expr $# - 1`
arguments=`echo $* | cut -d' ' -f1-$arg`
norecords=`echo $* | cut -d' ' -f$# | awk 'BEGIN { FS = "=" } { print $2} '`
timestamp=`echo $* | cut -d' ' -f$lookback | awk 'BEGIN { FS = "=" } { print $2} '`
delimiter1=:
delimiter2=-
flag=0
databasesubscriber="SubscriberID"
databaseaccountid="AccountID"
databasenetwork="Network"



currentDate=`date +%d%m%Y%H%M%S`
day=`echo $currentDate | cut -c1-2`
month=`echo $currentDate | cut -c3-4`
year=`echo $currentDate | cut -c5-8`
hour=`echo $currentDate | cut -c9-10`
minutes=`echo $currentDate | cut -c11-12`
seconds=`echo $currentDate | cut -c13-14`
ArrayForMonth=( 0 31 28 31 30 31 30 31 31 30 31 30 31 )

cleanUp()
{

rm -f $RUBY_UNIT_HOME/data/*

cp $RUBY_UNIT_HOME/Scripts/R01.txt $RUBY_UNIT_HOME/data/R01.txt
cp $RUBY_UNIT_HOME/data/R01.txt $RUBY_UNIT_HOME/databackup/ 

if test -f $RUBY_UNIT_HOME/tmpfile
then
        rm $RUBY_UNIT_HOME/tmpfile
        echo "tmpfile removed successfully"
fi

if test -f $RUBY_UNIT_HOME/tmpfile1
then
        rm $RUBY_UNIT_HOME/tmpfile1
        echo "tmpfile1 removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/ds_field_config_recharge.dat
then
        rm $RUBY_UNIT_HOME/data/ds_field_config_recharge.dat
        echo "ds_field_config_recharge.dat file removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/Final_ds_field_config_recharge.dat
then
        rm $RUBY_UNIT_HOME/data/Final_ds_field_config_recharge.dat
        echo "Final_ds_field_config_recharge.dat file removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/subinfo.dat
then
        rm $RUBY_UNIT_HOME/data/subinfo.dat
        echo "subinfo.dat file removed successfully"
fi

rm -f $RUBY_UNIT_HOME/databackup/*.*

return 0
}

getdsfieldconfig()
{

$DB_QUERY << EOF 

set termout off
set heading off
set echo off
spool $RUBY_UNIT_HOME/data/ds_field_config_recharge.dat
select name || '=>' || position from field_configs where record_config_id=2 and position >0;
spool $RUBY_UNIT_HOME/data/subinfo.dat
select ID || '|' || account_id || '|' || network_id || '|' || fieldutil.normalize(phone_number) from subscriber where status in (1,2) and Subscriber_type=0;

quit
 
EOF


sed -e "s/IMSI\/ESN Number/IMSI/g" $RUBY_UNIT_HOME/data/ds_field_config_recharge.dat > $RUBY_UNIT_HOME/data/ds_field_config_recharge_tmp.dat
sed -e "s/Amount (INR)/Amount/g" $RUBY_UNIT_HOME/data/ds_field_config_recharge_tmp.dat > $RUBY_UNIT_HOME/data/Final_ds_field_config_recharge.dat

cat $RUBY_UNIT_HOME/data/Final_ds_field_config_recharge.dat > $RUBY_UNIT_HOME/tmpfile
awk 'NF > 0' $RUBY_UNIT_HOME/tmpfile > $RUBY_UNIT_HOME/data/ds_field_config_recharge.dat
cat $RUBY_UNIT_HOME/data/ds_field_config_recharge.dat | tr -d " " > $RUBY_UNIT_HOME/tmpfile

}

generateOneRecord()
{
		for i in $arguments
		do
		   fieldname=`echo $i | awk 'BEGIN { FS = "=" } { print $1} '`
		   fieldvalue=`echo $i | awk 'BEGIN { FS = "=" } { print $2} '`
		   if [ "$fieldname" == "PhoneNumber" ]
		   then
			PhoneNumber=$fieldvalue
			echo $PhoneNumber
		   fi
	           if [ "$fieldname" = "TimeStamp" ] 		
                   then
			dt=`echo $fieldvalue | cut -c1-10`
			tm=`echo $fieldvalue | cut -c11-18`
                	fieldvalue=`echo $dt $tm`  
			getPosition;
			replaceDatafile;
		    else
			getPosition;
			replaceDatafile;
		    fi
		   			
		done
		for i in $arguments
		do
			case "$i" in
			        SubscriberID )  databasesubscriber="";;
			        AccountID)      databaseaccountid="";;
			        Network)        databasenetwork="";;
			esac
		done
		databasearguments=`echo $databasesubscriber $databaseaccountid $databasenetwork`
		k=1	
		if test "$databasearguments"
		then
			for i in $databasearguments
			do
			   fieldname=$i
			   fieldvalue=`cat $RUBY_UNIT_HOME/data/subinfo.dat | grep "$PhoneNumber" | cut -d"|" -f$k`
			   getPosition;
			   replaceDatafile;
			   k=`expr $k + 1`
			   echo $fieldname
			   echo $fieldvalue
			   echo $k				
			done
		fi
		
}

getPosition()

{

fieldposition=`awk 'BEGIN { FS = ">" } /'"${fieldname}="'/ { print $2 }' "$RUBY_UNIT_HOME/tmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f $RUBY_UNIT_HOME/Scripts/Info.awk $RUBY_UNIT_HOME/data/R01.txt > $RUBY_UNIT_HOME/data/Recharge.txt.$$
	mv $RUBY_UNIT_HOME/data/Recharge.txt.$$ $RUBY_UNIT_HOME/data/R01.txt
}

multipleRecords()
{
	i=1
	while [ $i -le $norecords ]
	do
		cat $RUBY_UNIT_HOME/data/R01.txt >> $RUBY_UNIT_HOME/data/R01.txt.new
	   	i=`expr $i + 1`
	done		       	

}

getDayMonthYear()
{
	if [ $timestamp -ge $day ]
	then
		month=`expr $month - 1`
		if [ $month -eq '0' ]
		then
			month=12
			year=`expr $year - 1`
			rest=`expr $timestamp - $day`
			day=`expr ${ArrayForMonth[$month]} - $rest`
		else
			rest=`expr $timestamp - $day`
			day=`expr ${ArrayForMonth[$month]} - $rest`
		fi
	else 
			day=`expr $day - $timestamp`
			day=`echo $(printf "%.2d" "$day")`
	fi

return 0
}

generateRecharge()

{

getDayMonthYear;
j=1
while [ $j -le $norecords ]
do

	head -$j $RUBY_UNIT_HOME/data/R01.txt.new | tail -1 >  $RUBY_UNIT_HOME/tmpfile1
	seconds=`expr $seconds + 02`
	if [ "$seconds" -ge 60 ]
	then
		minutes=`expr $minutes + 1`
		minutes=`echo $(printf "%.2d" "$minutes")` 
		if [ "$minutes" -ge 60 ]
		then
			hour=`expr $hour + 1`
			hour=`echo $(printf "%.2d" "$hour")` 
			if [ "$hour" -ge 24 ]
			then
				hour=00
			fi
		minutes=00		
	fi
	seconds=00
	fi

	FinalTimeStamp=`echo $year$delimiter2$month$delimiter2$day $hour$delimiter1$minutes$delimiter1$seconds`
    FinalTimeStamp2=`echo $FinalTimeStamp | sed 's/-/\//g'`
    echo $FinalTimeStamp2
	sed -e "s#%timestamp#$FinalTimeStamp2#g" $RUBY_UNIT_HOME/tmpfile1 >> $RUBY_UNIT_HOME/databackup/RechargeRecord.txt.$$
	j=`expr $j + 1`
done
return 0

}

addHeader()
{

`echo Recordtype:2 > $RUBY_UNIT_HOME/databackup/RechargeRecordWithHeader.txt.$$`
cat $RUBY_UNIT_HOME/databackup/RechargeRecord.txt.$$ >> $RUBY_UNIT_HOME/databackup/RechargeRecordWithHeader.txt.$$

}

processRecharge()
{

cp $RUBY_UNIT_HOME/databackup/RechargeRecordWithHeader.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/RechargeRecord.txt.$$

cp $RUBY_UNIT_HOME/databackup/RechargeRecordWithHeader.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/success/RechargeRecord.txt.$$

mv $RUBY_UNIT_HOME/databackup/RechargeRecordWithHeader.txt.$$ $RUBY_UNIT_HOME/database/RechargeRecord.txt.$$

return 0

}

waitforfiletopickup()
{

counter=0
while [ $counter -le 200 ]
  do
        if !(test -f $RANGERHOME/FMSData/DataRecord/Process/RechargeRecord.txt.$$)
        then
              break
         else
               sleep 0.1
               counter=`expr $counter + 1`
               echo "waiting for file pick up"
        fi
  done
}

main()
{
cleanUp;
getdsfieldconfig;
generateOneRecord;
multipleRecords;
generateRecharge;
addHeader;
processRecharge;
waitforfiletopickup;
}

main 
