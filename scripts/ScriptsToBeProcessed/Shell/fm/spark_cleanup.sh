#!/bin/bash

echo "Starting cleanup script."
sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD << EOF
delete from DATALOAD_STATISTICS where sdls_id not in (select sdls_id from DATALOAD_STATISTICS dst inner join USAGE_Server_PARTITION usp on usp.USP_ID = dst.USP_ID  inner join USAGE_PARTITION upn on upn.UPN_ID = usp.UPN_ID and upn.UPN_FROM = dst.SDLS_FROM_DTTM and upn.UPN_TO = dst.SDLS_TO_DTTM);
commit;
EOF

echo "Spark cleanup script completed."
