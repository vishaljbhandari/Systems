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
arguments=`echo $* | cut -d' ' -f1-$#`
echo $arguments

cleanUp()
{
rm $RUBY_UNIT_HOME/databackup/*.*
cp $RUBY_UNIT_HOME/Scripts/InternalS01.txt $RUBY_UNIT_HOME/data/InternalS01.txt
cp $RUBY_UNIT_HOME/data/InternalS01.txt $RUBY_UNIT_HOME/databackup/ 

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

if test -f $RUBY_UNIT_HOME/data/ds_field_config_for_internal_subscriber.dat
then
        rm $RUBY_UNIT_HOME/data/ds_field_config_for_internal_subscriber.dat
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
spool $RUBY_UNIT_HOME/data/ds_field_config_for_internal_subscriber.dat
select name || '|' || position from field_configs where record_config_id =70 and position > 0;
quit
 
EOF

cat $RUBY_UNIT_HOME/data/ds_field_config_for_internal_subscriber.dat > $RUBY_UNIT_HOME/subtmpfile
awk 'NF > 0' $RUBY_UNIT_HOME/subtmpfile > $RUBY_UNIT_HOME/data/ds_field_config_for_internal_subscriber.dat
cat $RUBY_UNIT_HOME/data/ds_field_config_for_internal_subscriber.dat | tr -d " " > $RUBY_UNIT_HOME/subtmpfile

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

fieldposition=`awk 'BEGIN { FS = "|" } /'"^${fieldname}\|"'/ { print $2 }' "$RUBY_UNIT_HOME/subtmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f $RUBY_UNIT_HOME/Scripts/InternalSubInfo.awk $RUBY_UNIT_HOME/data/InternalS01.txt > $RUBY_UNIT_HOME/data/InternalS01.txt.$$
	mv $RUBY_UNIT_HOME/data/InternalS01.txt.$$ $RUBY_UNIT_HOME/data/InternalS01.txt

}

addHeader()
{

`echo Recordtype:70 > $RUBY_UNIT_HOME/databackup/InternalSubscriberRecord.txt.$$`
cat $RUBY_UNIT_HOME/data/InternalS01.txt >> $RUBY_UNIT_HOME/databackup/InternalSubscriberRecord.txt.$$

}

processInternalSubscriber()
{
      cp $RUBY_UNIT_HOME/databackup/InternalSubscriberRecord.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/InternalSubscriberRecord.txt.$$
      cp $RUBY_UNIT_HOME/databackup/InternalSubscriberRecord.txt.$$ $RANGERHOME/FMSData/DataRecord/Process/success/InternalSubscriberRecord.txt.$$
      mv $RUBY_UNIT_HOME/databackup/InternalSubscriberRecord.txt.$$ $RUBY_UNIT_HOME/database/InternalSubscriberRecord.txt.$$ 
      
return 0

}

waitforfiletopickup()
{

counter=0
while [ $counter -le 200 ]
  do
        if !(test -f $RANGERHOME/FMSData/DataRecord/Process/InternalSubscriberRecord.txt.$$)
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
processInternalSubscriber; 
waitforfiletopickup;
}

main 
