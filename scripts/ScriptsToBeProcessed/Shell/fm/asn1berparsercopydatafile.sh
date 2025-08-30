rm $RANGERHOME/FMSData/Datasource/CDR/*
rm $RANGERHOME/FMSData/Datasource/CDR/success/*

cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/Datasource/CDR
cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/Datasource/CDR/success

if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
