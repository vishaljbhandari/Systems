grep "Invalid Number Of Fields : 2859060200|" $RANGERHOME/FMSData/RejectedPROV94/prov94.log
if test $? -ne 0
then
    exit 1 
fi

  
exit 0
