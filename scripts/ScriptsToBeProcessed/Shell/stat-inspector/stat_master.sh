#!/bin/bash

SCURR_DIR=`pwd`
SCRIPT_NAME="stat_master.sh"
CURRENTDATE=`date +"%Y%m%d"`
SCURRENTTIME=`date +"%Y%m%d%H%M%S"`

STAT_DATA_FILE_PREFIX="stat_report_file_"
STAT_DATA_FILE_SUFFIX=".txt"
STAT_DATA_FILE="stat_records.db"

FILE_SELECTOR_REGEX='\.h,|\.c,|\.cc,|\.cpp,'
FILE_DESELECTOR_REGEX="\.java,"
FILE_LOCATION_TAG=""
MASTER_DATA_FILE="stat_master.data"

function finish {
	cd $SCURR_DIR
}
trap finish EXIT
echo -e "[00002] Current Directory "$SCURR_DIR
if [ ! -f $SCURR_DIR/$INSPECTOR_CODE ] ; then
	echo -e "[ERROR] INVALID INSPECTOR DIRECTORY, Script Must Be Running From The Inspector Rood Directory"
	exit 999
fi
CONF_FILE=$SCURR_DIR"/conf/inspector.conf"
if [ ! -f $CONF_FILE ] ; then
	echo -e "[ERROR] Missing Config File "$CONF_FILE
	exit 998
fi
echo -e "[ Info] Config File : "$CONF_FILE

LANDING_DIR=$SCURR_DIR"/landing"
if [ ! -d $LANDING_DIR ] ; then
	echo -e "[ERROR] Missing Landing Directory "$LANDING_DIR
	exit 997
fi
echo -e "[ Info] Landing Directory : "$LANDING_DIR

CASE_ROOT=$CURR_DIR
CASE="cases"
CASES_DIR=$CURR_DIR/$CASE
if [ ! -d $CASES_DIR ] ; then
	echo -e "[ERROR] Missing Cases Directory "$CASES_DIR
	exit 997
fi
echo -e "[ Info] Cases Directory : "$CASES_DIR

cd $SCURR_DIR

DAY_DIFF='+0'
if [ -f $SCURR_DIR/.stat_adjustment ] ; then
	sed -i '/^$/d' $SCURR_DIR/.stat_adjustment
	grep -v '^ *$' $SCURR_DIR/.stat_adjustment > $SCURR_DIR/.stat_adjustment_tmp
	mv $SCURR_DIR/.stat_adjustment_tmp $SCURR_DIR/.stat_adjustment
	DAY_DIFF=`cat $SCURR_DIR/.stat_adjustment | head -1`
fi	
STAT_DATA_FILE_TIMESTAMP=`date -d "$DAY_DIFF day" +"%Y%m%d"`
echo -e "[ INFO] Running with Day Adjustment of ["$DAY_DIFF"] and file time stamp as "$STAT_DATA_FILE_TIMESTAMP
INPUT_STAT_FILE=$LANDING_DIR/$STAT_DATA_FILE_PREFIX$STAT_DATA_FILE_TIMESTAMP$STAT_DATA_FILE_SUFFIX

$RUN_DETAILS_FILE = $SCURR_DIR'/cases/.runDetails';
if [ ! -f $RUN_DETAILS_FILE ] ; then
	echo -e "[ERROR] Missing RUN_DETAILS File "$RUN_DETAILS_FILE
	exit 997
fi

ALREADY_PROCESSED=`grep $STAT_DATA_FILE_TIMESTAMP $RUN_DETAILS_FILE | wc -l`

if [ $ALREADY_PROCESSED -ge 1 ] ; then
	echo -e "[ERROR] Already Processed for the day "$STAT_DATA_FILE_TIMESTAMP", Hence skipping state processing"
	exit 0;
fi

if [ ! -f $INPUT_STAT_FILE ] ; then
	echo -e "[ERROR] Missing Input Stat File "$INPUT_STAT_FILE
	exit 997
fi
echo -e "[ INFO] Processing stat input file "$INPUT_STAT_FILE
dos2unix $INPUT_STAT_FILE

CASE_ROOT=$SCURR_DIR
CASE_DIR=$CASE_ROOT/$CASE"/C"$CURRENTDATE
if [ ! -d $CASE_DIR ] ; then
	echo -e "[ERROR] Missing CASE Directory "$CASE_DIR
	exit 777
