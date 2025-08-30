rm $RANGERHOME/FMSData/Datasource/CDR/*
rm $RANGERHOME/FMSData/Datasource/CDR/success/*
rm $RANGERHOME/FMSData/CDRData/*
rm $RANGERHOME/FMSData/CDRData/success/*

rm $RANGERHOME/FMSData/RejectedCDRData/Data/*

> $RANGERHOME/FMSData/RejectedCDRData/roamexcdr.log
> $RANGERHOME/LOG/cdr.log

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/Datasource/CDR

if test $? -ne 0
then
    exit $?
fi

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/Datasource/CDR/success

if test $? -ne 0
then
    exit $?
fi

exit 0
