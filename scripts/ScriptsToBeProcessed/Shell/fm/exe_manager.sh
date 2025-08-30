# ! /bin/bash

####################################################################################
### To start dusmscdrds, duEDCHds, dugprsds, dummscdrds, ducdrds, dusubscriberds ###
####################################################################################

. /nikira/.bashrc-ranger
cd /nikira/subex_working_area/temp
sqlplus -s $DB_USER/$DB_PASSWORD << EOF > /dev/null 
whenever sqlerror exit 5 ;
set heading off
set wrap off
set newpage none
set termout off
set trimspool on
spool du_data_adapter.dat;
select distinct NAME from RIP_DATASOURCE_INFO;
spool off;
/
EOF
grep -v "SQL" du_data_adapter.dat |grep -v "row" > du_data_adapter.txt
for adapter in `cat du_data_adapter.txt`
do
cd $REVMAXHOME/sbin
ps -ef |grep "dsprocessor $adapter" |grep -v grep
if [ $? = 1 ]
then
echo " Starting $adapter DS `date`"
nohup ./dsprocessor $adapter &
fi
done


####################################################################
### To start Rater DS - Voice, SMS, EDCH, GPRS and MMS  ###
####################################################################

. /nikira/.bashrc-ranger
ps -ef |grep "duraterds -R SMS" |grep -v "grep"
if [ $? = 1 ]
then
cd /nikira/RangerRoot/sbin/
nohup ./duraterds -R SMS &
echo " Starting SMS Rater `date`"
fi

ps -ef |grep "duraterds -R VOICE" |grep -v "grep"
if [ $? = 1 ]
then
cd /nikira/RangerRoot/sbin/
nohup ./duraterds -R VOICE &
echo " Starting VOICE Rater `date`"
fi

ps -ef |grep "duraterds -R EDCH" |grep -v "grep"
if [ $? = 1 ]
then
cd /nikira/RangerRoot/sbin/
nohup ./duraterds -R EDCH &
echo " Starting EDCH Rater `date`"
fi

ps -ef |grep "dugprsandmmsraterds -R GPRS" |grep -v "grep"
if [ $? = 1 ]
then
cd /nikira/RangerRoot/sbin/
nohup ./dugprsandmmsraterds -R GPRS &
echo " Starting GPRS Rater `date`"
fi

ps -ef |grep "dugprsandmmsraterds -R MMS" |grep -v "grep"
if [ $? = 1 ]
then
cd /nikira/RangerRoot/sbin/
nohup ./dugprsandmmsraterds -R MMS &
echo " Starting MMS Rater `date`"
fi

