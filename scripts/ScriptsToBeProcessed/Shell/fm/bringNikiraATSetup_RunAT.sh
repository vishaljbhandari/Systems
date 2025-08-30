#!/bin/bash

. $WATIR_HOME/Scripts/Server/configuration.sh
mailSubject="AT suite summary as executed on "
mailTo="ravindra.jain@subex.com"
#~ mailCC="-c xyz1@subexworld.com,xyz2@subexworld.com"

echo -e "\E[38;35mBringing Nikira Setup on the Server ....\x1b[38;0m"
. $WATIR_HOME/Scripts/Local/bringNikiraSetupOnServer.sh
    if [ $? -ne 0 ]
        then
                echo -e "\x1b[38;31mFailed to bring Nikira Setup on server !!!!\x1b[38;0m"
                exit 1
    fi

echo -e "\E[38;35mBringing AT Setup ....\x1b[38;0m"
. $WATIR_HOME/Scripts/Local/CEInit.sh
    if [ $? -ne 0 ]
        then
                echo -e "\x1b[38;31mFailed to bring AT setup on server !!!!\x1b[38;0m"
                exit 2
    fi

echo -e "\E[38;35mRunning Create Fixtures ....\x1b[38;0m"
ruby $WATIR_HOME/FixtureCreation/Create_Fixtures.rb
    if [ $? -ne 0 ]
        then
                echo -e "\x1b[38;31mFailed to create Fixtures !!!!\x1b[38;0m"
                exit 3
    fi

echo -e "\E[38;35mRunning AT Suite ....\x1b[38;0m"
reportFolderName=`date +%d-%m-%Y`
cd $WATIR_HOME/Testcases
ruby all_tests.rb
    if [ $? -ne 0 ]
        then
                echo -e "\x1b[38;31mFailed to run AT suite !!!!\x1b[38;0m"
                exit 4
    fi

echo -e "\E[38;35mSending Report Summary ....\x1b[38;0m"
cd $WATIR_HOME/Testcases/ATReport
tar -cvf $reportFolderName.tar $reportFolderName
scp $reportFolderName.tar $USER@$HOST:$WATIR_SERVER_HOME/BringNikiraSetup
ssh -l $USER $HOST tar -xvf $WATIR_SERVER_HOME/BringNikiraSetup/$reportFolderName.tar "-C $WATIR_SERVER_HOME/BringNikiraSetup"
ssh -l $USER $HOST $WATIR_SERVER_HOME/BringNikiraSetup/getATsuiteSummary.sh " $WATIR_SERVER_HOME/BringNikiraSetup/$reportFolderName atSuiteSummary.txt"
ssh -l $USER $HOST mail -s \"$mailSubject $reportFolderName\" $mailTo " $mailCC < $WATIR_SERVER_HOME/BringNikiraSetup/atSuiteSummary.txt"
    if [ $? -ne 0 ]
        then
                echo -e "\x1b[38;31mFailed to send mail for AT report summary !!!!\x1b[38;0m"
                exit 5
    fi

echo -e "\E[38;35mAll the steps executed successfully.\x1b[38;0m"