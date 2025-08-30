#!/bin/bash

#*******************************************************************************
# *  Copyright (c) Subex Limited 2008. All rights reserved.                    *
# *  The copyright to the computer program(s) herein is the property of Subex  *
# *  Limited. The program(s) may be used and/or copied with the written        *
# *  permission from Subex Limited or in accordance with the terms and         *
# *  conditions stipulated in the agreement/contract under which the program(s)*
# *  have been supplied.                                                       *
# ******************************************************************************
# Author - 

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

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

rm -f $WATIR_SERVER_HOME/data/*

cp $WATIR_SERVER_HOME/Scripts/IPDR_CDM.txt $WATIR_SERVER_HOME/data/IPDR_CDM.txt
cp $WATIR_SERVER_HOME/data/IPDR_CDM.txt $WATIR_SERVER_HOME/databackup/ 

if test -f $WATIR_SERVER_HOME/tmpfile
then
        rm $WATIR_SERVER_HOME/tmpfile
        echo "tmpfile removed successfully"
fi

if test -f $WATIR_SERVER_HOME/tmpfile1
then
        rm $WATIR_SERVER_HOME/tmpfile1
        echo "tmpfile1 removed successfully"
fi

if test -f $WATIR_SERVER_HOME/data/ds_field_config.dat
then
        rm $WATIR_SERVER_HOME/data/ds_field_config.dat
        echo "ds_field_config.dat file removed successfully"
fi

if test -f $WATIR_SERVER_HOME/data/subinfo.dat
then
        rm $WATIR_SERVER_HOME/data/subinfo.dat
        echo "subinfo.dat file removed successfully"
fi

rm -f $WATIR_SERVER_HOME/databackup/*.*

return 0
}

getdsfieldconfig()
{

if [ $DB_TYPE == "1" ]
then

sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

set termout off
set heading off
set echo off
spool $WATIR_SERVER_HOME/data/ds_field_config.dat
select name || '=>' || position from field_configs where record_config_id=83;
spool $WATIR_SERVER_HOME/data/subinfo.dat
select fieldutil.normalize(phone_number) || '|' || network_id || '|' from subscriber where subscriber_type=0 and status = 1;

quit

EOF

else


clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
set termout off
set heading off
set echo off
spool $WATIR_SERVER_HOME/data/ds_field_config.dat
select name || '=>' || position from field_configs where record_config_id=83;
spool $WATIR_SERVER_HOME/data/subinfo.dat
select fieldutil.normalize(phone_number) || '|' || network_id || '|' from subscriber where subscriber_type=0 and status = 1;

quit

EOF

fi

sed -e "s/Value (INR)/Value/g" $WATIR_SERVER_HOME/data/ds_field_config.dat > $WATIR_SERVER_HOME/data/Final_ds_field_config.dat

cat $WATIR_SERVER_HOME/data/Final_ds_field_config.dat | tr -d " " > $WATIR_SERVER_HOME/tmpfile

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
		
				
}

getPosition()

{

fieldposition=`awk 'BEGIN { FS = ">" } /'^"${fieldname}="'/ { print $2 }' "$WATIR_SERVER_HOME/tmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f $WATIR_SERVER_HOME/Scripts/Info.awk $WATIR_SERVER_HOME/data/IPDR_CDM.txt > $WATIR_SERVER_HOME/data/IPDR_CDM.txt.$$
	mv $WATIR_SERVER_HOME/data/IPDR_CDM.txt.$$ $WATIR_SERVER_HOME/data/IPDR_CDM.txt
}

multipleRecords()
{
	i=1
	while [ $i -le $norecords ]
	do
		cat $WATIR_SERVER_HOME/data/IPDR_CDM.txt >> $WATIR_SERVER_HOME/data/IPDR_CDM.txt.new
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

	head -$j $WATIR_SERVER_HOME/data/IPDR_CDM.txt.new | tail -1 >  $WATIR_SERVER_HOME/tmpfile1
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
	sed -e "s/%timestamp/$FinalTimeStamp/g" $WATIR_SERVER_HOME/tmpfile1 | sed 's/-/\//g' >> $WATIR_SERVER_HOME/databackup/IPDR_CDM.txt.$$
	j=`expr $j + 1`
done
return 0

}

addHeader()
{

`echo Recordtype:83 > $WATIR_SERVER_HOME/databackup/IPDRWithHeader.txt.$$`
cat $WATIR_SERVER_HOME/databackup/IPDR_CDM.txt.$$ >> $WATIR_SERVER_HOME/databackup/IPDRWithHeader.txt.$$

}

processIPDR()
{

echo "#####################"
	sleep 1
	echo $COMMON_MOUNT_POINT/FMSData/$IPDR_DATASOURCE_INPUTDIR/IPDR_CDM.txt.$$
cp $WATIR_SERVER_HOME/databackup/IPDR_CDM.txt.$$ $COMMON_MOUNT_POINT/FMSData/$IPDR_DATASOURCE_INPUTDIR/IPDR_CDM.txt.$$

return 0

}
waitforfiletopickup()
{
counter=0
echo "Waiting for file pick up ...."
while [ $counter -le 1000 ]
  do
        if !(test -f $COMMON_MOUNT_POINT/FMSData/$IPDR_DATASOURCE_INPUTDIR/IPDR_CDM.txt.$$)
        then
			echo "File Got picked."
            break
         else
               sleep 1
               counter=`expr $counter + 1`
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

processIPDR;

}

main 
