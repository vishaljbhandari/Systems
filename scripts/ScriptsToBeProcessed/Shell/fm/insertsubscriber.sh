sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from subscriber where id > 1024;
delete from account where id > 1024 ;
insert into account(ID,ACCOUNT_NAME) VALUES (1025, 'SUBEX ACCOUNT') ; 
insert into subscriber (ID, ACCOUNT_ID, PHONE_NUMBER, CONNECTION_TYPE, STATUS, PRODUCT_TYPE, IMSI) values(1025, 1025, '+14163155236',1,1,1, '12345678901');

commit ;

EOF

if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
 
