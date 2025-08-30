
> accumulatemem.log
for FILE in `ls -rt mpsta*`
do
	line=`cat $FILE | grep "Memory"`
	echo "$FILE|$line" >> accumulatemem.log
done
