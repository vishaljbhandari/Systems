#!/bin/bash

        backUpTablePrefix="BACKUPTABLE_"
        dbce_sql_file_name="dbce.sql"

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
bash $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh
#bash $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh taskcontroller ### Killing Spark TaskController
bash $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh TaskControllerService ### Killing Spark TaskController
#bash $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh Bootstrap             ### Killing Tomcat     
. $WATIR_SERVER_HOME/Scripts/ClearRecords.sh >/dev/null 2>&1

#For Oracle Setup, Please keep the following values
noOfIterations=200
sleepTimePerIteration="0.5"

#FOr DB2 Setup, Please keep the following Values
#noOfIterations=40
#sleepTimePerIteration="2"

rm -f $WATIR_SERVER_HOME/Scripts/Fixture/tmp/flagToRunProgrammanager
rm -f $WATIR_SERVER_HOME/Scripts/Fixture/tmp/sequenceFlag.lst
rm -rf $RANGERV6_CLIENT_HOME/tmp/cache/*

										 
kill -HUP `ps -fu$LOGNAME | grep httpd | grep -v grep | tr -s " " | cut -d " " -f3 | grep -v "^1$" | uniq`        #restart apache


if [ $IS_CDM_ENABLED -eq 1 ]
then
	cd $SPARK_DEPLOY_DIR/bin && bash nohup_tc.sh > /dev/null &

	bash $WATIR_SERVER_HOME/Scripts/truncateUsageTables.sh
fi

#bash $APACHE_BIN/startup.sh

bash $WATIR_SERVER_HOME/Scripts/copyData_FromTables_ToTables.sh 1 $backUpTablePrefix >/dev/null &

if [ $# -eq 0 ]
then # if normal CESetup then run dbce.sql and programmanager
	
	if [ $DB_TYPE == "1" ]
		then
		sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/Scripts/$dbce_sql_file_name >/dev/null &
		else
		clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF  > /dev/null &
SPOOL $WATIR_SERVER_HOME/Scripts/Fixture/tmp/dbce.sql.lst ;
@$WATIR_SERVER_HOME/Scripts/$dbce_sql_file_name
SPOOL OFF;
SPOOL $WATIR_SERVER_HOME/Scripts/Fixture/tmp/sequenceFlag.lst ;
spool off ;
EOF
	fi
	
    while [ $noOfIterations -gt 0 ]
    do
	if [[ -e $WATIR_SERVER_HOME/Scripts/Fixture/tmp/flagToRunProgrammanager && -e $WATIR_SERVER_HOME/Scripts/Fixture/tmp/sequenceFlag.lst ]] ; then
		bash $WATIR_SERVER_HOME/Scripts/RunProgramManager.sh
		exit 0
	else
		sleep $sleepTimePerIteration
	fi

        noOfIterations=`expr $noOfIterations - 1`
    done

else # else run dbceFixtureLevel100.sql in backgrounda
 while [ $noOfIterations -gt 0 ]
    do
        if [[ -e $WATIR_SERVER_HOME/Scripts/Fixture/tmp/flagToRunProgrammanager ]] ; then
		break
        else
                sleep $sleepTimePerIteration
        fi
done
		
	if [ ! -f $WATIR_SERVER_HOME/Scripts/dbceFixtureLevel100.sql ] # if dbce_PostFixture.sql does not exixts then generate it
        then
        	echo "Generating dbceFixtureLevel100.sql ..."
            bash $WATIR_SERVER_HOME/Scripts/generateEntireSequenceCreationList.sh 100 100 #it results sequence increment by 100*100
        fi

if [ $DB_TYPE == "1" ]
	then
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/Scripts/dbceFixtureLevel100.sql >/dev/null 
	else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF >/dev/null &
	SPOOL $WATIR_SERVER_HOME/Scripts/Fixture/tmp/dbceFixtureLevel100.sql.lst ;
	@$WATIR_SERVER_HOME/Scripts/dbceFixtureLevel100.sql
	SPOOL OFF;
	SPOOL $WATIR_SERVER_HOME/Scripts/Fixture/tmp/sequenceFlag.lst ;
	spool off;
EOF
	fi	
fi
