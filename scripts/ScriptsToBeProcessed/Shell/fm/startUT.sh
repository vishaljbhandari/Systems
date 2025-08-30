#!/usr/bin/env bash

. testFunctions.sh

RunProg "(cd ../Server && svn up)" "Updating Sever"
RunProg "(cd ../Client/querymanager && svn up)" "Updating querymanager" 
RunProg "(cd ../Client/notificationmanager && svn up)" "Updating notificationmanager" 
RunProg "(cd ../Client/src && svn up)" "Updating Client" 
RunProg "(cd ../DBSetup && svn up)" "Updating DBSetup"
RunProg "(cd ../Server/Scripts/RubyScripts && ruby options_runner.rb)" "Setting up For Compilation" 
RunProg "./build.sh" "Compiling Server" 
RunProg "(cd ../Client/src && ./generate_files.sh)" "Setting up Client"
RunProg "./runAllUTs.sh" "Running All UTs"
