#!/bin/bash

#*******************************************************************************
# *  Copyright (c) SubexAzure Limited 2006. All rights reserved.               *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Azure Limited. The program(s) may be used and/or copied with the written   *
# *  permission from SubexAzure Limited or in accordance with the terms and    *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************
# Author - Sandeep Kumar Gupta

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

cp $RUBY_UNIT_HOME/Scripts/C01_CDM.txt $RUBY_UNIT_HOME/data/C01_CDM.txt
cp $RUBY_UNIT_HOME/data/C01_CDM.txt $RUBY_UNIT_HOME/databackup/ 

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

if test -f $RUBY_UNIT_HOME/data/Final_ds_field_config.dat
then
        rm $RUBY_UNIT_HOME/data/Final_ds_field_config.dat
        echo "Final_ds_field_config.dat file removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/subinfo.dat
then
        rm $RUBY_UNIT_HOME/data/subinfo.dat
        echo "subinfo.dat file removed successfully"
fi

rm -f $RUBY_UNIT_HOME/databackup/*.*
if [ $IS_RATING_ENABLED = "1" ]
then 
rm -f $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/*.* 
rm -f $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/success/*.* 
rm -f $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/*.*
rm -f $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/success/*.*
fi
return 0
}

getdsfieldconfig()
{

sqlplus -s $DB_USER/$DB_PASSWORD << EOF

set termout off
set heading off
set echo off
spool $RUBY_UNIT_HOME/data/ds_field_config.dat
select name || '=>' || position from field_configs where record_config_id=1 and position>0;
spool $RUBY_UNIT_HOME/data/subinfo.dat
select fieldutil.normalize(phone_number) || '|' || network_id || '|' || fieldutil.normalize(IMEI) || '|' || fieldutil.normalize(IMSI) || '|'  from subscriber where status in (1,2) and subscriber_type=0;

quit
 
EOF


sed -e "s/IMSI\/ESN Number/IMSI/g" $RUBY_UNIT_HOME/data/ds_field_config.dat > $RUBY_UNIT_HOME/data/ds_field_config_tmp.dat
sed -e "s/Value (INR)/Value/g" $RUBY_UNIT_HOME/data/ds_field_config_tmp.dat > $RUBY_UNIT_HOME/data/Final_ds_field_config.dat

cat $RUBY_UNIT_HOME/data/Final_ds_field_config.dat > $RUBY_UNIT_HOME/tmpfile
awk 'NF > 0' $RUBY_UNIT_HOME/tmpfile > $RUBY_UNIT_HOME/data/ds_field_config.dat
cat $RUBY_UNIT_HOME/data/ds_field_config.dat | tr -d " " > $RUBY_UNIT_HOME/tmpfile

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

fieldposition=`awk 'BEGIN { FS = ">" } /'"${fieldname}="'/ { print $2 }' "$RUBY_UNIT_HOME/tmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f $RUBY_UNIT_HOME/Scripts/Info.awk $RUBY_UNIT_HOME/data/C01_CDM.txt > $RUBY_UNIT_HOME/data/C01_CDM.txt.$$
	mv $RUBY_UNIT_HOME/data/C01_CDM.txt.$$ $RUBY_UNIT_HOME/data/C01_CDM.txt
}

multipleRecords()
{
	i=1
	while [ $i -le $norecords ]
	do
		cat $RUBY_UNIT_HOME/data/C01_CDM.txt >> $RUBY_UNIT_HOME/data/C01_CDM.txt.new
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

generateCDR()

{

getDayMonthYear;
j=1
while [ $j -le $norecords ]
do

	head -$j $RUBY_UNIT_HOME/data/C01_CDM.txt.new | tail -1 >  $RUBY_UNIT_HOME/tmpfile1
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
	sed -e "s/%timestamp/$FinalTimeStamp/g" $RUBY_UNIT_HOME/tmpfile1 | sed 's/-/\//g' >> $RUBY_UNIT_HOME/databackup/CDRRecord.txt.$$
	j=`expr $j + 1`
done
return 0

}

addHeader()
{

`echo Recordtype:1 > $RUBY_UNIT_HOME/databackup/CDRRecordWithHeader.txt.$$`
cat $RUBY_UNIT_HOME/databackup/CDRRecord.txt.$$ >> $RUBY_UNIT_HOME/databackup/CDRRecordWithHeader.txt.$$

}

processCDR()
{
	sleep 1
if [ $IS_RATING_ENABLED = "1" ]
then 	
echo $RANGERHOME/FMSData/$CDR_DATASOURCE_INPUTDIR/CDRRecord.txt.$$
cp $RUBY_UNIT_HOME/databackup/CDRRecord.txt.$$ $RANGERHOME/FMSData/$CDR_DATASOURCE_INPUTDIR/CDRRecord.txt.$$
while [ ! -f  $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/CDRRecord.txt.$$ ]
	do
		sleep 2
	done 
for i in {1..3}
	do
	head -1 $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/CDRRecord.txt.$$ > $RUBY_UNIT_HOME/databackup/tmp1.txt
	tail -1 $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/CDRRecord.txt.$$ > $RUBY_UNIT_HOME/databackup/tmp2.txt
	sed "s/$/,/g" $RUBY_UNIT_HOME/databackup/tmp2.txt > $RUBY_UNIT_HOME/databackup/tmp3.txt 
	cat $RUBY_UNIT_HOME/databackup/tmp1.txt $RUBY_UNIT_HOME/databackup/tmp3.txt > $RUBY_UNIT_HOME/databackup/new.txt
	cp $RUBY_UNIT_HOME/databackup/new.txt $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/CDRRecord.txt.$$
	done
cp $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/CDRRecord.txt.$$ $RANGERHOME/FMSData/$RATER_CDR_INPUTDIR/success/ 
while [ ! -f  $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$ ]
	do sleep 2
	done
	cat $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$ > temp1.txt
 
for i in {1..3}
	do
	cat $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$  
	head -1  $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$ > $RUBY_UNIT_HOME/databackup/file1.txt
	tail -1  $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$ > $RUBY_UNIT_HOME/databackup/file2.txt
	sed 's/,$//' $RUBY_UNIT_HOME/databackup/file2.txt > $RUBY_UNIT_HOME/databackup/file3.txt
	cat $RUBY_UNIT_HOME/databackup/file1.txt $RUBY_UNIT_HOME/databackup/file3.txt > $RUBY_UNIT_HOME/databackup/filenew.txt
	cp $RUBY_UNIT_HOME/databackup/filenew.txt $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$
	done
cp $RANGERHOME/FMSData/$RATER_CDR_OUTPUTDIR/CDRRecord.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/success/
fi
return 0

}

waitforfiletopickup()
{

counter=0
echo "Waiting for file pick up ...."

while [ $counter -le 1000 ]
  do
        if !(test -f $RANGERHOME/FMSData/$CDR_DATASOURCE_INPUTDIR/CDRRecord.txt.$$)
        then
			echo "File Got picked."
            break
        else
            sleep 0.1
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
generateCDR;
processCDR;
#waitforfiletopickup;
}

main 
