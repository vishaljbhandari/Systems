#!/bin/bash

echo "Starting Spark script."
sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD << EOF
create table dataload_stat_temp as select * from dataload_statistics;
set autocommit off
BEGIN
delete from dataload_statistics where sdls_id in ( select sdls_id from dataload_stat_temp ) ;
insert into dataload_statistics select max(sdls_id) as sdls_id,max(tsk_id) as tsk_id, usp_id, usbp_id,tin_id, sdls_from_dttm,sdls_to_dttm,sum(sdls_record_count) as sdls_record_count  from dataload_stat_temp group by SDLS_FROM_DTTM, SDLS_TO_DTTM, TIN_ID, USP_ID, USBP_ID;
commit;
exception
when others then
ROLLBACK;
end;
/
drop table dataload_stat_temp;
EOF

echo "Spark shrink script completed."
