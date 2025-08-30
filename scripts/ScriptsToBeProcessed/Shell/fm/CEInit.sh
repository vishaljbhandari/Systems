#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
echo Killing existing programmanager and rangercse for this user
. $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh


export PATH=$PATH:$RANGER_HOME/sbin:$RANGER_HOME/bin

echo Setting up initial database
cd $DBSETUP_HOME
echo Running dbsetup.sh

if [ $DB_TYPE == "1" ]
then
	./dbsetup.sh $DB_SETUP_USER $DB_SETUP_PASSWORD
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/Scripts/index.sql
	
	cd $RANGERV6_SERVER_HOME
	echo running demosetup.sh
	cp $RANGERV6_SERVER_HOME/Scripts/customnicknamesetup.sh $RANGERHOME/bin/customnicknamesetup.sh
	


	rm -rf $WATIR_SERVER_HOME/data
	mkdir $WATIR_SERVER_HOME/data
	rm -rf $WATIR_SERVER_HOME/databackup
	mkdir $WATIR_SERVER_HOME/databackup
	mkdir -p $WATIR_SERVER_HOME/database
	rm -rf $WATIR_SERVER_HOME/Scripts/Fixture
	mkdir -p $WATIR_SERVER_HOME/Scripts/Fixture/tmp
	mkdir -p $WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles
	############################
	#if [ $IS_CDM_ENABLED -eq 1 ]
	#then
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DataSourceCDRData
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DataSourceGPRSData
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DataSourceRechargeData
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DiamondXMLs
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/Temp
	
	#    bash $WATIR_SERVER_HOME/Scripts/addingCDM_ATxml_Lookup.sh
	#    bash $WATIR_SERVER_HOME/Scripts/update_spark_configurations.sh
	#fi
	############################
	find $COMMON_MOUNT_POINT/FMSData/SubscriberDataRecord/Processed/ -name '*.*' -exec rm -f {} \;
	find $COMMON_MOUNT_POINT/FMSData/DataRecord/Processed/ -name '*.*' -exec rm -f {} \;
	find $RANGERHOME/LOG/ -name '*.*' -exec rm -f {} \;
	
	cp $WATIR_SERVER_HOME/Scripts/statsrule-*.conf $RANGERHOME/rbin/statistical_rule_evaluator/configurations/ #Copying the stats rule conf files to server

	echo Creating the dbce.sql and dbcecreatebackuptables.sql
	bash $WATIR_SERVER_HOME/Scripts/dbcecreatebacktables.sh
else

	./dbsetup.sh $DB2_SERVER_USER $DB2_SERVER_PASSWORD
	
	# clpplus -nw -s $DB_USER/$DB_PASSWORD@$DB2_HOST:$DB2_PORT/$DB2_DBNAME @$WATIR_SERVER_HOME/sqlfile.sql
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF > /dev/null
	@$WATIR_SERVER_HOME/Scripts/index.sql
EOF
	cd $RANGERV6_SERVER_HOME
	echo running demosetup.sh
	cp $RANGERV6_SERVER_HOME/Scripts/customnicknamesetup.sh $RANGERHOME/bin/customnicknamesetup.sh
	


	rm -rf $WATIR_SERVER_HOME/data
	mkdir $WATIR_SERVER_HOME/data
	rm -rf $WATIR_SERVER_HOME/databackup
	mkdir $WATIR_SERVER_HOME/databackup
	mkdir -p $WATIR_SERVER_HOME/database
	rm -rf $WATIR_SERVER_HOME/Scripts/Fixture
	mkdir -p $WATIR_SERVER_HOME/Scripts/Fixture/tmp
	mkdir -p $WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles
	############################
	#if [ $IS_CDM_ENABLED -eq 1 ]
	#then
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DataSourceCDRData
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DataSourceGPRSData
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DataSourceRechargeData
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/DiamondXMLs
	#    mkdir -p $COMMON_MOUNT_POINT/FMSData/Temp
	
	#    bash $WATIR_SERVER_HOME/Scripts/addingCDM_ATxml_Lookup.sh
	#    bash $WATIR_SERVER_HOME/Scripts/update_spark_configurations.sh
	#fi
	############################
	find $COMMON_MOUNT_POINT/FMSData/SubscriberDataRecord/Processed/ -name '*.*' -exec rm -f {} \;
	find $COMMON_MOUNT_POINT/FMSData/DataRecord/Processed/ -name '*.*' -exec rm -f {} \;
	find $RANGERHOME/LOG/ -name '*.*' -exec rm -f {} \;
	
	cp $WATIR_SERVER_HOME/Scripts/statsrule-*.conf $RANGERHOME/rbin/statistical_rule_evaluator/configurations/ #Copying the stats rule conf files to server

	echo Creating the dbce.sql and dbcecreatebackuptables.sql
	bash $WATIR_SERVER_HOME/Scripts/dbcecreatebacktables.sh
fi
