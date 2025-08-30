#!/bin/bash


if [ "$#" -ne 1 ]; then
	echo "Usage: $0 ConfileName" >&2
	exit 1
fi


presentWrkinDir=`pwd`

confFile=$1
echo "Using confile:- "$confFile " --for installer run"


time_stamp=`date | awk '{print $4}' | awk -F":" '{print $1"_"$2"_"$3}'`
logFile=`date|awk '{ print  $3 "_" $2 "_" $6 "" }'`"_"$time_stamp"_NikiraWrapper.log"
echo "logFile = $logFile"

function echo_log()
{
time_stamp=`date | awk '{print $4}' | awk -F":" '{print $1"_"$2"_"$3}'`
   echo "$@"
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'`$time_stamp$@">>$LogPath/$logFile
}

WRAPPER_HOME=`pwd`;

if [ ! -d "$WRAPPER_HOME/Logs" ]; then
mkdir $WRAPPER_HOME/Logs
#else
#rm -rf $WRAPPER_HOME/Logs/*
fi;


LogPath="$WRAPPER_HOME/Logs"


echo_log "...........Started Wrapper Script Run..............";


		for i in `cat $confFile|grep -v "#"`
 	        do

        if [ $? -ne 0 ] ;
        then
                echo "Error : Please check input conf file . Exiting Run" 
                exit 1
        fi


			echo $i | grep -q '^SPARK::' 	
			if [ $? -eq 1 ] 
				then
					echo $i | grep -q '^FMS::'			
					if [ $? -eq 1 ]
		            	then
							export $i
            		fi	
			fi;	
		done;
		
sparkOperator=""
sparkLicenseKey=""
function ValidatePermission
{

# read creatablespace

       for i in `cat $WRAPPER_HOME/$confFile |grep -v "#"`
                do
                        echo $i | grep -q '^FMS::'
                        if [ $? -eq 0 ]
                        then
                        i=`echo $i |awk -F '::' '{print $2}'`
                        export $i
                        else

                        echo $i | grep -q '^SPARK::'
                        if [ $? -eq 1 ]
                        then
                        export $i
                        fi

                        fi;

                done;



Columns=()

		echo_log "######Validating DB Permission#####"
		    if [[  "$CREATE_TABLESPACE" == "Y"  ||  "$CREATE_TABLESPACE" == "y"  ]] ;
			    then
         			    Columns=(
                user_tablespaces
                dba_data_files
        all_indexes
        all_ind_columns
        user_cons_columns
        user_constraints
        all_cons_columns
        all_constraints
        all_sequences
        user_tab_columns
        user_clusters
        user_indexes
        all_tab_columns
        all_tables
                )


				else
				    Columns=(
                user_tablespaces
		        all_indexes
		        all_ind_columns
		        user_cons_columns
		        user_constraints
		        all_cons_columns
		        all_constraints
		        all_sequences
		        user_tab_columns
		        user_clusters
		        user_indexes
		        all_tab_columns
		        all_tables
                )

			fi


for column in ${Columns[@]}	
do
sqlplus -s $DB_USER/$DB_PASSWORD@$FMS_ORACLE_SID  <<EOF > /dev/null

whenever sqlerror exit 5;
whenever oserror exit 6 ;
set serveroutput on;
set feedback off;
declare

l_count number;

begin

select count(*) into l_count from  $column;
EXCEPTION
   WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE ( 'ERROR:'||SQLERRM );

end;
/

exit;

EOF
        if [ $? -ne 0 ] ;
        then
                echo "Error : Select permission are not there for this table $column or table does not exits" 
                exit 1
        fi

done

if [[  "$CREATE_TABLESPACE" == "N"  ||  "$CREATE_TABLESPACE" == "n"  ]] ;
                then


sqlplus -s $DB_USER/$DB_PASSWORD@$FMS_ORACLE_SID  <<EOF > /dev/null
whenever sqlerror exit 5;
whenever oserror exit 6 ;
set serveroutput on;
set feedback off;
declare

l_count number;
fm_metadata      EXCEPTION;
begin

select count(*) into l_count from user_tablespaces;

if l_count < 0
then
       RAISE fm_metadata ;

end if;

EXCEPTION

    when fm_metadata then
        DBMS_OUTPUT.PUT_LINE ( 'No tablespaces found ');
		RAISE fm_metadata ; 
   WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE ( 'ERROR:'||SQLERRM );

end;
/

exit;

EOF

  if [ $? -ne 0 ] ;
        then
                echo "No tablespaces are found , please run create tablespace before retrying" 
                exit 1
        fi



fi



}

