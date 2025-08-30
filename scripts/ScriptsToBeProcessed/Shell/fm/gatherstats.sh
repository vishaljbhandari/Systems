> ioreport.csv
> ParsingIoStatics.csv
> LoadingIoStatics.csv

get_io_info()
{	
	sqlplus -s lalithk/lalithk            << EOF >> ioreport.csv
	set heading off ;
	set linesize 200 ;
	set feedback off ;
	select '$activity_type'||','||'$start_time'||','||'$end_time'||','||'$time_taken'||','||TRIM(TO_CHAR(AVG(READS_PER_SECOND),'999999999.99'))||','||TRIM(TO_CHAR(AVG(WRITE_PER_SECOND),'999999999.99'))||','||TRIM(TO_CHAR(AVG(KREADS_PER_SECOND),'999999999.99'))||','||TRIM(TO_CHAR(AVG(KWRITES_PER_SECOND),'999999999.99'))||','||TRIM(TO_CHAR(AVG(BUSY),'999999999.99999'))||','||DEVICE from temp_iops where time_stamp between to_date('$start_time','yyyy-mm-dd hh24:mi:ss') and to_date('$end_time','yyyy-mm-dd hh24:mi:ss') group by DEVICE ;

EOF
}


cat taskinformation.log | while read line
do
	echo $line | grep "Task started at:" > useless.log
	if [ $? -eq 0 ]
	then
		time_part=`echo $line | grep "Task started at:" | cut -d: -f2 | tr -s 'T' ' '`
		min_part=`echo $line | grep "Task started at:" | cut -d: -f3`
		sec_part=`echo $line | grep "Task started at:" | cut -d: -f4 | cut -d. -f1`
		start_time="$time_part:$min_part:$sec_part"
#		echo $start_time
		continue
	fi

	echo $line | grep "Bulk loading file" > useless.log
	if [ $? -eq 0 ]
	then
		activity_type="BulkLoading"
		continue
	fi
	echo $line | grep "Parsing data file" > useless.log
	if [ $? -eq 0 ]
	then
		activity_type="FileParsing"
		continue
	fi

	echo $line | grep "Task completed at" > useless.log
	if [ $? -eq 0 ]
	then
		time_part=`echo $line | grep "Task completed at" | cut -d: -f2 | tr -s 'T' ' '`
		min_part=`echo $line | grep "Task completed at" | cut -d: -f3`
		sec_part=`echo $line | grep "Task completed at" | cut -d: -f4 | cut -d. -f1`
		end_time="$time_part:$min_part:$sec_part"
		continue
	fi
	echo $line | grep "Task completed in:" > useless.log
	if [ $? -eq 0 ]
	then
		time_taken=`echo $line | cut -d: -f2 `
		echo $time_taken
		get_io_info
		continue
	fi
done


grep "^FileParsing" ioreport.csv > ParsingIoStatics.csv
grep "^BulkLoading" ioreport.csv > LoadingIoStatics.csv
