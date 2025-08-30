sqlplus -s $REF_DB_USER/$REF_DB_PASSWORD@$REF_DB_SID<<EOF 2>&1

SET ECHO            OFF
SET HEADING         OFF
SET FEEDBACK        OFF
SET LINESIZE        1024
SET WRAP            OFF
SET TRIMSPOOL       ON
SET TERM            OFF
SET SQLBLANKLINES   OFF
SET PAGESIZE        0

update changed_class set CDC_LAST_CHANGED_DTTM = sysdate where CDC_CLASS = 'com.azure.certo.diamond.parse.expression.SubscriberCache';

EOF
