#!/bin/bash

REPORT_FILE=public/reports/${1}.html
IMG_PATH=${REPORT_FILE}_files

perl -pi -e 's/\" width/.gif" width/g' $REPORT_FILE
perl -pi -e 's/\/px\" /\/px.gif" /g' $REPORT_FILE
perl -pi -e 's/img_1/img_1.gif/g' $REPORT_FILE

for file in `ls $IMG_PATH`; 
do
	mv $IMG_PATH/$file $IMG_PATH/${file}.gif;
done
