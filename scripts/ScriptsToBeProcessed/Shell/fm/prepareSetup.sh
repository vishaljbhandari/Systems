#!/usr/bin/env bash

# Path Resolve
BaseDir=$(dirname $0)
cd $BaseDir

. ./testFunctions.sh
AuckFile=.AuckPrepare

PrepareUpdateSetup()
{

	RepoUpdate "DBSetup" "Updating DBSetup" "(cd ../Server/Scripts/RubyScripts && ruby options_runner.rb)"
	RepoUpdate "Server" "Updating Sever" "BuildServer && (cd ../Server && ./Scripts/testsetup.sh)"
	RepoUpdate "Client/src" "Updating Client" "(cd ../Client/src && ./generate_files.sh)"
	RepoUpdate "Client/querymanager" "Updating querymanager"
	RepoUpdate "Client/notificationmanager" "Updating notificationmanager"
}

PrepareRunSetup()
{
	RunProg "BuildServer && (cd ../Server && ./Scripts/testsetup.sh)" "Setting up Server"
	RunProg "(cd ../Client/src && ./generate_files.sh)" "Setting up Client"
}

CheckTag()
{
	svn info | grep tags 2>&1 > /dev/null
	if [ $? -eq 0 ];then
		Tag=1
	else
		Tag=0
	fi
}

PrepareSetup()
{
	CheckTag
	if [ -f $AuckFile ] && [ $Tag -eq 0 ];then
		PrepareUpdateSetup
	else
		PrepareRunSetup
		echo "Prepared @ $(date)" > $AuckFile
	fi
}

if [ "x$1" = x ];then
	PrepareSetup >> $LogFile 2>&1
fi
