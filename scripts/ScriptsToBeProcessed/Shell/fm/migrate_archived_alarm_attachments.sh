#!/bin/bash

echo "Migrating Archived Alarm Attachments..."
echo "spool migrate_archived_alarm_attachments.log" > migrate_archived_alarm_attachments.sql
for filename in `ls -1 $RANGER5_4HOME/Attachments/ArchivedAlarm`
do
	tmp_file=`basename $filename`
	alarm_id=`echo $tmp_file | awk 'BEGIN { FS = "_"} {print $1}'`
	file_name=`echo $tmp_file | awk 'BEGIN { FS = "_"} { print substr($0, index($0,"_")+1) }'`

	mkdir -p $NIKIRACLIENT/public/Attachments/Alarm/$alarm_id

	cp $RANGER5_4HOME/Attachments/ArchivedAlarm/$filename $NIKIRACLIENT/public/Attachments/Alarm/$alarm_id/$file_name

	echo "INSERT INTO ARCHIVED_ALARM_ATTACHMENTS(ID, ALARM_ID, FILE_NAME) VALUES(alarm_attachments_seq.nextval, $alarm_id, '$file_name') ;" >> migrate_archived_alarm_attachments.sql
done

echo "commit ; " >> migrate_archived_alarm_attachments.sql
echo "quit ; " >> migrate_archived_alarm_attachments.sql

echo "Running Archived Alarm Attachment Migration Script..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_archived_alarm_attachments.sql 
echo "Migration completed"
