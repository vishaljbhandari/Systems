#! /bin/bash

partition_name="P247"

echo "Started Deleting duplicates at `date`" >>delete_duplicate_entries.log
sqlplus $DB_USER/$DB_PASSWORD <<EOF >>delete_duplicate_entries.log

create table delete_duplicate_entries (partition_name varchar2(50), batch_count number(10), batch_time date);

set serveroutput on ;
set feedback on;
declare
    commit_interval             number(10) ;
	total_entries_deleted		number(20) ;

    cursor cdr_cursor is
        select rid from ( select rowid rid,row_number() over (partition by CALLER_NUMBER, CALLED_NUMBER, TIME_STAMP, DURATION, SERVICE_TYPE, RECORD_TYPE order by rowid) rn
    from cdr partition($partition_name)) where rn <> 1;

    CDRRecord   cdr_cursor%ROWTYPE;

begin
    commit_interval := 0;
	total_entries_deleted := 0;
    open cdr_cursor;
    loop
        fetch cdr_cursor into CDRRecord;
            exit when cdr_cursor%NOTFOUND;

        delete from cdr partition($partition_name) where rowid=CDRRecord.rid ;
        commit_interval := commit_interval + 1;
		total_entries_deleted := total_entries_deleted + 1;
        if(commit_interval = 5000)
        then
			insert into cdr_dup_deletion_counter values('$partition_name',total_entries_deleted,sysdate);
            commit_interval := 0;
            commit;
        end if;
    end loop;
    close cdr_cursor;

	commit;
end;
/
EOF
if [ $? -ne 0 ]
then
    echo "Failed to delete duplicate entries from table `date`" >>delete_duplicate_entries.log
else
    echo "Deleted duplicate entries from table `date`" >>delete_duplicate_entries.log
fi