fi
echo -e "[00004] Current Case Directory : "$CASE_DIR

cd $SCURR_DIR
if [ ! -z "$FILE_LOCATION_TAG" -a "$FILE_LOCATION_TAG" != " " ]; then
	egrep $FILE_LOCATION_TAG $INPUT_STAT_FILE > $INPUT_STAT_FILE.TMP2
	mv $INPUT_STAT_FILE.TMP2 $INPUT_STAT_FILE
fi

if [ ! -z "$FILE_DESELECTOR_REGEX" -a "$FILE_DESELECTOR_REGEX" != " " ]; then
	egrep -v $FILE_DESELECTOR_REGEX $INPUT_STAT_FILE > $INPUT_STAT_FILE.TMP2
	mv $INPUT_STAT_FILE.TMP2 $INPUT_STAT_FILE
fi

if [ ! -z "$FILE_SELECTOR_REGEX" -a "$FILE_SELECTOR_REGEX" != " " ]; then
	egrep "'$FILE_SELECTOR_REGEX'" $INPUT_STAT_FILE > $INPUT_STAT_FILE.TMP2
	mv $INPUT_STAT_FILE.TMP2 $INPUT_STAT_FILE
fi

rm $INPUT_STAT_FILE.TMP* 2>/dev/null
#Field Mapping
#1 Project		-> 7
#2 Module		-> 8
#3 Priority		-> 2
#4 Category		-> 4
#5 FileName 	-> 1
#6 LineNumber	-> 3
#7 FilePath		-> 5
#8 Link			-> 6

awk -F"," '{ print $5"##"$3"##"$6"##"$4"##"$7"##"$8"##"$1"##"$2 }' $INPUT_STAT_FILE | sort -u > $INPUT_STAT_FILE.TMP1
mv $INPUT_STAT_FILE.TMP1 $LANDING_DIR/$MASTER_DATA_FILE

echo -e "[INFO] Stat report success fully generated from file "$INPUT_STAT_FILE
echo -e "[INFO] Stat report data is populated into "$LANDING_DIR/$MASTER_DATA_FILE

ERROR_DB_FILE_NAME=`cat $CONF_FILE | grep ERROR_DATABASE_FILE | head -1 | awk -F"=" '{ print $2}'`
ERROR_DB_FILE=$CASES_DIR/$ERROR_DB_FILE_NAME
if [ ! -f $ERROR_DB_FILE ]; then
	echo -e "[INFO] ERROR_DB_FILE "$ERROR_DB_FILE" is missing, Creating a new one"
	touch $ERROR_DB_FILE
fi
RUN_DETAIL_FILE=$CASES_DIR"/.runDetails"
sed -i '/^$/d' $RUN_DETAIL_FILE
grep -v '^ *$' $RUN_DETAIL_FILE | sort -r > $RUN_DETAIL_FILE.tmp && mv $RUN_DETAIL_FILE.tmp $RUN_DETAIL_FILE
PREVIOUS_RUN_DATE=`tail -1 $RUN_DETAIL_FILE`

SROOT_DIRECTORY=`cat $CONF_FILE | grep GIT_SOURCE_ROOT | grep -v src | head -1 | awk -F"=" '{ print $2}'`
echo -e "[ Info] Root Directory is set to "$SROOT_DIRECTORY

if [ ! -d $SROOT_DIRECTORY ] ; then
	echo -e "[ERROR] Invalid Root Directory : "$SROOT_DIRECTORY
    exit 997
fi
if [ ! -d $SROOT_DIRECTORY/src ] ; then
	echo -e "[ERROR] Invalid Root SRC Directory : "$SROOT_DIRECTORY"/src"
    exit 997
fi
cd $SROOT_DIRECTORY"/src"

TOTAL_ERRORS=`wc -l $LANDING_DIR/$MASTER_DATA_FILE | awk -F" " '{ print $1}'`

echo -e "[ INFO] Total errors reported in current run: "$TOTAL_ERRORS

