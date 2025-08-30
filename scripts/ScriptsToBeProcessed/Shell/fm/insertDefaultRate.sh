sqlplus $DB_USER/$DB_PASSWORD <<ENDSQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from  default_rates where rate_type='GS' and call_type='V';
delete from  default_rates where rate_type='GS' and call_type='S';
delete from  default_rates where rate_type='GS' and call_type='G';
delete from  default_rates where rate_type='IN' and call_type='G';
delete from  default_rates where rate_type='TF' and call_type='V';
delete from  default_rates where rate_type='TF' and call_type='S';
delete from  default_rates where rate_type='TF' and call_type='G';

insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'GS','S',20,11);
insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'GS','G',60,12);
insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'GS','V',40,2);
insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'IN','G',60,9);
insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'TF','V',60,3);
insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'TF','S',50,0.50);
insert into default_rates (id,rate_type,call_type,pulse,rate) values(default_rates_seq.nextval, 'TF','G',10,5);

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

