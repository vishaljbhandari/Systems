
sed -i "s/Ru575ModificationTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Ru575ModificationTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Ru575CreatedTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/g" seed_data.xml
sed -i "s/Ru575CreatedTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//g" seed_data.xml


sed -i "s/Th0128ModifiedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Th0128ModifiedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/g" seed_data.xml


sed -i "s/Sg7212ModifiedTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Sg7212ModifiedTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Sg7212CreatedTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/g" seed_data.xml
sed -i "s/Sg7212CreatedTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//g" seed_data.xml


sed -i "s/Rt3180DatasetStartTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Rt3180DatasetStartTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Rt3180DatasetEndTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/g" seed_data.xml
sed -i "s/Rt3180DatasetEndTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//g" seed_data.xml


sed -i "s/Rt0275RunEndTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Rt0275RunEndTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Rt0275RunStartTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/g" seed_data.xml
sed -i "s/Rt0275RunStartTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//g" seed_data.xml

sed -i "s/Ca547CreatedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Ca547CreatedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Ca547ModifiedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Ca547ModifiedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//" seed_data.xml
sed -i "s/Ft1135ModificationTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Ft1135ModificationTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Mh659CreatedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Mh659CreatedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Mh977LastArchiveDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Mh977LastArchiveDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Or7161IndexingStartDate=\"1970-01-01 00:00:00.0\"/LOOKUP=\"Or7161IndexingStartDate|Y|to_char(to_date('01-01-1970','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')#Or7161PartitionDate|Y|to_char(to_date('01-01-1970','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')#Or7161InsertionStartDate|Y|to_char(to_date('01-01-1970','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')#Or7161InsertionEndDate|Y|to_char(to_date('01-01-1970','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')#Or7161IndexingEndDate|Y|to_char(to_date('01-01-1970','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')\"/" seed_data.xml

sed -i  "s/Or7161PartitionDate=\"1970-01-01 00:00:00.0\"//" seed_data.xml
sed -i  "s/Or7161InsertionStartDate=\"1970-01-01 00:00:00.0\"//" seed_data.xml
sed -i  "s/Or7161InsertionEndDate=\"1970-01-01 00:00:00.0\"//" seed_data.xml
sed -i  "s/Or7161IndexingEndDate=\"1970-01-01 00:00:00.0\"//" seed_data.xml
sed  -i "s/Pa9134PasswordDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Pa9134PasswordDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed  -i "s/Pi9187LastRunRefTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Pi9187LastRunRefTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/"  seed_data.xml
sed  -i "s/Pl4148CreatedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Pl4148CreatedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Pl4148ModifiedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i  "s/Pl4148ModifiedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//" seed_data.xml
sed -i  "s/Th0128ModifiedDate=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Th0128ModifiedDate|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/"   seed_data.xml
sed  -i "s/Sg7212ModifiedTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"/LOOKUP=\"Sg7212ModifiedTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')#Sg7212CreatedTime|Y|to_char(sysdate,'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i  "s/Sg7212CreatedTime=\"[0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*.[0-9]*\"//" seed_data.xml
sed -i "s/Hl3153HolidayDate=\"2000-01-26 00:00:00.0\"/LOOKUP=\"Hl3153HolidayDate=|Y||to_char(to_date('26-01-2000','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Hl3153HolidayDate=\"2000-08-15 00:00:00.0\"/LOOKUP=\"Hl3153HolidayDate=|Y|to_char(to_date('15-08-2000','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Hl3153HolidayDate=\"2000-10-02 00:00:00.0\"/LOOKUP=\"Hl3153HolidayDate=|Y|to_char(to_date('02-10-2000','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')\"/" seed_data.xml
sed -i "s/Hl3153HolidayDate=\"2000-05-01 00:00:00.0\"/LOOKUP=\"Hl3153HolidayDate=|Y|to_char(to_date('01-05-2000','dd-mm-yyyy'),'yyyyMMdd HH:mi:ss')\"/" seed_data.xml

sed -i "s#Re9245LanguageKey=\"#La987Key=\"#g" seed_data.xml
sed -i "s#FpConfigurations Fc7193EntityId=\"#FpConfigurations Fc6138Id=\"#g" seed_data.xml

#removed for lookup changes
#sed -i 's#RegularExpressions Re9245D#RegularExpressions La987Key="" Re9245D#g' seed_data.xml
sed -i 's#FilterTokens Ft3154ExpressionId="" Fi789#FilterTokens Ex1163Id="" Fi789#g' seed_data.xml





sed  's/\&/\&amp;/' $1 > seed_noamp.xml
sed '{s/</\&lt;/2}' seed_noamp.xml > seed_nolt.xml
rev seed_nolt.xml | sed '{s/>/;tg\&/2}' | rev > $2