function SparkInstallation
{
		
		echo_log "######SparkDBSetupStarted#####"
		for i in `cat $confFile|grep -v "#"`
 	        do
			echo $i | grep -q '^SPARK::' 	
			if [ $? -eq 0 ] 
			then
			i=`echo $i |awk -F '::' '{print $2}'`
                        export $i
			else

			echo $i | grep -q '^FMS::'
			if [ $? -eq 1 ]
			then
			export $i
			fi
			
			fi;	
		
                done;
	
		if [ ! -d $ROC_SPARK_DEPLOY_PATH ]
                then
                        echo_log "$ROC_SPARK_DEPLOY_PATH doesnt exists...!";
			exit;
                fi;
		
		if [ ! -d $ROC_SPARK_DEPLOY_PATH/config ]
				then
						echo_log "$ROC_SPARK_DEPLOY_PATH/deploy/config doesnt exists...!";
						echo_log "Please copy the generated deploy folder from ROC_FM_SPARK...!";
			exit;
				fi;


	        if [ ! -d $ROC_SPARK_DEPLOY_PATH/bin ]
                then
						echo_log "$ROC_SPARK_DEPLOY_PATH/deploy/bin doesnt exists...!";
			exit;
				fi;

		# Path Replacement #	
		cd $ROC_SPARK_DEPLOY_PATH/config
		

	    cp $presentWrkinDir/installer.properties.parse.in installer.properties
		if [ "$DATABASE_TYPE" = "Oracle" ]
		then
		sed -i '8s/#//' installer.properties
		sed -i "/DB_TYPE=Oracle/,/#DB_TYPE=Oracle RAC/s/#HOST_NAME=<Hostname>/HOST_NAME=$HOST_NAME/g" installer.properties
		sed -i "/DB_TYPE=Oracle/,/#DB_TYPE=Oracle RAC/s/#INSTANCE=<Instance>/INSTANCE=$SPARK_ORACLE_SID/g" installer.properties
		sed -i "/DB_TYPE=Oracle/,/#DB_TYPE=Oracle RAC/s/#PORT_NO=<Port Number>/PORT_NO=$PORT_NO/g" installer.properties
		sed -i "/DB_TYPE=Oracle/,/#DB_TYPE=Oracle RAC/s/#UNICODE=<Y\/N>/UNICODE=$UNICODE/g" installer.properties
		    
		elif [ "$DATABASE_TYPE" = "Oracle_RAC" ]
		then
		sed -i '24s/#//' installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#HOST_NAME=<Hostname>/HOST_NAME=$HOST_NAME/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#PORT_NO=<Port Number>/PORT_NO=$PORT_NO/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#SERVICE_NAME=<Service Name>/SERVICE_NAME=$SERVICE_NAME/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#UNICODE=<Y\/N>/UNICODE=$UNICODE/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#ONS_CONFIG=<hostname1:portNo,hostname2:portNo,..>/ONS_CONFIG=$ONS_CONFIG/g" installer.properties				
		fi
	
 
			
		sed -i "1,/#DIST_CACHE_HOST_NAME=<Hostname>/s/#DIST_CACHE_HOST_NAME=<Hostname>/DIST_CACHE_HOST_NAME=$DIST_CACHE_HOST_NAME/g" installer.properties		
		sed -i "1,/#DIST_CACHE_INSTANCE=<Instance>/s/#DIST_CACHE_INSTANCE=<Instance>/DIST_CACHE_INSTANCE=$FMS_ORACLE_SID/g" installer.properties
		sed -i "1,/#DIST_CACHE_PORT_NO=<Port Number>/s/#DIST_CACHE_PORT_NO=<Port Number>/DIST_CACHE_PORT_NO=$DIST_CACHE_PORT_NO/g" installer.properties


		sed -i "s/WORKSPACE_FILE=<workspace file name, e.g. : spark.ciw>/WORKSPACE_FILE=$WORKSPACE_FILE/g" installer.properties
		
		sed -i "s#SYS_OPERATOR_NAME:<Operator Name>#SYS_OPERATOR_NAME:$SYS_OPERATOR_NAME#g" installer.properties
		sed -i "s#SYS_LICENCE_KEY:<Licence Key>#SYS_LICENCE_KEY:$SYS_LICENCE_KEY#g" installer.properties
		sed -i "s/MACHINE_NAME=<Machine Name>/MACHINE_NAME=$MACHINE_NAME/g" installer.properties
		sed -i "s/HOST_ADDRESS=<Host Address>/HOST_ADDRESS=$HOST_ADDRESS/g" installer.properties
		sed -i "s#DATA_DIR=<Data Dir>#DATA_DIR=$DATA_DIR#g" installer.properties
		
	
		DistributedCacheChanges;

	        sparkOperator=$SYS_OPERATOR_NAME
		sparkLicenseKey=$SYS_LICENCE_KEY

		cd $ROC_SPARK_DEPLOY_PATH/bin
		chmod +x *.sh
		
		echo_log "Refer the path for logs:= $ROC_SPARK_DEPLOY_PATH/logs"
		./PreSilentInstallationTasks.sh  $SPARK_DB_USER $SPARK_DB_PASSWORD $DB_USER $DB_PASSWORD
	
	    if [ $? -ne 0 ] ;
        then
                echo "Error : Spark PreSilentInstaller Failed . Exiting Run" 
                exit 1
        fi

		
   	./SilentInstaller.sh
		
        if [ $? -ne 0 ] ;
        then
                echo "Error : Spark SilentInstaller Failed . Exiting Run" 
                exit 1
        fi


		echo_log "######SparkDBSetupFinished#####"				
	
}

