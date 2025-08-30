rm -f $RANGERHOME/FMSData/Datasource/Subscriber/*
rm -f $RANGERHOME/FMSData/Datasource/Subscriber/success/*
rm -f $RANGERHOME/FMSData/Datasource/Subscriber/Log/gbxussubscriber.log

cp Datasource/GBXUS/TestData/testPhoneNumber.subdata $RANGERHOME/FMSData/Datasource/Subscriber/testPhoneNumber.subdata
touch $RANGERHOME/FMSData/Datasource/Subscriber/success/testPhoneNumber.subdata
