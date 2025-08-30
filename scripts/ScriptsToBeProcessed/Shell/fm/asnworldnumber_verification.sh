ACTIVE_SUBSCRIBER=0
FRAUD_SUBSCRIBER=1
FC_PHONE_NUMBER=2
FC_IMEI=3
FC_IMSI=4
FC_CELL_SITE_ID=14

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool asnworldnumber_verification.dat

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200005');
select 1 from dual where 1 = (select count(*) from account where id = 1025) ;
select 1 from dual where 1 = (select count(*) from subscriber where subscriber_type = $ACTIVE_SUBSCRIBER and id = 5200); 

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber 
									where phone_number='+919845200005'
										and subscriber_type = $ACTIVE_SUBSCRIBER);



select  1 from dual where 0 = (select count(*) from blacklist_profile_options) ;

select  1 from dual where 0 = (select count(*) from callto_callby_ph_numbers) ;

spool off
EOF

if [ $? -eq '5' ] || grep "no rows" asnworldnumber_verification.dat
then
	exitval=1
else
	exitval=0
fi
rm -f asnworldnumber_verification.dat
exit $exitval
