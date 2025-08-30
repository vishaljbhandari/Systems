#!/bin/bash
. ~/.bashrc


. $RUBY_UNIT_HOME/Scripts/configuration.sh
. $RANGERHOME/sbin/rangerenv.sh

rm $RUBY_UNIT_HOME/data/*01.txt*

cd $RUBY_UNIT_HOME/data

UNAME=`uname`

for i in `ls` 
do
	case $UNAME in
		SunOS ) dos2unix -437 $RUBY_UNIT_HOME/data/$i $RUBY_UNIT_HOME/data/$i ;;
		Linux ) dos2unix $RUBY_UNIT_HOME/data/$i ;;
		HP-UX ) dos2ux $RUBY_UNIT_HOME/data/$i $RUBY_UNIT_HOME/data/$i ;;
		AIX   ) perl -pi -e 's/\r\n/\n/g' *.* ;;
    esac
	echo "dos2unix: converting file ${i} to UNIX format..."
done

#dos2unix *.txt
cd -

RecordType="`head -1 $RUBY_UNIT_HOME/data/*.txt | cut -d ':' -f2`"
# echo "$RecordType"
if [ "$RecordType" == "3" -o "$RecordType" == "4" ]
then
    # echo "Sub"
    cp $RUBY_UNIT_HOME/data/*.txt $RANGERHOME/FMSData/SubscriberDataRecord/Process/
    cp $RUBY_UNIT_HOME/data/*.txt $RANGERHOME/FMSData/SubscriberDataRecord/Process/success/
elif [ "$RecordType" == "160" ]
then
 echo "Order"
    cp $RUBY_UNIT_HOME/data/*.txt $RANGERHOME/FMSData/InlineDataRecord/Process
    cp $RUBY_UNIT_HOME/data/*.txt $RANGERHOME/FMSData/InlineDataRecord/Process/success/
else
    # echo "CDR"
    cp $RUBY_UNIT_HOME/data/*.txt $RANGERHOME/FMSData/DataRecord/Process/
    cp $RUBY_UNIT_HOME/data/*.txt $RANGERHOME/FMSData/DataRecord/Process/success/
fi

mv $RUBY_UNIT_HOME/data/*.txt $RUBY_UNIT_HOME/database/

