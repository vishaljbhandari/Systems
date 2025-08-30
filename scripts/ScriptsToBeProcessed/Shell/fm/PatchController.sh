#!/bin/bash

PROG_NAME=$0;
NBIT_HOME=`pwd`
String=".";

mkdir -p $NBIT_HOME/PATCHES/FMSROOT
mkdir -p $NBIT_HOME/PATCHES/FMSTOOLS
mkdir -p $NBIT_HOME/PATCHES/FMSCLIENT

if [ -d $NBIT_HOME/PATCHES/NIKIRACLIENT ]
then
	rm -rf $NBIT_HOME/PATCHES/NIKIRACLIENT
fi;

if [ -d $NBIT_HOME/PATCHES/NIKIRATOOLS ]
then
 	rm -rf $NBIT_HOME/PATCHES/NIKIRATOOLS
fi;

if [ -d $NBIT_HOME/PATCHES/NIKIRAROOT ]
then
    rm -rf $NBIT_HOME/PATCHES/NIKIRAROOT
fi;

os=`uname`
if [ "$os" == "SunOS" ]
then
	tar_command="gtar"
else
	tar_command="tar"
fi

logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NBIT_PatchController.log" }'`
echo "logFile = $logFile"

#-----------------------------------------------------Functions Start----------------------------------------------------------------------------#

