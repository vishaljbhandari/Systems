#!/bin/bash
#*******************************************************************************
#* The Script may be used and/or copied with the written permission from
#* Subex Ltd. or in accordance with the terms and conditions stipulated in
#* the agreement/contract (if any) under which the program(s) have been supplied
#*
#* AUTHOR:Saktheesh & Vijayakumar & Mahendra Prasad S
#* Modified: 10-July-2015

#* Description:
#*	This script will install(porting,not compilation) Nikira 6.1 on production box

#*******************************************************************************
suffix=`date +%d%b%Y`
logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NBIT_Install.log" }'`
echo "logFile = $logFile"
Installation_Time=`date +%Dr#%H:%M:%S`;
NBIT_HOME=`pwd`;
COREUNTARED=0
DB_ENCRYPT=0

os=`uname`
if [ "$os" == "SunOS" ]
then
	tar_command="gtar"
else
	tar_command="tar"
fi

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

function ExportSRCPaths
{
	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep -v "TAR.SOURCE"|grep -v "PATH.SOURCE"`
	do
		export $i
	done;

	if [ ! -d $SOURCE_NIKIRATOOLS ]
	then
		echo_log "$SOURCE_NIKIRATOOLS doesnt exists...!";
	fi;
	NTdir=`basename $SOURCE_NIKIRATOOLS 2>/dev/null` ;

	if [ ! -d $SOURCE_NIKIRACLIENT ]
	then
		echo_log "$SOURCE_NIKIRACLIENT doesnt exists...!";
	fi;
	NCdir=`basename $SOURCE_NIKIRACLIENT 2>/dev/null`
	
	if [ ! -d $SOURCE_NIKIRAROOT ]
	then
		echo_log "$SOURCE_NIKIRAROOT doesnt exists...!";
	fi;
	NRdir=`basename $SOURCE_NIKIRAROOT 2>/dev/null`

	if [ ! -d $SOURCE_DBSETUP ]
	then
		echo_log "$SOURCE_DBSETUP doesnt exists...!";
	fi;
	DBdir=`basename $SOURCE_DBSETUP 2>/dev/null`;

	if [ ! -d $SOURCE_COMMONLIBDIR ]
	then
		echo_log "$SOURCE_COMMONLIBDIR doesnt exists...!";
	fi;
	COMdir=`basename $SOURCE_COMMONLIBDIR 2>/dev/null`;
}


function InstallDirCheck_And_CopySource
{
	echo_log " "
	echo_log "############# Install Directory Check  ###########################################################"

	if [ "$FMS_INSTALL_DIR" == "" ]
	then
		echo_log "Nikira Installation Directory is Not set in bashrc (FMS_INSTALL_DIR)"
		exit 1;
	fi;

	if [ ! -d $FMS_INSTALL_DIR ]
	then
		echo_log "Nikira Installation Directory Doesnot Exist . Exiting..."
		exit 2;
	fi		

	echo_log "Moving NikiraCoreRelease.tar to  $FMS_INSTALL_DIR";

	if [ ! -f ~/NikiraCoreRelease.tar ]
	then
		"Expected Nikira Release File(NikiraCoreRelease.tar) Not exist in $HOME"
		exit 111;
	fi;		
	echo_log "############# Install Directory Check  Succeeded #################################################"

	echo_log "Moving Install file to Install dir";
	mv ~/NikiraCoreRelease.tar $FMS_INSTALL_DIR/  >>$NBIT_HOME/$logFile 2>&1
	mv ~/CheckSumNikira.txt $FMS_INSTALL_DIR/  >>$NBIT_HOME/$logFile 2>&1
	echo_log "Install file (NikiraCoreRelease.tar) to $FMS_INSTALL_DIR" ;
}

function CleanModule
{
	echo_log " "
	echo_log "############# Cleaning old $1  ###################################################################"
	CModule=$1;
	MODULE_OLD="${CModule}_OLD_${suffix}"
	echo_log "Cleaning old $CModule if any old Installation Done...(if old $CModule avail it will be moved as $MODULE_OLD";
	mv $CModule $MODULE_OLD >>$NBIT_HOME/$logFile 2>&1
	echo_log "############# Cleaned old $1  ####################################################################"
}

function CleanAll
{
	echo_log " "
	echo_log "############# Cleaning All Modules  ##############################################################"
	FMSCLIENT_OLD="FMSCLIENT_OLD_$suffix"
	FMSROOT_OLD="FMSROOT_OLD_$suffix"
	FMSTOOLS_OLD="FMSTOOLS_OLD_$suffix"
	DBSETUP_OLD="DBSETUP_OLD_$suffix"
	COMMONLIBDIR_OLD="COMMONLIBDIR_$suffix"

	echo_log "Cleaning old Folders if any old Installation Done...";

	mv FMSCLIENT $FMSCLIENT_OLD >>$NBIT_HOME/$logFile 2>&1
	mv FMSROOT $FMSROOT_OLD >>$NBIT_HOME/$logFile 2>&1
	mv FMSTOOLS $FMSTOOLS_OLD >>$NBIT_HOME/$logFile 2>&1
	mv DBSETUP $DBSETUP_OLD >>$NBIT_HOME/$logFile 2>&1
	mv COMMONLIBDIR $COMMONLIBDIR_OLD>>$NBIT_HOME/$logFile 2>&1

	rm NikiraClient.tar  NikiraRoot.tar RubyRoot.tar ApacheRoot.tar Boost.tar Dame.tar CLucene.tar DBSetup.tar COMMONLIBDIR.tar  >>$NBIT_HOME/$logFile  2>&1
	echo_log "############# Cleaned All Modules  ###############################################################"
}

function UnTarCore
{
	if [ $COREUNTARED -gt 0 ]
	then
		echo_log "Core already untar'ed....."
		cd $FMS_INSTALL_DIR
		return 0;
	fi
	echo_log " "
	echo_log "############# UnTaring Core Tar File (NikiraCoreRelease.tar) #####################################"
	cd $FMS_INSTALL_DIR
	cksum NikiraCoreRelease.tar>CheckSumNikiraPRD.txt
	perl -pi -e 's/\t/\ /g' CheckSumNikiraPRD.txt
	perl -pi -e 's/\t/\ /g' CheckSumNikira.txt
	diff CheckSumNikiraPRD.txt CheckSumNikira.txt
	
	if [ $? -ne 0 ]
	then
		echo_log "Checksum Error: Tar file(NikiraCoreRelease.tar) is not transfered properly or checksum file(CheckSumNikira.txt) is corrupted.";
		exit 999;
	fi;
	
	cd $FMS_INSTALL_DIR
	$tar_command -xvf NikiraCoreRelease.tar >>$NBIT_HOME/$logFile
	if [ -f COMMONLIBDIR.tar ]
	then
		rm -rf $COMdir>>$NBIT_HOME/$logFile 2>&1
		$tar_command -xvf COMMONLIBDIR.tar
		mv $COMdir COMMONLIBDIR >>$NBIT_HOME/$logFile 2>&1
		chmod -R 777 COMMONLIBDIR
		rm COMMONLIBDIR.tar
	fi;
	echo_log "############# UnTaring Core Tar File Done  #######################################################"
	export COREUNTARED=1
}

function UnTarNikiraRoot
{
	echo_log "Creating FMSROOT Directory from NikiraRoot.tar"
	rm -rf $NRdir
	$tar_command -xvf NikiraRoot.tar >>extract.log
	mv $NRdir FMSROOT>>$NBIT_HOME/$logFile 2>&1
	echo_log "Created FMSROOT Directory from NikiraRoot.tar"
	echo_log "Deleteing NikiraRoot.tar"
	rm NikiraRoot.tar
	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep "PATH.SOURCE_NIKIRAROOT"`
	do
		path=`echo $i | cut -d '=' -f2`
		Replace_paths $path 'FMSROOT' $SOURCE_NIKIRAROOT
	done;
}

