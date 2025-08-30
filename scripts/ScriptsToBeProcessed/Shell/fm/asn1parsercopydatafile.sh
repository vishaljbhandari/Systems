rm $RANGERHOME/FMSData/Datasource/CDR/*
rm $RANGERHOME/FMSData/Datasource/CDR/success/*
rm $RANGERHOME/FMSData/CDRData/*
rm $RANGERHOME/FMSData/CDRData/success/*

rm $RANGERHOME/FMSData/RejectedCDRData/Data/*


cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/Datasource/CDR
cp ./Datasource/ConfigurableDS/TestData/$1 $RANGERHOME/FMSData/Datasource/CDR/success

if test $? -ne 0
then
    exit $?
fi

if [ "$2" != "holdDB" ]
then
sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from cdr_file_detail where DS_TYPE = 'ROAMEX' ;

commit;
EOF
fi

if test $? -eq 5
then
	exit 1
fi

exit 0