function echo_log()
{
    echo "$@"
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function ExportSRCPaths
{
		for i in `cat Extract.conf|grep -v "#"|grep -v "TAR.SOURCE"|grep -v "PATH.SOURCE"`
		do
			export $i
		done;
}

function log_only()
{
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function Changes_ApacheRoot
{

		echo_log " "
		echo_log "####################### APACHE INSTALLATION STARTS################################################"
		cd $APACHEROOT
		n=$APACHEROOT
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/HTTPD=.*/HTTPD='$b\/bin\/httpd -f $b\/conf\/httpd.conf'/g" $APACHEROOT/bin/apachectl
		cd $NBIT_HOME
		mv $APACHEROOT/conf/httpd.conf $APACHEROOT/conf/httpd.conf.bkp
		cp $NBIT_HOME/httpd.conf.template $APACHEROOT/conf/httpd.conf

		n=$APACHEROOT
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/APACHEROOT/$b/g" $APACHEROOT/conf/httpd.conf

		n=$FMSCLIENT
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/NIKIRACLIENT/$b/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/NIKIRA_PORT/$NIKIRA_PORT/g" $APACHEROOT/conf/httpd.conf
		USER_GROUP=`id|cut -d"(" -f3|cut -d")" -f1`
		perl -pi -e "s/LOG_NAME/$LOGNAME/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/GROUP_NAME/$USER_GROUP/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/IP_ADDRESS/$IP_ADDRESS/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/DB_USER_VALUE/$DB_USER/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/DB_PASSWORD_VALUE/$DB_PASSWORD/g" $APACHEROOT/conf/httpd.conf

		if [ "$DATABASE_TYPE" = "db2" ];then # db2
			DATABASE_HOME_VAR=DB2DIR
			DATABASE_HOME_VALUE=$DB2DIR
			DATABASE_INSTANCE_VAR=DB2INSTANCE
			DATABASE_INSTANCE_VALUE=$DB2INSTANCE
		elif [ "$DATABASE_TYPE" = "oracle" ];then
			DATABASE_HOME_VAR=ORACLE_HOME
			DATABASE_HOME_VALUE=$ORACLE_HOME
			DATABASE_INSTANCE_VAR=ORACLE_SID
			DATABASE_INSTANCE_VALUE=$ORACLE_SID
		else
			echo "DATABASE_TYPE : $DATABASE_TYPE is invalid it can be [oracle|db2]"
		fi;
		n=$DATABASE_HOME_VALUE
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/DATABASE_HOME_VALUE/$b/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/DATABASE_HOME_VAR/$DATABASE_HOME_VAR/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/DATABASE_INSTANCE_VALUE/$DATABASE_INSTANCE_VALUE/g" $APACHEROOT/conf/httpd.conf
		perl -pi -e "s/DATABASE_INSTANCE_VAR/$DATABASE_INSTANCE_VAR/g" $APACHEROOT/conf/httpd.conf


		n=$FMSROOT
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/NIKIRAROOT_VALUE/$b/g" $APACHEROOT/conf/httpd.conf
		
		n=$RUBYROOT
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/RUBYROOT_VALUE/$b/g" $APACHEROOT/conf/httpd.conf

		if [ `uname` == "HP-UX" ]
		then
		n=$LD_PRELOAD
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/#LD_PRELOAD/DefaultInitEnv LD_PRELOAD $b/g" $APACHEROOT/conf/httpd.conf
		fi

		n=$COMMONLIBDIR
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		perl -pi -e "s/COMMONLIB/$b/g" $APACHEROOT/conf/httpd.conf

		mv $APACHEROOT/bin/apachectl $APACHEROOT/bin/apachectl.bkp
		echo "#! /bin/sh">2.txt
		file $APACHEROOT/bin/httpd|egrep -i "elf-64|64-bit"
        if [ $? -eq 1 ]
        then
            echo "Apache is 32 bit,Necessary Changes in apachectl is done.";
            echo "LD_LIBRARY_PATH=$COMMONLIBDIR/lib32:\$LD_LIBRARY_PATH">>2.txt
            echo "export LD_LIBRARY_PATH">>2.txt
        else
            echo "Apache is 64 Bit";
        fi;
		grep -v "^#" $APACHEROOT/bin/apachectl.bkp>>2.txt
		mv 2.txt $APACHEROOT/bin/apachectl
		chmod +x $APACHEROOT/bin/apachectl
		
		echo_log "####################### APACHE INSTALLED##########################################################"
}


function Replace_paths_ApacheRoot
{
		echo_log " "
		echo_log "####################### APACHEROOT PATH REPLACER STARTED #########################################"
		
		n=$SOURCE_APACHEROOT;	
		n1=`basename $SOURCE_APACHEROOT`;
		if [ "$n" == "" ]
		then
			echo_log "Skipping the Path Replacement for APACHEROOT........"
		else
			echo_log "Source Machine: APACHEROOT=$n";
			a=`echo $n|perl -p -e 's#\/#\\\/#g'`
			n="$FMS_INSTALL_DIR/FMSTOOLS/$n1"
			echo_log "Dest Machine: APACHE ROOT=$n";
			b=`echo $n|perl -p -e 's#\/#\\\/#g'`
			cd $APACHEROOT
			find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|ruby script|script text|bash script|shell script|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
				find . -name ".svn"|xargs rm -rf
			echo_log "Replaced Successfully in APACHEROOT"
		fi

		echo_log "####################### APACHEROOT  REPLACER FINISHED ############################################"
}

function Replace_paths_RubyRoot
{
		echo_log " "
		echo_log "####################### RUBYROOT PATH REPLACER STARTED ###########################################"

		n=$SOURCE_RUBYROOT;	
		n1=`basename $SOURCE_RUBYROOT`;

		if [ "$n" == "" ]
		then
			echo_log "Skipping the Path Replacement for RUBYROOT........"
		else
			echo_log "Source Machine: RUBYROOT=$n";	
			a=`echo $n|perl -p -e 's#\/#\\\/#g'`
			n="$FMS_INSTALL_DIR/FMSTOOLS/$n1";
			echo_log "Dest Machine: RUBYROOT=$n";	
			b=`echo $n|perl -p -e 's#\/#\\\/#g'`
			cd $RUBYROOT
			find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
				find . -name ".svn"|xargs rm -rf

			echo_log "Replaced Successfully in RUBYROOT"
		fi

		echo_log "####################### RUBYROOT PATH REPLACER FINISHED  #########################################"
}

function Changes_NikiraClient
{

		echo_log " "
		echo_log "####################### FMS CLIENT CHANGES ####################################################"
		cd $FMSCLIENT
		echo "#! $RUBYROOT/bin/ruby" >1.txt
		echo "$:<<'$RUBYROOT/lib/ruby/gems/1.8/gems/soap4r-1.5.8/lib'">>1.txt
		echo "$:<<'$RUBYROOT/lib/ruby'"    >>1.txt
		echo "$:<<'$RUBYROOT/lib/ruby/1.8'">>1.txt
		echo "$:<<'$RUBYROOT/lib/ruby/1.8/$OS_SPEC_LIB'">>1.txt
		echo "$:<<'$RUBYROOT/lib/ruby/site_ruby/1.8'">>1.txt
		echo "$:<<'$RUBYROOT/lib/ruby/site_ruby/1.8/$OS_SPEC_LIB'">>1.txt
		echo "$:<<'$RUBYROOT/lib/ruby/gems/1.8/gems'">>1.txt
		echo " ">>1.txt
		echo "require \"$RUBYROOT/lib/ruby/gems/1.8/gems/fcgi-0.8.7/lib/fcgi.rb\"">>1.txt
		echo " ">>1.txt
		echo "APP_CONTAINER='fcgi'">>1.txt
		echo " ">>1.txt
		echo "require File.dirname(__FILE__) + \"/../config/environment\"">>1.txt
		echo "require 'fcgi_handler'">>1.txt
		echo " ">>1.txt
		echo " ">>1.txt
		echo "RailsFCGIHandler.process!">>1.txt
		echo " ">>1.txt
		log_only "temp Dispatch fcgi is created";
		fcgi_dir=`find . -name "dispatch.fcgi"|grep public`
		log_only "fcgi_dir=$fcgi_dir";
		
		if [ "$fcgi_dir" == "" ]
		then
			echo_log "Warning: public folder which contains dispatch.fcgi is not Available"
			echo_log "Exiting without proper Installation...";
			exit 9;
		fi;

		mv $fcgi_dir "$fcgi_dir.bk"
		mv 1.txt $fcgi_dir
		chmod +x $fcgi_dir 
		echo_log "Dispatch fcgi is recreated.......";
		yml_dir=`find . -name "database.yml"`
		perl -pi -e "s/DB_USER=.*/DB_USER=$DB_USER/g" $yml_dir 
		perl -pi -e "s/DB_PASSWORD=.*/DB_PASSWORD=$DB_PASSWORD/g" $yml_dir
		perl -pi -e "s/username:.*/username: $DB_USER/g" $yml_dir 
		perl -pi -e "s/password:.*/password: $DB_PASSWORD/g" $yml_dir
		env_dir=`find . -name "environment.rb"`
		perl -pi -e "s/RAILS_GEM_VERSION =.*/RAILS_GEM_VERSION ='$RAILS_GEM_VERSION'/g" $env_dir
		echo "\$SERVER_IP = '$IP_ADDRESS'">>$env_dir
		echo "\$SERVER_LOGIN_ID = '$LOGNAME'">>$env_dir
		echo "\$SERVER_PASSWORD = '$LOGIN_PASSWORD'">>$env_dir
		echo "\$WSDL_FILE_URL = 'http://$IP_ADDRESS:3000/s2_k_services/wsdl'">>$env_dir

	#	if [ ! -f $FMSCLIENT/config/asset_mapping.yml ]
	#	then
	#			echo_log "WARNING : generatefiles.sh is not ran,please run after installation!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	#	fi;		

		echo_log "####################### FMS CLIENT CHANGES DONE ###############################################"


}

function Replace_paths_NikiraClient
{
		echo_log " "
		echo_log "####################### FMSCLIENT PATH REPLACER STARTED #######################################"
		
		n=$SOURCE_NIKIRACLIENT;	
		if [ "$n" == "" ]
		then
			echo_log "Skipping the Path Replacement for FMSCLIENT........"
		else
			echo_log "Source Machine: FMSCLIENT=$n";	
			a=`echo $n|perl -p -e 's#\/#\\\/#g'`
			n="$FMS_INSTALL_DIR/FMSCLIENT"
			echo_log "Dest Machine: FMSCLIENT=$n";	
			b=`echo $n|perl -p -e 's#\/#\\\/#g'`
			cd $FMSCLIENT
			find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
				find . -name ".svn"|xargs rm -rf

			echo_log "Replaced Successfully in FMSCLIENT"
		fi

		echo_log "####################### FMSCLIENT PATH REPLACER FINISHED  #####################################"
}

function Changes_NikiraRoot
{
		echo_log " "
		echo_log "####################### FMS ROOT CHANGES ######################################################"
		cat ~/.bashrc >> $FMSROOT/sbin/rangerenv.sh
		perl -pi -e "s/DB_USER=.*/DB_USER=$DB_USER/g" $FMSROOT/sbin/ranger.conf
		perl -pi -e "s/DB_PASSWORD=.*/DB_PASSWORD=$DB_PASSWORD/g" $FMSROOT/sbin/ranger.conf
		perl -pi -e "s/DB_PASSWORD=.*/DB_PASSWORD=$DB_PASSWORD/g" $FMSROOT/sbin/rangerenv.sh
		if [ $DATABASE_TYPE = "db2" ];then
			perl -pi -e "s/DB2_HOST=.*/DB2_HOST=$DB2_HOST/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
			perl -pi -e "s/DB2_PORT=.*/DB2_PORT=$DB2_PORT/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
			perl -pi -e "s/DB2DIR=.*/DB2DIR=$DB2DIR/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
			perl -pi -e "s/DATABASENAME=.*/DATABASENAME=$DB2_DBNAME/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
			perl -pi -e "s/DB2_DBNAME=.*/DB2_DBNAME=$DB2_DBNAME/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
		fi;
		echo_log "####################### FMS ROOT CHANGES DONE #################################################"
}

function Start_Apache
{
		echo_log " "
		echo_log "Trying to stop old apache"
		if [ ! -f $APACHEROOT/bin/apachectl ]
		then
				echo_log "Apache is Not installed Properly.. Exiting...";
				exit 10;
		fi;		
		$APACHEROOT/bin/apachectl stop
		$APACHEROOT/bin/apachectl start
		ApacheStatus=`ps -fu $LOGNAME |grep http|wc -l`;

		if [ $ApacheStatus -gt 1 ]
		then
				echo_log "APACHE STARTED(port:$NIKIRA_PORT).................................................."
				echo_log "Client URL: http://$IP_ADDRESS:$NIKIRA_PORT "
		else
				echo_log "APACHE Not started check the log files for the error...."
		fi;

		Installation_Time=`date +%Dr#%H:%M:%S`;
}
function Replace_paths_NikiraRoot
{
		echo_log " "
		echo_log "####################### FMSROOT PATH REPLACER STARTED #########################################"
		n=$SOURCE_NIKIRAROOT;	

		if [ "$n" == "" ]
		then
			echo_log "Skipping the Path Replacement for FMSROOT........"
		else
			echo_log "Source Machine: FMSROOT=$n";	
			a=`echo $n|perl -p -e 's#\/#\\\/#g'`
			n="$FMS_INSTALL_DIR/FMSROOT"
			echo_log "Dest Machine: FMSROOT=$n";	
			b=`echo $n|perl -p -e 's#\/#\\\/#g'`	
			cd $FMSROOT
			find .|grep -v "/\." | xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -v "ELF.*executable"| egrep -i "ascii text|commands text|program text|English text|ruby script|Unicode text|ASCII text|ASCII English text|script text|bash script|shell script|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
			find . -name ".svn"|xargs rm -rf
			echo_log "Replaced Successfully in FMSROOT"
		fi
		echo_log "####################### FMSROOT PATH REPLACER FINISHED  #######################################"
}

function Display()
{
		string=$1;
		Format=$2;
		(( i = 0 )) ;
		Empty=".";
		while [ $i -lt $Format ]
		do
				string="$string$Empty" ;
				(( i = $i + 1 )) ;
		done;
		command="echo \"$string\"|awk '{print substr(\$0,0,$Format)'}>l;"
		echo $command>s.sh;
		sh s.sh;
		string=`cat l`
		String=`echo $string|perl -p -e 's#\.# #g'`
		rm l s.sh
}

function List()
{
		ModuleName=$1;

		if [ ! -f $NBIT_HOME/.current_version_$ModuleName  -o ! -f  $NBIT_HOME/.version_$ModuleName ]
		then
				echo_log "No Installation Done For $ModuleName"
				exit 777;
		fi;		
		
		FullText=`cat $NBIT_HOME/.current_version_$ModuleName`
		CrrVer=`echo $FullText |cut -d "|" -f1`
		CrrLogname=`echo $FullText |cut -d "|" -f2`
		CrrTime=`echo $FullText |cut -d "|" -f3`
		CurrVerNo=`echo $CrrVer|cut -f1 -d"_"`
		Current_version=$CrrVer;
		Current_versionNo=$CurrVerNo;
		echo "++++++++++++++++++++++++++++++++++++++++++++ ${ModuleName} VERSIONS +++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
		echo "Current Version :$CrrVer";
		echo "*******************************************************************************************************************";
		echo "* Version	ReleaseName		Mode 	InstalledBy		InstalledTime  			          *";	
		echo "*******************************************************************************************************************";
		for i in `cat  $NBIT_HOME/.version_$ModuleName`
		do
				CrrVer=`echo $i |cut -d "|" -f1`
				CrrLogname=`echo $i |cut -d "|" -f2`
				CrrDate=`echo $i |cut -d "|" -f3`
				CrrTime=`echo $i |cut -d "#" -f2`
				CurrVerNo=`echo $i|cut -f1 -d"_"`
			#	echo "Current_version=$Current_version version=$CrrVer";
			#	echo "if [ \"$Current_version\" \> \"$CrrVer\" -o \"$Current_version\" == \"$CrrVer\" ]";
				if [ $Current_versionNo -gt $CurrVerNo -o $Current_versionNo -eq $CurrVerNo ]
				then
					Mode="Active";
				else
					Mode="Rolled"
				fi;	

				
				Display "$CurrVerNo" 7;
				CurrVerNo=$String;
				Display "$CrrVer" 20;
				CrrVer=$String;
				Display "$CrrLogname" 20;
				CrrLogname=$String;
				Display "$CrrDate" 8;
				CrrDate=$String;
				Display "$CrrTime" 8;
				CrrTime=$String;
				
				
		echo "	$CurrVerNo $CrrVer   $Mode      $CrrLogname $CrrDate $CrrTime	"
		done;
		echo " "
		echo " "
		echo " "
		echo " "
}

function Range_Installer()
{
		PatchLocation=$1;
		ModuleName=$2;

		echo_log "Range Installation Started....."

		StartVersion=`echo $RValue|cut -d"-" -f1|cut -d"_" -f1`;
		EndVersion=`echo  $RValue|cut -d"-" -f2|cut -d"_" -f1`;

		if [ $StartVersion -gt $EndVersion ]
		then
				echo_log "Range Installation: Start Version( $StartVersion)  greater than End Version($EndVersion) .exiting...";
				exit;
		fi;

		echo_log "Checking for Required tar files.......";
		(( j = 0 ));
		(( k = 0 ));
		
		for ((  i = $StartVersion ;  i <= $EndVersion;  i++  ))
		do

				 Install_Patch=`ls $backup_dir|egrep "^${i}_.*tar|^0*${i}_.*tar"|grep -v "roll" `;	  		
				 Install_Precheck=`ls $backup_dir|egrep "^${i}_.*tar|^0*${i}_.*tar"|grep "roll" `;	  		
				 if [ "$Install_Precheck" != "" ]
				 then
						 echo_log "Installation range includes Already available version";
						 exit 8;
				 fi;	
				 
				 if [ "$Install_Patch" == "" ]
				 then
						MissingFiles[$j]=$i
						(( j ++ )) ;
				 else
						InstallVersions[$k]=$i
						InstallFiles[$k]=`echo $Install_Patch|cut -f1 -d"."`;
						(( k ++ )) ;
				 fi;		
		done;
		echo "missing files count  ${#MissingFiles[*]}";
		if [ ${#MissingFiles[*]} -gt 0 ]
		then
				echo_log "Missing Files for the following versions ${MissingFiles[@]}";
				if [ ${#InstallVersions[*]} -ne 0 ]
				then
						echo_log "Installation can be done for ..(${InstallVersions[@]}).." ; 
						echo_log "Do You Want to continue with this [Y/N](Default N) ";
						read Status;
						log_only "Status" ;

						if [ "$Status" == "y" -o "$Status" == "Y" ]
						then
								echo_log "Continuing with partial patch Installation";
						else
								echo_log "Range Installation Exiting....";exit 9;
						fi	
				else
						echo_log "No Installation files Available..Quiting..";
						exit 10;
				fi		
		else		
			echo_log "Tar files for all the patches in the Range are Available ";  
		fi;	  

		echo ${InstallFiles[@]} ;
		for i in `echo ${InstallFiles[@]}`
		do
			Installer $ModuleName $i ;	
		done;

}

function Installer()
{
		PatchModule=$1;
		PatchReleaseName=$2;
		PatchReleaseVersion=`echo $PatchReleaseName|cut -f1 -d"|"`;
		PatchLocation="$NBIT_HOME/PATCHES/$PatchModule";
		PatchTarName="$PatchReleaseName.tar";

		if [ ! -f $NBIT_HOME/.version_$PatchModule ]
		then
		LReleaseName="0_core"
		Installation_Time=`date +%Dr#%H:%M:%S`;
		echo "$LReleaseName|$LOGNAME|$Installation_Time" > $NBIT_HOME/.version_$PatchModule
		echo "$LReleaseName|$LOGNAME|$Installation_Time" > $NBIT_HOME/.current_version_$PatchModule
		fi

		CURRENT_VERSION_NO=`cat $NBIT_HOME/.current_version_$PatchModule|cut -f1 -d"|"|cut -f1 -d"_"`
		RELEASE_NO=`echo $PatchReleaseName|cut -f1 -d"_"`;

		if [ $CURRENT_VERSION_NO -gt $RELEASE_NO ]
		then
					echo_log "A higher/current version[$CURRENT_VERSION_NO] is already available!!"
					exit 3;
		fi

		cd $PatchLocation
		echo_log "Expecting patch to be present in $PatchLocation dir" ;

		cd $FMS_INSTALL_DIR/$PatchModule
		$tar_command -tf $PatchLocation/$PatchTarName|grep -v "/$" > tar_file_list.txt
		$tar_command -cvf $backup_dir/${PatchReleaseName}_roll.tar -T tar_file_list.txt  >/dev/null 2>&1
		rm tar_file_list.txt
		$tar_command -xvf $PatchLocation/$PatchTarName>/dev/null 2>&1
		LReleaseName=$PatchReleaseName
		Installation_Time=`date +%D#%H:%M:%S`;
		echo "$LReleaseName|$LOGNAME|$Installation_Time" >> $NBIT_HOME/.version_$PatchModule
		echo "$LReleaseName|$LOGNAME|$Installation_Time" > $NBIT_HOME/.current_version_$PatchModule
		echo_log "Installation of new patch $PatchReleaseName is DONE...";
}

function Get_Encrypted_Password
{
		
	perl -pi -e "s#LIB=.*#LIB=DL.dlopen(ENV['WEBSERVER_RANGER_HOME'] +'/lib/libencode.so')#g" $FMSCLIENT/lib/framework/fieldutil.rb
			cd $NBIT_HOME
			if [ -f $RANGERHOME/rbin/framework/fieldutil.rb ]
			then
				chmod +x ruby_fieldutil.rb
				DB_PASSWORD=`ruby ruby_fieldutil.rb $DB_PASSWORD`
				echo_log "Encrypted Password is $DB_PASSWORD";
				export DB_PASSWORD;
			else
				echo_log "make sure $RANGERHOME/rbin/framework/fieldutil.rb and libencode dependent libraries are present and proper path set in bashrc... "
			    exit 999;
			fi;
   	echo_log "Generated Password is $DB_PASSWORD";
}

function Controller()
{
		PatchModule=$1;
		To_version=$2;

		PatchLocation="$NBIT_HOME/PATCHES/$PatchModule";

		From_version=`cat $NBIT_HOME/.current_version_$PatchModule|cut -f1 -d"|"`


		if [ "$To_version" == "$From_version" ]
		then
				echo_log "The Installed version and Release version are same [$To_version] ....Exiting...";
				exit 7;
		fi;		

		cd $FMS_INSTALL_DIR/$PatchModule;
		Fline=`cat $NBIT_HOME/.version_$PatchModule|cut -f1 -d"|" |grep -n $From_version|cut -d ":" -f1`
		Tline=`cat $NBIT_HOME/.version_$PatchModule |cut -f1 -d"|"|grep -n $To_version|cut -d ":" -f1`

		if [ "$Fline" == "" -o "$Tline" == "" ]
		then
				echo_log "Release version_$PatchModule mentioned is wrong. Exiting.....";
				exit 2;
		fi

		if [ $Fline -gt $Tline ]
		then
			echo_log "Rollbacking $PatchModule from $From_version to $To_version ";	 
			hd=$Fline;
			tl=`expr $Fline - $Tline`;
			head -$hd $NBIT_HOME/.version_$PatchModule|cut -f1 -d"|" |tail -$tl|sort -nr
			for i in `head -$hd $NBIT_HOME/.version_$PatchModule|cut -f1 -d"|" |tail -$tl|sort -nr`
			do
				$tar_command -xvf $backup_dir/${i}_roll.tar ;
				$tar_command -tf $backup_dir/${i}_roll.tar>a.txt ;
				$tar_command -tf $PatchLocation/${i}.tar>b.txt ;
				diff a.txt b.txt|grep '^>'|cut -d'>' -f2|xargs rm >/dev/null 2>&1;
			done

			Installation_Time=`date +%D#%H:%M:%S`;
			echo "$To_version|$LOGNAME|$Installation_Time">$NBIT_HOME/.current_version_$PatchModule
		else
			echo_log "Upgrading $PatchModule from $From_version to $To_version ";	
			hd=`expr $Tline`;
			tl=`expr $Tline - $Fline`;
			for i in `head -$hd $NBIT_HOME/.version_$PatchModule|cut -f1 -d"|" |tail -$tl`
			do
				$tar_command -xvf $PatchLocation/${i}.tar;
			done

			Installation_Time=`date +%D#%H:%M:%S`;
			echo "$To_version|$LOGNAME|$Installation_Time">$NBIT_HOME/.current_version_$PatchModule
			
		fi

		if [ $Fline -gt $Tline ]
		then
			echo_log "Rolled Back from $From_version to $To_version"
		else
			echo_log "Upgraded from $From_version to $To_version"
		fi
}


function CheckIsList
{

		if [ "$option" == "l" ]
		then
				if [ "$ModuleName" != "" ]
				then
						List $ModuleName;	
				else
						List "FMSROOT";
						List "FMSCLIENT";
						List "FMSTOOLS" ;
				fi;		
				exit 0;
		fi;		
}

function Validations1
{
		if [ "$FMS_INSTALL_DIR" == "" ]
		then
				echo_log "Nikira Installation Directory is Not set in bashrc (FMS_INSTALL_DIR)"
				exit 1;
		fi;

		if [ "$DB_PASSWORD" == "" ]
		then
				echo_log "DB_PASSWORD is not set in bashrc.Please set it and proceed ."
				exit 2;
		fi;

		if [ "$ModuleName" != "FMSROOT" -a "$ModuleName" != "FMSTOOLS" -a "$ModuleName" != "FMSCLIENT" ]
		then
				echo_log "Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT]";
				exit 3;
		fi		
		PatchLocation="$NBIT_HOME/PATCHES/$ModuleName";

		echo "Enter the backup direcory path."
		read backup_dir
		echo "Entered path is-  $backup_dir"
		
		if [ ! -d "$backup_dir" ] 
		then
			echo_log " Entered $backup_dir does not exists... please create directory and proceed ... exiting!!! "
			exit 4;
		fi

}

function Validations2
{
		if [ "$VersionName" == "" ]
		then
				echo_log "Version name and/or Module Name is missing.Try -h option for more information ";
				exit 4;
		fi;

		if [ "$VersionName" == "0" ]
		then
				VersionName="0_core";
		fi;

		if [ ! -f $PatchLocation/$VersionName.tar -a "$VersionName" != "0_core" ] 
		then
				Install_Patch=`ls $backup_dir|egrep "^${VersionName}_.*tar|^0*${VersionName}_.*tar"|grep -v "roll" `;	
				if [ $? -ne 0 ]
				then
				echo_log "Required installation file $PatchLocation/$VersionName.tar  missing";
				exit 5;
				fi;
				VersionName=`echo $Install_Patch|cut -d "." -f1`;
		fi

		VersionNo=`echo $VersionName|cut -d "_" -f1`;
		VersionNo=`expr $VersionNo - 1 + 1`		
		
		if [ $? -eq 3 ]
		then
			echo_log "Version No should be integer: Make sure prefix(_) of Version name is integer";
			exit 999;
		fi;	
		
}

function PostInstallChanges
{
		if [ "$ModuleName" == "FMSROOT" ]
		then
				Replace_paths_NikiraRoot;
				Changes_NikiraRoot;
				exit 0;		
		fi;		
				
		if [ "$ModuleName" == "FMSCLIENT" ]
		then
				Replace_paths_NikiraClient;
				Get_Encrypted_Password ;
				Changes_NikiraClient ;
				exit 0;
		fi;
		
		if [ "$ModuleName" == "FMSTOOLS" ]
		then
				echo_log "Your Tool release includes  RUBYROOT [Y/N]";
				read a;
				log_only "$a";

				if [ "$a" == "Y" -o "$a" == "y" ]
				then
						Replace_paths_RubyRoot ;
				fi;
				
				echo_log "Your Tool release includes  APACHEROOT [Y/N]";
				read a;
				log_only "$a";

				if [ "$a" == "Y" -o "$a" == "y" ]
				then
						Replace_paths_ApacheRoot ;
						Changes_ApacheRoot ;
						Start_Apache;

				fi;

				exit 0;
		fi;

}

function Help
{
		echo_log "Usage: $PROG_NAME -m ModuleName -v ReleaseName ";
		echo_log "or    $PROG_NAME -[lh] ";
		echo_log "-v will run the migration for particular release or the latest.";
		echo_log "-l will list the Current Migration version and Available Migration Versions.";
		echo_log "-h Help.";
		echo_log "Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT]";
}

function CheckDoRangeInstallation
{
		if [ "$Range" == "RangeInstallation" ]
		then
				echo_log "Range Installation ........";
				Range_Installer $PatchLocation $ModuleName ;
				PostInstallChanges ;
				exit 0;
		fi;
}		

function NewInstallationOrVersionControl
{
		if [ ! -f $backup_dir/${VersionName}_roll.tar -a "$VersionName" != "0_core" ] 
		then
				echo_log "This is a New patch installation ($VersionName)";
				Installer $ModuleName $VersionName ;
				PostInstallChanges ;
		else
				echo_log "Version Control Starts ..............";
				Controller $ModuleName $VersionName ;
				PostInstallChanges ;
		fi;
}

#-----------------------------------------------------Functions End----------------------------------------------------------------------------#

# Main Starts Here..............................................

while getopts v:m:r:lh OPTION
do
		case ${OPTION} in
                v) VersionName="${OPTARG}" ;;
				m) ModuleName="${OPTARG}" ;;		 		
				r) Range="RangeInstallation";RValue="${OPTARG}";; 	
                l) option="l";;
                h) Help;exit 0;;
				\?) Help;		
                    exit 2;;
        esac
done


CheckIsList;
Validations1;
ExportSRCPaths;
CheckDoRangeInstallation;
Validations2;
NewInstallationOrVersionControl;
exit 0;

# Main Ends Here..............................................
