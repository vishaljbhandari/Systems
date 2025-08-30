sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from businessrule where businessrule_name = 'TEST_FIELD_ARITHMATIC_RULE' ;
delete from field_arithmatic_rule   ;
delete from field_arithmatic ;
delete from date_format ;

insert into businessrule values ('MTX12LNP',1,10,'TEST_FIELD_ARITHMATIC_RULE', 'brfieldarithmatic') ;
insert into field_arithmatic_rule values ('TEST_FIELD_ARITHMATIC_RULE','TEST_SUB','1');
insert into field_arithmatic values ('TEST_SUB','SUB','LG','{RECIEVE_TIME_SECS}','{START_TIME_SECS}','DURATION',1) ;

commit;
EOF

if test $? -eq 5
then
    exit 1
fi

exit 0

