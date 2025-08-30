rm -f $RANGERHOME/FMSData/Datasource/CDR/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/success/*
rm -f $RANGERHOME/FMSData/RejectedCDRData/gbxuscdr.log

cp Datasource/GBXUS/TestData/testpopulatesubscriberid.cdrdata $RANGERHOME/FMSData/Datasource/CDR/testpopulatesubscriberid.cdrdata
touch $RANGERHOME/FMSData/Datasource/CDR/success/testpopulatesubscriberid.cdrdata


sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from subscriber where id > 1024;
delete from account where id > 1024 ;

insert into account (id, account_type, account_name, frd_indicator) values (1810, 0, '18101964',0) ;
insert into subscriber (id, account_id, phone_number,connection_type, status, product_type) values (1810, 1810, '+110226006491005',1,1,1) ;

--insert into subscriber (ID, ACCOUNT_ID, FIRST_NAME, MIDDLE_NAME, LAST_NAME, GROUP_NAME, CREDIT_LIMIT, STATUS, STATIC_CREDIT_LIMIT, DYNAMIC_CREDIT_LIMIT,
--        TOTAL_SLIPPAGE, OUTSTANDING_AMOUNT, TOTAL_PAYMENT, UNBILLED_AMOUNT, LAST_DUE_DATE, LAST_PAY_DATE, NO_OF_SUSPENSIONS, NO_OF_FULL_PAYMENTS,
--        NO_OF_PART_PAYMENTS, DATE_OF_ACTIVATION, HOME_PHONE_NUMBER, OFFICE_PHONE_NUMBER, CONTACT_PHONE_NUMBER, MCN1, MCN2)
--        values(1810,  '18101964',  'mathew',  'TP',  'mathew', '', 0, 1, 0, 0, 0, 0, 0, 0, sysdate, sysdate, 0, 0, 0, null,'', '', '', '', '') ;
--
--insert into subscriber_phone_number (subscriber_ID, PHONE_NUMBER, IMSI_NUMBER, EQUIPMENT_ID, CONNECTION_TYPE, SERVICES, PHONE_NUMBER_STATUS)
--        values(1810, '+10226006491005', '','',3,0,1) ;

commit;
quit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

