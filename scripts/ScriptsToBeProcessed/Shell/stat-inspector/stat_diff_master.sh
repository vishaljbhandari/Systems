#!/bin/bash
###################################
###################################
# STAT MASTER REPORT
# AUTHOR  : VISHAL BHANDARI 
###################################
%configs=(-SCRIPT_NAME => "stat_diff_master.sh", -STAT_DATABASE_FILE => "stat_records.db", -MASTER_DATA_FILE => "stat_master.data");

if [ $# -eq 1 ] ; then
	echo -e "[ERROR] Invalid Usage : perl " $configs{-SCRIPT_NAME}
    exit 1
fi



# INPUT : fileprefix_date.txt
# fileprefix : stat_report_file
# date : 20191230		-> Extract Date
STAT_DATA_FILE_PREFIX="stat_report_file_"
STAT_DATA_FILE_SUFFIX=".txt"
STAT_DATABASE_FILE="stat_records.db"


FILE_SELECTOR_REGEX='\.h,|\.c,|\.cc,|\.cpp,'
FILE_DESELECTOR_REGEX="\.java,"

STAT_DATA_FILE="stat_report.csv"
FILE_SELECTOR_REGEX='\.h,|\.c,|\.cc,|\.cpp,'
FILE_DESELECTOR_REGEX="\.java,"
FILE_LOCATION_TAG=""
CURRENTTIME=`date +"%Y%m%d%H%M%S"`
MASTER_DATA_FILE="stat_master.data"

CURR_DIR=`pwd`
cd $CURR_DIR

ARCHIVE_ROOT=$CURR_DIR
ARCHIVE="ARCHIVE"
CURRENTDATE=`date +"%Y%m%d"`
ARCHIVE_DIR=$ARCHIVE_ROOT/$ARCHIVE-$CURRENTDATE
if [ -d $ARCHIVE_DIR ] ; then
	mkdir $ARCHIVE_DIR $ARCHIVE_DIR/STAT_FILES
fi
if [ -d $ARCHIVE_DIR/STAT_FILES ] ; then
	mkdir $ARCHIVE_DIR/STAT_FILES
fi

#IP_ADDRESS=`ip addr show bond0 | grep 'inet ' | awk -F" " '{print $2}'`
HOST_NAME=`hostname --long`
CURRENTTIME=`date +"%Y%m%d%H%M%S"`
SELFPID=`ps -eaf | grep {$0} | awk -F" " '{ print $2}'`
SLEEP_INT=20
# Mailing list, seperated by comma
LOG_FILE=$CURR_DIR/stat_diff_master.$CURRENTTIME.log


#Step 1: Read stat file
#Step 2: 


cd $CURR_DIR


dos2unix $CURR_DIR/$STAT_DATA_FILE_PREFIX*



#mv $CURR_DIR/$STAT_DATA_FILE_PREFIX$CURRENTDATE$STAT_DATA_FILE_SUFFIX $ARCHIVE_DIR/STAT_FILES/
if [ -f $STAT_DATABASE_FILE ] ; then
	mv $STAT_DATABASE_FILE $ARCHIVE_DIR/STAT_FILES/$STAT_DATABASE_FILE"_"$CURRENTDATE
fi
touch $STAT_DATABASE_FILE


mv $CURR_DIR/$MASTER_DATA_FILE $ARCHIVE_DIR/$MASTER_DATA_FILE
touch $CURR_DIR/$MASTER_DATA_FILE
mv $CURR_DIR/stat_master*.log $ARCHIVE_DIR/
mv $CURR_DIR/$STAT_DATA_FILE $ARCHIVE_DIR/$STAT_DATA_FILE$CURRENTTIME
touch $CURR_DIR/$STAT_DATA_FILE
cd $CURR_DIR

for stat_file in `ls -t $CURR_DIR/$STAT_DATA_FILE_PREFIX*`
do 
	cat $stat_file | sed '1d' >> $CURR_DIR/$STAT_DATA_FILE; 
	echo -e "[INFO] Stat report file{"$stat_file"} is merged into "$CURR_DIR/$STAT_DATA_FILE >> $LOG_FILE
	echo -e "[INFO] Stat report file{"$stat_file"} is merged into "$CURR_DIR/$STAT_DATA_FILE
done
cd $CURR_DIR
if [ ! -z "$FILE_LOCATION_TAG" -a "$FILE_LOCATION_TAG" != " " ]; then
	egrep $FILE_LOCATION_TAG $CURR_DIR/$STAT_DATA_FILE > $CURR_DIR/$STAT_DATA_FILE.TMP2
	mv $CURR_DIR/$STAT_DATA_FILE.TMP2 $CURR_DIR/$STAT_DATA_FILE
fi
if [ ! -z "$FILE_DESELECTOR_REGEX" -a "$FILE_DESELECTOR_REGEX" != " " ]; then
	egrep -v $FILE_DESELECTOR_REGEX $CURR_DIR/$STAT_DATA_FILE > $CURR_DIR/$STAT_DATA_FILE.TMP2
	mv $CURR_DIR/$STAT_DATA_FILE.TMP2 $CURR_DIR/$STAT_DATA_FILE
fi

if [ ! -z "$FILE_SELECTOR_REGEX" -a "$FILE_SELECTOR_REGEX" != " " ]; then
	egrep $FILE_SELECTOR_REGEX $CURR_DIR/$STAT_DATA_FILE > $CURR_DIR/$STAT_DATA_FILE.TMP2
	mv $CURR_DIR/$STAT_DATA_FILE.TMP2 $CURR_DIR/$STAT_DATA_FILE
fi

rm $CURR_DIR/$STAT_DATA_FILE.TMP*
#Field Mapping
#1 Project		-> 7
#2 Module		-> 8
#3 Priority		-> 2
#4 Category		-> 4
#5 FileName 	-> 1
#6 LineNumber	-> 3
#7 FilePath		-> 5
#8 Link			-> 6
# generate error code -> 9

awk -F"," '{ print $5"##"$3"##"$6"##"$4"##"$7"##"$8"##"$1"##"$2 }' $CURR_DIR/$STAT_DATA_FILE | sort -u > $CURR_DIR/$STAT_DATA_FILE.TMP1
mv $CURR_DIR/$STAT_DATA_FILE.TMP1 $MASTER_DATA_FILE

echo -e "[INFO] Stat report success fully generated from file "$CURR_DIR/$MASTER_DATA_FILE
echo -e "[INFO] Stat report data is populated into "$CURR_DIR/$STAT_DATA_FILE

