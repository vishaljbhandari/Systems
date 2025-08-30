#!/bin/bash

############################################################################################################################
# *  Copyright (c) Subex Limited 2006. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from SubexAzure Limited or in accordance with the terms and     *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
#
#     This Script brings up the Nikira setup with the help of configuration mentioned in following conf files:
#	1: bringNikiraSetup.conf          - used to mention necessary checkout paths and target directory
#	2: serverSetup.conf       - used to mention required server modules to be configured/enabled
#	3: dbSetup.conf           - used to mention required db configuration to be enabled
#	4: clientSetupServer.conf - used to mention desired ports on which various applications will be run
#
#       Author: Sandeep Kumar Gupta                                      Last Modified Date: June 11, 2009
#
############################################################################################################################

if [ -f ~/.bashrc ]
then
        . ~/.bashrc
fi

checkSuccessfulExecution()
{
        if [ $? -ne 0 ]
        then
                echo "Last statement not executed successfully !!!!"
				/bin/mail -s "bringNikiraSetup.sh failed at $CurrentAct" \
				$EMAIL < $LOGFILE 

				case "$CurrentAct" in
					"compiling Client")		doNotRunClientUT=1
											bringNikiraSetupFailed=1
											failedProcess="$failedProcess$CurrentAct "
											return 1;;

					"running Server Setup")	doNotRunServerUT=1
											bringNikiraSetupFailed=1
											failedProcess="$failedProcess$CurrentAct "
											return 1;;

					"Compiling Server")		doNotRunServerUT=1
											bringNikiraSetupFailed=1
											failedProcess="$failedProcess$CurrentAct "
											return 1;;

					"running ClientUTs"|"running ServerUTs")	return 2;;

					*)						bringNikiraSetupFailed=1
											failedProcess="$failedProcess$CurrentAct "
											exit 3;;
				esac
        fi
}


fetchCheckoutConf()
{
CONF_DIRECTORY=$( cd -P -- "$(dirname -- "$0")" && pwd -P )
	if [ ! -f $CONF_DIRECTORY/bringNikiraSetup.conf ]
	then
		echo "conf file missing."
		exit 1
	fi
CLIENT_CHECKOUTPATH=`awk 'BEGIN {FS = "="} /CLIENT_CHECKOUTPATH/ {print $2}' $CONF_DIRECTORY/bringNikiraSetup.conf`
SERVER_CHECKOUTPATH=`awk 'BEGIN {FS = "="} /SERVER_CHECKOUTPATH/ {print $2}' $CONF_DIRECTORY/bringNikiraSetup.conf`
DB_CHECKOUTPATH=`awk 'BEGIN {FS = "="} /DB_CHECKOUTPATH/ {print $2}' $CONF_DIRECTORY/bringNikiraSetup.conf`
TARGET_DIRECTORY=`awk 'BEGIN {FS = "="} /TARGET_DIRECTORY/ {print $2}' $CONF_DIRECTORY/bringNikiraSetup.conf`
EMAIL=`awk 'BEGIN {FS = "="} /EMAIL/ {print $2}' $CONF_DIRECTORY/bringNikiraSetup.conf`
takeUpdateInExistingSetup=`awk 'BEGIN {FS = "="} /takeUpdateInExistingSetup/ {print $2}' $CONF_DIRECTORY/bringNikiraSetup.conf`
}

takeUpdate()
{
CurrentAct="taking update"
cd $TARGET_DIRECTORY/Client
echo "Taking updates in Client ...."
svn update
	checkSuccessfulExecution

cd $TARGET_DIRECTORY/Server
echo "Taking updates in Server ...."
svn update
    checkSuccessfulExecution

cd $TARGET_DIRECTORY/DBSetup
echo "Taking updates in DBSetup ...."
svn update
    checkSuccessfulExecution

}

takeCheckout()
{
CurrentAct="taking Checkout"
cd $TARGET_DIRECTORY

echo "Taking Client checkout ..."
svn co $CLIENT_CHECKOUTPATH
	checkSuccessfulExecution

echo "Taking Server checkout ..."
svn co $SERVER_CHECKOUTPATH
	checkSuccessfulExecution

echo "Taking DB checkout ..."
svn co $DB_CHECKOUTPATH
	checkSuccessfulExecution
}

runServerSetup()
{
CurrentAct="running Server Setup"
cd $TARGET_DIRECTORY/Server
bash $TARGET_DIRECTORY/Server/setup.sh <<-FETCH_SERVER_CONF
	`awk -F= '/<>serverSetup.conf<>/ { i++ }{if (i==1 && i<2) if($0 ~ /^[a-zA-Z]/)print $2;;}' $CONF_DIRECTORY/bringNikiraSetup.conf`
	`echo "Y" #it has been added for confirmation`
	FETCH_SERVER_CONF
	checkSuccessfulExecution

}

