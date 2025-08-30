#!/usr/bin/env bash

. testFunctions.sh

main()
{
	RunProg "./runUT.sh" "Running Server UTs" "Failures found in Server UTs"
	RunProg "(cd ../DBSetup && ./dbsetup.sh $DB_USER) && (cd ../Server/Scripts/RubyScripts && rake)" "Failures Found in Rubscript UTs"
	RunProg "(cd ../DBSetup && ./dbsetup.sh $DB_USER) && (cd ../Server/Scripts && ./Tests/checkscripts.sh)" "Failures Found in Script UTs"
	RunProg "(cd ../DBSetup && ./dbsetup.sh $DB_USER) && (cd ../Server/RecordLoader/RecordLoader && rake)" "Failures Found in RecordLoader UTs"
	RunProg "(cd ../DBSetup && ./dbsetup.sh $DB_USER) && (cd ../Client/src && rake)" "Failures Found in Client UTs"
	RunProg "(cd ../DBSetup && ./dbsetup.sh $DB_USER) && (cd ../Client/querymanager && rake)" "Failures Found in querymanager UTs"
	RunProg "(cd ../Client/notificationmanager && rake)" "Failures Found in notificationmanager UTs"
	RunProg "(cd ../DBSetup && ./dbsetup.sh $DB_USER && ./Tests/runtests.sh $DB_USER)" "Failures Found in DBSetup UTs"
	
	BackupLog
}

main
