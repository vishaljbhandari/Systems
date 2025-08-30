#!/bin/bash
GCURR_DIR=`pwd`
GSCRIPT_NAME="git_master.sh"
CURRENTDATE=`date +"%Y%m%d"`
GCURRENTTIME=`date +"%Y%m%d%H%M%S"`
CommitReg='commit\ *'
function finish {
	cd $GCURR_DIR
}
trap finish EXIT
echo -e "[00002] Current Directory "$GCURR_DIR
if [ ! -f $GCURR_DIR/$INSPECTOR_CODE ] ; then
	echo -e "[ERROR] INVALID INSPECTOR DIRECTORY, Script Must Be Running From The Inspector Rood Directory"
	exit 999
fi
CONF_FILE=$GCURR_DIR"/conf/inspector.conf"
if [ ! -f $CONF_FILE ] ; then
	echo -e "[ERROR] Missing Config File "$CONF_FILE
	exit 998
fi
echo -e "[ Info] Config File : "$CONF_FILE

CASE_ROOT=$CURR_DIR
CASE="cases"
CASES_DIR=$CASE_ROOT/$CASE
CASE_DIR=$CASE_ROOT/$CASE"/C"$CURRENTDATE
if [ ! -d $CASE_DIR ] ; then
	mkdir -p $CASE_DIR $CASE_DIR/git 2>/dev/null
fi
echo -e "[00004] Current Case Directory : "$CASE_DIR

LANDING_DIR=$GCURR_DIR"/landing"
if [ ! -d $LANDING_DIR ] ; then
	echo -e "[ERROR] Missing Landing Directory "$LANDING_DIR
	exit 997
fi
echo -e "[ Info] Landing Directory : "$LANDING_DIR

GIT_SUMMARY_REPORT_PREFIX=`cat $CONF_FILE | grep GIT_SUMMARY_REPORT_PREFIX | head -1 | awk -F"=" '{ print $2}'`
GIT_DETAILED_FILE_PREFIX=`cat $CONF_FILE | grep GIT_DETAILED_FILE_PREFIX | head -1 | awk -F"=" '{ print $2}'`
GROOT_DIRECTORY=`cat $CONF_FILE | grep GIT_SOURCE_ROOT | grep -v src | head -1 | awk -F"=" '{ print $2}'`
echo -e "[ Info] Root Directory is set to "$GROOT_DIRECTORY

if [ ! -d $GROOT_DIRECTORY ] ; then
	echo -e "[ERROR] Invalid Root Directory : "$GROOT_DIRECTORY
    exit 997
fi
if [ ! -d $GROOT_DIRECTORY/src ] ; then
	echo -e "[ERROR] Invalid Root SRC Directory : "$GROOT_DIRECTORY"/src"
    exit 997
fi
cd $GROOT_DIRECTORY"/src"

GIT_STARTING_DATE=`cat $CONF_FILE | grep GIT_STARTING_DATE | head -1 | awk -F"=" '{ print $2}'`

echo -e "[ INFO] Preparing summary report from "$GROOT_DIRECTORY"/src"
git log --pretty=format:"%h#-#%H#-#%an#-#%ae#-#%cd#-#%ct#-#%s" --since="$GIT_STARTING_DATE" > $LANDING_DIR/$GIT_SUMMARY_REPORT_PREFIX$GCURRENTTIME
echo -e "[ INFO] Preparing detailed report from "$GROOT_DIRECTORY"/src"
git log --stat --since="$GIT_STARTING_DATE" > $LANDING_DIR/$GIT_DETAILED_FILE_PREFIX$GCURRENTTIME

summary_success=`grep 'Not a git repository' $LANDING_DIR/$GIT_SUMMARY_REPORT_PREFIX$GCURRENTTIME | wc -l`
if [ $summary_success -ne 0 ] ; then
	echo -e "[ERROR] Failed to extract git summary report from "$GROOT_DIRECTORY"/src"
	cat $LANDING_DIR/$GIT_SUMMARY_REPORT_PREFIX$GCURRENTTIME
    exit 897
fi

detailed_success=`grep 'Not a git repository' $LANDING_DIR/$GIT_DETAILED_FILE_PREFIX$GCURRENTTIME | wc -l`
if [ $detailed_success -ne 0 ] ; then
	echo -e "[ERROR] Failed to extract git detailed report from "$GROOT_DIRECTORY"/src"
	cat $LANDING_DIR/$GIT_DETAILED_FILE_PREFIX$GCURRENTTIME
    exit 898
fi


#git log --since="last month" --pretty=format:"%h#-#%H#-#%an#-#%ae#-#%cd#-#%ct#-#%s" > $GCURR_DIR/$GIT_SUMMARY_REPORT_PREFIX$GCURRENTTIME
#git log --stat --since="last month" > $GCURR_DIR/$GIT_DETAILED_FILE_PREFIX$GCURRENTTIME

GIT_DETAILED_FILE=$LANDING_DIR/$GIT_DETAILED_FILE_PREFIX$GCURRENTTIME

egrep "\.h\ \|\ |\.cc\ \|\ |\.c\ \|\ |commit\ " $GIT_DETAILED_FILE > $GIT_DETAILED_FILE.TMP

mv $GIT_DETAILED_FILE.TMP $GIT_DETAILED_FILE