function DistributedCacheChanges
{		

		cd $ROC_SPARK_DEPLOY_PATH/config/DistributedCacheManagerResources
	        sed -i "s#^SERVER_ADDRESS_LIST.*#SERVER_ADDRESS_LIST=$SERVER_ADDRESS_LIST#" distributed_cache_client.properties
	        sed -i "s#^ERROR_RETRY_COUNT.*#ERROR_RETRY_COUNT=$ERROR_RETRY_COUNT#" distributed_cache_client.properties
	        sed -i "s#^DEBUGDISTCACHE.*#DEBUGDISTCACHE=$DEBUGDISTCACHE#" distributed_cache_client.properties
	        sed -i "s#^CACHE_NAME.*#CACHE_NAME=$CACHE_NAME#" distributed_cache_client.properties

                sed -i "s#cluster=\".*\"#cluster=\"$CLUSTER_NAME\"#" distributed_cache_configuration.xml

}




function CreateDbLinks
{
	



	
		echo_log "######DB links Creation started#######"
		
		>dbfmlink.sql
		>dbsparklink.sql
		>dbusagelink.sql

        echo " whenever sqlerror exit 5;
				whenever oserror exit 6 ;
				set serveroutput on;
				set feedback off;
			" >> dbfmlink.sql;

        echo " whenever sqlerror exit 5;
                whenever oserror exit 6 ;
                set serveroutput on;
                set feedback off;
            " >> dbsparklink.sql;


        echo " whenever sqlerror exit 5;
                whenever oserror exit 6 ;
                set serveroutput on;
                set feedback off;
            " >> dbusagelink.sql;


       echo " declare v_count number; begin select  count(1) into v_count  from    USER_DB_LINKS  where   lower(DB_LINK)  = '$SPARK_DB_LINK' ; if v_count = 1 then    execute immediate ' drop database link $SPARK_DB_LINK  ';  end if; end;   " >> dbfmlink.sql
        echo "/" >> dbfmlink.sql
		   
		   echo " declare v_count number; begin select  count(1) into v_count  from    USER_DB_LINKS  where   lower(DB_LINK)  = '$SPARK_REF_TO_USG_DB_LINK' ; if v_count = 1 then    execute immediate ' drop database link  $SPARK_REF_TO_USG_DB_LINK '; end if; end;   " >> dbfmlink.sql

        echo "/" >> dbfmlink.sql
       echo " declare v_count number; begin select  count(1) into v_count  from    USER_DB_LINKS  where   lower(DB_LINK)  = '$FMS_DB_LINK' ; if v_count = 1 then    execute immediate ' drop database link $FMS_DB_LINK  ';  end if; end;   " >> dbsparklink.sql
        echo "/" >> dbsparklink.sql
       echo " declare v_count number; begin select  count(1) into v_count  from    USER_DB_LINKS  where   lower(DB_LINK)  = '$SPARK_REF_TO_USG_DB_LINK' ; if v_count = 1 then    execute immediate ' drop database link $SPARK_REF_TO_USG_DB_LINK  '; end if; end;   " >> dbsparklink.sql
        echo "/" >> dbsparklink.sql
       echo " declare v_count number; begin select  count(1) into v_count  from    USER_DB_LINKS  where   lower(DB_LINK)  = '$SPARK_DB_LINK' ; if v_count = 1 then    execute immediate ' drop database link $SPARK_DB_LINK  ';  end if; end;   " >> dbusagelink.sql
        echo "/" >> dbusagelink.sql
       echo " declare v_count number; begin select  count(1) into v_count  from    USER_DB_LINKS  where   lower(DB_LINK)  = '$FMS_DB_LINK'  ; if v_count = 1 then    execute immediate ' drop database link  $FMS_DB_LINK ';  end if; end;    " >> dbusagelink.sql 
        echo "/" >> dbusagelink.sql



		echo "create database link $SPARK_DB_LINK connect to $SPARK_DB_USER identified by $SPARK_DB_PASSWORD using '$SPARK_ORACLE_SID';" >> dbfmlink.sql;
		echo "create database link $SPARK_REF_TO_USG_DB_LINK connect to $SPARK_USG_DB_USER identified by $SPARK_USG_DB_PASSWORD using '$SPARK_USG_ORACLE_SID';" >> dbfmlink.sql;
		echo "exit;" >> dbfmlink.sql;


		echo "create database link $FMS_DB_LINK connect to $DB_USER identified by $DB_PASSWORD using '$NIKIRA_ORACLE_SID';" >> dbsparklink.sql;
		echo "create database link $SPARK_REF_TO_USG_DB_LINK  connect to $SPARK_USG_DB_USER identified by $SPARK_USG_DB_PASSWORD using '$SPARK_USG_ORACLE_SID';" >> dbsparklink.sql;
		echo "exit;" >>dbsparklink.sql
		

		
		echo "create database link $SPARK_DB_LINK  connect to $SPARK_DB_USER identified by $SPARK_DB_PASSWORD using '$SPARK_ORACLE_SID';" >> dbusagelink.sql;
		echo "create database link $FMS_DB_LINK  connect to $DB_USER identified by $DB_PASSWORD using '$NIKIRA_ORACLE_SID';" >> dbusagelink.sql;
	
		echo "exit;" >> dbusagelink.sql;


		sqlplus -s $SPARK_DB_USER/$SPARK_DB_USER@$SPARK_ORACLE_SID  @dbsparklink.sql >>$LogPath/$logFile 

        if [ $? -ne 0 ] ;
        then
                echo "Error : DB link creation failed for $SPARK_DB_USER . Exiting Run" 
                exit 1
        fi
		sqlplus -s $SPARK_USG_DB_USER/$SPARK_USG_DB_PASSWORD@$SPARK_USG_ORACLE_SID  @dbusagelink.sql >>$LogPath/$logFile

        if [ $? -ne 0 ] ;
        then
                echo "Error : DB link creation failed for $SPARK_USG_DB_USER . Exiting Run " 
                exit 1
        fi

		sqlplus -s $DB_USER/$DB_PASSWORD@$FMS_ORACLE_SID  @dbfmlink.sql >>$LogPath/$logFile

	        if [ $? -ne 0 ] ;
        then
                echo "Error : DB link creation failed for $DB_USER . Exiting Run " 
                exit 1
        fi


		rm dbfmlink.sql
		rm dbusagelink.sql
		rm dbsparklink.sql
		echo_log "###DB links Creation finished####"

}

