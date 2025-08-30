#!/usr/bin/env bash

ResetLogs()
{
	touch log/generate_files.log
}

doneMessage()
{
	ruby -e "puts \"\x1b[38;32mDone\x1b[38;0m\""
}

failedMessage()
{
	ruby -e "puts \"\x1b[38;31mFailed\x1b[38;0m\""
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

GenerateCode()
{
	printf "\nSaving the non generatable code files ... "
	SaveNonGeneratableFiles 
	doneMessage

	printf "\nRemoving the previously generated code ... "
	rake code > tmp/code_files.tmp.1 2>&1
	status=`echo $?`
	if [ $status -eq 0 ]
	then
		countoflines=`cat tmp/code_files.tmp.1 | wc -l`
		countoflines=`expr $countoflines - 2`
		tail -$countoflines tmp/code_files.tmp.1 > tmp/code_files.tmp  2>&1
		rm -f tmp/code_files.tmp.1 >> log/generate_files.log 2>&1
		rm `cat tmp/code_files.tmp` >> log/generate_files.log 2>&1
		doneMessage

		printf "Restoring non generatable code files ... "
		tar -xvf tmp/non_generated_files.tar >> log/generate_files.log 2>&1
		doneMessage

		printf "Generating new code ... "
		rake code >> log/generate_files.log 2>&1
		rm -f tmp/code_files.tmp >> log/generate_files.log 2>&1
		doneMessage
	else
		failedMessage
	fi
}

SaveNonGeneratableFiles()
{
echo "app/controllers/rate_plan_controller.rb
app/models/fraud_type.rb
app/models/free_number.rb
app/models/rater_special_number.rb
app/models/configuration.rb
public/javascripts/case_reason.js
app/models/prepaid_top_up.rb
app/models/sdr_rate.rb
app/models/time_zone_rate_type.rb
app/models/case_reason.rb
app/models/analyst_action.rb
app/views/rate_plan/_form.rhtml
app/views/rate_plan/_cannot_add_message.rhtml
app/models/non_fraud_type.rb
app/models/pending_fraud_type.rb
app/models/rate_per_call.rb
app/controllers/fraud_type_controller.rb
app/controllers/time_zone_rate_type_controller.rb
app/controllers/prepaid_top_up_controller.rb
app/controllers/rate_per_call_controller.rb" > tmp/non_generatable_files.txt
tar -cv -T tmp/non_generatable_files.txt -f tmp/non_generated_files.tar >> log/generate_files.log 2>&1
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

GenerateCode
PackageStylesheetsAndJavascripts
ClearCacheFiles
printf "\nCheck log/generate_files.log for status.\n\n"
