
rm $RANGERHOME/FMSData/DataSourceMTX12LNP/*
rm $RANGERHOME/FMSData/DataSourceMTX12LNP/success/*
rm $RANGERHOME/FMSData/CDRData/*
rm $RANGERHOME/FMSData/CDRData/success/*

rm $RANGERHOME/FMSData/RejectedMTX12LNP/Data/*

> $RANGERHOME/FMSData/RejectedMTX12LNP/mtx12lnp.log
> $RANGERHOME/LOG/cdr.log

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/DataSourceMTX12LNP

if test $? -ne 0
then
    exit $?
fi

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/DataSourceMTX12LNP/success

if test $? -ne 0
then
    exit $?
fi

exit 0
