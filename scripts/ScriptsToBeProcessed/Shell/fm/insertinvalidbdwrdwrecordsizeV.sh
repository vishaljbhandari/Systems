grep "^Unable to Parse Record : F7BB1" $RANGERHOME/FMSData/RejectedCDRData/mtx12lnp.log

if test $? -ne 0
then
    exit 1
fi

exit 0

