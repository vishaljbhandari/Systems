#!/bin/bash
DATE=$(date +%y%m%d)
if [ $# -ne 1 ] && [ $# -ne 2 ]
then
	echo "Usage: ./report_font.sh font_name [Back up directory Absolute Path]";
        exit 1
fi
echo -e "The Font you have preferred is '$1'.\n\nDo you want to continue? [y/n]":
read input
if [ $input != 'y' ] && [ $input != 'Y' ]
then
	exit 1
fi
if [ "$NIKIRA_REPORTS_DIR" == "" ] 
then
	cd ../
	nikira_src=$(pwd)
	NIKIRA_REPORTS_DIR=$nikira_src/../Reports
fi
if [ $# -eq 2 ]
then
	cd ${NIKIRA_REPORTS_DIR}/reports
	if [ -d  $2 ]
	then
		cp * $2
        	reports_dest_dir=$2
	else
		echo "Invalid Path. Enter the Absolute path for Back up"
		exit 1
	fi
else
	cd $NIKIRA_REPORTS_DIR
	mkdir -p reports_bak_$DATE
	reports_tmp_dir=$NIKIRA_REPORTS_DIR/reports_bak_$DATE
	cd ${NIKIRA_REPORTS_DIR}/reports
	cp * $reports_tmp_dir
	reports_dest_dir=${NIKIRA_REPORTS_DIR}/reports
fi
cd $reports_dest_dir
perl -pi -e 's/pdfFontName=\"[\w-]*\"//g' *
perl -pi -e 's/fontName=\"[\w-]*\"//g' *
perl -pi -e s/\<font/\<font\ fontName=\"$1\"/g *
files=$(ls)
cd $NIKIRA_REPORTS_DIR/../src
for file in ${files[@]}
do
        rake reportrunner:compile $reports_dest_dir/$file >> $NIKIRA_REPORTS_DIR/../src/log/report_font.log 2>&1
done
if [ $# -eq 2 ]
then
	echo "The Reports are now available in $2 with the font '$1'"
	echo "Do you want these changes permenant? [y/n]:"
	read input
	if [ $input == "y" ] || [ $input == "Y" ]
	then
		cd $2
		cp * $NIKIRA_REPORTS_DIR/reports	
	fi
else
	echo "Now Reports are changed with the font '$1'"
	echo "Back up of old reports are taken in $NIKIRA_REPORTS_DIR/reports_bak_$DATE"
fi
