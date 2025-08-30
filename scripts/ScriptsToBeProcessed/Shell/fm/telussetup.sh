#! /bin/bash
#################################################################################
#  Copyright (c) Subex Azure Limited 2006. All rights reserved.                 #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

if [ "$RANGERHOME" == "" ];then
	echo "Warning: RANGERHOME is not set"
	echo -en "\nEnter RANGERHOME [press <enter> to quit]: "
	read RANGERHOME
	if [ "$RANGERHOME" == "" ];then
		exit 0
	fi
fi

mkdir -p $RANGERHOME/FMSData/Datasource/CDR/ISUP
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/ISUP/success
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/ISUP/Log
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/ISUP/Rejected

mkdir -p $RANGERHOME/FMSData/Datasource/CDR/EEM
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/EEM/success
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/EEM/Log
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/EEM/Rejected

mkdir -p $RANGERHOME/FMSData/Datasource/CDR/TRAFIC
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/TRAFIC/success
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/TRAFIC/Log
mkdir -p $RANGERHOME/FMSData/Datasource/CDR/TRAFIC/Rejected

mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber
mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber/success
mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber/Log
mkdir -p $RANGERHOME/FMSData/Datasource/Subscriber/Rejected

mkdir -p $RANGERHOME/FMSData/Datasource/Rater/
mkdir -p $RANGERHOME/FMSData/Datasource/Rater/success/

mkdir -p $RANGERHOME/FMSData/DiamondXMLs/

if [ $? -ne 0 ]; then
		echo -e "Error occured in in setup\n"
        exit 5
fi

echo -e "***************Starting Nikira V6.0 Database Setup****************\n"
#./telusdbsetup.sh
