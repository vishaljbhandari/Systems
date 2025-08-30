#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
  echo Killing existing programmanager for this user
. $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh


export PATH=$PATH:$RANGER_HOME/sbin:$RANGER_HOME/bin

echo Setting up initial database
cd $GBX_DBSETUP_HOME
echo Running gbx_dbsetup.sh
./gbxusdbsetup.sh
cd $RANGERV6_SERVER_HOME
cp $RANGERV6_SERVER_HOME/Scripts/customnicknamesetup.sh $RANGERHOME/bin/customnicknamesetup.sh
echo Creating the dbce.sql and dbcecreatebackuptables.sql
bash $WATIR_SERVER_HOME/Scripts/dbcecreatebacktables.sh
cd 

rm -rf Watir/data
mkdir Watir/data
rm -rf Watir/databackup
mkdir Watir/databackup
mkdir -p Watir/database
rm -rf Watir/sitedata
mkdir -p Watir/sitedata

sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @Watir/Scripts/index.sql
