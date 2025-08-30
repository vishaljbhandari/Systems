if test $1 -eq 1
then
sqlplus -s $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off;

	delete $2 ;
	commit ;
	quit ;

EOF

elif test $1 -eq 0
then
sqlplus -s $DB_USER/$DB_PASSWORD << EOF 
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off;

    insert into $2 (select n.id , T.id  from $3 T , networks n) ;
    commit ;
    quit ;

EOF
fi

if [ $? -ne 0 -a $1 -eq 0 ]
then
sqlplus -s $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off;

    insert into $2 (select T.id, n.id from $3 T , networks n) ;
    commit ;
    quit ;

EOF

fi


if test $? -ne 0
then
	exitval=1
else
	exitval=0
fi

exit $exitval
