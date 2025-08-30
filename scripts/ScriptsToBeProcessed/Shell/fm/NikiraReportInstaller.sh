#/bin/bash
. ~/.bashrc
Dat=`date +%Dr%H:%M:%S`
logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NBIT_ReportInstall.log" }'`
echo "logFile = $logFile"
Installation_Time=`date +%Dr#%H:%M:%S`;
NBIT_HOME=`pwd`;

#----------------------------------------Customized Report Installation uncomment the following and set appropriate paths and change in -----
#export JPORT=3030
#export REPORT_SERVER_PORT=9192
#export REPORT_DIR=$NIKIRA_INSTALL_DIR/NIKIRATOOLS/REPORTSDIRRRR
#export CUST_REPORT_DIR=$NIKIRA_INSTALL_DIR/NIKIRATOOLS/CUSTREPORTSRRR
#export MYSQL_SOCKET_FILE=$JASPER_SERVER_HOME/mysql/tmp/mysql.sock
#export MYSQL_ROOT_PASSWORD=root
#export MYSQL_HOME=$NIKIRA_INSTALL_DIR/NIKIRATOOLS/MYSQL_HOMERRR
#export USE_DEFAULT_MYSQL_FOR_JSERV=1
#export USE_DEFAULT_JAVA_FOR_JSERV=1
#export JASPER_SERVER_HOME=$NIKIRA_INSTALL_DIR/NIKIRACLIENT/jasperserver
#------------------------------------------------------------------------------------------------------------------------------------------#

