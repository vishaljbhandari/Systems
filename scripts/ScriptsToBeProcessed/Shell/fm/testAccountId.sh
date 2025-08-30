rm -f $RANGERHOME/FMSData/Datasource/Subscriber/*
rm -f $RANGERHOME/FMSData/Datasource/Subscriber/success/*
rm -f $RANGERHOME/FMSData/Datasource/Subscriber/Log/gbxussubscriber.log

cp Datasource/GBXUS/TestData/testAccountId.subdata $RANGERHOME/FMSData/Datasource/Subscriber/testAccountId.subdata
touch $RANGERHOME/FMSData/Datasource/Subscriber/success/testAccountId.subdata
