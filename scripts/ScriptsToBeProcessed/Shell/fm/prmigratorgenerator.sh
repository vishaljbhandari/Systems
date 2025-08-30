#!/bin/env bash

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

prmigratorScriptGenerator()
{
	$SQL_COMMAND -s /nolog <<EOF > $COMMON_MOUNT_POINT/LOG/prmigrate.log
	CONNECT_TO_SQL
    set serveroutput on ;
	set lines 380;
    set feedback off ;
	DECLARE
		number_of_slaves number(20) := 0 ;
    BEGIN
		dbms_output.put_line('if [ -f \$COMMON_MOUNT_POINT/LOG/MigratorCorruptFiles.log ] ; then ') ;
		dbms_output.put_line('cp \$COMMON_MOUNT_POINT/LOG/MigratorCorruptFiles.log \$COMMON_MOUNT_POINT/LOG/MigratorCorruptFiles.log_' || SYSDATE ) ;
		dbms_output.put_line('rm \$COMMON_MOUNT_POINT/LOG/MigratorCorruptFiles.log') ;
		dbms_output.put_line('fi ') ;
        for record in (select * from in_memory_counter_details where PR_FILE_PATH like '%Participated%' and COUNTER_CATEGORY != 4 )
        loop
			begin
				if (record.COUNTER_CATEGORY <> 5 )   
				then
					dbms_output.put_line('cp -r '||record.PR_FILE_PATH ||' ' || record.PR_FILE_PATH || '_' || SYSDATE ) ;
					dbms_output.put_line(' status=\$? ' ) ;
					dbms_output.put_line(' if [ \$status -ne 0 ] ') ;
					dbms_output.put_line('then ') ;
					dbms_output.put_line('echo "Failed to take the backup for '|| record.PR_FILE_PATH || ' and hence migrator is not run ' || '"') ;
					dbms_output.put_line('else ') ;

					dbms_output.put_line('declare -A pids ') ;
		       		for l_i in 0 .. record.NUMBER_OF_SLAVES -1 
			   		loop
					dbms_output.put_line('echo " Migrating Online PR Store ' || record.PR_FILE_PATH || ' For Slave ' || l_i || '"' ) ;
					dbms_output.put_line('./prstoremigrator -s '||record.PR_FILE_PATH ||' -d ' || record.PR_FILE_PATH || ' -p ' || l_i ||	' -h ' || record.PR_HASH_BUCKET_SIZE || ' &'  ) ;
					dbms_output.put_line('pid=\$! ') ;
					dbms_output.put_line('pids[\$pid]=' || l_i) ;
	        		end loop ;

					dbms_output.put_line('for pid in "\${!pids[@]}"; do ') ;
					dbms_output.put_line('wait \$pid ') ;
					dbms_output.put_line('statuscode=\$? ') ;
					dbms_output.put_line('if [ \$statuscode -ne 0 ] ') ;
					dbms_output.put_line('then ') ;
					dbms_output.put_line('echo "Failed to migrate for '|| record.PR_FILE_PATH ||' For slave \${pids[\$pid]}' || '"') ; 
					dbms_output.put_line('fi ') ;
					dbms_output.put_line('if ! ps "\$pid" >/dev/null; then ') ;
					dbms_output.put_line('unset pids[\$pid] ') ;
					dbms_output.put_line('fi ') ;
					dbms_output.put_line('done ') ;

					dbms_output.put_line(' fi ') ;


				else
					dbms_output.put_line('cp -r '||record.PR_FILE_PATH ||' ' || record.PR_FILE_PATH || '_' || SYSDATE ) ;
					dbms_output.put_line(' status=\$? ' ) ;
					dbms_output.put_line(' if [ \$status -ne 0 ] ') ;
					dbms_output.put_line('then ') ;
					dbms_output.put_line('echo "Failed to take the backup for '|| record.PR_FILE_PATH || ' and hence migrator is not run ' || '"') ;
					dbms_output.put_line('else ') ;

					dbms_output.put_line('declare -A pids ') ;
		       		for l_i in 0 .. record.NUMBER_OF_PARTITIONS_PER_SLAVE -1 
			   		loop
					dbms_output.put_line('echo " Migrating Offline PR Store ' || record.PR_FILE_PATH || ' For Partition ' || l_i || '"' ) ;
					dbms_output.put_line('./prstoremigrator -s '||record.PR_FILE_PATH ||' -d ' || record.PR_FILE_PATH || ' -p ' || l_i ||	' -h ' || record.PR_HASH_BUCKET_SIZE || ' &'  ) ;
					dbms_output.put_line('pid=\$! ') ;
					dbms_output.put_line('pids[\$pid]=' || l_i) ;
	        		end loop ;

					dbms_output.put_line('for pid in "\${!pids[@]}"; do ') ;
					dbms_output.put_line('wait \$pid ') ;
					dbms_output.put_line('statuscode=\$? ') ;
					dbms_output.put_line('if [ \$statuscode -ne 0 ] ') ;
					dbms_output.put_line('then ') ;
					dbms_output.put_line('echo "Failed to migrate for '|| record.PR_FILE_PATH ||' For slave \${pids[\$pid]}' || '"') ; 
					dbms_output.put_line('fi ') ;
					dbms_output.put_line('if ! ps "\$pid" >/dev/null; then ') ;
					dbms_output.put_line('unset pids[\$pid] ') ;
					dbms_output.put_line('fi ') ;
					dbms_output.put_line('done ') ;

					dbms_output.put_line(' fi ') ;

				end if ;
				end ;
		end loop ;
		dbms_output.put_line('if [ -f \$COMMON_MOUNT_POINT/LOG/MigratorCorruptFiles.log ] ; then ') ;
		dbms_output.put_line('echo "Please refer the log file \$COMMON_MOUNT_POINT/LOG/MigratorCorruptFiles.log for the list of files that are not migrated "') ;
		dbms_output.put_line('fi ') ;
		commit ;
	end ;
/
EOF
	if [ $? -ne 0 ]
	then
		echo " PR Generator failed with status $? "
        exit $?
    fi
	
	echo "#!/bin/env bash" > prmigrator.sh
	cat $COMMON_MOUNT_POINT/LOG/prmigrate.log >> prmigrator.sh

	chmod +x prmigrator.sh 
	echo " PR Generator Successful generated prmigrator.sh "
	echo " Ensure that prstoremigrator is copied into the current folder ( Source Location NIKIRASOURCE/Server/Processor/InMemoryCounterDebugger ) " 
	echo " Usage: ./prmigrator.sh " 

	if [ $? -ne 0 ]
	then
        exit $?
    fi
}

main ()
{
		prmigratorScriptGenerator
}

main $* 
