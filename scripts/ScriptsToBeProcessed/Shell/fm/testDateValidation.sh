rm -f $RANGERHOME/FMSData/Datasource/CDR/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/success/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/Log/gbxuscdr.log

cp Datasource/GBXUS/TestData/testdatevalidation.cdrdata $RANGERHOME/FMSData/Datasource/CDR/testdatevalidation.cdrdata
touch $RANGERHOME/FMSData/Datasource/CDR/success/testdatevalidation.cdrdata