function UnTarNikiraClient
{
	echo_log "Creating FMSCLIENT Directory from NikiraClient.tar"
	rm -rf $NCdir
	$tar_command -xvf NikiraClient.tar >>extract.log
	mv $NCdir FMSCLIENT >>$NBIT_HOME/$logFile 2>&1
	echo_log "Created FMSCLIENT Directory from NikiraClient.tar"
	echo_log "Deleteing NikiraClient.tar"
	rm NikiraClient.tar
	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep "PATH.SOURCE_NIKIRACLIENT"`
	do
		path=`echo $i | cut -d '=' -f2`
		Replace_paths $path 'FMSCLIENT' $SOURCE_NIKIRACLIENT
	done;
}

function UnTarNikiraTools
{
	echo_log " "
	echo_log "############# UnTaring All Modules Tar Files  ####################################################"
	TarFiles=`$tar_command -tf NikiraCoreRelease.tar |grep "tar"|grep -v "DBSetup.tar"|grep -v "NikiraRoot.tar"|grep -v "NikiraClient.tar"|grep -v "COMMONLIBDIR.tar"`
	mkdir -p FMSTOOLS
	mv $TarFiles FMSTOOLS/
	cd FMSTOOLS
	echo_log "Creating FMSTOOLS  Directory from $TarFiles"
	for i in `echo $TarFiles`
	do
		mv $FMS_INSTALL_DIR/$i $FMS_INSTALL_DIR/FMSTOOLS/
		$tar_command -xvf $i;
		rm $i;	
	done;
	
	mv $FMS_INSTALL_DIR/FMSTOOLS/$NTdir/* $FMS_INSTALL_DIR/FMSTOOLS/
	cd $FMS_INSTALL_DIR/FMSTOOLS/
    rmdir $NTdir

	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep "PATH.SOURCE_NIKIRATOOLS"`
	do
		path=`echo $i | cut -d '=' -f2`
		Replace_paths $path 'FMSTOOLS' $SOURCE_NIKIRATOOLS
	done;

	cd $NBIT_HOME
	echo_log "############# UnTaring All Modules Tar Files Done  ##############################################"
}

