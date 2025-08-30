#!/bin/sh
#################################################################################
# MASTER REPORT
# AUTHOR  : VISHAL BHANDARI 
#################################################################################
CURR_DIR=`pwd`
SCRIPT_NAME="run.sh"
ROOT_PRODUCT_TAG=""

STAT_DIR="stat-reports"
MASTER_STAT_FILE="stat_master.data"

GIT_DIR="git-reports"
GIT_SUMMARY_MASTER_FILE="git_summary_commit_master.txt"
GIT_DETAILED_MASTER_FILE="git_detailed_commits_master.txt"

ARCHIVE_ROOT=$CURR_DIR
ARCHIVE="ARCHIVE"
CURRENTDATE=`date +"%Y%m%d"`
ARCHIVE_DIR=$ARCHIVE_ROOT/$ARCHIVE-$CURRENTDATE
mkdir -p $ARCHIVE_DIR

NumericReg='^[0-9]+$'
CURRENTTIME=`date +"%Y%m%d%H%M%S"`
LOG_FILE=$CURR_DIR/git_generator.$CURRENTTIME.log
touch $LOG_FILE
function finish {
	touch $CURR_DIR/.$SCRIPT_NAME.$CURRENTTIME.lock
	rm $CURR_DIR/.$SCRIPT_NAME.$CURRENTTIME.lock
	echo -e "Exiting Script.."
	echo -e "Deleting lock ."$SCRIPT_NAME.$CURRENTTIME.lock
	
	# Cleaning Section
	touch .tmpCRITICAL && rm .tmpCRITICAL
	touch .tmpHIGH && rm .tmpHIGH
}
trap finish EXIT
touch $CURR_DIR/.$SCRIPT_NAME.tmp.lock
running_count=`ls $CURR_DIR/.$SCRIPT_NAME.*.lock | wc -l`
if [ $running_count -gt 1 ] ; then
	echo -e "[ERROR] "$SCRIPT_NAME" already running, cant start"
    exit 99
fi

