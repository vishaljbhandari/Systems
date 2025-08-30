#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

getEntityValue()
{
	if [ $DB_TYPE == "1" ]
then
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		select $ENTITY from $TABLE where $WHEREFIELD = '$WHEREVAL' ;
EOF

else
		clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		select $ENTITY from $TABLE where $WHEREFIELD = '$WHEREVAL' ;
EOF
fi
}

getEntityValueWithNoConditions()
{
if [ $DB_TYPE == "1" ]
then
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		select $ENTITY from $TABLE ;
EOF

else
		clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		select $ENTITY from $TABLE ;
EOF
fi
}

#checkInputsAndExecute()
#{
	TABLE=$1
	ENTITY=$2

	if [ "$ENTITY" == "count" ]
	then
		ENTITY="count(*)"
	fi

	if [ $# -gt 2 ]
	then
		WHEREFIELD=$3
		WHEREVAL=$4

#		echo "$TABLE $ENTITY $WHEREFIELD $WHEREVAL"
		val=`getEntityValue $TABLE $ENTITY $WHEREFIELD $WHEREVAL | tr -d "   [A-Za-z\.]\012"`
		echo $val
	else
#		echo "$TABLE $ENTITY"
		val=`getEntityValueWithNoConditions $TABLE $ENTITY | tr -d "   [A-Za-z\.]\012"`
		echo $val
	fi
#	return $val
#}
	
#main()
#{
#val=`checkInputsAndExecute`
#echo $val
#}

#main $*


