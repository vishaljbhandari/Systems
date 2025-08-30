#!/bin/bash

if [ -z $SPARK_DB_USER ]
then
	echo 'SPARK_DB_USER Not Set'
	exit 1
fi

if [ -z $SPARK_DB_PASSWORD ]
then
	echo 'SPARK_DB_PASSWORD Not Set'
exit 1
fi 


if [ -z $SPARK_ORACLE_SID ]
then
	echo 'SPARK_ORACLE_SID Not Set'
	exit 1
fi


sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID<<EOF
spool Migration_from_8.0.2-patch9_to_8.1_sparkref_sql.log
set feedback on ;
set serveroutput on ;

update service_pack set spk_reference_mappings_file='/com/subex/nikira/database/hibernate/mappings/reference/mappings.hbm.xml', spk_reference_schema_file='/com/subex/nikira/database/metadata/nikira_schema_v1.0.0_sp1.xml' where IVN_ID=(select IVN_ID from INSTALL_VERSION where IVN_NAME='ROC Fraud Management') ;

commit;
quit;
EOF
