sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from subscriber_group_map where subscriber_id >= 1024 ;
delete from subscriber_phone_number  where subscriber_id > 1024;
delete from subscriber where id > 1024;

insert into subscriber (network_id, ID,ACCOUNT_ID,FIRST_NAME,MIDDLE_NAME,LAST_NAME,GROUP_NAME,CREDIT_LIMIT,STATUS,STATIC_CREDIT_LIMIT,DYNAMIC_CREDIT_LIMIT,TOTAL_SLIPPAGE,OUTSTANDING_AMOUNT,TOTAL_PAYMENT,UNBILLED_AMOUNT,LAST_DUE_DATE,LAST_PAY_DATE,NO_OF_SUSPENSIONS,NO_OF_FULL_PAYMENTS,NO_OF_PART_PAYMENTS,DATE_OF_ACTIVATION,HOME_PHONE_NUMBER,OFFICE_PHONE_NUMBER,CONTACT_PHONE_NUMBER,MCN1,MCN2)
values(1024, 1025, '18101964','','','','Default',1,1,0,0,0,0,0,0,sysdate,sysdate,0,0,0,null,'34523456','3497581','+191456789','1234564','2345678');

insert into subscriber_phone_number (subscriber_ID, PHONE_NUMBER, IMSI_NUMBER, EQUIPMENT_ID, CONNECTION_TYPE, SERVICES,
PHONE_NUMBER_STATUS) values(1025,'+14163155236', '302370122351505','4090598203456',1,1024,1) ;

commit ;

EOF

if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
 
