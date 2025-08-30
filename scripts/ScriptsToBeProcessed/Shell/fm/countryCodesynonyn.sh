#!/bin/bash


sqlplus -s  $DB_USER/$DB_PASSWORD@$NIKIRA_ORACLE_SID << EOF

create or replace view cdm_country_codes as select rownum id,CODE,DESCRIPTION from country_codes order by rownum;
commit;
quit;
EOF

