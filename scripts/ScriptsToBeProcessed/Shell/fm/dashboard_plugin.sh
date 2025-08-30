#! /bin/bash
#################################################################################
#  Copyright (c) Subex Limited. All rights reserved.                            #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

# Usage: 
# For Nbit installation
#  > sh dashboard_plugin.sh -nbit 
# For Normal installation
#  > sh dashboard_plugin.sh 


User=$DB_USER ;
Password=$DB_PASSWORD ;

if [ "${DASHBOARD_DB_USER}x" == "x" ]
then
	DASHBOARD_DB_USER='dashboard'
fi

if [ "${DASHBOARD_DB_PASSWORD}x" == "x" ]
then
	DASHBOARD_DB_PASSWORD='dashboard'
fi

if [ "${ORACLE_PORT}x" == "x" ]
then
	ORACLE_PORT='1521'
fi

if [ "${DASHBOARD_PORT}x" == "x" ]
then
  echo "Dashboard port is not set in bashrc so defaulting to '8080'"
	DASHBOARD_PORT='8080'
fi

dashboard_report_folder=$DASHBOARD_REPORT_FOLDER
if [ "x${DASHBOARD_REPORT_FOLDER}" == "x" ]
then
  dashboard_report_folder="$HOME/report_data"
fi
  mkdir -p $dashboard_report_folder 


option=$1 ;
DBScriptsFolder=`dirname $0` ;
echo "DBScriptsFolder=$DBScriptsFolder"

if [ "x${option}" == "x" ]
then 
  is_nbit='N'
else
  if [ "x${option}" == "x-nbit" ]
  then 
    is_nbit='Y'
  fi
fi

tomcat_port=$DASHBOARD_PORT
if [ $is_nbit == 'N' ]
then
#  read -p "Enter Dashboard Server port [8080]: " tomcat_port
  if [ "x${tomcat_port}" == "x" ]
  then
    tomcat_port='8080'
  fi
fi

echo "Dashboard will be installed on DB user:'${DASHBOARD_DB_USER}' at PORT:'${tomcat_port}'"

get_ip() {
    os=`uname`
    ip=""
    if [ "$os" == "Linux" ]
    then
        ip=`/sbin/ifconfig  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`
    else
        if [ "$os" == "SunOS" ]
        then
            ip=`ifconfig -a | grep inet | grep -v '127.0.0.1' | awk '{ print $2} '`
	else
            ip='127.0.0.1'
        fi
    fi
    echo $ip
}

ipaddress=$IP_ADDRESS
if [ "x$ipaddress" == "x" ]
then
	ipaddress=`get_ip`
fi

Setup_dashboard()
{
  echo "---------------------------------------------------------------------------"
  echo "Creating tables and packages for Dashboard, check the log in dashboard.log"
  echo "---------------------------------------------------------------------------"

sqlplus -l -s ${DASHBOARD_DB_USER}/${DASHBOARD_DB_PASSWORD} << EOF > dashboard.log
@$DBScriptsFolder/drop-dashboard_ddl.sql
@$DBScriptsFolder/dashboard_ddl.sql
@$DBScriptsFolder/dashboard_dml.sql
GRANT SELECT ON PARTITION_TBL TO $User ;
GRANT SELECT ON ROLE_TBL TO $User ;
GRANT SELECT ON USER_TBL TO $User ;
GRANT UPDATE ON USER_TBL TO $User ;
GRANT INSERT ON USER_TBL TO $User ;
GRANT UPDATE ON MACHINE_ADDRESS TO $User ;
GRANT UPDATE ON TOKEN_MAPPING TO $User ;
GRANT INSERT ON DASHBOARD_SERVER TO $User ;
GRANT DELETE ON DASHBOARD_SERVER TO $User ;
GRANT UPDATE ON DASHBOARD_SERVER TO $User ;
GRANT EXECUTE ON DB_USER TO $User ;
PROMPT INSTALLING PACKAGE...
@$DBScriptsFolder/dashboard.sql

--declare
--	rol_id NUMBER(20) ;
--begin
--	SELECT ROL_ID INTO rol_id FROM ROLE_TBL WHERE ROL_NAME = 'Create' ;
--	BEGIN DB_USER.delete_user('nadmin'); END;
--	BEGIN DB_USER.CREATE_USER('nadmin', rol_id, DB_USER.TMP_PARTITION_TBL(2,3,4)); END;
--end ;
--/

-- show err ;

UPDATE MACHINE_ADDRESS SET MAD_HOST_ADDRESS='$ipaddress' ;
UPDATE TOKEN_MAPPING SET TOM_CONTEXT_PATH='$dashboard_report_folder' ;
UPDATE DASHBOARD_SERVER SET DER_USER_NAME ='$DB_USER', DER_PASSWORD = '$DB_PASSWORD', 
DER_URL = 'jdbc:oracle:thin:@${ipaddress}:$ORACLE_PORT:$ORACLE_SID' WHERE DER_NAME = 'ROC Fraud Management Server' ;

EOF
}


