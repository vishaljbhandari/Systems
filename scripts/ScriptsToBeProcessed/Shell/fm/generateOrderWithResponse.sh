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

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/WaitForReloadToComplete.sh
response=$1
echo $response
shift

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
rm $WATIR_SERVER_HOME/databackup/*.*
cp $WATIR_SERVER_HOME/Scripts/Or01.txt $WATIR_SERVER_HOME/data/Or01.txt
cp $WATIR_SERVER_HOME/data/Or01.txt $WATIR_SERVER_HOME/databackup/ 

if test -f $WATIR_SERVER_HOME/ordtmpfile
then
        rm $WATIR_SERVER_HOME/ordtmpfile
        echo "tmpfile removed successfully"
fi

if test -f $WATIR_SERVER_HOME/tmpfile1
then
        rm $WATIR_SERVER_HOME/tmpfile1
        echo "tmpfile1 removed successfully"
fi

if test -f $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat
then
        rm $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat
        echo "ds_field_config.dat file removed successfully"
fi

if test -f $WATIR_SERVER_HOME/data/Final_field_config_for_order.dat
then
        rm $WATIR_SERVER_HOME/data/Final_field_config_for_order.dat
        echo "Final_field_config.dat file removed successfully"
fi

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
spool $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat
select name || '|' || position from field_configs where record_config_id =160;
quit
 
EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
set termout off
set heading off
set echo off
spool $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat
select name || '|' || position from field_configs where record_config_id =160;
quit
 
EOF

fi


sed -e "s/IMSI\/ESN/IMSI/g" $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat  >  $WATIR_SERVER_HOME/data/Final_field_config_for_order.dat

cat $WATIR_SERVER_HOME/data/Final_field_config_for_order.dat > $WATIR_SERVER_HOME/ordtmpfile
awk 'NF > 0' $WATIR_SERVER_HOME/ordtmpfile > $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat
cat $WATIR_SERVER_HOME/data/ds_field_config_for_order.dat | tr -d " " > $WATIR_SERVER_HOME/ordtmpfile

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

fieldposition=`awk 'BEGIN { FS = "|" } /'"^${fieldname}\|"'/ { print $2 }' "$WATIR_SERVER_HOME/ordtmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F '|' -f $WATIR_SERVER_HOME/Scripts/SubInfo.awk $WATIR_SERVER_HOME/data/Or01.txt > $WATIR_SERVER_HOME/data/Or01.txt.$$
	mv $WATIR_SERVER_HOME/data/Or01.txt.$$ $WATIR_SERVER_HOME/data/Or01.txt

}

addHeader()
{

`echo Recordtype:160 > $WATIR_SERVER_HOME/databackup/OrderRecordWithHeader.txt.$$`
cat $WATIR_SERVER_HOME/data/Or01.txt >> $WATIR_SERVER_HOME/databackup/OrderRecordWithHeader.txt.$$

}

processSubscriber()
{
      cp $WATIR_SERVER_HOME/databackup/OrderRecordWithHeader.txt.$$ $COMMON_MOUNT_POINT/FMSData/InlineDataRecord/Process/InlineRecord.txt.$$
      cp $WATIR_SERVER_HOME/databackup/OrderRecordWithHeader.txt.$$ $COMMON_MOUNT_POINT/FMSData/InlineDataRecord/Process/success/InlineRecord.txt.$$
      mv $WATIR_SERVER_HOME/databackup/OrderRecordWithHeader.txt.$$ $WATIR_SERVER_HOME/database/InlineRecord.txt.$$ 
}

waitforfiletopickup()
{

counter=0
while [ $counter -le 200 ]
  do
   	if !( test -f $COMMON_MOUNT_POINT/FMSData/InlineDataRecord/Process/InlineRecord.txt.$$)
   	then
              break
       	 else
               sleep 0.1
       	       counter=`expr $counter + 1`
               echo "waiting for file pick up"
   	fi
  done
}

checkforresponse()
{
	responsefile=`cat $COMMON_MOUNT_POINT/FMSData/InlineDataRecord/Response/InlineRecord.txt.$$|cut -f5- -d " "|cut -c2-`
	echo "responsefile" $responsefile >> $WATIR_SERVER_HOME/Scripts/syeda123
	if [ "$response" == "$responsefile" ]
	then
		echo 'puts "Expected results matched"'|ruby
	else
		echo 'puts "Expected results are not matched"'|ruby
	fi
}

main()
{
cleanUp;
getdsfieldconfig;
generateOneRecord;
addHeader;
processSubscriber;
waitforfiletopickup;
checkforresponse; 
}

main 
