FMS_ORACLE_HOSTNAME=10.113.58.86
FMS_ORACLE_PORT=1521
FMS_ORACLE_SID=$ORACLE_SID
FMS_DB_USER=$DB_USER
FMS_DB_PASSWORD=$DB_PASSWORD

HIBERNATE_CONFIG_TEMPLATE_PATH="./hibernate_fms.template.cfg.xml"

param_list="FMS_ORACLE_HOSTNAME FMS_ORACLE_PORT FMS_ORACLE_SID FMS_DB_USER FMS_DB_PASSWORD"

expression=""
for param in $param_list
do
    
    value=$(eval "echo $`echo $param`");
    expression="s/\$$param/$value/g; $expression";
    echo $expression
done

sed "$expression" $HIBERNATE_CONFIG_TEMPLATE_PATH>hibernate_fms.cfg.xml
