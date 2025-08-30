rm -f $RANGERHOME/FMSData/Datasource/CDR/*
rm -f $RANGERHOME/FMSData/Datasource/CDR/success/*
rm -f $RANGERHOME/FMSData/RejectedCDRData/gbxuscdr.log

cp Datasource/GBXUS/TestData/testPopulateCallType.cdrdata  $RANGERHOME/FMSData/Datasource/CDR/testPopulateCallType.cdrdata
touch $RANGERHOME/FMSData/Datasource/CDR/success/testPopulateCallType.cdrdata


if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi
