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

arguments=`echo $* | cut -d' ' -f1-$#`
echo $arguments

cleanUp()
{
rm $RUBY_UNIT_HOME/databackup/*.*
cp $RUBY_UNIT_HOME/Scripts/A01.txt $RUBY_UNIT_HOME/data/A01.txt
cp $RUBY_UNIT_HOME/data/A01.txt $RUBY_UNIT_HOME/databackup/ 

if test -f $RUBY_UNIT_HOME/accounttmpfile
then
        rm $RUBY_UNIT_HOME/accounttmpfile
        echo "tmpfile removed successfully"
fi

if test -f $RUBY_UNIT_HOME/tmpfile1
then
        rm $RUBY_UNIT_HOME/tmpfile1
        echo "tmpfile1 removed successfully"
fi

if test -f $RUBY_UNIT_HOME/data/ds_field_config_for_account.dat
then
        rm $RUBY_UNIT_HOME/data/ds_field_config_for_account.dat
        echo "ds_field_config.dat file removed successfully"
fi

return 0
}

getdsfieldconfig()
{

$DB_QUERY << EOF | $GREP_QUERY

set termout off
set heading off
set echo off
spool $RUBY_UNIT_HOME/data/ds_field_config.dat
select name || '=>' || position from field_configs where record_config_id=1;
spool $RUBY_UNIT_HOME/data/subinfo.dat
select ID || '|' || fieldutil.normalize(phone_number) || '|' || network_id || '|' || fieldutil.normalize(IMEI) || '|' || fieldutil.normalize(IMSI) || '|'  from subscriber where status in (1,2) and subscriber_type=0;

quit
 
EOF



cat $RUBY_UNIT_HOME/data/ds_field_config_for_account.dat > $RUBY_UNIT_HOME/accounttmpfile
awk 'NF > 0' $RUBY_UNIT_HOME/accounttmpfile > $RUBY_UNIT_HOME/data/ds_field_config_for_account.dat
cat $RUBY_UNIT_HOME/data/ds_field_config_for_account.dat | tr -d " " > $RUBY_UNIT_HOME/accounttmpfile

}

generateOneRecord()
{
	for i in $arguments
	do
		fieldname=`echo $i | awk 'BEGIN { FS = "=" } { print $1} '`
		fieldvalue=`echo $i | awk 'BEGIN { FS = "=" } { print $2} '`
		getPosition;
		replaceDatafile;
	done
}

getPosition()

{

fieldposition=`awk 'BEGIN { FS = "|" } /'"^${fieldname}\|"'/ { print $2 }' "$RUBY_UNIT_HOME/accounttmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f $RUBY_UNIT_HOME/Scripts/AccInfo.awk $RUBY_UNIT_HOME/data/A01.txt > $RUBY_UNIT_HOME/data/A01.txt.$$
	mv $RUBY_UNIT_HOME/data/A01.txt.$$ $RUBY_UNIT_HOME/data/A01.txt
	cat $RUBY_UNIT_HOME/data/A01.txt

}

addHeader()
{

`echo Recordtype:4 > $RUBY_UNIT_HOME/databackup/AccountRecordWithHeader.txt.$$`
cat $RUBY_UNIT_HOME/data/A01.txt >> $RUBY_UNIT_HOME/databackup/AccountRecordWithHeader.txt.$$

}

processAccount()
{
      cp $RUBY_UNIT_HOME/databackup/AccountRecordWithHeader.txt.$$ $RANGERHOME/FMSData/SubscriberDataRecord/Process/AccountRecordWithHeader.txt.$$

      cp $RUBY_UNIT_HOME/databackup/AccountRecordWithHeader.txt.$$ $RANGERHOME/FMSData/SubscriberDataRecord/Process/success/AccountRecordWithHeader.txt.$$

      mv $RUBY_UNIT_HOME/databackup/AccountRecordWithHeader.txt.$$ $RUBY_UNIT_HOME/database/AccountRecordWithHeader.txt.$$ 
   return 0

}

waitforfiletopickup()
{

counter=0
while [ $counter -le 200 ]
  do
        if !(test -f $RANGERHOME/FMSData/SubscriberDataRecord/Process/AccountRecordWithHeader.txt.$$)
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
processAccount; 
waitforfiletopickup;
}

main 
