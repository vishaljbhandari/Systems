#!/bin/bash

export DB_TYPE=1                        # 1=Oracle, 2=DB2

if [ x$DB_USER == x ]
then
echo "Please set DB_USER environement variable"
exit 2
fi

if [ x$DB_PASSWORD == x ]
then
echo "Please set DB_PASSWORD environement variable"
exit 3
fi

if [ $# -le 4 ]
then
echo "Usage : ./generateTestData.sh 'Parameter1=Value1' 'Parameter2=Value2' ..... 'ParameterN=ValueN' 'OutPutFile=test.txt' 'DS=CDR' 'lookback=0' 'Records=N'"
exit 1
fi

if [ x$DataRecordsDir == x ]
then
DataRecordsDir=`pwd`
fi

b=$#
lookback=`expr $# - 1`
output1=`expr $# - 3`
ds=`expr $# - 2`
a=\"\${$ds}\"
c=\"\${$b}\"
output1=\"\${$output1}\"
norecords=`eval echo $c | awk 'BEGIN { FS = "=" } { print $2} '`
output=`eval echo $output1 | awk 'BEGIN { FS = "=" } { print $2} '`
output="$DataRecordsDir/$output"
DS=`eval echo $a | awk 'BEGIN { FS = "=" } { print $2} '`
timestamp=`echo "$@"|awk ' { print ( $(NF-1) ) }' | cut -d "=" -f2`
delimiter1=:
delimiter2=-
flag=0

cleanUp()
{
if test -f value.dat
then
        rm value.dat
fi

if test -f id.txt
then
        rm id.txt
fi

if test -f testfile.txt
then
        rm testfile.txt
fi


cp "$DS.txt" testfile.txt

if test -f testfile.txt.new
then
        rm testfile.txt.new
fi

if test -f tmpfile
then
        rm tmpfile
fi

if test -f tmpfile1
then
        rm tmpfile1
fi

if test -f ds_field_config.dat
then
        rm ds_field_config.dat
fi

if test -f Final_ds_field_config.dat
then
        rm Final_ds_field_config.dat
fi

return 0
}

getdsfieldconfig()
{
sqlplus -s $DB_USER/$DB_PASSWORD << EOF 2>&1> /dev/null
set feedback off
set termout off
set heading off
set echo off
spool ds_field_config.dat
select name || '=>' || position from field_configs where record_config_id=(select distinct(record_config_id) from record_view_configs where name='$DS') and position>0;
quit
EOF

cat ds_field_config.dat > tmpfile
awk 'NF > 0' tmpfile > ds_field_config.dat
cat ds_field_config.dat | tr -d " " > tmpfile

}

gettime()
{
if [ $DB_TYPE == "1" ]
then
sqlplus -s $DB_USER/$DB_PASSWORD << EOF 2>&1> /dev/null
set feedback off
set termout off
set heading off
set echo off
spool id.txt
select distinct(record_config_id) from record_view_configs where name='$DS';
quit
EOF
fi
id=`cat id.txt | sed 's/ //g' `
id=`echo $id | sed 's/ //g'`

if [ $DB_TYPE == "1" ]
then
sqlplus -s $DB_USER/$DB_PASSWORD << EOF 2>&1> /dev/null
set feedback off
set termout off
set heading off
set echo off
spool value.dat
select value from im_counter_thread_configs where config_key='UNDO_TO_BE_APPLIED_0_$id' and counter_instance_id=0;
quit
EOF
fi

value=`cat value.dat | sed 's/ //g' `
currentDate=`echo $value | sed 's/ //g'`
if [ $currentDate == "norowsselected" ]
then
currentDate=`date +%d/%m/%Y%H:%M:%S`
fi
#echo " date:$currentDate"
day=`echo $currentDate | cut -c1-2`
month=`echo $currentDate | cut -c4-5`
year=`echo $currentDate | cut -c7-10`
hour=`echo $currentDate | cut -c11-12`
minutes=`echo $currentDate | cut -c14-15`
seconds=`echo $currentDate | cut -c17-18`
ArrayForMonth=( 0 31 28 31 30 31 30 31 31 30 31 30 31 )

}

generateOneRecord()
{
		for i in "$@"
		do
		   fieldname=`echo $i | awk 'BEGIN { FS = "=" } { print $1} '`
		   fieldvalue=`echo $i | awk 'BEGIN { FS = "=" } { print $2} '`
	           if [ "$fieldname" = "TimeStamp" ] 		
                   then
			dt=`echo $fieldvalue | cut -c1-10`
			tm=`echo $fieldvalue | cut -c12-19`
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

fieldposition=`awk 'BEGIN { FS = ">" } /'"${fieldname}="'/ { print $2 }' "tmpfile"`

}

replaceDatafile()
{
	#echo "P -FP - *$fieldposition*"
	#echo "P -FV - *$fieldvalue*"
	if [ "x$fieldposition" != "x"  ]
	then
	awk -v POSITION="$fieldposition" -v VALUE="$fieldvalue" -F ',' -f Info.awk testfile.txt > testfile.txt.$$
	mv testfile.txt.$$ testfile.txt
	
	fi
}

multipleRecords()
{
	i=1
	while [ $i -le $norecords ]
	do
		cat testfile.txt >> testfile.txt.new
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

	head -$j testfile.txt.new | tail -1 >  tmpfile1
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
	sed -e "s/%timestamp/$FinalTimeStamp/g" tmpfile1 | sed 's/-/\//g' >> $output
	j=`expr $j + 1`
done
return 0
}


main()
{
cleanUp;
gettime;
getdsfieldconfig;
generateOneRecord "$@";
multipleRecords;
generateCDR;
}

main "$@"
echo "Generating data for datastream $DS and file $output"
