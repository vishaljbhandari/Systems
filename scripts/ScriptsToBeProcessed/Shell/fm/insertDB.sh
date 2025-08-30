> dynamicscript.sql ;

if [ "$1" == "holiday_lists" -o "$1" == "free_numbers" ]
then
	echo "delete from "$1"_networks ;" >> dynamicscript.sql ;
fi


if [ $4 = "true" ]
then
  	if [ "$1" == "rater_special_numbers" ]
  	then
  		echo "delete from networks_"$1" ;" >> dynamicscript.sql ;
  	fi
  	echo "delete "$1 $3 ";" >> dynamicscript.sql ;
fi

if [ $1 != "ds_configuration" -a $1 != "sdr_rate" -a $1 != "holiday_lists" -a $1 != "holiday_lists_networks" -a $1 != "free_numbers" -a $1 != "free_numbers_networks" -a $1 != "rater_special_numbers" -a $1 != "networks_rater_special_numbers" ]
then
	echo "insert into "$1" values  ("$1"_seq.nextval, "$2");" >> dynamicscript.sql;
	sqlplus -s $DB_USER/$DB_PASSWORD @TestScripts/ratertest.sql 
else
	echo "insert into "$1" values  ("$2");" >> dynamicscript.sql;
	sqlplus -s $DB_USER/$DB_PASSWORD @TestScripts/ratertest.sql 
fi

if test $? -eq 5 
then
	exitval=1
else
	exitval=0
fi
rm -f  dynamicscript.sql 
exit $exitval

