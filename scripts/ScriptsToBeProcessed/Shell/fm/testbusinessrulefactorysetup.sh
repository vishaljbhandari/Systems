sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

delete from datasource_info where datasource_name like '%GENERICDS%';
delete from BUSINESSRULE where datasource_name like  'GENERICDS%'  ;
delete from RATE_CONFIGURATION where datasource_name like  '%GENERICDS%'  ;
delete from default_rates ;

insert into datasource_info values ('GENERICDS', 2);
insert into BUSINESSRULE values ('GENERICDS',1,1,'GENERICDS_BR', 'brrate') ;

insert into default_rates values (default_rates_seq.nextval, 'GS','V',1, 500);
insert into RATE_CONFIGURATION values ('GENERICDS','ASN_RECORD_TYPE','ASN_TIME_STAMP','ASN_DURATION','ASN_CALLER_NUMBER', 'ASN_CALLED_NUMBER','ASN_FORWARDED_NUMBER','ASN_SERVICE_TYPE','ASN_NETWORK','ASN_PHONE_NUMBER','ASN_CALL_TYPE','ASN_VALUE', 'SUBSCRIBER_ID', 'PMN');

commit ;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
