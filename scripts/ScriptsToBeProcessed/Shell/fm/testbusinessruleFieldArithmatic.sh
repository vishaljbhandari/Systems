sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from businessrule where businessrule_name = 'TEST_FIELD_ARITHMATIC_RULE' ;
delete from field_arithmatic_rule   ;
delete from field_arithmatic ;
delete from date_format ;

insert into businessrule values ('MTX12LNP',1,10,'TEST_FIELD_ARITHMATIC_RULE', 'brfieldarithmatic') ;
insert into field_arithmatic_rule values ('TEST_FIELD_ARITHMATIC_RULE','TEST_ADD','1');
insert into field_arithmatic values ('TEST_ADD','ADD','DT','{TIME_STAMP}','{DURATION}','RECEIVE_TIME',1000) ;
insert into date_format values (1000,'%Y/%m/%d %H:%M:%S','S','%Y/%m/%d %H:%M:%S') ;

commit;
EOF

if test $? -eq 5
then
    exit 1
fi

exit 0

