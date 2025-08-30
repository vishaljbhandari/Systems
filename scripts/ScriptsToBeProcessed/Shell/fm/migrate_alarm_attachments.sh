#!/bin/bash

echo "Migrating Attachments..."
echo "spool migrate_attachments.log" > migrate_attachments.sql
for filename in `ls -1 $RANGER5_4HOME/Attachments/Alarm`
do
	tmp_file=`basename $filename`
	alarm_id=`echo $tmp_file | awk 'BEGIN { FS = "_"} {print $1}'`
	file_name=`echo $tmp_file | awk 'BEGIN { FS = "_"} { print substr($0, index($0,"_")+1) }'`

	mkdir -p $NIKIRACLIENT/public/Attachments/Alarm/$alarm_id

	cp $RANGER5_4HOME/Attachments/Alarm/$filename $NIKIRACLIENT/public/Attachments/Alarm/$alarm_id/$file_name

	echo "INSERT INTO ALARM_ATTACHMENTS(ID, ALARM_ID, FILE_NAME) VALUES(alarm_attachments_seq.nextval, $alarm_id, '$file_name') ;" >> migrate_attachments.sql
done

echo "commit ; " >> migrate_attachments.sql
echo "quit ; " >> migrate_attachments.sql

echo "Running Alarm Attachment Migration Script..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_attachments.sql 
echo "Migration completed"
