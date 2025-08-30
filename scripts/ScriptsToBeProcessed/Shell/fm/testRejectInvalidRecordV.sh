grep "All Numeric Validation Failed For SUBSCRIBER_IMSI : 2E59060200" $RANGERHOME/FMSData/RejectedPROV94/prov94.log
if test $? -ne 0
then
    exit 1
fi

grep "PROV94_TEST_DATE Formatting Failed For SUBSCRIBER_SUBSCRIPTION_DATE : 2859060200||3200|MJ|SMX|3200|BUS||3200|01|01/s1/2005|" $RANGERHOME/FMSData/RejectedPROV94/prov94.log
if test $? -ne 0
then
    exit 2 
fi

grep "SUBS_MAP Formatting Failed For SUBSCRIBER_SUBSCRIPTION_STATUS : 2859060200|" $RANGERHOME/FMSData/RejectedPROV94/prov94.log
if test $? -ne 0
then
    exit 3 
fi

exit 0