#-----------------------------------------------------Functions Start----------------------------------------------------------------------------#
function echo_log()
{
    echo "$@"
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function log_only()
{
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function ExportVariables
{
		echo_log "Exporting the variables";
	#	export REPORT_DIR=$NIKIRA_INSTALL_DIR/Reports
	#	export CUST_REPORT_DIR=$NIKIRA_INSTALL_DIR/Reports
		export CONNECTION_STRING="jdbc:oracle:thin:\@$IP_ADDRESS:1521:$ORACLE_SID"
}

function CheckCustReportAvail
{
		CR=0;
		if [ "$REPORT_DIR" == "$CUST_REPORT_DIR" ]
		then
			echo_log "No Seperate Customer Report Folder Available...."; 	
			CR=1;
		fi;	
}
function RangerDataSourceXMLChanges
{
		echo_log "RangerDataSourceXMLChanges Started ";
		cd $REPORT_DIR/ji-catalog/resources/RangerReports/datasource
		perl -pi -e "s/\<connectionUser.*/\<connectionUser\>$DB_USER\<\/connectionUser\>/g" RangerDataSource.xml
		perl -pi -e "s/\<connectionPassword.*/\<connectionPassword\>$DB_PASSWORD\<\/connectionPassword\>/g" RangerDataSource.xml
		perl -pi -e "s/\<connectionUrl.*/\<connectionUrl\>$CONNECTION_STRING\<\/connectionUrl\>/g" RangerDataSource.xml
		echo_log "RangerDataSourceXMLChanges Finished ";

		if [ $CR -eq 0 ]
		then
				echo_log "RangerDataSourceXMLChanges ifor Customized Reports Started ";
				cd $CUST_REPORT_DIR/ji-catalog/resources/RangerReports/datasource
				perl -pi -e "s/\<connectionUser.*/\<connectionUser\>$DB_USER\<\/connectionUser\>/g" RangerDataSource.xml
				perl -pi -e "s/\<connectionPassword.*/\<connectionPassword\>$DB_PASSWORD\<\/connectionPassword\>/g" RangerDataSource.xml
				perl -pi -e "s/\<connectionUrl.*/\<connectionUrl\>$CONNECTION_STRING\<\/connectionUrl\>/g" RangerDataSource.xml
				echo_log "RangerDataSourceXMLChanges for Customized Reports Finished ";
		fi;		
}

function JasperPortChanges
{
		echo_log "JasperPortChanges Started ";
		cd $JASPER_SERVER_HOME
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" apache-tomcat/webapps/jasperserver/META-INF/context.xml
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" apache-tomcat/webapps/jasperserver/META-INF/context.xml
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" apache-tomcat/webapps/jasperserver/WEB-INF/jsp/reportDataSourceFlow/jdbcPropsForm.jsp
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" apache-tomcat/webapps/jasperserver/WEB-INF/jsp/olapDataSourceFlow/jdbcPropsForm.jsp
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/ji-export-util/jdbc-mysql.properties
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/ji-export-util/jdbc.properties
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/installer/mysql/myscript.sh
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/config/js.jdbc.properties
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/ji-catalog/resources/olap/datasources/FoodmartDataSource.xml
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/ji-catalog/resources/olap/datasources/SugarCRMDataSource.xml
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/ji-catalog/resources/datasources/JServerJdbcDS.xml
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/js-catalog/resources/datasources/JServerJdbcDS.xml
		perl -pi -e "s#localhost\:\d+/#localhost\:$JPORT/#g" scripts/jasperAccess.sh
		echo_log "JasperPortChanges Finished ";
}

function ReportServerPortChanges
{
		echo_log "ReportServerPortChanges Started ";
		cd $JASPER_SERVER_HOME/apache-tomcat/conf
		perl -pi -e "s#Connector port=\"\d+\" URIEncoding#Connector port=\"$REPORT_SERVER_PORT\" URIEncoding#g" server.xml
		$SQL_COMMAND $DB_USER/$DB_PASSWORD$DB_CONNECTION_STRING<< EOF
		whenever sqlerror exit 5 ;
		set feedback off ;
		update configurations set value='$REPORT_SERVER_PORT' where config_key='REPORT_SERVER_PORT' ;
		commit;
EOF
		if [ $? -eq 5 ]
		then
		  echo "Error updating REPORT_SERVER_PORT.DBsetup might not Ran. please update the port after this installation....(Skipping and Moving Forward)"
		fi
		echo_log "ReportServerPortChanges Finished ";
}

function KillingOldMysqlProcess
{
		echo_log "KillingOldMysqlProcess Started ";
		ps -u $LOGNAME | grep mysqld_safe|awk '{print $1}'|xargs kill -9
		ps -u $LOGNAME | grep mysqld|awk '{print $1}'|xargs kill -9
		echo_log "KillingOldMysqlProcess Finished ";
}

function StartMYSQL
{
		echo_log "Trying to Start StartMYSQL ";
		cd $MYSQL_HOME
		 rm data/*-bin*
		./bin/safe_mysqld --port=$JPORT --socket=$MYSQL_HOME/tmp/mysql.sock --old-passwords --datadir=$MYSQL_HOME/data --log-error=$MYSQL_HOME/data/mysqld.log --pid-file=$MYSQL_HOME/data/mysqld.pid --default-table-type=InnoDB &
		sleep 10
		echo "My process sql status :"
		ps -u $LOGNAME | grep mysqld
		MySQLStatus=`ps -u $LOGNAME | grep mysqld|wc -l`

		if [ $MySQLStatus -ne 2 ]
		then
				echo "My SQL not started.......";
				exit 6;
		fi		
		echo_log "MYSQL Started";
}


function JasperReStart
{
		echo_log "Starting Jasper Server";
		cd $JASPER_SERVER_HOME
		./jasperctl.sh restart
}

function CleanMYSqlDatabase
{
		echo_log "Cleaning MYSQL Database "
		cd $JASPER_SERVER_HOME/scripts
		./jasperserver_mysql_setup.sh
		echo_log "Cleaning is Over "
}

function ImportReports
{
		echo_log "Importing Reports"
		if [ $CR -eq 0 ]
		then
			echo_log "Importing Customized Reports"
			./js-import.sh --input-dir $CUST_REPORT_DIR/ji-catalog/
			echo_log "Customized Reports Imported"
		fi;		
		./js-import.sh --input-dir $REPORT_DIR/ji-catalog/
		echo_log "Reports Imported"
}

function InstallNikiraReports
{
		ExportVariables
		CheckCustReportAvail
		RangerDataSourceXMLChanges
		JasperPortChanges
		ReportServerPortChanges
		KillingOldMysqlProcess
		StartMYSQL
		JasperReStart
		CleanMYSqlDatabase
		ImportReports
}

#-----------------------------------------------------Functions End----------------------------------------------------------------------------#

InstallNikiraReports
