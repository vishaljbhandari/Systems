sqlplus $DB_USER/$DB_PASSWORD <<ENDSQL
whenever sqlerror exit 5;
whenever oserror exit 5;
delete from default_rate where rate_type='GS' and call_type='S';
insert into default_rate(rate_type,call_type,pulse,rate) values('GS','S',20,11.5);
commit;
quit;
ENDSQL

if test $? -eq 5
	then
	   exitval=1
	 else
	   exitval=0
fi
exit $exitval

