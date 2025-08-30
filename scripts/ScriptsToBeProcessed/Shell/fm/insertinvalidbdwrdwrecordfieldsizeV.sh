grep "^Unable to Parse Record Fields : F7BB12" $RANGERHOME/FMSData/RejectedCDRData/mtx12lnp.log
if test $? -ne 0
then
    exit 1
fi

exit 0
