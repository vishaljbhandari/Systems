#!/bin/bash

function CheckParam()
{	
    echo "Checking parameters of DB"
	rm -f -- *.log
	
	if [ -z $DB_USER ]
	then
		echo 'DB_USER Not Set. Please set the environment variable DB_USER'
		exit 1
	fi
	
	if [ -z $DB_PASSWORD ]
	then
		echo 'DB_PASSWORD Not Set. Please set the environment variable DB_PASSWORD'
		exit 1
	fi
	 
	if [ -z $NIKIRA_ORACLE_SID ]
	then
		echo 'NIKIRA_ORACLE_SID Not Set. Please set the environment variable NIKIRA_ORACLE_SID'
		exit 1
	fi
 
	Nikira_DB_User=$DB_USER
	Nikira_DB_Password=$DB_PASSWORD
	Nikira_Oracle_SID=$NIKIRA_ORACLE_SID
	echo "the Nikira_DB_User= $Nikira_DB_User and Nikira_DB_Password=$Nikira_DB_Password" 
}


function UpdateTasks_1
{
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >UpdateTasks_1.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever sqlerror exit 5 rollback ;
   	 	whenever oserror exit 5 ;

		@TaskMetaTable2.sql 
		
		
		DECLARE 
   	 	   cursor task is select * from tasks ;
           TSK_OLD_ID NUMBER(10) := 0;
		   TSK_NEW_ID NUMBER(10) := 0;
		   TSK_OLD_PARENT_ID NUMBER(10) := 0;
		   TSK_NEW_PARENT_ID NUMBER(10) := 0;
		   TSK_OLD_NAME VARCHAR2(256) := '';
		   TSK_NEW_NAME VARCHAR2(256) := '';
		
			
		BEGIN
			for tk in task
				loop
                    select OLD_ID,OLD_NAME,OLD_PARENT_ID,NEW_ID,NEW_NAME,NEW_PARENT_ID into TSK_OLD_ID,TSK_OLD_NAME,TSK_OLD_PARENT_ID,TSK_NEW_ID,TSK_NEW_NAME,TSK_NEW_PARENT_ID from tasks_meta_2 where tk.ID = tasks_meta_2.OLD_ID ;
   					
			   		update tasks set ID = TSK_NEW_ID, PARENT_ID = TSK_NEW_PARENT_ID, NAME = TSK_NEW_NAME where ID = tk.id and tk.id = TSK_OLD_ID ;
					
				end loop;
		   @Migration_2.sql; 	
		   @Migration_3.sql; 
		   @Migration_4.sql;
		   @Migration_5.sql;
		   @migration_6.sql;
		   @migration_7.sql;
		   commit;
		END;   
   	 	
/   	 	
EOF

}

function Main()
{
	CheckParam
	UpdateTasks_1
}
Main
