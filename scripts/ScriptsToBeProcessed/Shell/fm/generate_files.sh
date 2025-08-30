#!/usr/bin/env bash

ResetLogs()
{
	if [ -f log/generate_files.log ]
	then
		> log/generate_files.log
	else
		touch log/generate_files.log
	fi
}

doneMessage()
{
	ruby -e "puts \"\x1b[38;32mDone\x1b[38;0m\""
}

failedMessage()
{
	ruby -e "puts \"\x1b[38;31mFailed\x1b[38;0m\""
}

remove_modified_file_names()
{
	> temporary.tmp
	while read line; do
		file=`echo $line | tr -s ' ' | cut -d ' ' -f 2`
		svn info $file > /dev/null 2>&1
		if (($? == 1 ))
		then
			echo $file >> temporary.tmp
		fi
	done <tmp/code_files.tmp.1

	mv temporary.tmp tmp/code_files.tmp.1
}

GenerateCode()
{
	svn up testnetwork >> log/generate_files.log 2>&1
	status=`echo $?`
	svn up testnetwork > log/checksvnworkingcopy.log 2>&1
	grep "Skipped" log/checksvnworkingcopy.log >> log/generate_files.log 2>&1
	statussvnworkcopy=`echo $?`
	rm log/checksvnworkingcopy.log
	if [ $status -eq 0 -a $statussvnworkcopy -eq 1 ] 
	then
		printf "\nRemoving the previously generated code ... "
		rake code > tmp/code_files.tmp.1 2>&1
		status=`echo $?`
		if [ $status -eq 0 ]
		then
		remove_modified_file_names
		countoflines=`cat tmp/code_files.tmp.1 | wc -l`
		countoflines=`expr $countoflines - 2`
		tail -$countoflines tmp/code_files.tmp.1 > tmp/code_files.tmp  2>&1
		rm -f tmp/code_files.tmp.1 >> log/generate_files.log 2>&1
		rm `cat tmp/code_files.tmp` >> log/generate_files.log 2>&1
		doneMessage

		printf "Restoring checked-in files ... "
		svn up `cat tmp/code_files.tmp` >> log/generate_files.log 2>&1
		doneMessage

		printf "Generating new code ... "
		rake code >> log/generate_files.log 2>&1
		rm -f tmp/code_files.tmp >> log/generate_files.log 2>&1
		doneMessage
		else
		failedMessage
		fi
	else
		ruby -e "puts \"\x1b[38;31mSkipping Code Generation as svn update failed\x1b[38;0m\""
	fi
}

GenerateExpressionValidator()
{
	printf "\nBuilding Expression Validator ... "
	racc lib/helpers/expr_checker.y -o lib/helpers/expr_checker.rb >> log/generate_files.log 2>&1
	status=`echo $?`
	if [ $status -eq 0 ]
	then
		doneMessage
	else
		failedMessage
	fi
}

GenerateWebServiceFiles()
{
	printf "\nRemoving the previously generated webservice files ... "
	rake webservice:clean >> log/generate_files.log 2>&1
	doneMessage

	printf "Generating new webservice files code ... "
	rake webservice:generate >> log/generate_files.log 2>&1
	doneMessage
}

SetupReportEnvironment()
{
	if [ ! -d "../Reports" ]; then
		ruby -e "puts \"\x1b[38;31mSkipping Report Setup as Report directory is missing\x1b[38;0m\""
	else
		printf "\nClearing old report data ... "
		rm -rf report_schedule >> log/generate_files.log 2>&1
		rm `find data -type f` > /dev/null 2>&1
		doneMessage

		printf "Creating report folders ... "
		mkdir -p $COMMON_MOUNT_POINT/Client/tmp/report_outputs/jrprints >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/tmp/report_outputs/flash_viewer_xmls >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/tmp/report_outputs/export_outputs >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/tmp/report_outputs/saved_parameters >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/data/shared_folder >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/data/user_folders >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/data/user_folders/0 >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/data/user_folders/3 >> log/generate_files.log 2>&1
		mkdir -p $COMMON_MOUNT_POINT/Client/report_schedule/report_parameters >> log/generate_files.log 2>&1
		doneMessage

		printf "Building reports ... "
		rake reportrunner:clean >> log/generate_files.log 2>&1
		rake reportrunner:install >> log/generate_files.log 2>&1
		status=`echo $?`
		if [ $status -eq 0 ]
		then
			doneMessage
		else
			failedMessage
		fi
	fi 
}

