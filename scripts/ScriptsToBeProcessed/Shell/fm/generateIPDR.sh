#!/bin/bash

#*******************************************************************************
# *  Copyright (c) Subex Limited 2008. All rights reserved.                    *
# *  The copyright to the computer program(s) herein is the property of Subex  *
# *  Limited. The program(s) may be used and/or copied with the written        *
# *  permission from Subex Limited or in accordance with the terms and         *
# *  conditions stipulated in the agreement/contract under which the program(s)*
# *  have been supplied.                                                       *
# ******************************************************************************
# Author - Chandan P

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

cp $RUBY_UNIT_HOME/Scripts/IPDR.txt $RUBY_UNIT_HOME/data/IPDR.txt
cp $RUBY_UNIT_HOME/data/IPDR.txt $RUBY_UNIT_HOME/databackup/ 

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

if test -f $RUBY_UNIT_HOME/data/ds_field_config.dat
then
        rm $RUBY_UNIT_HOME/data/ds_field_config.dat
        echo "ds_field_config.dat file removed successfully"
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
spool $RUBY_UNIT_HOME/data/ds_field_config.dat
select name || '=>' || position from field_configs where record_config_id=83 and position > 0;
spool $RUBY_UNIT_HOME/data/subinfo.dat
select ID || '|' || fieldutil.normalize(phone_number) || '|' || network_id || '|' from subscriber where subscriber_type=0 and status = 1;

quit

EOF

sed -e "s/Value (INR)/Value/g" $RUBY_UNIT_HOME/data/ds_field_config.dat > $RUBY_UNIT_HOME/data/Final_ds_field_config.dat

cat $RUBY_UNIT_HOME/data/Final_ds_field_config.dat | tr -d " " > $RUBY_UNIT_HOME/tmpfile

}

generateOneRecord()
{
		for i in $arguments
		do
		   fieldname=`echo $i | awk 'BEGIN { FS = "=" } { print $1} '`
		   fieldvalue=`echo $i | awk 'BEGIN { FS = "=" } { print $2} '`
		   if [ "$fieldname" == "SubscriberID" ]
		   then	
			flag=1
		   fi
		   if [ "$fieldname" == "PhoneNumber" ]
		   then
			PhoneNumber=$fieldvalue
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
		
		if [ $flag -eq 0 ]
		then
			fieldname=SubscriberID
			fieldvalue=`cat $RUBY_UNIT_HOME/data/subinfo.dat | grep "$PhoneNumber" | cut -d"|" -f1`
			getPosition;
			replaceDatafile;
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
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f $RUBY_UNIT_HOME/Scripts/Info.awk $RUBY_UNIT_HOME/data/IPDR.txt > $RUBY_UNIT_HOME/data/IPDR.txt.$$
	mv $RUBY_UNIT_HOME/data/IPDR.txt.$$ $RUBY_UNIT_HOME/data/IPDR.txt
}

multipleRecords()
{
	i=1
	while [ $i -le $norecords ]
	do
		cat $RUBY_UNIT_HOME/data/IPDR.txt >> $RUBY_UNIT_HOME/data/IPDR.txt.new
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

generateIPDR()

{

getDayMonthYear;
j=1
while [ $j -le $norecords ]
do

	head -$j $RUBY_UNIT_HOME/data/IPDR.txt.new | tail -1 >  $RUBY_UNIT_HOME/tmpfile1
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
	echo $FinalTimeStamp
	sed -e "s/%timestamp/$FinalTimeStamp/g" $RUBY_UNIT_HOME/tmpfile1 | sed 's/-/\//g' >> $RUBY_UNIT_HOME/databackup/IPDR.txt.$$
	j=`expr $j + 1`
done
return 0

}

addHeader()
{

`echo Recordtype:83 > $RUBY_UNIT_HOME/databackup/IPDRWithHeader.txt.$$`
cat $RUBY_UNIT_HOME/databackup/IPDR.txt.$$ >> $RUBY_UNIT_HOME/databackup/IPDRWithHeader.txt.$$

}

processIPDR()
{

cp $RUBY_UNIT_HOME/databackup/IPDRWithHeader.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/IPDR.txt.$$
cp $RUBY_UNIT_HOME/databackup/IPDRWithHeader.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/success/IPDR.txt.$$
mv $RUBY_UNIT_HOME/databackup/IPDRWithHeader.txt.$$ $RUBY_UNIT_HOME/database/IPDR.txt.$$

return 0

}
waitforfiletopickup()
{
counter=0
while [ $counter -le 200 ]
  do
        if !(test -f $RANGERHOME/FMSData/DataRecord/Process/IPDR.txt.$$)
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
generateIPDR;
addHeader;
processIPDR;
waitforfiletopickup;
}

main 
