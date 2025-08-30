FRD_SUBSCRIBER=1
FC_PHONE_NUMBER=2
FC_IMEI=3
FC_CELL_SITE_ID=14

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool lifestyleprofilegenerationsubscriberlessV.dat

select  1 from dual where 1 = (select count(*) from subscriber_profile_dates
				where subscriber_id  = 1025
				and (to_number(to_char(date_of_generate, 'ddd')) = to_number(to_char(sysdate, 'ddd')))) ;

select  1 from dual where 0 = (select count(*) from suspect_values
                where value = ''
                    and trunc(expiry_date) = trunc(sysdate + 30)
                    and entity_type = $FC_CELL_SITE_ID);

spool off
EOF

if [ $? -eq '5' ] || grep "no rows" lifestyleprofilegenerationsubscriberlessV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f lifestyleprofilegenerationsubscriberlessV.dat
exit $exitval