function FmInstallation
{
	   for i in `cat $WRAPPER_HOME/$confFile |grep -v "#"`
                do
                        echo $i | grep -q '^FMS::'
                        if [ $? -eq 0 ]
                        then
                        i=`echo $i |awk -F '::' '{print $2}'`
                        export $i
                        else

                        echo $i | grep -q '^SPARK::'
                        if [ $? -eq 1 ]
                        then
                        export $i
                        fi

                        fi;

                done;

	echo_log "########FMDbSetUpStarted####"

	 if [ ! -d $ROC_FM_DEPLOY_PATH ]
         then
         echo_log "$ROC_FM_DEPLOY_PATH doesnt exists...!";
         exit;
         fi;

	if [ ${#FMS_TABLESPACENAME_PREFIX} -ge 5 ]; 
	then echo_log "TableSpace Prefix should not be more than 4 characters" ; 
	exit;
	fi;
    

##FMS path replacement code

	cd $ROC_FM_DEPLOY_PATH/bin
	chmod +x *.sh

# filePathModifier.sh tableSPacePrefix commMountPath TableSpacePAth FmsRootPath sparkDbscripts <ROC_FM_DEPLOYPATH> fmcorefilepath fmgdofilepath createtable<Y/N>
	sh filePathModifier.sh $FMS_TABLESPACENAME_PREFIX $COMMON_MOUNT_POINT $TABLESPACE_PATH  $FMSROOT $ROC_FM_DEPLOY_PATH/metadata/sparkdbscripts/  $ROC_FM_DEPLOY_PATH $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core $ROC_FM_DEPLOY_PATH/metadata/fmdbfiles/core $CREATE_TABLESPACE
		if [ $? -ne 0 ] ;
        then
                echo "Error : FM Path Replacement Failed . Exiting Run" 
                exit 1
        fi



		cd $ROC_FM_DEPLOY_PATH/config		
	    cp $presentWrkinDir/installer.properties.parse.in installer.properties
		if [ "$DATABASE_TYPE" = "Oracle" ]
		then
		sed -i '8s/#//' installer.properties
		sed -i "1,/#HOST_NAME=<Hostname>/s/#HOST_NAME=<Hostname>/HOST_NAME=$HOST_NAME/g" installer.properties
		sed -i "1,/#INSTANCE=<Instance>/s/#INSTANCE=<Instance>/INSTANCE=$FMS_ORACLE_SID/g" installer.properties
		sed -i "1,/#PORT_NO=<Port Number>/s/#PORT_NO=<Port Number>/PORT_NO=$PORT_NO/g" installer.properties
		sed -i "1,/#UNICODE=<Y\/N>/s/#UNICODE=<Y\/N>/UNICODE=$UNICODE/g" installer.properties
		    
		elif [ "$DATABASE_TYPE" = "Oracle_RAC" ]
		then
		sed -i '24s/#//' installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#HOST_NAME=<Hostname>/HOST_NAME=$HOST_NAME/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#PORT_NO=<Port Number>/PORT_NO=$PORT_NO/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#SERVICE_NAME=<Service Name>/SERVICE_NAME=$SERVICE_NAME/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#UNICODE=<Y\/N>/UNICODE=$UNICODE/g" installer.properties
		sed -i "/DB_TYPE=Oracle RAC/,/#DB_TYPE=MS SQL Server/s/#ONS_CONFIG=<hostname1:portNo,hostname2:portNo,..>/ONS_CONFIG=$ONS_CONFIG/g" installer.properties				
		fi
	
		sed -i "s/WORKSPACE_FILE=<workspace file name, e.g. : spark.ciw>/WORKSPACE_FILE=$WORKSPACE_FILE/g" installer.properties
		sed -i "s#SYS_OPERATOR_NAME:<Operator Name>#SYS_OPERATOR_NAME:$sparkOperator#g" installer.properties
		sed -i "s#SYS_LICENCE_KEY:<Licence Key>#SYS_LICENCE_KEY:$sparkLicenseKey#g" installer.properties
		sed -i "s/MACHINE_NAME=<Machine Name>/MACHINE_NAME=$MACHINE_NAME/g" installer.properties
		sed -i "s/HOST_ADDRESS=<Host Address>/HOST_ADDRESS=$HOST_ADDRESS/g" installer.properties
		sed -i "s#DATA_DIR=<Data Dir>#DATA_DIR=$DATA_DIR#g" installer.properties
	
        if [ $? -ne 0 ] ;
        then
                echo "Error : FM Path Replacement Failed . Exiting Run" 
                exit 1
        fi

		cd $ROC_FM_DEPLOY_PATH/bin
		chmod +x *.sh
		echo_log "refer the path for the logs:=$ROC_FM_DEPLOY_PATH/logs"
		./FmsPreSilentInstaller.sh $DB_USER $DB_PASSWORD 

        if [ $? -ne 0 ] ;
        then
                echo "Error : FM SilentInstaller Failed . Exiting Run" 
                exit 1
        fi


      	./FmsSilentInstaller.sh

		        if [ $? -ne 0 ] ;
        then
                echo "Error : FM SilentInstaller Failed . Exiting Run" 
                exit 1
        fi

	

	echo_log "#####FMDbSetUpFinished#######"
}

function SparkScripts
{
	cd $ROC_FM_DEPLOY_PATH/metadata/sparkdbscripts/
		chmod +x *.sh
# dbsetup FMDBUSER FMDbpassword FM_ORACLE_SID sparkDBUSer sparkDBPassword sparkOracleSId fmDBLINK  sparkdblink 
	./dbsetup.sh $DB_USER $DB_PASSWORD $FMS_ORACLE_SID $SPARK_DB_USER $SPARK_DB_PASSWORD $SPARK_ORACLE_SID $FMS_DB_LINK $SPARK_DB_LINK

if [ $? -ne 0 ];then
    echo "fmdb changes in spark failed"
    exit 1 ;
fi

 sh ./createsynonym.sh $DB_USER $SPARK_DB_USER $SPARK_DB_PASSWORD $SPARK_ORACLE_SID $FMS_DB_LINK $FMS_ORACLE_SID $DB_PASSWORD>spark_ref_create_synonym.log 2>&1

if [ $? -ne 0 ];then
    echo "Synonym creation in $SPARK_DB_USER failed"
    exit 1 ;
fi

 sh ./createsynonym.sh $DB_USER $SPARK_USG_DB_USER $SPARK_USG_DB_PASSWORD $SPARK_USG_ORACLE_SID $FMS_DB_LINK $FMS_ORACLE_SID $DB_PASSWORD > spark_usage_create_synonym.log 2>&1

if [ $? -ne 0 ];then
    echo "Synonym creation $SPARK_USG_DB_USER failed"
    exit 1 ;
fi


}

function PreveaInstallation	
{

	echo_log "####PreveaDBSetUpStared########"
	  if [ ! -d $PREVEA_ROOT_PATH ]
         then
         echo_log "$PREVEA_ROOT_PATH doesnt exists...!";
         exit;
         fi;

	  if [ ! -d $PREVEA_INTEGRATION_PATH ]
         then
         echo_log "$PREVEA_INTEGRATION_PATH doesnt exists...!";
         exit;
         fi;

	
	cd $PREVEA_ROOT_PATH/bin
	sed -i "s#@prefix@#$PREVEA_ROOT_PATH#g " ontrack-demo-exec.sql
	sed -i "s#/home/nikira/PREVEAHOME#$PREVEA_ROOT_PATH#g " ontrack-demo-exec.sql
	./ontrack-db-setup.sh >>$LogPath/$logFile 

	cd $PREVEA_INTEGRATION_PATH
	echo "exit;" >>prevea-DDL-nikira-integrated-exec.sql
	sqlplus -s $PREVEA_DB_USER/$PREVEA_DB_PASSWORD @prevea-DDL-nikira-integrated-exec.sql >>$LogPath/$logFile

        sed -i "s#@prefix@#$PREVEA_ROOT_PATH#g " prevea-DML-nikira-integrated-exec.sql
        sed -i "s#/home/nikira/PREVEAHOME#$PREVEA_ROOT_PATH#g " prevea-DML-nikira-integrated-exec.sql
	echo "exit;" >>prevea-DML-nikira-integrated-exec.sql
	sqlplus -s $PREVEA_DB_USER/$PREVEA_DB_PASSWORD @prevea-DML-nikira-integrated-exec.sql >>$LogPath/$logFile
	
#step 13	
	cd $PREVEA_ROOT_PATH/bin
	
	./polldirectorysetup.sh

	        if [ $? -ne 0 ] ;
        then
                echo "Error : Prevea Polldirectory Failed . Exiting Run" 
                exit 1
        fi

#step 14
     cd $PREVEA_ROOT_PATH/sbin
	 sed -i "s#<property name=\"connection.username\">[A-Za-z0-9]*<\/property>#<property name=\"connection.username\">$PREVEA_DB_USER<\/property>#g" hibernate.cfg.xml
	 sed -i "s#<property name=\"connection.password\">[A-Za-z0-9]*<\/property>#<property name=\"connection.password\">$PREVEA_DB_PASSWORD<\/property>#g" hibernate.cfg.xml
	 sed -i "s#<property name=\"connection.url\">jdbc:oracle:thin:.*<\/property>#<property name=\"connection.url\">jdbc:oracle:thin:@[$PREVEA_SERVER_IP]:1521:$FMS_ORACLE_SID<\/property>#g" hibernate.cfg.xml
#step 15
	cd $PREVEA_ROOT_PATH/bin
	./createsynonyms.sh >>$LogPath/$logFile

        if [ $? -ne 0 ] ;
        then
                echo "Error : Prevea Create Synonym Failed . Exiting Run" 
                exit 1
        fi



	>preveadb.sql
	echo "update configurations set VALUE ='$PREVEA_ROOT_PATH/data/subscriber'  where config_key='PREVEA_OUTPUT_DIRECTORY';" >>preveadb.sql
	echo "update configurations set VALUE ='$PREVEA_SERVER_IP' where config_key='PREVEA_SERVER_IP';" >>preveadb.sql
	echo "exit;" >>preveadb.sql
	sqlplus -s $DB_USER/$DB_PASSWORD @preveadb.sql>>$LogPath/$logFile
	
	
	echo_log "####PreveaDBSetUpCompleted########"
	
}

function VailidateAllPaths
{


      if [ ! -d $PREVEA_ROOT_PATH ]
         then
         echo_log "$PREVEA_ROOT_PATH doesnt exists...!";
         exit;
         fi;

      if [ ! -d $PREVEA_INTEGRATION_PATH ]
         then
         echo_log "$PREVEA_INTEGRATION_PATH doesnt exists...!";
         exit;
         fi;


        if [ ! -d $ROC_SPARK_DEPLOY_PATH ]
                then
                        echo_log "$ROC_SPARK_DEPLOY_PATH doesnt exists...!";
            exit;
                fi;

        if [ ! -d $ROC_SPARK_DEPLOY_PATH/config ]
                then
                        echo_log "$ROC_SPARK_DEPLOY_PATH/config doesnt exists...!";
                        echo_log "Please copy the generated deploy folder from ROC_FM_SPARK...!";
            exit;
                fi;


            if [ ! -d $ROC_SPARK_DEPLOY_PATH/bin ]
                then
                        echo_log "$ROC_SPARK_DEPLOY_PATH/bin doesnt exists...!";
            exit;
                fi;


     if [ ! -d $ROC_FM_DEPLOY_PATH ]
         then
         echo_log "$ROC_FM_DEPLOY_PATH doesnt exists...!";
         exit;
         fi;




}


function runWrapper
{

VailidateAllPaths;

ValidatePermission
CreateDbLinks;
SparkInstallation; 
FmInstallation;
SparkScripts;
PreveaInstallation;
}


runWrapper;
