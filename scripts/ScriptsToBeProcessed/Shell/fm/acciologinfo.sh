
> accumulated.log
for FILE in `ls -rt iostat*`
do
	cat $FILE | while read line
	do
		echo $line | grep "extended device statistics" 
		if [ $? -eq 0 ]
		then
			continue
		fi

		echo $line | grep "Mar" 
		if [ $? -eq 0 ]
		then
			start_date=$line
			continue
		fi

#		echo "$start_date|$line" | tr -s " " "|"
		temp_var=`echo "$line" | tr -s " " "|"`
		echo "$start_date|$temp_var" >> accumulated.log
	done
done