Setup_nikira()
{
  echo "---------------------------------------------------------------------------"
  echo "Updating Nikira DB for Dashboard, check the log in nikira.log"
  echo "---------------------------------------------------------------------------"

sqlplus -l -s $User/$Password << EOF > nikira.log
Prompt Creating View DASHBOARD_PARTITION_TASK_MAP...
CREATE OR REPLACE VIEW DASHBOARD_PARTITION_TASK_MAP AS SELECT P.PTN_ID PARTITION_ID, T.ID TASK_ID FROM $DASHBOARD_DB_USER.PARTITION_TBL P, TASKS T WHERE T.IMAGE_SRC = P.PTN_NAME ;

DELETE CONFIGURATIONS WHERE CONFIG_KEY = 'DASHBOARD_URL' ;
INSERT INTO CONFIGURATIONS(ID, CONFIG_KEY, VALUE, IS_VISIBLE) VALUES(CONFIGURATIONS_SEQ.NEXTVAL,'DASHBOARD_URL','http://$ipaddress:$tomcat_port/Spark2/',1) ;

Prompt Inserting Tasks...
declare
  is_dashboard_task_exist number ;
begin

select count(*) into is_dashboard_task_exist from TASKS WHERE link LIKE '/dashboard_%' ;
if is_dashboard_task_exist = 0
then
  --DELETE FROM RANGER_GROUPS_TASKS WHERE TASK_ID in (SELECT ID FROM TASKS T WHERE LINK LIKE '%dashboard%') ;
  --DELETE FROM TASKS WHERE link LIKE '/dashboard_%' ;
INSERT INTO TASKS (ID, PARENT_ID, NAME, LINK, IS_DEFAULT, IS_MENU_ITEM,PACKAGE_IDS) VALUES (TASKS_SEQ.NEXTVAL, GetParentID('Tasks'), 'Dashboard', '/dashboard_root', 0, 0, '0,1,2,3') ;
INSERT INTO TASKS (ID, PARENT_ID, NAME, LINK, IS_DEFAULT, IS_MENU_ITEM, PACKAGE_IDS) VALUES (TASKS_SEQ.NEXTVAL, GetParentID('Dashboard'), 'Create/Modify Dashboard', '/dashboard_create', 0, 0, '0,1,2,3') ;
INSERT INTO TASKS (ID, PARENT_ID, NAME, LINK, IS_DEFAULT, IS_MENU_ITEM, PACKAGE_IDS) VALUES (TASKS_SEQ.NEXTVAL, GetParentID('Dashboard'), 'View Dashboard', '/dashboard_view', 0, 0, '0,1,2,3') ;

INSERT INTO TASKS (ID, PARENT_ID, NAME, LINK, IS_DEFAULT, IS_MENU_ITEM, IMAGE_SRC,PACKAGE_IDS ) VALUES (TASKS_SEQ.NEXTVAL, GetParentID('View Dashboard'), 'IT Group', '/dashboard_it', 0, 0, 'IT', '0,1,2,3') ;
INSERT INTO TASKS (ID, PARENT_ID, NAME, LINK, IS_DEFAULT, IS_MENU_ITEM, IMAGE_SRC, PACKAGE_IDS) VALUES (TASKS_SEQ.NEXTVAL, GetParentID('View Dashboard'), 'Business Group', '/dashboard_business', 0, 0, 'Business', '0,1,2,3') ;
INSERT INTO TASKS (ID, PARENT_ID, NAME, LINK, IS_DEFAULT, IS_MENU_ITEM, IMAGE_SRC, PACKAGE_IDS) VALUES (TASKS_SEQ.NEXTVAL, GetParentID('View Dashboard'), 'Executive Group', '/dashboard_executive', 0, 0, 'Executive', '0,1,2,3') ;

INSERT INTO RANGER_GROUPS_TASKS (SELECT (select id from ranger_groups where name = 'nadmin') ranger_group_id, ID FROM TASKS T WHERE LINK LIKE '%dashboard%') ;
end if ;

exception 
  when others then
  dbms_output.put_line('ERROR: ' || SQLERRM) ;
end ;
/

Prompt Creating view RULE_BASED_SUMMARY...
CREATE OR REPLACE VIEW RULE_BASED_SUMMARY AS 
  SELECT EVENT_INSTANCE_ID RULE_ID,
    COUNT(DECODE(STATUS,2,1,NULL)) AS FR,
    COUNT(DECODE(STATUS,4,1,NULL)) AS NFR
  FROM ALERTS A, ALARMS B, RULES R
  WHERE A.ALARM_ID=B.ID AND STATUS IN (2,4) 
        AND B.CREATED_DATE > TRUNC(SYSDATE) - 30
		AND R.KEY = EVENT_INSTANCE_ID
		AND (R.CATEGORY IN ('CalledToCalledBy', 'FINGERPRINT_RULE', 'MANUAL_ALARM_RULES', 'STATISTICAL_RULE', 'INLINE_RULES', 'MANUAL.PRECHECK', 'SMART_PATTERN') OR R.CATEGORY IS NULL )
  GROUP BY EVENT_INSTANCE_ID ;

Prompt Creating Tables required for Default Charts...
DROP TABLE RAM_LOG  ;
CREATE TABLE RAM_LOG (ID NUMBER, TIME DATE, TOTAL VARCHAR2(16), FREE VARCHAR2(16), MACHINE_NAME VARCHAR2(32)) ;
DROP SEQUENCE RAM_LOG_ID ;
CREATE SEQUENCE RAM_LOG_ID INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

DROP TABLE DISK_LOG  ;
CREATE TABLE DISK_LOG (ID NUMBER, TIME DATE, FILE_SYSTEM VARCHAR2(256), TOTAL NUMBER, USED NUMBER, FREE NUMBER, CAPACITY NUMBER, MOUNTED_ON VARCHAR2(256), MACHINE_NAME VARCHAR2(32)) ;
DROP SEQUENCE DISK_LOG_ID ;
CREATE SEQUENCE DISK_LOG_ID INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

DROP TABLE CPU_LOG  ;
CREATE TABLE CPU_LOG (ID NUMBER, TIME DATE, USR NUMBER, SYS NUMBER, WIO NUMBER, IDLE NUMBER, MACHINE_NAME VARCHAR2(32)) ;
DROP SEQUENCE CPU_LOG_ID ;
CREATE SEQUENCE CPU_LOG_ID INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

commit ;

--set serveroutput on 100000
declare
  query varchar2(2000) := '' ;
  create_role_id number ;
  view_role_id number ;
  create_task_id number ;
  partition_id number ;
  is_create_task number ;
  partition_ids varchar2(32) := 'NULL' ;

  type  all_users_type is record (id number, name varchar2(32), role_id number) ;
  all_users all_users_type ;

  cursor all_users_cur is 
    select ranger_user_id, name, ranger_group_id from ranger_users , ranger_groups_ranger_users 
    where id = ranger_user_id order by ranger_user_id;

  cursor partition_id_cur(role_id number) is
    select partition_id from dashboard_partition_task_map d, ranger_groups_tasks r
    where d.task_id = r.task_id
    and ranger_group_id = all_users.role_id order by partition_id ;

begin
  
  SELECT ROL_ID into create_role_id FROM $DASHBOARD_DB_USER.ROLE_TBL WHERE ROL_NAME = 'Create' ;
  SELECT ROL_ID into view_role_id FROM $DASHBOARD_DB_USER.ROLE_TBL WHERE ROL_NAME = 'View' ;
  SELECT ID into create_task_id FROM tasks WHERE link = '/dashboard_create' ;

  open all_users_cur ;
  
  loop 
    fetch all_users_cur into all_users ;
    exit when all_users_cur%NOTFOUND;

    partition_ids := 'NULL' ;
    open partition_id_cur(all_users.role_id) ;
    loop
      fetch partition_id_cur into partition_id;
      exit when partition_id_cur%NOTFOUND;
      if partition_ids = 'NULL'
      then
        partition_ids := to_char (partition_id) ;
      else
        partition_ids := partition_ids || ',' || to_char (partition_id) ;
      end if;
    end loop ;
    close partition_id_cur ;

    if partition_ids != 'NULL'
    then
      -- Create User
      -- Check if he has a create role permission;
      select count(1) into is_create_task from ranger_groups_tasks 
        where ranger_group_id = all_users.role_id and task_id = create_task_id ;
      
      if is_create_task != 0
      then
        query := 'BEGIN $DASHBOARD_DB_USER.DB_USER.CREATE_USER(''' || all_users.NAME || ''', ' || create_role_id || ', $DASHBOARD_DB_USER.DB_USER.TMP_PARTITION_TBL('||partition_ids||')); END;' ;
      else
        query := 'BEGIN $DASHBOARD_DB_USER.DB_USER.CREATE_USER(''' || all_users.NAME || ''', ' || view_role_id || ', $DASHBOARD_DB_USER.DB_USER.TMP_PARTITION_TBL('||partition_ids||')); END;' ;
      end if ;
      
      execute immediate ('BEGIN $DASHBOARD_DB_USER.DB_USER.DELETE_USER('''||all_users.NAME||'''); END;') ;
      execute immediate (query) ;
    end if ;
  end loop ;
  
  close all_users_cur ;
exception
  when others then 
    dbms_output.put_line('ERROR: ' || SQLERRM) ;
   
end ;
/

quit ;
EOF
}

Setup_dashboard

Setup_nikira

exit 0 ;
