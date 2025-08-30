
> accumulatedcpu.log
for FILE in `ls -rt sar*`
do
    sar -f $FILE | while read line
    do
		echo "$FILE|$line" | grep -v "SunOS derby" | grep -v "Average" |  tr -s " " "|" >> accumulatedcpu.log
    done
done
