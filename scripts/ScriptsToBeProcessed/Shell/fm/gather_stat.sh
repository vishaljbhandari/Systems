if [ "$1" == "" ]
then
	echo "Usage: ./gather_stat.sh <DB_USER> <DB_PASSWORD>"
	exit ;
fi
if [ "$2" == "" ]
then
	echo "Usage: ./gather_stat.sh <DB_USER> <DB_PASSWORD>"
	exit ;
fi
echo "Gathering DB $1 Schema stats."
sqlplus -s $1/$2 << EOF
exec dbms_stats.gather_schema_stats('$1') ;
exit ;
EOF
