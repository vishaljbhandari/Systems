#!/bin/env bash

# Usage: $0 [<No of Times to run test>] [Executable] [Suite Name] [Test Case Name]
# 
# No of Times to run test : 2 is only supported if u want to run the test case twice
# Executable : Name of the executable for which testcases need to be run
# Suite Name : Name of the Suite for which testcases need to be run
# Test Case Name : Name of the testcase to be run

for file in core*
do
	IS_CORE_FILE=`file ${file} | grep "\<core\>"`
	if [ "${IS_CORE_FILE}x" != "x" ]
	then
		rm -rf $file
	fi
done

User=nikira5
Password=nikira5

if [ -z "$User" ];then
	echo -e "Please set DEV_DB_USER and DEV_DB_PASSWORD env variables in .bashrc to not get this prompt.\n\n"
#    read -p "Enter DB Username: " User
#    read -p "Enter DB Password: " Password
elif [ -z "$Password" ];then
    Password="$User"
fi;
executables="Framework/runframeworktests
Processor/Function/MatchFunction/bitmapindexbuilder
Common/runcommontests
Common/scriptlaunchertests
ProgramManager/programmanager
License/licenseengine
Processor/Common/runprocessorcommontests
Processor/Function/runfunctiontests
Processor/Counter/runprocessorcountertests 
Processor/InMemoryCounterManager/inmemorycountermanager
Processor/InMemoryCounterDebugger/inmemorycounterdebugger
MachineLearning/Lib/HMM/runmllibtests 
MachineLearning/Manager/runmlmanagertests
MachineLearning/Store/HMM/runmlstoretests
MachineLearning/AlarmQualifier/Interface/HMM/runmlinterfacetests 
MachineLearning/AlarmQualifier/Processor/HMM/runmlprocessortests
MachineLearning/AlarmQualifier/Trainer/HMM/runmltrainertests
InlineWrapper/IRMAgent/irmagent
InlineWrapper/runinlinewrappertest
Processor/InvalidSubscriberProcessor/invalidsubscriberprocessor
Processor/FPPreProcessor/fppreprocessor		
Processor/CDRProcessor/recorddispatcher
Processor/CDRProcessor/recordprocessor
Processor/ParticipationController/participationcontroller
AlarmDenormalizer/alarmdenormalizer
Processor/Action/runprocessoractiontest 
SmartPatternProcessor/smartpatternprocessor 
Processor/FingerPrintGenerator/ondemandfingerprintgenerator
AlarmGenerator/alarmgenerator"

if [ $# -ge 1 ];then
	if [ "$1" = "2" ];then
		Twice=1
		shift ;
	else
		Twice=0
	fi;

	if [ ! -z "$3" ];then
		RunCmd="s"
	elif [ ! -z "$2" ];then
		RunCmd="t"
	elif [ ! -z "$1" ];then
		RunCmd="r"
	fi

	FirstCmd="$RunCmd
$2
$3"

	if [ $Twice -eq 1 ];then
		SecondCmd="$RunCmd
$2
$3"
	fi;

	FullCmd="$FirstCmd
$SecondCmd
q"

	echo "Running $1..."
	./$1 -T $User $Password << x
	$(echo $FullCmd)
x
else
	##(cd ../DBSetup && ./dbsetup.sh $User $Password)
	##. ./Scripts/testsetup.sh
	for executable in $executables
	do
		if [ -x $executable ];then
			echo ""
			echo "Running Tests for $executable..."
			./$executable -T $User $Password << x
			r
			r
			q
x
		else
			echo "Warning: Executable $executable not available. Skipping Test"
		fi
	done

fi
echo
