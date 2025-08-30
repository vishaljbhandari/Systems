#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh

############################################################################################################################
# *  Copyright (c) Subex Limited 2006. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from SubexAzure Limited or in accordance with the terms and     *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
#
#     This Script computes no of AT's passed, failed, error and total time taken by a suite
#       It computes summary for report folder passed as an argument (else considers present working directory)
#       This script considers summary.html file to compute summary (so individual AT report updated won't be considered)
#
#       Author: Sandeep Kumar Gupta                                      Last Modified Date: August 18, 2009
#
############################################################################################################################

totalNoOfAT=1217

CURRENT_DIRECTORY=$( cd -P -- "$(dirname -- "$0")" && pwd -P )

if [ $# -lt 2 ]
then
	echo -e "\n[IMPROPER USAGE] !!!! Two Parameters required\n\t1: Report Directory path\n\t2: Output File name\n"
	exit 1
else
	reportDirectoryName=$1
	outputFileName=$2
	echo -e "\nGenerating summary for all reports in \"$reportDirectoryName\" directory .....\n">$CURRENT_DIRECTORY/generatedSummary.txt
fi
echo -e "Note: summary generated on the basis of summary.html\n\n">>$CURRENT_DIRECTORY/generatedSummary.txt


find $reportDirectoryName -name "summary.html" > $CURRENT_DIRECTORY/allSummaryFiles.txt


echo -e "Pass\tFail\tError\t%Pass\tTotalTime(sec)\tSuiteName\n">>$CURRENT_DIRECTORY/generatedSummary.txt
echo "---------------------------------------------------------------------------------------------------------------">>$CURRENT_DIRECTORY/generatedSummary.txt

>$CURRENT_DIRECTORY/suiteSummary.txt
noOfSummaryFiles=`cat $CURRENT_DIRECTORY/allSummaryFiles.txt | wc -l`

for (( i=1 ; i<=noOfSummaryFiles ; i++ ))
do
	suiteName=`cat $CURRENT_DIRECTORY/allSummaryFiles.txt | head -$i | tail -1 | awk 'BEGIN {FS = "summary.html"} {print $1}'`
	printSuiteName=`echo $suiteName | awk 'BEGIN {FS = "[0-9]*-[0-9]*-[0-9]*/"} {print $2}'`

	awk -v PRINTSUITENAME="$printSuiteName" ' 
		BEGIN {
			noOfErrors=0;
			noOfFailures=0;
			noOfPass=0;
			totalTime=0;

			FS = "</*td>";

			}
		/"Failure"/ {noOfFailures++} 
		/"Error"/ {noOfErrors++} 
		/"Pass"/ {noOfPass++} 
		/<td>[0-9]*\.[0-9]*<\/td>/ {totalTime=totalTime+$2}

		END {
			printf noOfPass"\t"; 
			printf noOfFailures"\t";
			printf noOfErrors"\t";
			printf ("%d%s", noOfPass/(noOfPass+noOfFailures+noOfErrors)*100, " %\t");
			printf totalTime"\t\t";
			printf PRINTSUITENAME"\n";
			}
	' "$suiteName/summary.html">>$CURRENT_DIRECTORY/suiteSummary.txt

done

# This Awk computes total summary for the entire suite saved in file suiteSummary.txt
	awk '
		BEGIN {
			totalATPassed=0;
			totalATFailed=0;
			totalATError=0;
			totalSuiteTime=0;

			}
		{print $0}
		{totalATPassed=totalATPassed+$1}
		{totalATFailed=totalATFailed+$2}
		{totalATError=totalATError+$3}
		{totalSuiteTime=(totalSuiteTime+$6)}
		END {
			print "---------------------------------------------------------------------------------------------------------------"
			printf totalATPassed"\t";
			printf totalATFailed"\t";
			printf totalATError"\t";
			printf ("%3.2f%s", totalATPassed/(totalATPassed+totalATFailed+totalATError)*100, "%\t");
			printf ("%g%s", totalSuiteTime/3600, "(Hrs.)");
			}
		' $CURRENT_DIRECTORY/suiteSummary.txt>>$CURRENT_DIRECTORY/generatedSummary.txt

echo -e "\n---------------------------------------------------------------------------------------------------------------\n">>$CURRENT_DIRECTORY/generatedSummary.txt
rm $WATIR_SERVER_HOME/BringNikiraSetup/svndetails.txt
cd $SERVER_CHECKOUT_PATH
svn info>$WATIR_SERVER_HOME/BringNikiraSetup/svndetails.txt
URL=`cat $WATIR_SERVER_HOME/BringNikiraSetup/svndetails.txt |grep URL`
revision=`cat $WATIR_SERVER_HOME/BringNikiraSetup/svndetails.txt |grep Revision`

################## the below portion generates the header summary for mailing ################

echo -e "\nThe Summary for the Executed AT suite is as under:\n">$CURRENT_DIRECTORY/summaryHeader.txt
summaryComponents=`tail -3 "$CURRENT_DIRECTORY/generatedSummary.txt" | head -1`

noOfpassAT=`echo $summaryComponents | cut -d " " -f 1`
noOfFailAT=`echo $summaryComponents | cut -d " " -f 2`
noOfErrorAT=`echo $summaryComponents | cut -d " " -f 3`
passPercentage=`echo $summaryComponents | cut -d " " -f 4`
totalExecutionTime=`echo $summaryComponents | cut -d " " -f 5`

totalATExecuted=`expr $noOfpassAT + $noOfFailAT + $noOfErrorAT`
totalATnotExecuted=`expr $totalNoOfAT - $totalATExecuted`
totalATnotPassed=`expr $noOfFailAT + $noOfErrorAT`
fail100=`echo $totalATnotPassed \\* 100 | bc`
failPercentage=`echo "scale = 2; $fail100 / $totalATExecuted" | bc`
executed100=`echo $totalATExecuted \\* 100 | bc`
executedPercentage=`echo "scale = 2; $executed100 / $totalNoOfAT" | bc`

echo "Total Testcases   : $totalNoOfAT"			>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Total not executed: $totalATnotExecuted"		>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Total Executed    : $totalATExecuted"		>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Total time taken  : $totalExecutionTime"		>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Total Passed      : $noOfpassAT"			>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Total Failed      : $totalATnotPassed"		>>$CURRENT_DIRECTORY/summaryHeader.txt
echo ""							>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Percentage of AT executed: $executedPercentage%"	>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Pass percentage          : $passPercentage"	>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "Fail percentage          : $failPercentage%"	>>$CURRENT_DIRECTORY/summaryHeader.txt
echo ""							>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "SVN Details"					>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "svn "	       	         $URL			>>$CURRENT_DIRECTORY/summaryHeader.txt
echo "svn "	                 $revision	        >>$CURRENT_DIRECTORY/summaryHeader.txt						
echo ""                                                 >>$CURRENT_DIRECTORY/summaryHeader.txt

cat $CURRENT_DIRECTORY/summaryHeader.txt $CURRENT_DIRECTORY/generatedSummary.txt >$CURRENT_DIRECTORY/$outputFileName