CreateFolders()
{
	printf "Creating cache folders ... "
	mkdir -p tmp/cache >> log/generate_files.log 2>&1
#	mkdir -p $COMMON_MOUNT_POINT/Client/tmp/cache >> log/generate_files.log 2>&1
	mkdir -p $COMMON_MOUNT_POINT/Client/private/Attachments/Alarm >> log/generate_files.log 2>&1
	mkdir -p $COMMON_MOUNT_POINT/Client/private/Attachments/Case >> log/generate_files.log 2>&1
	mkdir -p $COMMON_MOUNT_POINT/Client/private/Workflow >> log/generate_files.log 2>&1
	rake tmp:create
	doneMessage
}

ClearCacheFiles()
{
	printf "Clearing Rails cache files ... "
	rake tmp:cache:clear >> log/generate_files.log 2>&1
	status=`echo $?`
	if [ $status -eq 0 ]
	then
		doneMessage
	else
		failedMessage
	fi
}

PackageStylesheetsAndJavascripts()
{
	printf "\nRemoving compressed javascript and stylesheets ... "
	rake asset:packager:delete_all >> log/generate_files.log 2>&1
	doneMessage

	printf "Removing asset packager configuration files ... "
	rm config/asset_packages.yml config/asset_mapping.yml >> log/generate_files.log 2>&1
	doneMessage

	printf "Creating asset packager configuration files ... "
	rake asset:packager:create_yml_preserve_structure >> log/generate_files.log 2>&1
	doneMessage

	printf "Creating packaged javascript files ... "
	rake asset:packager:build_all >> log/generate_files.log 2>&1
	doneMessage
}

CreateInternationalizationSetUp()
{
	printf "\nCreating Dummy Internationalization Setup ... "
	rake updatepo >> log/generate_files.log 2>&1
	status=`echo $?`
	if [ $status -eq 0 ]
	then
	mkdir -p po/dm
	cp po/ranger.pot po/dm/ranger.po
	cp public/stylesheets/bluelight_en.css public/stylesheets/bluelight_dm.css
	ruby  lib/helpers/po_formatter.rb po/dm/ranger.po --DUMMY >> log/generate_files.log 2>&1
	rake makemo >> log/generate_files.log 2>&1
	doneMessage
	else
		failedMessage
		ruby -e "puts \"\x1b[38;31mSkipping Internationalization setup\x1b[38;0m\""
	fi
}

ExtractHelpFiles()
{
    printf "\nCreating Help Files ..."
    mkdir -p public/help_files/
    tar -C public/help_files/ -xzf resources/en_help_files.tar.gz
    doneMessage
}

GenerateFmInfinispanCacheLibs()
{

    printf "\nGenerating FM Infinispan Library ..."
	if [ ! -d "../infinispan_distributed_cache_src" ]; then
		failedMessage
		ruby -e "puts \"\x1b[38;31mUnable to proceed FM Infinispan Library Generation as  ../infinispan_distributed_cache_src directory is missing\x1b[38;0m\""
	else
		./infinispan_cache_lib_generate.sh >> log/GenerateFmInfinispanCacheLibs.log 2>&1
		status=`echo $?`
		if [ $status -eq 0 ]
		then
    		doneMessage
		else
			failedMessage
			ruby -e "puts \"\x1b[38;31mFailed to Generate FM Infinispan Library, please check log/GenerateFmInfinispanCacheLibs.log\x1b[38;0m\""
		fi
	fi
}

ResetLogs

GenerateCode
GenerateExpressionValidator
CreateFolders
SetupReportEnvironment
GenerateWebServiceFiles
PackageStylesheetsAndJavascripts
ClearCacheFiles
CreateInternationalizationSetUp
ExtractHelpFiles
GenerateFmInfinispanCacheLibs
printf "\nCheck log/generate_files.log for status.\n\n"