LINE_PREFIX=""
touch $GIT_DETAILED_FILE.TMP && rm $GIT_DETAILED_FILE.TMP && touch $GIT_DETAILED_FILE.TMP
while IFS= read -r line
do
	if [[ $line = $CommitReg ]] ; then
		LINE_PREFIX=`cut -d' ' -f2 <<< $line`
	fi
	trimmed_line=$(echo $line |sed 's/^ *//g' | sed 's/ *$//g')
	echo $LINE_PREFIX"##"$trimmed_line >> $GIT_DETAILED_FILE.TMP
done < $GIT_DETAILED_FILE

egrep "\.h\ \|\ |\.cc\ \|\ |\.c\ \|\ " $GIT_DETAILED_FILE.TMP > $GIT_DETAILED_FILE.TMP1

if [ ! -z "$ROOT_PRODUCT_TAG" -a "$ROOT_PRODUCT_TAG" != " " ]; then
	grep $ROOT_PRODUCT_TAG $GIT_DETAILED_FILE.TMP1 > $GIT_DETAILED_FILE.TMP2
	mv $GIT_DETAILED_FILE.TMP2 $GIT_DETAILED_FILE.TMP
fi

touch $GIT_DETAILED_FILE.TMP2 && rm $GIT_DETAILED_FILE.TMP2 && touch $GIT_DETAILED_FILE.TMP2
while IFS= read -r line
do
	commit_hash=`echo $line | awk -F"##" '{print $1}'`
	file_part=`echo $line | awk -F"##" '{print $2}' | awk -F"|" '{ print $1}'`
	file_name=$(echo $file_part |sed 's/^ *//g' | sed 's/ *$//g')
	line_part=`echo $line | awk -F"##" '{print $2}' | awk -F"|" '{ print $2}' | awk -F" " '{ print $1}'`
	line_number=$(echo $line_part |sed 's/^ *//g' | sed 's/ *$//g')
	echo $commit_hash"##src/"$file_name"##"$line_number >> $GIT_DETAILED_FILE.TMP2
done < $GIT_DETAILED_FILE.TMP

egrep -v 'src\/commit |src\/\.\.\.' $GIT_DETAILED_FILE.TMP2 > $GIT_DETAILED_FILE.TMP3

rm $GIT_DETAILED_FILE.TMP $GIT_DETAILED_FILE.TMP1 $GIT_DETAILED_FILE $GIT_DETAILED_FILE.TMP2

GIT_DETAILED_MASTER_FILE=`cat $CONF_FILE | grep GIT_DETAILED_MASTER_FILE | awk -F"=" '{ print $2}'`
GIT_SUMMARY_MASTER_FILE=`cat $CONF_FILE | grep GIT_SUMMARY_MASTER_FILE | awk -F"=" '{ print $2}'`

cp $GIT_DETAILED_FILE.TMP3 $CASE_DIR/git/$GIT_DETAILED_MASTER_FILE
mv $GIT_DETAILED_FILE.TMP3 $CASES_DIR/$GIT_DETAILED_MASTER_FILE

if [ -f $CASES_DIR/$GIT_DETAILED_MASTER_FILE.bkp ] ; then
	cat $CASES_DIR/$GIT_DETAILED_MASTER_FILE.bkp > $CASES_DIR/$GIT_DETAILED_MASTER_FILE.bkp1
	cat $CASES_DIR/$GIT_DETAILED_MASTER_FILE >> $CASES_DIR/$GIT_DETAILED_MASTER_FILE.bkp1
	mv $CASES_DIR/$GIT_DETAILED_MASTER_FILE.bkp1 $CASES_DIR/$GIT_DETAILED_MASTER_FILE
else
	cp $CASES_DIR/$GIT_DETAILED_MASTER_FILE $CASES_DIR/$GIT_DETAILED_MASTER_FILE.bkp
fi

cp $LANDING_DIR/$GIT_SUMMARY_REPORT_PREFIX$GCURRENTTIME $CASE_DIR/git/$GIT_SUMMARY_MASTER_FILE
mv $LANDING_DIR/$GIT_SUMMARY_REPORT_PREFIX$GCURRENTTIME $CASES_DIR/$GIT_SUMMARY_MASTER_FILE

if [ -f $CASES_DIR/$GIT_SUMMARY_MASTER_FILE.bkp ] ; then
	cat $CASES_DIR/$GIT_SUMMARY_MASTER_FILE.bkp > $CASES_DIR/$GIT_SUMMARY_MASTER_FILE.bkp1
	cat $CASES_DIR/$GIT_SUMMARY_MASTER_FILE >> $CASES_DIR/$GIT_SUMMARY_MASTER_FILE.bkp1
	mv $CASES_DIR/$GIT_SUMMARY_MASTER_FILE.bkp1 $CASES_DIR/$GIT_SUMMARY_MASTER_FILE 
else
	cp $CASES_DIR/$GIT_SUMMARY_MASTER_FILE $CASES_DIR/$GIT_SUMMARY_MASTER_FILE.bkp
fi

echo -e "[ INFO] Master GIT file is "$CASES_DIR/$GIT_DETAILED_MASTER_FILE
echo -e "[ Info] Summary GIT Report File "$CASES_DIR/$GIT_SUMMARY_MASTER_FILE