function UnTarALL
{
	echo_log " "
	echo_log "############# Untaring All Modules and Making proper Directory Structure #########################"

	echo_log "Creating FMSCLIENT Directory from NikiraClient.tar"

	if [ ! -f NikiraClient.tar -o  ! -f NikiraRoot.tar ]
	then
		echo_log "NikiraClient/NikiraRoot tar is not avail.. Exiting.."	
		exit 121;
	fi;	

	rm -rf $NCdir>>$NBIT_HOME/$logFile 2>&1
	$tar_command -xvf NikiraClient.tar
	mv $NCdir FMSCLIENT>>$NBIT_HOME/$logFile 2>&1
	echo_log "Deleting NikiraClient.tar"
	rm NikiraClient.tar

	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep "PATH.SOURCE_NIKIRACLIENT"`
	do
		path=`echo $i | cut -d '=' -f2`
		Replace_paths $path 'FMSCLIENT' $SOURCE_NIKIRACLIENT
	done;

	echo_log "Creating FMSROOT Directory from NikiraRoot.tar"
	rm -rf $NRdir>>$NBIT_HOME/$logFile 2>&1
	$tar_command -xvf NikiraRoot.tar
	mv $NRdir FMSROOT>>$NBIT_HOME/$logFile 2>&1
	echo_log "Deleting NikiraRoot.tar"
	rm NikiraRoot.tar

	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep "PATH.SOURCE_NIKIRAROOT"`
	do
		path=`echo $i | cut -d '=' -f2`
		Replace_paths $path 'FMSROOT' $SOURCE_NIKIRAROOT
	done;

	echo_log "Creating DBSETUP  Directory from DBSetup.tar"
	mkdir -p FMSTOOLS

	if [ ! -f DBSetup.tar ]
	then
		echo_log "DBsetup tar is not avail.. Skiping DBSetup untar...."	
	else	
		mv DBSetup.tar FMSTOOLS/
		cd  FMSTOOLS
		rm -rf $DBdir>>$NBIT_HOME/$logFile 2>&1
		$tar_command -xvf DBSetup.tar
		rm DBSetup.tar
	fi;		

	cd $FMS_INSTALL_DIR
	echo_log "Creating COMMONLIBDIR  Directory from COMMONLIBDIR.tar"
	if [ ! -f COMMONLIBDIR.tar ]
	then
		echo_log "COMMONLIBDIR tar is not avail.. Skiping COMMONLIBDIR untar...."	
	else	
		rm -rf $COMdir>>$NBIT_HOME/$logFile 2>&1
		$tar_command -xvf COMMONLIBDIR.tar
		mv $COMdir COMMONLIBDIR >>$NBIT_HOME/$logFile 2>&1
		chmod -R 777 COMMONLIBDIR
		rm COMMONLIBDIR.tar
	fi;		

	mkdir -p FMSTOOLS
	cd $FMS_INSTALL_DIR/FMSTOOLS
	echo_log "Creating FMSTOOLS  Directory from $TarFiles"
	for i in `echo $TarFiles`
	do
		mv $FMS_INSTALL_DIR/$i $FMS_INSTALL_DIR/FMSTOOLS/
		$tar_command -xvf $i;
		rm $i;
	done;

	mv $FMS_INSTALL_DIR/FMSTOOLS/$NTdir/* $FMS_INSTALL_DIR/FMSTOOLS/
	cd $FMS_INSTALL_DIR/FMSTOOLS/
    rmdir $NTdir

	for i in `cat $NBIT_HOME/Extract.conf|grep -v "#"|grep "PATH.SOURCE_NIKIRATOOLS"`
	do
		path=`echo $i | cut -d '=' -f2`
		Replace_paths $path 'FMSTOOLS' $SOURCE_NIKIRATOOLS
	done;

	cd $NBIT_HOME
	echo_log "############# Untaring All Modules and Making proper Directory Done  #############################"
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
		perl -pi -e "s#LIBRARY[ ]*=.*#LIBRARY=DL.dlopen(ENV['RANGERHOME'] +'/lib/liblicensecrypto.so')#g" bin/license_crypto.rb	
		perl -pi -e   "s:\@\@dl = .*$:\@\@dl = DL.dlopen(ENV['RANGERHOME'] +'/lib/libencode.so'):g" rbin/framework/fieldutil.rb
		
			find . -name ".svn"|xargs rm -rf

		echo_log "Replaced Successfully in FMSROOT"
		cd $FMS_INSTALL_DIR
	fi
	echo_log "####################### FMSROOT PATH REPLACER FINISHED  #######################################"
}

function Replace_paths()
{
	Path=$1 ;
	Module=$2 ;
	Source_Module=$3
	echo_log " "
	echo_log "####################### $Module: $Path -- PATH REPLACER STARTED #######################################"
    if [ "$Path" == "" -a "$Source_Module" == "" ]
	then
		echo_log "Skipping the Path Replacement for $Module ........"
	else
		echo_log "Source Machine: $Module=$Source_Module";
		a=`echo $Source_Module|perl -p -e 's#\/#\\\/#g'`
		n="$FMS_INSTALL_DIR/$Module"
		echo_log "Dest Machine: $Module=$n";
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`
		cd $FMS_INSTALL_DIR/$Module/$Path
        find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|ruby script|script text|bash script|shell script|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g"
		>/dev/null 2>&1
		echo_log "Replaced Successfully in $Module"
		cd $FMS_INSTALL_DIR
	fi;
    echo_log "####################### $Module: $Path -- PATH REPLACER FINISHED  #####################################"
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
		find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|ruby script|script text|bash script|shell script|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
		perl -pi -e "s#LIB[ ]*=.*#LIB=DL.dlopen(ENV['WEBSERVER_RANGER_HOME'] +'/lib/libencode.so')#g" lib/framework/fieldutil.rb
			find . -name ".svn"|xargs rm -rf

		echo_log "Replaced Successfully in FMSCLIENT"
		cd $FMS_INSTALL_DIR
	fi

	echo_log "####################### FMSCLIENT PATH REPLACER FINISHED  #####################################"
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
		find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|ruby script|script text|bash script|shell script|script"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
			find . -name ".svn"|xargs rm -rf

		echo_log "Replaced Successfully in RUBYROOT"
	fi

	echo_log "####################### RUBYROOT PATH REPLACER FINISHED  #########################################"
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

function Replace_paths_DBSetup
{
	echo_log " "
	echo_log "####################### DBSETUP PATH REPLACER STARTED ############################################"
	n=$SOURCE_NIKIRAROOT;	

	if [ "$n" == "" ]
	then
		echo_log "Skipping the Path Replacement for DBSETUP........"
	else
		a=`echo $n|perl -p -e 's#\/#\\\/#g'`
		log_only "Production Hard Code PATH inside DBSETUP"
		n="$FMS_INSTALL_DIR/FMSROOT"
		log_only $n
		b=`echo $n|perl -p -e 's#\/#\\\/#g'`	
		cd $DBSETUP
		find .|xargs grep -sl "$a"|xargs file 2>/dev/null|egrep -i "ascii text|commands text|program text|English text|ruby script|script text|bash script|shell script|script|sql"|awk '{ print $1 }'|cut -d ":" -f1|xargs perl -pi -e "s/$a/$b/g" >/dev/null 2>&1
			find . -name ".svn"|xargs rm -rf

		echo_log "Replaced Successfully in DBSETUP"
	fi
	echo_log "####################### DBSETUP  REPLACER FINISHED ###############################################"
}

function Changes_NikiraRoot
{
	echo_log " "
	echo_log "####################### FMS ROOT CHANGES ######################################################"
	cat ~/.bashrc > $FMSROOT/sbin/rangerenv.sh
	cat ~/.bashrc > $FMSROOT/sbin/rangerenv32.sh
	perl -pi -e "s/DB_USER=.*/DB_USER=$DB_USER/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/EVENT_LOGGER_HOST=.*/EVENT_LOGGER_HOST=$NM_HOST/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/EVENT_LOGGER_PORT=.*/EVENT_LOGGER_PORT=$NM_PORT/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/DB_PASSWORD=.*/DB_PASSWORD=$DB_PASSWORD/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/APP_SERVER_IP=.*/APP_SERVER_IP=$IP_ADDRESS/g" $FMSROOT/sbin/ranger.conf
#	perl -pi -e "s/DB_PASSWORD=.*/DB_PASSWORD=$DB_PASSWORD/g" $FMSROOT/sbin/rangerenv.sh
	perl -pi -e "s/SPARK_DB_PASSWORD=.*/SPARK_DB_PASSWORD=$SPARK_DB_PASSWORD/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/APP_SERVER_LOGIN_USER.*/APP_SERVER_LOGIN_USER=$APP_SERVER_LOGIN_USER/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/APP_SERVER_LOGIN_PASSWORD.*/APP_SERVER_LOGIN_PASSWORD=$APP_SERVER_LOGIN_PASSWORD/g" $FMSROOT/sbin/ranger.conf
	perl -pi -e "s/DATABASENAME=.*/DATABASENAME=$DATABASENAME/g" $FMSROOT/sbin/ranger.conf
#	perl -pi -e "s/SPARK_DB_PASSWORD=.*/SPARK_DB_PASSWORD=$SPARK_DB_PASSWORD/g" $FMSROOT/sbin/rangerenv.sh
	perl -pi -e "s/USERID=.*/USERID=$DB_USER\/$DB_PASSWORD/g" $FMSROOT/bin/parameter.file
	perl -pi -e "s#LIB[ ]*=.*#LIB=DL.dlopen(ENV['WEBSERVER_RANGER_HOME'] +'/lib/libencode.so')#g" $FMS_INSTALL_DIR/FMSCLIENT/src/lib/framework/fieldutil.rb
	if [ $DATABASE_TYPE = "db2" ];then
		perl -pi -e "s/DB2_HOST=.*/DB2_HOST=$DB2_HOST/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
		perl -pi -e "s/DB2_PORT=.*/DB2_PORT=$DB2_PORT/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
		perl -pi -e "s!DB2DIR=.*!DB2DIR=$DB2DIR!g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
		perl -pi -e "s/DB2_DBNAME=.*/DB2_DBNAME=$DB2_DBNAME/g" $FMSROOT/sbin/ranger.conf $FMSROOT/sbin/rangerenv.sh $FMSROOT/sbin/rangerenv32.sh
	fi;
	echo_log "####################### FMS ROOT CHANGES DONE #################################################"
}

