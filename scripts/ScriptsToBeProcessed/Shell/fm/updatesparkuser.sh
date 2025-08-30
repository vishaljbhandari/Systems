#!/bin/bash

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

 sqlplus -s ${SPARK_REFERENCE_DB[1]}/${SPARK_REFERENCE_DB[2]} << EOF > /dev/null
 whenever oserror exit 1 ;
 SPOOL $WATIR_SERVER_HOME/Scripts/tmp/updatesparkuser.lst;
 SET FEEDBACK ON ;
SET SERVEROUTPUT ON
declare
        cursor c1 is select USR_NAME from USER_TBL where USR_NAME not in ('Root','Administrator','nadmin') and USR_NAME not like 'userupdated%';
        count_of_users number (20) ;
         begin
         select count(*)+4 into count_of_users from USER_TBL where USR_NAME like 'userupdated%';
        for i in c1
        loop
        
       DBMS_OUTPUT.PUT_LINE(count_of_users) ;
        update USER_TBL set USR_NAME ='userupdated'||count_of_users where USR_NAME not in ('Root','Administrator','nadmin') and USR_NAME not like 'userupdated%' and USR_NAME = i.USR_NAME;
      DBMS_OUTPUT.PUT_LINE(i.USR_NAME) ;
      count_of_users := count_of_users+1;
        end loop;

             end;
                  /
commit; 


SPOOL OFF ;
 quit;
EOF
