#!/bin/bash
## This script will help in updating the spark reference DB configurations required for Distributed Cache
## WITH APPROPRIATE VALUES.
. $RUBY_UNIT_HOME/Scripts/configuration.sh


processSQLScripts()
{
	echo ${SPARK_REFERENCE_DB[1]}/${SPARK_REFERENCE_DB[2]}@${SPARK_REFERENCE_DB[0]} @$1
	sqlplus -s ${SPARK_REFERENCE_DB[1]}/${SPARK_REFERENCE_DB[2]}@${SPARK_REFERENCE_DB[0]} @$1 <<EOF
		commit;
EOF
}

main()
{
	if [ $# -ne 1 ]
	then
		echo "Improper Usage: please check spark ref DB credentials in configuration.sh and file name being passed"
		exit 1
	else
	processSQLScripts $*
	fi
}

main $*