function Changes_NikiraClient
{
	echo_log " "
	echo_log "####################### FMS CLIENT CHANGES ####################################################"
	cd $FMSCLIENT
	echo "#! $RUBYROOT/bin/ruby" >1.txt
	echo "APP_CONTAINER = 'fcgi'"             >>1.txt
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
	perl -pi -e "s/username:.*/username: $DB_USER/g" $yml_dir 
	perl -pi -e "s/password:.*/password: $DB_PASSWORD/g" $yml_dir
	tr '\n' '^' < $yml_dir | sed 's/username:/spark_username:/4' | tr '^' '\n' > nbit_sample.txt
	tr '\n' '^' < nbit_sample.txt | sed 's/password:/spark_password:/4' | tr '^' '\n' > nbit_sample1.txt
	tr '\n' '^' < nbit_sample1.txt | sed 's/username:/test_username:/2' | tr '^' '\n' > nbit_sample2.txt
	tr '\n' '^' < nbit_sample2.txt | sed 's/password:/test_password:/2' | tr '^' '\n' > nbit_sample3.txt
	rm nbit_sample.txt
	rm nbit_sample1.txt
	rm nbit_sample2.txt
	perl -pi -e "s/spark_username:.*/username: $SPARK_DB_USER/g" nbit_sample3.txt
	perl -pi -e "s/spark_password:.*/password: $SPARK_DB_PASSWORD/g" nbit_sample3.txt
	perl -pi -e "s/database:.*/database: $SPARK_ORACLE_SID/g" nbit_sample3.txt
	perl -pi -e "s/test_username:.*/username: $TEST_DB_USER_NAME/g" nbit_sample3.txt
	perl -pi -e "s/test_password:.*/password: $TEST_DB_PASSWORD/g" nbit_sample3.txt
	mv nbit_sample3.txt $yml_dir
	if [ "$DATABASE_TYPE" = "db2" ];then
		perl -pi -e "s/host:.*/host: $DB2_HOST/g" $yml_dir 
		perl -pi -e "s/port:.*/port: $DB2_PORT/g" $yml_dir 
		perl -pi -e "s/database:.*/database: $DB2_DBNAME/g" $yml_dir 
		perl -pi -e "s/adapter:.*/adapter: ibm_db/g" $yml_dir 
	elif [ "$DATABASE_TYPE" = "oracle" ];then
		perl -pi -e "s/adapter:.*/adapter: oci/g" $yml_dir 
	fi
	env_dir=`find . -name "environment.rb"`
	perl -pi -e "s/RAILS_GEM_VERSION =.*/RAILS_GEM_VERSION ='$RAILS_GEM_VERSION'/g" $env_dir
	echo "\$SERVER_IP = '$IP_ADDRESS'">>$env_dir
	echo "\$SERVER_LOGIN_ID = '$LOGNAME'">>$env_dir
	echo "\$SERVER_PASSWORD = '$LOGIN_PASSWORD'">>$env_dir
	echo "\$SERVER_RANGER_HOME = '$FMSROOT'">>$env_dir
	echo "\$WSDL_FILE_URL = 'http://$IP_ADDRESS:3000/s2_k_services/wsdl'">>$env_dir
	echo "\$LOGGER_HOST = '$NM_HOST'">>$env_dir
	echo "\$LOGGER_PORT = '$NM_PORT'">>$env_dir
	Changes_query_manager ;
	cd $FMSCLIENT 
	chmod +x setup_servers.rb 
	. ~/.bashrc
	ruby setup_servers.rb --nbit

	if [ $? -eq 1 ]
	then
		echo_log "Setup server failed, Please check the logs and export the missing environment variable(s).\n Run NikiraEncryptPassword.sh"
		exit 1
	fi
    mv $QUERY_MANAGER_HOME/config/query_manager.yml_BK $QUERY_MANAGER_HOME/config/query_manager.yml
	echo_log "####################### NIKIRA CLIENT CHANGES DONE ###############################################"
}