touch $CURR_DIR/.$SCRIPT_NAME.$CURRENTTIME.lock
if [ $# -eq 1 ] ; then
	echo -e "[ERROR] Invalid Usage : source "$SCRIPT_NAME
    exit 1
fi

if [ ! -d $CURR_DIR/$STAT_DIR ] ; then
	echo -e "[ERROR] Missing DIRECTORY : "$CURR_DIR/$STAT_DIR
    exit 11
fi

if [ ! -f $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE ] ; then
	echo -e "[ERROR] Missing FILE : "$CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE
    exit 12
fi

if [ ! -d $CURR_DIR/$GIT_DIR ] ; then
	echo -e "[ERROR] Missing DIRECTORY : "$CURR_DIR/$STAT_DIR
    exit 21
fi

if [ ! -f $CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE ] ; then
	echo -e "[ERROR] Missing FILE : "$CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE
    exit 22
fi

if [ ! -f $CURR_DIR/$GIT_DIR/$GIT_DETAILED_MASTER_FILE ] ; then
	echo -e "[ERROR] Missing FILE : "$CURR_DIR/$GIT_DIR/$GIT_DETAILED_MASTER_FILE
    exit 23
fi

OUTPUT_REPORT=$CURR_DIR/user-git-stat-report.html
mv $OUTPUT_REPORT $ARCHIVE_DIR/
touch $OUTPUT_REPORT
echo -e "<!DOCTYPE html><html><head><title>" >> $OUTPUT_REPORT
echo -e "DP - USER STAT GIT Report As On "$CURRENTDATE >> $OUTPUT_REPORT
cat html_header_part2.txt >> $OUTPUT_REPORT


SummaryReport()
{
	echo -e "<h2>Summary Report</h2>" >> $OUTPUT_REPORT
	echo -e '<button class="collapsible">Expand</button><div class="content"><p><table>' >> $OUTPUT_REPORT
	echo -e "<tr><td>Run Date</td><td>" $(date) "</td></tr>" >> $OUTPUT_REPORT
	echo -e "<tr><td>Run Duration</td><td>1 Month</td></tr>" >> $OUTPUT_REPORT
	echo -e "<tr><td>Total Commits</td><td>" $(wc -l $CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE | awk -F" " '{ print $1}') "</td></tr>" >> $OUTPUT_REPORT
	user_count=$(cat $CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE | awk -F"#-#" '{ print $4}' | uniq | wc -l)
	echo -e "<tr><td>Total Users, Who Commited</td><td>" $user_count "</td></tr>" >> $OUTPUT_REPORT
	echo -e "<tr><td>Total Errors("$(wc -l $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE | awk -F" " '{ print $1}')")</td><td>" >> $OUTPUT_REPORT
	echo -e "<table>" >> $OUTPUT_REPORT	
	critical_err=$(awk -F"##" '{ print $2}' $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE | grep -i "critical" | wc -l)	
	echo -e "<tr><td>Critical</td><td>"$critical_err"</td></tr>" >> $OUTPUT_REPORT
	high_err=$(awk -F"##" '{ print $2}' $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE | grep -i "high" | wc -l)	
	echo -e "<tr><td>High</td><td>"$high_err"</td></tr>" >> $OUTPUT_REPORT
	low_err=$(awk -F"##" '{ print $2}' $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE | grep -i "low" | wc -l)	
	echo -e "<tr><td>Low</td><td>"$low_err"</td></tr>" >> $OUTPUT_REPORT	
	medium_err=$(awk -F"##" '{ print $2}' $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE | grep -i "medium" | wc -l)	
	echo -e "<tr><td>Medium</td><td>"$medium_err"</td></tr>" >> $OUTPUT_REPORT
	echo -e "</table>" >> $OUTPUT_REPORT
	echo -e "</td></tr>" >> $OUTPUT_REPORT
	total_files=$(awk -F"##" '{ print $2}' $CURR_DIR/$GIT_DIR/$GIT_DETAILED_MASTER_FILE | uniq | wc -l)	
	echo -e "<tr><td>Total Files Modified(.c,.cc,.h) </td><td>" $total_files "</td></tr>" >> $OUTPUT_REPORT
	files_error=$(awk -F"," '{ print $5}' $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE | egrep '\.cc$|\.c$|\.h$' | uniq | wc -l)
	echo -e "<tr><td>Total Files with errors(.c,.cc,.h)</td><td>" $files_error "</td></tr>" >> $OUTPUT_REPORT
	echo -e "<tr><td>File with Maximum Errors</td><td>xxx</td></tr>" >> $OUTPUT_REPORT
	echo -e "<tr><td>File with Maximum Errors(Critical)</td><td>xxx</td></tr>" >> $OUTPUT_REPORT
	echo -e '</table></p></div>' >> $OUTPUT_REPORT
}
SummaryReport

ErrorTagging()
{
	ERROR_TYPE=`echo $1  | tr [a-z] [A-Z]`	
	cd $CURR_DIR
	ERROR_GREP="##"$ERROR_TYPE"##"
	grep -i $ERROR_GREP $CURR_DIR/$STAT_DIR/$MASTER_STAT_FILE > $CURR_DIR/.tmp$ERROR_TYPE
	echo -e "Creating Error Tagging Report for "$ERROR_TYPE
	while IFS= read -r line
	do
		error_id=`echo $line | awk -F"##" '{print $9}'`		#ErrorTag
		error_name=`echo $line | awk -F"##" '{print $4}'` 	#Category
		file_name=`echo $line | awk -F"##" '{print $1}'`	#FileName
		file_path=`echo $line | awk -F"##" '{print $5}'`	#FilePath
		error_link=`echo $line | awk -F"##" '{print $6}'`	#Link
		line_number=`echo $line | awk -F"##" '{print $3}'`	#LineNumber
		grep $file_path $CURR_DIR/$GIT_DIR/$GIT_DETAILED_MASTER_FILE | awk -F"##" '{ print $1}' | sort -ur > $CURR_DIR/.tmpCommitId
		total_commits_on_file=$(wc -l $CURR_DIR/.tmpCommitId | awk -F" " '{ print $1}')
		echo -e '<button class="collapsible '$ERROR_TYPE'">' >> $OUTPUT_REPORT
		#echo -e $error_id " - " $ERROR_TYPE " - " $error_name " @" $line_number " line" >> $OUTPUT_REPORT
		echo -e  $ERROR_TYPE " - " $error_name " at ["$file_path" : "$line_number"]  with ["$total_commits_on_file"] commits" >> $OUTPUT_REPORT
		echo -e '</button><div class="content">' >> $OUTPUT_REPORT
			
		echo -e '<h4>File : <i><a href="#">'$file_path"</a></i> with ["$total_commits_on_file"] commits in last <i>[1 month]</i>, " >> $OUTPUT_REPORT
		echo -e '<i><a href="'$error_link'">Click here for error description</a></i></h4>' >> $OUTPUT_REPORT
		
		if [ $total_commits_on_file -gt 0 ] ; then
		
			echo -e '<p><b>Commit history in chronological order</b></br><table>' >> $OUTPUT_REPORT
			#echo -e $CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE"---- commit file --- "
			echo -e '<tr><th>Commit id</th><th>User Info</th><th>Date</th><th>Comments</th></tr>' >> $OUTPUT_REPORT
			while IFS= read -r commit_idn
			do
				#echo -e $CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE"---- commit file --- "$commit_idn
				grep $commit_idn $CURR_DIR/$GIT_DIR/$GIT_SUMMARY_MASTER_FILE | uniq > $CURR_DIR/.tmpCommit.$commit_idn
				while IFS= read -r commit_information
				do
					#echo -e $commit_information" --- "
					echo -e '<tr>' >> $OUTPUT_REPORT			
					echo -e '<td>'$commit_idn'</br>['$(echo $commit_information | awk -F"#-#" '{print $1}')']</td>' >> $OUTPUT_REPORT			
					echo -e '<td>'$(echo $commit_information | awk -F"#-#" '{print $3}')'</br>'$(echo $commit_information | awk -F"#-#" '{print $4}')'</td>' >> $OUTPUT_REPORT			
					echo -e '<td>'$(echo $commit_information | awk -F"#-#" '{print $5}')'</td>' >> $OUTPUT_REPORT			
					echo -e '<td>'$(echo $commit_information | awk -F"#-#" '{print $7}')'</td>' >> $OUTPUT_REPORT			
					echo -e '</tr>' >> $OUTPUT_REPORT
				done < $CURR_DIR/.tmpCommit.$commit_idn
				touch $CURR_DIR/.tmpCommit.$commit_idn && rm $CURR_DIR/.tmpCommit.$commit_idn
				
			done < $CURR_DIR/.tmpCommitId
			touch $CURR_DIR/.tmpCommitId && rm $CURR_DIR/.tmpCommitId
			echo -e "</table></p>" >> $OUTPUT_REPORT
		
		fi
		echo -e "</div>" >> $OUTPUT_REPORT
	done < $CURR_DIR/.tmp$ERROR_TYPE
	touch $CURR_DIR/.tmp$ERROR_TYPE && rm $CURR_DIR/.tmp$ERROR_TYPE
}
echo -e "<h2>Error Tagging Report</h2>" >> $OUTPUT_REPORT
ErrorTagging "critical"
ErrorTagging "high"
ErrorTagging "medium"
ErrorTagging "low"

cat html_footer_part1.txt >> $OUTPUT_REPORT
echo -e "Report ["$OUTPUT_REPORT"] is ready for output"