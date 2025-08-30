rm $RANGERHOME/FMSData/DataSourcePROV94/*
rm $RANGERHOME/FMSData/DataSourcePROV94/success/*
rm $RANGERHOME/FMSData/DataRecord/Proces*
rm $RANGERHOME/FMSData/DataRecord/Process/success/*

rm $RANGERHOME/FMSData/RejectedPROV94/Data/*

> $RANGERHOME/FMSData/RejectedPROV94/prov94.log
> $RANGERHOME/LOG/prov94.log

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/DataSourcePROV94

if test $? -ne 0
then
    exit $?
fi

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/DataSourcePROV94/success

if test $? -ne 0
then
    exit $?
fi

exit 0