compileServer()
{
CurrentAct="Compiling Server"
cd $TARGET_DIRECTORY/Server
echo "Replacing \"make -j2\" with \"make\" in mk ..."
perl -pi -e "s/make\ *-j[0-9]/make/" mk
	grep -vo "make\ *-j" mk
	checkSuccessfulExecution

echo "Replacing RangerRoot with NIKIRAROOT ..."
perl -pi -e "s/RangerRoot/NIKIRAROOT/" mk
	grep -vo "RangerRoot" mk
	checkSuccessfulExecution

echo "Running mk ..."
chmod 777 config*
./mk
	checkSuccessfulExecution

echo "Running make install ..."
make install
	checkSuccessfulExecution

echo "Running testSetup.sh ...."
bash Scripts/testsetup.sh
	checkSuccessfulExecution

}

createDBSetup()
{
CurrentAct="running DBSetup"
cd $TARGET_DIRECTORY/DBSetup
echo "Running DBSetup/dropall.sh ...."
bash dropall.sh $DB_USER $DB_PASSWORD
	checkSuccessfulExecution

echo "Running DBSetup/Setup.sh ...."
bash setup.sh <<-FETCH_DB_CONF
	`awk -F= '/<>dbSetup.conf<>/ { i++ }{if (i==1 && i<2) if($0 ~ /^[a-zA-Z]/)print $2;;}' $CONF_DIRECTORY/bringNikiraSetup.conf`
	`echo "Y" #it has been added for confirmation`
	FETCH_DB_CONF
	checkSuccessfulExecution

if [ "x$PREVEAHOME" == "x" ]
then 
	echo "export PREVEAHOME = 'x'" >> $RANGERHOME/sbin/rangerenv.sh
fi

echo "Removing execution step to run oraevn from dbsetup.sh ..." # this step causes dbset to ask for orasid and oraclehome each time dbsetup is run
grep -v "oraenv" dbsetup.sh > dbsetup_systembackup
mv dbsetup_systembackup dbsetup.sh
chmod 777 dbsetup.sh

echo "Running DBSetup/dbsetup.sh ...."
bash dbsetup.sh $DB_USER $DB_PASSWORD
	checkSuccessfulExecution
}

replaceEncryptedPassword()
{
CurrentAct="replacing Encrypted Password"
echo "Updating ranger.conf for DB_user, DB_Password and event_logger_host ...."
encryptedPassword=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF 2> /dev/null | grep -v ^$
        set heading off ;
        set feedback off ;
                select fieldutil.encode('$DB_PASSWORD') from dual;
        quit ;
        EOF`
	echo $encryptedPassword | grep -vo "ORA-[0-9]*"
	checkSuccessfulExecution

encryptedDBuser=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF 2> /dev/null | grep -v ^$
        set heading off ;
		set feedback off ;
			select fieldutil.encode('$DB_USER') from dual;
		quit ;
		EOF`
	echo $encryptedDBuser | grep -vo "ORA-[0-9]*"
    checkSuccessfulExecution

    echo "encryptedDBuser   = $encryptedDBuser"
	echo "encryptedPassword = $encryptedPassword"


echo "Replacing DB_user and DB_Password in ranger.conf ...."
perl -pi -e "s/DB_USER=\ */DB_USER=$encryptedDBuser/" $RANGERHOME/sbin/ranger.conf
perl -pi -e "s/DB_PASSWORD=\ */DB_PASSWORD=$encryptedPassword/" $RANGERHOME/sbin/ranger.conf
perl -pi -e "s/EVENT_LOGGER_HOST=[0-9\.]*\ */EVENT_LOGGER_HOST=$IP_ADDRESS/" $RANGERHOME/sbin/ranger.conf
}

compileClient()
{
CurrentAct="compiling Client"
cd $TARGET_DIRECTORY/Client/src
echo "Updating Client/src/config/database.yml ...." ####### it is done as the DB_USER and password don't get fetched automatically
perl -pi -e "s/.*username:.*/  username: $encryptedDBuser/g" config/database.yml
perl -pi -e "s/.*password:.*/  password: $encryptedPassword/g" config/database.yml

echo "Running Client/src/setup_servers.rb ..."
./setup_servers.rb <<-FETCH_CLIENTSETUPSERVER_CONF
	`awk -F= '/<>clientSetupServer.conf<>/ { i++ }{if (i==1 && i<2) if($0 ~ /^[a-zA-Z]/)print $2;;}' $CONF_DIRECTORY/bringNikiraSetup.conf`
	FETCH_CLIENTSETUPSERVER_CONF
	checkSuccessfulExecution

echo "Running Client/src/generate_files.sh ..."
bash generate_files.sh
	checkSuccessfulExecution

}

