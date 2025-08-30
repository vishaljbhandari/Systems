
> accumulatedvm.log
for FILE in `ls vmstat*`
do
    cat $FILE | while read line
    do
		echo "$FILE|$line" | grep -v "memory" | grep -v "swap" |  tr -s " " "|" >> accumulatedvm.log
    done
done
