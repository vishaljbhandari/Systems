rm -f $RANGERHOME/FMSData/Datasource/CDR/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/success/*
rm -f $RANGERHOME/FMSData/RejectedCDRData/gbxuscdr.log

cp Datasource/GBXUS/TestData/testPopulatePhoneNumber.cdrdata  $RANGERHOME/FMSData/Datasource/CDR/testPopulatePhoneNumber.cdrdata
touch $RANGERHOME/FMSData/Datasource/CDR/success/testPopulatePhoneNumber.cdrdata


sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from subscriber_phone_number  where subscriber_id > 1024;
delete from subscriber where id > 1024;

commit;
quit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi
