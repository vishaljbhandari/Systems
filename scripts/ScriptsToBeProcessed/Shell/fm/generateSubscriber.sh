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
. $RUBY_UNIT_HOME/Scripts/WaitForReloadToComplete.sh
. $RUBY_UNIT_HOME/Scripts/configuration.sh

echo $* > $RUBY_UNIT_HOME/Scripts/test
array=()
counter=0
loop_counter=`expr $# - 1`
for i in $*
	do 
		array[$counter]=$1
		shift 1
		counter=`expr $counter + 1`
	done

cleanUp()
{
rm $RUBY_UNIT_HOME/databackup/*.*
cp $RUBY_UNIT_HOME/Scripts/S01.txt $RUBY_UNIT_HOME/data/S01.txt
cp $RUBY_UNIT_HOME/data/S01.txt $RUBY_UNIT_HOME/databackup/ 

if test -f $RUBY_UNIT_HOME/subtmpfile
then
        rm $RUBY_UNIT_HOME/subtmpfile
        echo "tmpfile removed successfully"
fi

if test -f $RUBY_UNIT_HOME/tmpfile1
then
        rm $RUBY_UNIT_HOME/tmpfile1
        echo "tmpfile1 removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/ds_field_config_for_subscriber.dat
then
        rm $RUBY_UNIT_HOME/data/ds_field_config_for_subscriber.dat
        echo "ds_field_config.dat file removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/Final_field_config_for_subscriber.dat
then
        rm $RUBY_UNIT_HOME/data/Final_field_config_for_subscriber.dat
        echo "Final_field_config.dat file removed successfully"
fi

return 0
}

getdsfieldconfig()
{

$DB_QUERY << EOF | $GREP_QUERY

set termout off
set heading off
set echo off
spool $WATIR_SERVER_HOME/data/ds_field_config_for_subscriber.dat
select name || '|' || position from field_configs where record_config_id =3;

quit
 
EOF

sed -e "s/IMSI\/ESN/IMSI/g" $RUBY_UNIT_HOME/data/ds_field_config_for_subscriber.dat  >  $RUBY_UNIT_HOME/data/Final_field_config_for_subscriber.dat

cat $RUBY_UNIT_HOME/data/Final_field_config_for_subscriber.dat > $RUBY_UNIT_HOME/subtmpfile
awk 'NF > 0' $RUBY_UNIT_HOME/subtmpfile > $RUBY_UNIT_HOME/data/ds_field_config_for_subscriber.dat
cat $RUBY_UNIT_HOME/data/ds_field_config_for_subscriber.dat | tr -d " " > $RUBY_UNIT_HOME/subtmpfile

}

generateOneRecord()
{
while [ $loop_counter -ge 0 ]
	do 
		fieldname=`echo ${array[$loop_counter]}| awk 'BEGIN { FS = "=" } { print $1} '`
		fieldvalue=`echo ${array[$loop_counter]}| awk 'BEGIN { FS = "=" } { print $2} '`
		getPosition;
		replaceDatafile;
		loop_counter=`expr $loop_counter - 1`
	done
}

getPosition()

{

fieldposition=`awk 'BEGIN { FS = "|" } /'"^${fieldname}\|"'/ { print $2 }' "$RUBY_UNIT_HOME/subtmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F '|' -f $RUBY_UNIT_HOME/Scripts/SubInfo.awk $RUBY_UNIT_HOME/data/S01.txt > $RUBY_UNIT_HOME/data/S01.txt.$$
	mv $RUBY_UNIT_HOME/data/S01.txt.$$ $RUBY_UNIT_HOME/data/S01.txt

}

addHeader()
{

`echo Recordtype:3 > $RUBY_UNIT_HOME/databackup/SubscriberRecordWithHeader.txt.$$`
cat $RUBY_UNIT_HOME/data/S01.txt >> $RUBY_UNIT_HOME/databackup/SubscriberRecordWithHeader.txt.$$

}

processSubscriber()
{
      cp $RUBY_UNIT_HOME/databackup/SubscriberRecordWithHeader.txt.$$ $RANGERHOME/FMSData/SubscriberDataRecord/Process/SubscriberRecord.txt.$$
      cp $RUBY_UNIT_HOME/databackup/SubscriberRecordWithHeader.txt.$$ $RANGERHOME/FMSData/SubscriberDataRecord/Process/success/SubscriberRecord.txt.$$
      mv $RUBY_UNIT_HOME/databackup/SubscriberRecordWithHeader.txt.$$ $RUBY_UNIT_HOME/database/SubscriberRecord.txt.$$ 
}

waitforfiletopickup()
{

counter=0
while [ $counter -le 200 ]
  do
   	if !(test -f $RANGERHOME/FMSData/SubscriberDataRecord/Process/SubscriberRecord.txt.$$)
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
addHeader;
processSubscriber;
waitforfiletopickup; 
}

main 
