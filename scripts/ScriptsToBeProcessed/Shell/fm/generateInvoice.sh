#!/bin/bash

#*******************************************************************************
# *  Copyright (c) SubexAzure Limited 2006. All rights reserved.               *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Azure Limited. The program(s) may be used and/or copied with the written   *
# *  permission from SubexAzure Limited or in accordance with the terms and    *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************
# Author - Rohitesh

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


arg=`expr $# - 2`
lookback=`expr $# - 1`
arguments=`echo $* | cut -d' ' -f1-$arg`
norecords=`echo $* | cut -d' ' -f$# | awk 'BEGIN { FS = "=" } { print $2} '`
timestamp=`echo $* | cut -d' ' -f$lookback | awk 'BEGIN { FS = "=" } { print $2} '`
delimiter1=:
delimiter2=/
flag=0
databaseaccountid="AccountID"
databaseaccountname="AccountNumber"

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

cp $WATIR_SERVER_HOME/Scripts/Inv01.txt $WATIR_SERVER_HOME/data/Inv01.txt
cp $WATIR_SERVER_HOME/data/Inv01.txt $WATIR_SERVER_HOME/databackup/ 

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

if test -f $WATIR_SERVER_HOME/data/ds_field_config_invoice.dat
then
        rm $WATIR_SERVER_HOME/data/ds_field_config_invoice.dat
        echo "ds_field_config_invoice.dat file removed successfully"
fi

if test -f $WATIR_SERVER_HOME/data/Final_ds_field_config_invoice.dat
then
        rm $WATIR_SERVER_HOME/data/Final_ds_field_config_invoice.dat
        echo "Final_ds_field_config_invoice.dat file removed successfully"
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

sqlplus -s $PREVEA_DB_SETUP_USER/$PREVEA_DB_SETUP_USER << EOF
set termout off
set heading off
set echo off
spool $WATIR_SERVER_HOME/data/ds_field_config_invoice.dat
select name || '=>' || INPUT_FIELD_POSITION from field_configurations where TABLE_NAME='invoices';
spool $WATIR_SERVER_HOME/data/subinfo.dat
select ACCOUNT_ID||'|'||ACCOUNT_NAME||'|'||PHONE_NUMBER from ACCOUNT_PHONE_NUMBERS where status in (1,2) and subscriber_type=0;
quit
 
EOF

cat $WATIR_SERVER_HOME/data/ds_field_config_invoice.dat > $WATIR_SERVER_HOME/tmpfile
awk 'NF > 0' $WATIR_SERVER_HOME/tmpfile > $WATIR_SERVER_HOME/data/ds_field_config_invoice.dat
cat $WATIR_SERVER_HOME/data/ds_field_config_invoice.dat | tr -d " " > $WATIR_SERVER_HOME/tmpfile

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
	           if [ "$fieldname" = "InvoiceDate" ] || [ "$fieldname" = "DueDate" ]
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
			        AccountID)  databaseaccountid="";;
			        AccountNumber)      databaseaccountname="";;
			esac
		done
		databasearguments=`echo $databaseaccountid $databaseaccountname`
		k=1	
		if test "$databasearguments"
		then
			for i in $databasearguments
			do
			   fieldname=$i
			   fieldvalue=`cat $WATIR_SERVER_HOME/data/subinfo.dat | grep "$PhoneNumber" | cut -d"|" -f$k`
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

fieldposition=`awk 'BEGIN { FS = ">" } /'"^${fieldname}="'/ { print $2 }' "$WATIR_SERVER_HOME/tmpfile"`

}

replaceDatafile()
{
	echo "P -FP - *$fieldposition*"
	echo "P -FV - *$fieldvalue*"
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F '|' -f $WATIR_SERVER_HOME/Scripts/Info2.awk $WATIR_SERVER_HOME/data/Inv01.txt > $WATIR_SERVER_HOME/data/invoice.txt.$$
	mv $WATIR_SERVER_HOME/data/invoice.txt.$$ $WATIR_SERVER_HOME/data/Inv01.txt
}

multipleRecords()
{
	i=1
	while [ $i -le $norecords ]
	do
		cat $WATIR_SERVER_HOME/data/Inv01.txt >> $WATIR_SERVER_HOME/data/Inv01.txt.new
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

generateinvoice()

{

getDayMonthYear;
j=1
while [ $j -le $norecords ]
do

	head -$j $WATIR_SERVER_HOME/data/Inv01.txt.new | tail -1 >  $WATIR_SERVER_HOME/tmpfile1
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
	sed -e "s#%timestamp#$FinalTimeStamp#g" $WATIR_SERVER_HOME/tmpfile1 | sed 's/-/\//g' >> $WATIR_SERVER_HOME/databackup/InvoiceRecord.txt.$$
	j=`expr $j + 1`
done
return 0

}

processinvoice()
{

cp $WATIR_SERVER_HOME/databackup/InvoiceRecord.txt.$$ $PREVEAHOME/data/invoice/InvoiceRecord.txt.$$

cp $WATIR_SERVER_HOME/databackup/InvoiceRecord.txt.$$ $PREVEAHOME/data/invoice/success/InvoiceRecord.txt.$$

mv $WATIR_SERVER_HOME/databackup/InvoiceRecord.txt.$$ $WATIR_SERVER_HOME/database/InvoiceRecord.txt.$$

return 0

}

waitforfiletopickup()
{

counter=0
while [ $counter -le 200 ]
  do
        if !(test -f $PREVEAHOME/data/invoice/InvoiceRecord.txt.$$)
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
generateinvoice;
processinvoice;
waitforfiletopickup;
}

main 
