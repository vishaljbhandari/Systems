# !/bin/bash


on_exit ()
{
        rm -f $RANGERHOME/RangerData/Scheduler/WhiteListLoaderPID*
        exit;
}

trap on_exit EXIT


. $RANGERHOME/sbin/rangerenv.sh

cd $RANGER5_4HOME/RangerData/DataSourceSubscriberData/
#cd $RANGERHOME/RangerData/5_4_DS_DATA/Subscriber/Process/

sqlplus -s /nolog << EOF > /dev/null
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
set heading off
set wrap off
set newpage none
set termout off
set feedback off;
set trimspool on
set linesize 3000 ;

insert into du_white_deactive(PHONE_NUMBER,IMSI,ACCOUNT_ID)(select B.Phone_Number,B.IMSI,A.ACCOUNT_ID 
from du_white_sub A, Subscriber B where A.IMSI=B.IMSI and A.PHONE_NUMBER ! = B.PHONE_NUMBER and B.STATUS=1);

commit;

spool DU_WHITELIST_DEACTIVE.lst;

select '|'||ACCOUNT_ID||'|'||'||||||||||||||||'||'|'||PHONE_NUMBER||'|'||'d'||'|'||IMSI||'|'||'||'||'777'||'|||||||||||||||||||'
 from du_white_deactive;

spool off;
/
EOF

white_date=`date +"%m%d%H%M%S"`
mv DU_WHITELIST_DEACTIVE.lst DU_WHITELIST_DEACTIVE."$white_date"

cat DU_WHITELIST_DEACTIVE."$white_date" |grep "[0-9]" > /dev/null 2>&1

if [ $? = 0 ]
then 
touch $RANGER5_4HOME/RangerData/DataSourceSubscriberData/success/DU_WHITELIST_DEACTIVE."$white_date"
else
rm DU_WHITELIST_DEACTIVE."$white_date"
fi
##sleep 100

sqlplus -s /nolog << EOF > /dev/null
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
set heading off
set wrap off
set newpage none
set termout off
set feedback off;
set trimspool on
set linesize 3000 ;
spool DU_WHITELIST_NEW.lst;

select '|'||ACCOUNT_ID||'|'||FIRST_NAME||'|'||MIDDLE_NAME||'|'||LAST_NAME||'|'||ADDRESS_LINE1||'|'||
 ADDRESS_LINE2||'|'||ADDRESS_LINE3||'||||'||'|'||CONTACT_PHONE_NUMBER||'|'||'|'||'|'||GROUP_NAME
 ||'|'||'|'||'|'|| to_char(DATE_OF_ACTIVATION,'dd/mm/yyyy')||'|'||PHONE_NUMBER||'|'||STATUS||'|'||IMSI||'|'||'|'||11110110
 ||'|'||'777'||'|'||'GSM'||'|'||'|'||'|'||to_char(END_SERVICE_DATE,'dd/mm/yyyy')||'|'||'||||||||||||||' from du_white_sub;

spool off;

 delete from du_white_sub;
 delete from du_white_deactive;
commit;

/
EOF

white_date=`date +"%m%d%H%M%S"`
mv DU_WHITELIST_NEW.lst DU_WHITELIST_NEW."$white_date"

cat DU_WHITELIST_NEW."$white_date" |grep "[0-9]" > /dev/null 2>&1

if [ $? = 0 ]
then
touch $RANGER5_4HOME/RangerData/DataSourceSubscriberData/success/DU_WHITELIST_NEW."$white_date"
else 
rm DU_WHITELIST_NEW."$white_date"
fi