updateDispatchFcgi()
{
	CurrentAct="Updating dispatch.fcgi"
	cd $TARGET_DIRECTORY/Client/src/public

	perl -pi -e "s;^\#\!.*;\#\!$RUBYROOT/bin/ruby;" dispatch.fcgi

	requireLineNo=`grep -n "require" dispatch.fcgi | head -1 | cut -d ":" -f 1`
	noOfLinesBefore=`expr $requireLineNo - 1`
	totalNoOfLines=`cat dispatch.fcgi | wc -l`
	noOfLinesAfter=`expr $totalNoOfLines - $noOfLinesBefore`

	head -$noOfLinesBefore dispatch.fcgi > dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby/gems/1.8/gems/soap4r-1.5.8/lib\'>>dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby\'>>dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby/1.8\'>>dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby/1.8/i686-linux\'>>dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby/site_ruby/1.8\'>>dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby/site_ruby/1.8/i686-linux\'>>dispatch.fcgi_BNSetup
	echo \$:\<\<\'$RUBYROOT/lib/ruby/gems/1.8/gems\'>>dispatch.fcgi_BNSetup
	tail -$noOfLinesAfter dispatch.fcgi >> dispatch.fcgi_BNSetup

	mv dispatch.fcgi_BNSetup dispatch.fcgi
	checkSuccessfulExecution

}

generateLicense()
{
	CurrentAct="Generating License"
	cd $TARGET_DIRECTORY/Server/License/LicenseGenerator/
	
	ruby generate_license.rb license.yaml
		checkSuccessfulExecution

	cp license.lcf ~/NIKIRAROOT/sbin/
		checkSuccessfulExecution
}


runClientUTs()
{
CurrentAct="running ClientUTs"
cd $TARGET_DIRECTORY/Client/src
echo "Running Client UT's ..."
	rake 
	checkSuccessfulExecution
}


runServerUTs()
{
CurrentAct="running ServerUTs"
cd $TARGET_DIRECTORY/Server
echo "Running Server UT's ..."
	./runtestapp.sh 
	checkSuccessfulExecution
}

main()
{
fetchCheckoutConf
logFileDirectoryName="Log_`date +%d%m%Y_%k%M%S | tr -d ' '`"
	mkdir -p $TARGET_DIRECTORY/LOG/$logFileDirectoryName

logDirectory="$TARGET_DIRECTORY/LOG/$logFileDirectoryName"
doNotRunClientUT=1
doNotRunServerUT=1
CurrentAct=""
bringNikiraSetupFailed=0
failedProcess=""
encryptedPassword=""
encryptedDBuser=""

echo -e "All logs are available in directory: \x1b[38;32m$logDirectory/\x1b[38;0m"

if [ $takeUpdateInExistingSetup -eq 1 ]
then
	echo -n "Taking update in existing setup ...."
	LOGFILE="$logDirectory/takeUpdate.log"
	takeUpdate &>$LOGFILE 2>&1
	echo -e "\x1b[38;32m Done\x1b[38;0m"
else
	echo -n "Taking Checkouts ...."
	LOGFILE="$logDirectory/checkout.log"
	takeCheckout &>$LOGFILE 2>&1
	echo -e "\x1b[38;32m Done\x1b[38;0m"
fi

# if the following fails then RangerRoot doesn't get created.
echo -n "Running Server/Setup.sh ...."  
LOGFILE="$logDirectory/serverSetup.log"
runServerSetup &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

echo -n "Compiling Server ...."
LOGFILE="$logDirectory/serverCompile.log"
compileServer &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

echo -n "Creating DB Setup ...."
LOGFILE="$logDirectory/setupDB.log"
createDBSetup &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

echo -n "Replacing encrypted password in ranger.conf ...."
LOGFILE="$logDirectory/passwordReplacing.log"
replaceEncryptedPassword &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

if [ $doNotRunServerUT -eq 0 ]
then 
	echo -n "Running Server UT's ...."
	LOGFILE="$logDirectory/ServerUTs.log"
	runServerUTs &>$LOGFILE 2>&1
	echo -e "\x1b[38;32m Done\x1b[38;0m"
	/bin/mail -s "bringNikiraSetup: Finished $CurrentAct" \
					$EMAIL < $LOGFILE 
fi

echo -n "Compiling Client ...."
LOGFILE="$logDirectory/compileClient.log"
compileClient &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

if [ $doNotRunClientUT -eq 0 ]
then 
	echo -n "Running Client UT's ...."
	LOGFILE="$logDirectory/ClientUTs.log"
	runClientUTs &>$LOGFILE 2>&1
	echo -e "\x1b[38;32m Done\x1b[38;0m"
	/bin/mail -s "bringNikiraSetup: Finished $CurrentAct" \
				$EMAIL < $LOGFILE 
fi

echo -n "updating Client/src/public/dispatch.fcgi ...."
LOGFILE="$logDirectory/updateDispatch.fcgi"
updateDispatchFcgi &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

echo -n "Generating License ...."
LOGFILE="$logDirectory/generatingLicense.log"
generateLicense &>$LOGFILE 2>&1
echo -e "\x1b[38;32m Done\x1b[38;0m"

if [ $bringNikiraSetupFailed -eq 1 ]
then
	echo -e "\x1b[38;31mNikira installation Failed in Process \"$failedProcess\".\x1b[38;0m"
else
	echo -e "\x1b[38;32mNikira Setup completed successfully.\x1b[38;0m"
fi
return $bringNikiraSetupFailed
}

main