function Changes_query_manager
{
	echo $QUERY_MANAGER_HOME/config/query_manager.yml
	echo_log " "

	echo_log "#######################  PATH REPLACEMENT FOR QUERY MANAGER STARTS ##############################################"
	echo $DB_USER $SPARK_DB_USER $DB_PASSWORD $SPARK_DB_PASSWORD

	b=`echo $FMSCLIENT|perl -p -e 's#\/#\\\/#g'` 
	perl -pi -e "s/:DB_USER:.*/:DB_USER: $DB_USER/g" $QUERY_MANAGER_HOME/config/query_manager.yml
	perl -pi -e "s/:SPARK_DB_USER:.*/:SPARK_DB_USER: $SPARK_DB_USER/g" $QUERY_MANAGER_HOME/config/query_manager.yml
    perl -pi -e "s/:DB_PASSWORD:.*/:DB_PASSWORD: $DB_PASSWORD/g" $QUERY_MANAGER_HOME/config/query_manager.yml
    perl -pi -e "s/:SPARK_DB_PASSWORD:.*/:SPARK_DB_PASSWORD: $SPARK_DB_PASSWORD/g" $QUERY_MANAGER_HOME/config/query_manager.yml
	perl -pi -e "s/:rails_root:.*/:rails_root: $b/g" $QUERY_MANAGER_HOME/config/query_manager.yml
	perl -pi -e "s/:pid_file:.*/:pid_file: $b\/..\/querymanager\/tmp\/pids\/mongrel.pid/g" $QUERY_MANAGER_HOME/config/query_manager.yml
	perl -pi -e "s/:log_file:.*/:log_file: $b\/..\/querymanager\/LOG\/query_manager.log/g" $QUERY_MANAGER_HOME/config/query_manager.yml
    perl -pi -e "s/:spark_database:.*/:spark_database: $SPARK_ORACLE_SID/g" $QUERY_MANAGER_HOME/config/query_manager.yml
	perl -pi -e "s/:spark_adapter:.*/:spark_adapter: $SPARK_ADAPTER/g" $QUERY_MANAGER_HOME/config/query_manager.yml
	cp $QUERY_MANAGER_HOME/config/query_manager.yml $QUERY_MANAGER_HOME/config/query_manager.yml_BK
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
	if [ "x$NIKIRA_HTTPS_PORT" == "x" ]
	then
		cp $NBIT_HOME/httpd.conf.template $APACHEROOT/conf/httpd.conf
	else
		cp $NBIT_HOME/httpd.conf.template.https $APACHEROOT/conf/httpd.conf
	fi

	n=$APACHEROOT
	b=`echo $n|perl -p -e 's#\/#\\\/#g'`
	perl -pi -e "s/APACHEROOT/$b/g" $APACHEROOT/conf/httpd.conf

	n=$FMSCLIENT
	b=`echo $n|perl -p -e 's#\/#\\\/#g'`
	perl -pi -e "s/NIKIRACLIENT/$b/g" $APACHEROOT/conf/httpd.conf
	perl -pi -e "s/NIKIRA_PORT/$NIKIRA_PORT/g" $APACHEROOT/conf/httpd.conf
	perl -pi -e "s/NIKIRA_HTTPS_PORT/$NIKIRA_HTTPS_PORT/g" $APACHEROOT/conf/httpd.conf
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
	
	n=$IP_ADDRESS
	b=`echo $n|perl -p -e 's#\.#\\\\\\\\.#g'`
 	perl -pi -e "s/ESCAPED_IP/$b/g" $APACHEROOT/conf/httpd.conf
		
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
	perl -pi -e "s/COMMONLIBDIR_VALUE/$b/g" $APACHEROOT/conf/httpd.conf

	n=$JAVA_HOME
	b=`echo $n|perl -p -e 's#\/#\\\/#g'`
	perl -pi -e "s/JAVA_HOME_VALUE/$b/g" $APACHEROOT/conf/httpd.conf

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

function DODBsetup
{
	echo_log " "
	echo_log "####################### DBSETUP RUNNING (Require Inputs) #########################################"
	db="Y";
	if [ "$PartitionedDB" == "NO" ]
	then
		echo_log "Do You Want to run DBSetup Default(N)[Y/N]:";
		read db;
		log_only "$db";
	fi
	if [ "$db" == "Y" -o "$db" == "y" ]
	then 	
		dbLogFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NBIT_dbsetup.log" }'`
		echo_log "##########################################################################"
		echo_log "Please wait till the DBSetup runs completely[For details check $dbLogFile]"
		echo_log "##########################################################################"
		cd $DBSETUP
		Replace_paths_DBSetup ;
		
		if [ "$PartitionedDB" != "NO" ]
		then
			cd $DBSETUP/DBSetup_Partitioned
			perl -pi -e "s/\.[ ]*oraenv .*//g" dbsetup_partitioned_new.sh
			perl -pi -e "s/\.[ ]*rangerenv.sh.*//g" dbsetup_partitioned_new.sh
			./dbsetup_partitioned_new.sh $DB_USER $DB_PASSWORD 1 >>${NBIT_HOME}/$dbLogFile
	    else
			perl -pi -e "s/\.[ ]*oraenv .*//g" $DBSETUP/dbsetup.sh
			perl -pi -e "s/\.[ ]*rangerenv.sh.*//g" $DBSETUP/dbsetup.sh
			./dbsetup.sh $DB_USER $DB_PASSWORD 1 >>${NBIT_HOME}/$dbLogFile
	    fi

		cat ${NBIT_HOME}/$dbLogFile|grep "^Error">/dev/null 2>&1
		if [ "$?" == "0" ]
		then
			mv ${NBIT_HOME}/$dbLogFile ${NBIT_HOME}/${dbLogFile}_${suffix}
			echo_log "Errors While Running DBSetup. Please check log files .. ${NBIT_HOME}/${dbLogFile}_${suffix} ..";
			echo "Would you like to continue with the installation? [Y/N] "
			read i;
			if [ "$i" == "n" -o "$i" == "N" ]
			then
				exit 1;
			fi;
		fi;


		echo "0_core|$LOGNAME|Installation_Time" >$NBIT_HOME/DBCARE/.current_version.dbmig
		echo "0_core|$LOGNAME|Installation_Time" >$NBIT_HOME/DBCARE/.version.dbmig
		echo_log "####################### DBSETUP FINISHED #########################################################"
	else
		echo_log "####################### DBSETUP SKIPPED ##########################################################"
	fi;
}

function ClearLogs
{
	echo_log " "
	echo_log "Clearing the logs "
	>$APACHEROOT/logs/error_log
	>$APACHEROOT/logs/access_log
	>$FMSCLIENT/log/production.log
	>$FMSCLIENT/log/ranger.log
	>$FMSCLIENT/log/server.log
	>$FMSCLIENT/log/fastcgi.crash.log
	>$FMSCLIENT/log/custom.log
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
	cd $FMSCLIENT
	. ~/.bashrc
	./fmsclientctl.sh restart
	Installation_Time=`date +%Dr#%H:%M:%S`;
}

function UpdateVersionFile
{
	Mod=$1;	
	echo "0_core|$LOGNAME|$Installation_Time"  >$NBIT_HOME/.current_version_$Mod
	echo "0_core|$LOGNAME|$Installation_Time"  >$NBIT_HOME/.current_version_$Mod
}

function EnvironmentCheck
{
	log_only "FMSROOT=$FMSROOT FMSCLIENT=$FMSCLIENT RUBYROOT=$RUBYROOT APACHEROOT=$APACHEROOT DB_USER=$DB_USER DB_PASSWORD=$DB_PASSWORD COMMONLIBDIR=$COMMONLIBDIR XMLRPCINSTALLDIR=$XMLRPCINSTALLDIR BITMAPINSTALLDIR=$BITMAPINSTALLDIR JUDYINSTALLDIR=$JUDYINSTALLDIR CURLINSTALLDIR=$CURLINSTALLDIR YAJLINSTALLDIR=$YAJLINSTALLDIR OPENSSLDIR=$OPENSSLDIR LIBICONVDIR=$LIBICONV_PATH AUTOCODEINSTALLDIR=$AUTOCODEINSTALLDIR";
	if [ "$FMSROOT" == "" -o "$FMSCLIENT" == "" -o "$RUBYROOT" == "" -o "$APACHEROOT" == "" -o "$DB_USER" == "" -o "$DB_PASSWORD" == "" -o "$COMMONLIBDIR" == "" -o "$XMLRPCINSTALLDIR" == "" -o "$BITMAPINSTALLDIR" == "" -o "$JUDYINSTALLDIR" == "" -o "$CURLINSTALLDIR" == "" -o "$YAJLINSTALLDIR" == "" -o "$OPENSSLDIR" == "" -o "$YAMLCPPINSTALLDIR" == "" -o "$PCRE_HOME" == "" -o "$LIBICONVDIR" == "" -o "$AUTOCODEINSTALLDIR" == "" ]
	then
		echo_log "The environmnet is not proper!!! Please edit $HOME/.bashrc and retry"
		echo_log "Specifially the following environment variables be properly set"
		echo_log "FMSROOT, FMSCLIENT, RUBYROOT, APACHEROOT, DB_USER, DB_PASSWORD, COMMONLIBDIR, XMLRPCINSTALLDIR, PCRE_HOME, YAMLCPPINSTALLDIR, BITMAPINSTALLDIR [HP-UX::LD_PRELOAD ], LIBICONVDIR, AUTOCODEINSTALLDIR"
		echo_log "FMSROOT=$FMSROOT,
        FMSCLIENT=$FMSCLIENT, 
        RUBYROOT=$RUBYROOT, 
        APACHEROOT=$APACHEROOT, 
        DB_USER=$DB_USER, 
        DB_PASSWORD=$DB_PASSWORD, 
        COMMONLIBDIR=$COMMONLIBDIR, 
        XMLRPCINSTALLDIR=$XMLRPCINSTALLDIR, 
        BITMAPINSTALLDIR=$BITMAPINSTALLDIR,
		JUDYINSTALLDIR=$JUDYINSTALLDIR,
		CURLINSTALLDIR=$CURLINSTALLDIR,
		YAJLINSTALLDIR=$YAJLINSTALLDIR,
		PCRE_HOME=$PCRE_HOME,
		YAMLCPPINSTALLDIR=$YAMLCPPINSTALLDIR,
		OPENSSLDIR=$OPENSSLDIR,
		LIBICONVDIR=$LIBICONVDIR,
		AUTOCODEINSTALLDIR=$AUTOCODEINSTALLDIR"
		echo_log "Exiting.........."
		exit 1;
	fi
}

function CurrentDirectoryCheck
{
	NBIT_HOME=`dirname $0`

	if [ "$NBIT_HOME" != "." ]
	then
		cd $NBIT_HOME
	fi
	NBIT_HOME=`pwd`
	export NBIT_HOME
	echo_log "changed dir to : `pwd`"
}

function Validations
{
	if [ $Exclude -gt 0 ]
	then 
		echo_log "Module to Exclude is $ModuleName";
		if [ "$ModuleName" == "ALL" ]
		then
		    echo_log "Cannot Exclude ALL modules. Give only one of [FMSROOT/FMSTOOLS/FMSCLIENT]";
		    Help
		    exit 1
		fi

	else
		echo_log "Module is $ModuleName";
	fi

	if [ "$ModuleName" != "FMSROOT" -a "$ModuleName" != "FMSTOOLS" -a "$ModuleName" != "FMSCLIENT"  -a "$ModuleName" != "ALL"-a "$ModuleName" != "LIBENCODE" ]
	then
		echo_log "Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT]";
		exit 3;
	fi
}

function Get_Encrypted_Password
{
     	if [ $DB_ENCRYPT -gt 0 ]
	 	then
		 	return 0;
	 	fi;

		cd $NBIT_HOME
		if [ -f $RANGERHOME/rbin/framework/fieldutil.rb ]
		then
			chmod +x ruby_fieldutil.rb
    		DB_USER=`ruby ruby_fieldutil.rb $DB_USER`
			echo_log "Encrypted Username is $DB_USER";
	   		export DB_USER;

	   		DB_PASSWORD=`ruby ruby_fieldutil.rb $DB_PASSWORD`
       		echo_log "Encrypted Password is $DB_PASSWORD";
	   		export DB_PASSWORD;

       		SPARK_DB_USER=`ruby ruby_fieldutil.rb $SPARK_DB_USER`
       		echo_log "Encrypted SPARK Username is $SPARK_DB_USER";
	   		export SPARK_DB_USER;
			
	   		SPARK_DB_PASSWORD=`ruby ruby_fieldutil.rb $SPARK_DB_PASSWORD`
	   		echo_log "Encrypted Spark Password is $SPARK_DB_PASSWORD";
	   		export SPARK_DB_PASSWORD;

			TEST_DB_USER_NAME=`ruby ruby_fieldutil.rb $TEST_DB_USER_NAME`
			echo_log "Encrypted Production DB User is $TEST_DB_USER_NAME";
			export TEST_DB_USER_NAME ;

			TEST_DB_PASSWORD=`ruby ruby_fieldutil.rb $TEST_DB_PASSWORD`
			echo_log "Encrypted Production DB Password is $TEST_DB_PASSWORD";
			export TEST_DB_PASSWORD ;

			APP_SERVER_LOGIN_USER=`ruby ruby_fieldutil.rb $APP_SERVER_LOGIN_USER`
			echo_log "Encrypted Login Username is $APP_SERVER_LOGIN_USER";
			export APP_SERVER_LOGIN_USER

			APP_SERVER_LOGIN_PASSWORD=`ruby ruby_fieldutil.rb $APP_SERVER_LOGIN_PASSWORD`
			echo_log "Encrypted Login Password is $APP_SERVER_LOGIN_PASSWORD";
			export APP_SERVER_LOGIN_PASSWORD

			export DB_ENCRYPT=1;
		else
			echo_log "make sure $RANGERHOME/rbin/framework/fieldutil.rb and libencode dependent libraries are present and proper path set in bashrc... "
			exit 999;
		fi;

}

function DoInstall
{

	if [ "$ModuleName" == "LIBENCODE" ]
	then
			mkdir -p $FMSROOT/lib/
			cd $FMS_INSTALL_DIR
			cksum NikiraCoreRelease.tar>CheckSumNikiraPRD.txt
			perl -pi -e 's/\t/\ /g' CheckSumNikiraPRD.txt
			perl -pi -e 's/\t/\ /g' CheckSumNikira.txt
			diff CheckSumNikiraPRD.txt CheckSumNikira.txt
			if [ $? -ne 0 ]
			then
				echo_log "Checksum Error: Tar file(NikiraCoreRelease.tar) is not transfered properly or checksum file(CheckSumNikira.txt) is corrupted.";
	        exit 999;
			fi;

			cd $FMS_INSTALL_DIR
			$tar_command -xvf NikiraCoreRelease.tar >>$NBIT_HOME/$logFile    
			$tar_command -xvf LIBENCODE.tar >>$NBIT_HOME/$logFile

			if [ ! -f $FMS_INSTALL_DIR/LIBENCODE/libencode.so ]
			then
				echo_log "Expected File($FMS_INSTALL_DIR/LIBENCODE/libencode.so) Not exist"
				exit 111;
			fi;

			cp $FMS_INSTALL_DIR/LIBENCODE/libencode.so $FMSROOT/lib/
			rm NikiraCoreRelease.tar CheckSumNikiraPRD.txt CheckSumNikira.txt LIBENCODE.tar 
			rm -rf $FMS_INSTALL_DIR/LIBENCODE/
			return ;
     fi;


	cd $FMS_INSTALL_DIR
	TarFiles=`$tar_command -tf NikiraCoreRelease.tar |grep "tar"|grep -v "DBSetup.tar"|grep -v "NikiraRoot.tar"|grep -v "NikiraClient.tar"|grep -v "COMMONLIBDIR.tar"`

	if [ $Exclude -le 0 ]
	then 
		if [ "$ModuleName" == "ALL" ]
		then
			CleanAll;
			echo_log "Do You Want To Remove NikiraCoreRelease.tar after Installation (Not Recomended,if u have space problem then say Yes) [N/Y] ?";
			read CoreRemove;
			echo_log "$CoreRemove";
			UnTarCore;

			if [ $CoreRemove == "Y" -o "$CoreRemove" == "y" ]
			then
				echo_log "Untar of NikiraCoreRelease.tar Over So removing NikiraCoreRelease.tar.";
				rm $FMS_INSTALL_DIR/NikiraCoreRelease.tar;
			fi;		

			UnTarALL;
			Replace_paths_NikiraRoot;
			Replace_paths_NikiraClient;
			Replace_paths_RubyRoot;
			Replace_paths_ApacheRoot;
			UpdateVersionFile "FMSROOT";	
			UpdateVersionFile "FMSCLIENT";	
			UpdateVersionFile "FMSTOOLS";	

			DODBsetup;
			Get_Encrypted_Password;
			Changes_NikiraRoot ;
			Changes_ApacheRoot ;
			Changes_NikiraClient ;
			
		elif  [ "$ModuleName" == "FMSROOT" ]
			then
			ExtractNikiraRoot
		elif [ "$ModuleName" == "FMSCLIENT" ]
		then
			ExtractNikiraClient
		elif [ "$ModuleName" == "FMSTOOLS" ]
		then
			ExtractNikiraTools
		fi;

	else 
		if [ "$ModuleName" == "FMSCLIENT" ]
		then
			ExtractNikiraRoot
			ExtractNikiraTools
		elif [ "$ModuleName" == "FMSROOT" ]
		then
			ExtractNikiraTools
			ExtractNikiraClient
		elif [ "$ModuleName" == "FMSTOOLS" ]
		then
			ExtractNikiraRoot
			ExtractNikiraClient
		fi;
	fi
	ClearLogs;
	Start_Apache ;
}

function ExtractNikiraRoot
{
	CleanModule FMSROOT ;
	UnTarCore;
	UnTarNikiraRoot;
	Replace_paths_NikiraRoot;
	Get_Encrypted_Password;
	Changes_NikiraRoot ;		 
	UpdateVersionFile "FMSROOT";	
}

function ExtractNikiraClient
{
	CleanModule FMSCLIENT ;
	UnTarCore;
	UnTarNikiraClient;
	Replace_paths_NikiraClient;
	Get_Encrypted_Password;	
	Changes_NikiraClient ;
	UpdateVersionFile "FMSCLIENT";	
}

function ExtractNikiraTools
{
	CleanModule FMSTOOLS ;
	UnTarCore;
	UnTarNikiraTools;

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
	fi;

	UpdateVersionFile "FMSTOOLS";	
}

function GenerateNewConf
{
	echo "Do you want to generate new program manager conf file [Y/N] ";
	read a;
	log_only "$a";
	if [ "$a" == "Y" -o "$a" == "y" ]
	then
		log_only "generating new program manager conf file";
		cd $NBIT_HOME
		chmod 777 GetPMConfFile.sh ;
		./GetPMConfFile.sh ;
		if [ ! -f new.conf ]
		then
           log_only "Make sure entered port range is more than actual no of ports required.";
		   exit 2;
		fi;
		mv new.conf $FMSROOT/sbin 
		echo_log "$FMSROOT/sbin contains new conf file ";
		echo_log "Manually update the ports in ranger.conf ";
	fi;



	echo "Do yo want to move non-working program manager conf files to backup? [Y/N] ";
	read b;
	log_only "$b";
	if [ "$b" == "Y" -o "$b" == "y" ]
	then
		cd $FMSROOT
		mkdir $FMSROOT/sbin/prgrm_mngr_backup
		echo "Enter the working program manager file name example -- programmanager.conf.tcp";
		read pgm_file;
		if [ ! -f sbin/$pgm_file ]
		then
             echo_log "programmanager.conf file doen not exists";
			 exit 0;
		fi;

		sed '/./,$!d' $FMSROOT/sbin/$pgm_file  >  tmp_file && mv tmp_file $FMSROOT/sbin/$pgm_file
		for filename in $FMSROOT/sbin/*
		do
			if [ -f "$filename" -a `basename $filename` != "$pgm_file" ]
			then
			sed '/./,$!d' $filename  >  tmp_file && mv tmp_file $filename
				if [[ "$(head -1 $filename)" == "$(head -1 $FMSROOT/sbin/$pgm_file)" ]]
				then
					mv $filename $FMSROOT/sbin/prgrm_mngr_backup
				fi;
			fi;
		done;
		echo "Files are moved to $FMSROOT/sbin/prgrm_mngr_backup ";
	fi;

	cd $FMS_INSTALL_DIR
	rm -f NikiraClient.tar  NikiraRoot.tar RubyRoot.tar ApacheRoot.tar Boost.tar Dame.tar CLucene.tar DBSetup.tar COMMONLIBDIR.tar
				

}

#-----------------------------------------------------Functions End----------------------------------------------------------------------------#

