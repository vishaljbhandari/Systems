#! /bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh

generateInsertQuery()
{
    if [ $DB_TYPE == "1" ]
    then
        sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
        set serveroutput on
                    set heading off
                    insert into accumulation_field_units (id, accumulation_field_id, units, field_type) values (2, 49, 'Bytes', 1);
                    insert into accumulation_field_units (id, accumulation_field_id, units, field_type) values (3, 50, 'Bytes', 1);
                    insert into accumulation_field_units (id, accumulation_field_id, units, field_type) values (4, 51, 'Bytes', 1);
                    INSERT INTO accumulation_field_units (id, accumulation_field_id, units, field_type) VALUES (5, 12, 'Seconds', 2) ;
                    commit;

                    quit
EOF

    else
clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v
     "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
     set termout off
     set heading off
     set echo off
      insert into accumulation_field_units (id, accumulation_field_id, units, field_type) values (2, 49, 'Bytes', 1);
      insert into accumulation_field_units (id, accumulation_field_id, units, field_type) values (3, 50, 'Bytes', 1);
      insert into accumulation_field_units (id, accumulation_field_id, units, field_type) values (4, 51, 'Bytes', 1);
      INSERT INTO accumulation_field_units (id, accumulation_field_id, units, field_type) VALUES (5, 12, 'Seconds', 2) ;
      commit;
      quit
EOF
         fi
}

main()
{
    generateInsertQuery;
}
main
