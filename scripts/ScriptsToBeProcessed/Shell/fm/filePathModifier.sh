#! /bin/bash

if [ $# -lt 8 ]
then
        echo "Usage  $0 <TableSpacePrefix> <CommonMountPoint> <Tablespacepath> <RangerRoot> <FmDBScriptsPath> <ROC_FM_DEPLOY_PATH> <CoreFilePath> <GdoFilePath> <CreateTableSpace>"
    exit 2;
fi


	if [ ! -d $2 ] 
	then
	echo "$2 path mention does not exist"
	exit 2;
	fi;

	if [ ! -d $3 ] 
	then
        echo "$3 path mention does not exist"    
        exit 3;
        fi;
	
	if [ ! -d $4 ]
	then
        echo "$4 path mention does not exist"    
        exit 4;
        fi;

	if [ ! -d $5 ]
        then
        echo "$5 path mention does not exist"    
        exit 5;
        fi;


    if [ ! -d $6 ]
        then
        echo "$6 path mention does not exist"    
        exit 5;
        fi;


    if [ ! -d $7 ]
        then
        echo "$7 path mention does not exist"    
        exit 5;
        fi;


    if [ ! -d $8 ]
        then
        echo "$8 path mention does not exist"    
        exit 5;
        fi;

    if [ ! "$9" ]
		then
        echo "$9 create tablespace option not specified"    
        exit 5;
        fi;



oldTableSPacePrefix="@TblSpacePrefix@"
oldCommountMountPointPath="@common_mount_point@"
oldTableSpacePath="@tableSpacePath@"

newPrefix=$1
commonMountPointPathCheck=$2

tablespacepathCheck=$3
rangerRootPathCheck=$4
dbScriptsPathCheck=$5
ROC_FM_DEPLOY_PATH_check=$6
coreFilePathCheck=$7
gdoFilePathCheck=$8
CreateTableSpace=$9
tablespacepath=$(echo $tablespacepathCheck|sed -e 's#/$##g')
commonMountPointPath=$(echo $commonMountPointPathCheck|sed -e 's#/$##g')
rangerRootPath=$(echo $rangerRootPathCheck|sed -e 's#/$##g')
dbScriptsPath=$(echo $dbScriptsPathCheck|sed -e 's#/$##g')
coreFilePath=$(echo $coreFilePathCheck|sed -e 's#/$##g')
gdoFilePath=$(echo $gdoFilePathCheck|sed -e 's#/$##g')
ROC_FM_DEPLOY_PATH=$(echo $ROC_FM_DEPLOY_PATH_check|sed -e 's#/$##g')

function ValidatePath
{

if [ ! -f $coreFilePath/xmlfiles/V830_SP0/fms_core_schema_v1.xml.parse.in ] 
then
echo "$coreFilePath/xmlfiles/V830_SP0/fms_core_schema_v1.xml.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $gdoFilePath/xmlfiles/V830_SP0/fms_gdo_schema_v1.xml.parse.in ] 
then
echo "$gdoFilePath/xmlfiles/V830_SP0/fms_gdo_schema_v1.xml.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $coreFilePath/xmlfiles/V830_SP0/fms_core_tablespace_data.xml.parse.in ] 
then
echo "$coreFilePath/xmlfiles/V830_SP0/fms_core_tablespace_data.xml.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $gdoFilePath/sqlfiles/V830_SP0/gdo_procedure_definitions.sql.parse.in ] 
then
echo "$gdoFilePath/sqlfiles/V830_SP0/gdo_procedure_definitions.sql.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $coreFilePath/sqlfiles/V830_SP0/temporaryCounterTables.sql.parse.in ] 
then
echo "$coreFilePath/sqlfiles/V830_SP0/temporaryCounterTables.sql.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $coreFilePath/sqlfiles/V830_SP0/coreTableSpaceFileCreation.sql.parse.in ] 
then
echo "$coreFilePath/sqlfiles/V830_SP0/coreTableSpaceFileCreation.sql.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $coreFilePath/xmlfiles/V830_SP0/fms_core_seed_data.xml.parse.in ] 
then
echo "$coreFilePath/xmlfiles/V830_SP0/fms_core_seed_data.xml.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $gdoFilePath/sqlfiles/V830_SP0/gdo_packages_definitions.sql.parse.in ] 
then
echo "$gdoFilePath/sqlfiles/V830_SP0/gdo_packages_definitions.sql.parse.in does not exits"    
exit 1; 
fi;

if [ ! -f $gdoFilePath/sqlfiles/V830_SP0/gdo_directory_definitions.sql.parse.in  ] 
then
echo "$gdoFilePath/sqlfiles/V830_SP0/gdo_directory_definitions.sql.parse.in  does not exits"    
exit 1;
fi;


if [ ! -f $dbScriptsPath/spark_trend_summary_DDL_setup.sh.parse.in ] 
then
echo "$dbScriptsPath/spark_trend_summary_DDL_setup.sh.parse.in does not exits"    
exit 1;
fi;

if [ ! -f $dbScriptsPath/trend_summary_temp_tables.sql.parse.in ] 
then
echo "$dbScriptsPath/trend_summary_temp_tables.sql.parse.in does not exits"    
exit 1;
fi;


}

ValidatePath


    sed "s#@nikira.deploy.dir@#$ROC_FM_DEPLOY_PATH#g" $ROC_FM_DEPLOY_PATH/metadata/fms_core.ciw.parse.in > $ROC_FM_DEPLOY_PATH/metadata/fms_core.ciw
    sed "s#@nikira.deploy.dir@#$ROC_FM_DEPLOY_PATH#g" $ROC_FM_DEPLOY_PATH/metadata/fms_gdo.ciw.parse.in > $ROC_FM_DEPLOY_PATH/metadata/fms_gdo.ciw
    sed "s#@nikira.deploy.dir@#$ROC_FM_DEPLOY_PATH#g" $ROC_FM_DEPLOY_PATH/metadata/spark_core.ciw.parse.in > $ROC_FM_DEPLOY_PATH/metadata/spark_core.ciw

#tablespace switch-off/on
	if [[  "$CreateTableSpace" == "Y"  ||  "$CreateTableSpace" == "y"  ]] ;
	then
	sed -i 's#RunTableSpaceCreation="[a-z]*"#RunTableSpaceCreation="true"#g' $ROC_FM_DEPLOY_PATH/metadata/fms_gdo.ciw
	sed -i 's#RunTableSpaceCreation="[a-z]*"#RunTableSpaceCreation="true"#g' $ROC_FM_DEPLOY_PATH/metadata/fms_core.ciw
	else
	sed -i 's#RunTableSpaceCreation="[a-z]*"#RunTableSpaceCreation="false"#g' $ROC_FM_DEPLOY_PATH/metadata/fms_core.ciw
	sed -i 's#RunTableSpaceCreation="[a-z]*"#RunTableSpaceCreation="false"#g' $ROC_FM_DEPLOY_PATH/metadata/fms_gdo.ciw

	fi
    sed "s#@nikira.deploy.dir@#$ROC_FM_DEPLOY_PATH#g" $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core/xmlfiles/V830_SP0/rawClobTableCreation.xml.parse.in > $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core/xmlfiles/V830_SP0/rawClobTableCreation.xml
    sed "s#@nikira.deploy.dir@#$ROC_FM_DEPLOY_PATH#g" $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core/xmlfiles/V830_SP0/preSeedSQLs.xml.parse.in > $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core/xmlfiles/V830_SP0/preSeedSQLs.xml
    sed "s#@nikira.deploy.dir@#$ROC_FM_DEPLOY_PATH#g" $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core/xmlfiles/V830_SP0/postSeedSQLs.xml.parse.in > $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core/xmlfiles/V830_SP0/postSeedSQLs.xml








sed "s/$oldTableSPacePrefix/$newPrefix/gi " $coreFilePath/xmlfiles/V830_SP0/fms_core_schema_v1.xml.parse.in > $coreFilePath/xmlfiles/V830_SP0/fms_core_schema_v1.xml
sed  "s/$oldTableSPacePrefix/$newPrefix/gi " $gdoFilePath/xmlfiles/V830_SP0/fms_gdo_schema_v1.xml.parse.in > $gdoFilePath/xmlfiles/V830_SP0/fms_gdo_schema_v1.xml
sed  "s/$oldTableSPacePrefix/$newPrefix/gi " $coreFilePath/xmlfiles/V830_SP0/fms_core_tablespace_data.xml.parse.in > $coreFilePath/xmlfiles/V830_SP0/fms_core_tablespace_data.xml
sed  "s/$oldTableSPacePrefix/$newPrefix/gi " $gdoFilePath/sqlfiles/V830_SP0/gdo_procedure_definitions.sql.parse.in > $gdoFilePath/sqlfiles/V830_SP0/gdo_procedure_definitions.sql
sed  "s/$oldTableSPacePrefix/$newPrefix/gi " $coreFilePath/sqlfiles/V830_SP0/temporaryCounterTables.sql.parse.in > $coreFilePath/sqlfiles/V830_SP0/temporaryCounterTables.sql

sed  "s/$oldTableSPacePrefix/$newPrefix/gi " $coreFilePath/sqlfiles/V830_SP0/coreTableSpaceFileCreation.sql.parse.in > $coreFilePath/sqlfiles/V830_SP0/coreTableSpaceFileCreation.sql

sed -i "s#$oldTableSpacePath#$tablespacepath#g " $coreFilePath/xmlfiles/V830_SP0/fms_core_tablespace_data.xml
sed -i "s#$oldCommountMountPointPath#$commonMountPointPath#g " $coreFilePath/sqlfiles/V830_SP0/coreTableSpaceFileCreation.sql
sed "s#$oldCommountMountPointPath#$commonMountPointPath#g " $coreFilePath/xmlfiles/V830_SP0/fms_core_seed_data.xml.parse.in > $coreFilePath/xmlfiles/V830_SP0/fms_core_seed_data.xml
sed "s#$oldCommountMountPointPath#$commonMountPointPath#g " $gdoFilePath/sqlfiles/V830_SP0/gdo_packages_definitions.sql.parse.in > $gdoFilePath/sqlfiles/V830_SP0/gdo_packages_definitions.sql
sed "s#$oldCommountMountPointPath#$commonMountPointPath#g " $gdoFilePath/sqlfiles/V830_SP0/gdo_directory_definitions.sql.parse.in > $gdoFilePath/sqlfiles/V830_SP0/gdo_directory_definitions.sql

existingRangePath="@RangerHomePath@"
sed -i "s#$existingRangePath#$rangerRootPath#gi" $coreFilePath/xmlfiles/V830_SP0/fms_core_seed_data.xml


sed  "s#$oldTableSPacePrefix#$newPrefix#g" $dbScriptsPath/spark_trend_summary_DDL_setup.sh.parse.in > $dbScriptsPath/spark_trend_summary_DDL_setup.sh
sed -i "s#$oldTableSpacePath#$tablespacepath#g" $dbScriptsPath/spark_trend_summary_DDL_setup.sh
sed "s#$oldTableSPacePrefix#$newPrefix#g" $dbScriptsPath/trend_summary_temp_tables.sql.parse.in > $dbScriptsPath/trend_summary_temp_tables.sql

chmod +x $dbScriptsPath/spark_trend_summary_DDL_setup.sh
chmod 777 $tablespacepath

