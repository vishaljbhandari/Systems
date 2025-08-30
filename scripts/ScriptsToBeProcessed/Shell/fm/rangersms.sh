smsval=`echo $1 | sed 's/,/ /g'`
for i in $smsval
do 
echo $i '|' `cat $2` >> $RANGERHOME/smsnotification.txt
done

rm -f $2
