rm -f $RANGERHOME/FMSData/Datasource/CDR/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/success/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/Log/gbxuscdr.log

cp Datasource/GBXUS/TestData/testreceivetimevalidation.cdrdata $RANGERHOME/FMSData/Datasource/CDR/testreceivetimevalidation.cdrdata
touch $RANGERHOME/FMSData/Datasource/CDR/success/testreceivetimevalidation.cdrdata


sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from subscriber where id > 1024;
delete from account where id > 1024;

commit;
quit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi


