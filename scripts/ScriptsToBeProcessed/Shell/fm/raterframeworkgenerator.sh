#! /bin/bash

#set -x
testName="" ;
while [ "$testName" = "" ]
do
	read -p "Enter the Test Name : " testName ;
done

echo "Test Name : $testName" ;
raterDirectory="`pwd`/../$testName/Rater/"
if [ ! -d ../$testName ]
then
	echo -e "\t\tSpecified Test Name Customization Directory Not Found"
	echo -e "\t\tExpecting `pwd`/../$testName to exists"
	exit 1;
else
	mkdir -p `pwd`/../$testName/Rater/
fi

custNameLowerCase=`echo $testName | tr [A-Z] [a-z]`
custNameConfigKey=`echo $testName | tr [a-z] [A-Z]`

cp -R * $raterDirectory
cd $raterDirectory

rm -rf `find . -name .svn` 

for fil in `find *  -exec grep -li test {} \; |grep -v .svn`
do
	echo "$fil"
	cat $fil |grep -i test >/dev/null
	if [ $? -eq 0 ]
	then
		newFileName=`echo $fil | sed "s/test/$custNameLowerCase/g"`
		mv $fil $newFileName
		sed -e "s/Test/$testName/g" -e "s/test/$custNameLowerCase/g" -e "s/TEST/$custNameConfigKey/g" -e "s:Test/Rater/:$testName/Rater/:g" $newFileName >$newFileName.tmp
		mv $newFileName.tmp  $newFileName
	else
		sed -e "s/test/$custNameLowerCase/g" -e "s/TEST/$custNameConfigKey/g" -e "s:Test/Rater/:$testName/Rater/:g"  $fil > $fil.tmp
		mv $fil.tmp  $fil
	fi
done

cat <<EOF
Completed Generating the Framework Code
	Update the below files for completion
	1) raterconstants.h for removing unwanted definitions
	2) insert the config_enties into DML.sql files
		$custNameConfigKey.GSM.RATER.INPUT
		$custNameConfigKey.GSM.RATER.OUTPUT
		$custNameConfigKey.GSM.RATER.REJECTED
		$custNameConfigKey.GSM.RATER.LOG
	3) configure.in
	4) Makefile.am under Datasource/$testName/
	5) Refer test_rater.sql.in for required DBEntries.
EOF
	
