
if [ -z "$VAR" ]; then
    echo "msg"
    exit 1
fi

if [ $# -lt 1 ]
then
        echo "Usage: scriptlauncher $0 SampleDataFile "
        exit 2
fi

SampleData=$1

if [ ! -f $SampleData ]
then
        echo "File Does not Exist: $SampleData"
        exit 3
else
        sed '/^[ \t]*$/d' < $SampleData > $SampleData.tmp
        mv $SampleData.tmp  $SampleData
        echo "$SampleData" | grep "\." > /dev/null 2>&1
        if [ $? -ne 0 ]
        then
                mv $SampleData $SampleData.dat
        fi
fi

countoffilerecordswithheader=`cat $SampleData | wc -l`
countoffilerecords=`expr $countoffilerecordswithheader - 1`
if [ $countoffilerecords -eq 0 ]
then
    echo "Data File is empty"
    exit 4
fi

sqlplus -s /nolog << eof
CONNECT_TO_SQL

set serveroutput off;
set feedback off;
set lines 120;
set colsep ,
set pages 0;
set heading off;
spool filename.tmp

any select query

spool off;
quit;
eof

sed '/^[ \t]*$/d' < filename.tmp > filename.tmp.tmp
mv filename.tmp.tmp filename.tmp

countofdbrecords=`cat filename.tmp`
if [ $countofdbrecords -eq 0 ]
then
    echo "DB Table Empty, Loading Failed"
    exit 5
fi

if [ $countofdbrecords -ne $countoffilerecords ]
then
    echo "All Data records are not loaded to database"
    exit 6
fi
rm vpmnloader.tmp
echo "Verification Successfull..."