touch $CASE_DIR/.tmpStater
while IFS= read -r line
do	
	ERR_LINE_NUMBER=`echo $line | awk -F"##" '{print $3}'`
	ERR_FILE_NAME=`echo $line | awk -F"##" '{print $1}'`
	ERR_FILE_PATH=`echo $line | awk -F"##" '{print $5}'`
	
	BASE_DIR=`dirname $CASE_DIR/$ERR_FILE_PATH`
	mkdir -p $BASE_DIR
	cp -rp $SROOT_DIRECTORY/$ERR_FILE_PATH $CASE_DIR/$ERR_FILE_PATH
	
	ERR_LINE_NUMBER_PLUS=$(( $ERR_LINE_NUMBER + 2 ))
	ERR_LINE_NUMBER_MINUS=$(( $ERR_LINE_NUMBER - 2 ))
	sed -n "$ERR_LINE_NUMBER_MINUS, $ERR_LINE_NUMBER_PLUS p" $CASE_DIR/$ERR_FILE_PATH > $CASE_DIR/$ERR_FILE_PATH.part
	
	F_HASH=`sha224sum $CASE_DIR/$ERR_FILE_PATH | awk -F" " '{ print $1}'`
	
	ERR_FIVE_LINE_HASH=`shasum $CASE_DIR/$ERR_FILE_PATH.part | awk -F" " '{ print $1}'`
	ERR_ERROR_ID=`echo $ERR_FILE_NAME"#"$ERR_FIVE_LINE_HASH | cksum | awk -F" " '{ print $1}'`
	ERR_FILE_HASH=$F_HASH
	
	PREV_ERROR_COUNT=`grep $ERR_FIVE_LINE_HASH $ERROR_DB_FILE | wc -l`
	ERR_ERROR_VERSION=$(( $PREV_ERROR_COUNT + 1 ))
	
	ERR_PROJECT=`echo $line | awk -F"##" '{print $7}'`
	ERR_MODULE=`echo $line | awk -F"##" '{print $8}'`
	ERR_LINK=`echo $line | awk -F"##" '{print $6}'`
	ERR_PRIORITY=`echo $line | awk -F"##" '{print $2}'`
	ERR_CATEGORY=`echo $line | awk -F"##" '{print $4}'`
	
	echo -e $STAT_DATA_FILE_TIMESTAMP"##"$ERR_FILE_PATH"##"$ERR_ERROR_VERSION"##"$ERR_ERROR_ID"##"$ERR_FILE_NAME"##"$ERR_PRIORITY"##"$ERR_CATEGORY"##"$ERR_LINE_NUMBER"##"$ERR_FIVE_LINE_HASH"##"$ERR_FILE_HASH"##"$ERR_PROJECT"##"$ERR_MODULE"##"$ERR_LINK >> $CASE_DIR/.tmpStater
done < $LANDING_DIR/$MASTER_DATA_FILE

cat $CASE_DIR/.tmpStater >> $ERROR_DB_FILE
sort -u $ERROR_DB_FILE > $ERROR_DB_FILE.tmp
mv $ERROR_DB_FILE.tmp $ERROR_DB_FILE

mv  $CASE_DIR/.tmpStater $CASE_DIR/stat/day_error_status.$SCURRENTTIME.txt
sort -u $CASE_DIR/stat/day_error_status.$SCURRENTTIME.txt $CASE_DIR/stat/day_error_status.$SCURRENTTIME.txt.tmp
mv $CASE_DIR/stat/day_error_status.$SCURRENTTIME.txt.tmp $CASE_DIR/stat/day_error_status.$SCURRENTTIME.txt

echo -e "[INFO] Current ERROR DB File is written at "$CASE_DIR"/stat/day_error_status."$SCURRENTTIME".txt"
echo -e "[INFO] Master ERROR DB File is written at "$ERROR_DB_FILE
echo -e "[INFO] State Master Run Completed"

#01 Date		=> Rundate
#02 FilePath	=> 5th token of line, sep by ##
#03 ErrorVersion	=> 
#04 ErrorId		=> cksum of FileName, ERR_FIVE_LINE_HASH
#05 FileName	=> 1st token of line, sep by ##
#06 Priority	=> 2nd token of line, sep by ##
#07 Category	=> 4th token of line, sep by ##
#08 LineNumber	=> 3rd token of line, sep by ##
#09 5LineErrorHashCode	=> LineNumber-2 to LineNumber+2 Lines of error file and get hash
#10 FileHash	=> Hash of file content
#11 Project		=> 7th token of line, sep by ##
#12 Module		=> 8th token of line, sep by ##
#13 Link		=> 6th token of line, sep by ##