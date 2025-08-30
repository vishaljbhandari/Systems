#! /bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

# usage $0 <lookback> <cdrsp> <gprssp> <rechargesp>
if [ $# -lt 1 ] ; then
	echo "Usage : $0 <number_of_lookback_days> [$1 <number_of_cdr_subpartitions>] [$2 <number_of_gprs_subpartitions>] [$3 <number_of_rechargelog_subpartitions>]"
	exit 2
fi

NumberOfLookBackDays=$1

if [ $# -gt 1 ] ; then
	NumberOfCDRSubpartitions=$2
	 if [ $# -gt 2 ] ; then
		NumberOfGPRSSubpartitions=$3
			if [ $# -gt 3 ] ; then
				NumberOfRechargeSubpartitions=$4
			else
				NumberOfRechargeSubpartitions=24
			fi
	else
		NumberOfGPRSSubpartitions=24
		NumberOfRechargeSubpartitions=24
	fi
else
	NumberOfCDRSubpartitions=24
	NumberOfGPRSSubpartitions=24
	NumberOfRechargeSubpartitions=24
fi

NumberOfSubpartitions=0 

GetDayRange()
{
        if [ $DB_TYPE == "1" ]
then
        sqlplus -s /NOLOG << EOF > sql.out  2>&1
        CONNECT $DB_SETUP_USER/$DB_SETUP_PASSWORD
        WHENEVER OSERROR  EXIT 6 ;
        WHENEVER SQLERROR EXIT 5 ;
        SPOOL day.lst
                SELECT to_char(sysdate, 'ddd') from dual ;
        SPOOL OFF ;
EOF

else

	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
	WHENEVER OSERROR  EXIT 6 ;
        WHENEVER SQLERROR EXIT 5 ;
        SPOOL day.lst
                SELECT to_char(sysdate, 'ddd') from dual ;
        SPOOL OFF ;
EOF

fi
        sqlretcode=`echo $?`

        if [ $sqlretcode -eq 5 -o $sqlretcode -eq 6 ]
        then
                echo "Terminating due to SQL Error. `date`"
                exit 0
        fi
        rm sql.out

		DayOfYear=`grep "[0-9][0-9]*" day.lst`
		EndDay=`echo $DayOfYear | tr -d " " `
		rm day.lst
		if [ $EndDay -gt $NumberOfLookBackDays ]
		then		
			StartDay=`expr $EndDay - $NumberOfLookBackDays`
		else
			StartDay=1
		fi
				
		EndDay=`expr $EndDay + 5`
}

GenerateHeader()
{
	> generate.sh
	echo "#! /bin/bash" >> generate.sh
	if [ $DB_TYPE == "1" ]
	then
	echo "sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF " >>generate.sh
	else
	echo "clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."" >>generate.sh
	fi
	echo "whenever oserror exit 5;" >> generate.sh
	echo "set heading off" >> generate.sh
}

GenerateCDRTableCreation()
{
	echo "drop table cdr ;" >> generate.sh
	echo "	CREATE TABLE CDR (" >> generate.sh
	echo "		ID                  		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		NETWORK_ID          		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		CALLER_NUMBER       		VARCHAR2(40)," >> generate.sh
	echo "		CALLED_NUMBER       		VARCHAR2(40)," >> generate.sh
	echo "		FORWARDED_NUMBER    		VARCHAR2(40)," >> generate.sh
	echo "		RECORD_TYPE         		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		DURATION            		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		TIME_STAMP          		DATE        NOT NULL," >> generate.sh
	echo "		EQUIPMENT_ID        		VARCHAR2(40)," >> generate.sh
	echo "		IMSI_NUMBER         		VARCHAR2(40) ," >> generate.sh
	echo "		GEOGRAPHIC_POSITION 		VARCHAR2(32)," >> generate.sh
	echo "		CALL_TYPE           		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		SUBSCRIBER_ID       		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		VALUE               		NUMBER(16,6)    NOT NULL," >> generate.sh
	echo "		CDR_TYPE            		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		SERVICE_TYPE        		NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "		CDR_CATEGORY        		NUMBER(20,0)    DEFAULT 1 NOT NULL," >> generate.sh
	echo "		IS_COMPLETE         		NUMBER(20)      NOT NULL," >> generate.sh
	echo "		IS_ATTEMPTED        		NUMBER(20)      NOT NULL," >> generate.sh
	echo "		SERVICE             		NUMBER(20,0) DEFAULT 2047," >> generate.sh
	echo "		PHONE_NUMBER        		VARCHAR2(40)," >> generate.sh
	echo "		DAY_OF_YEAR         		NUMBER(20,0)    DEFAULT 0," >> generate.sh
	echo "		HOUR_OF_DAY         		NUMBER(20,0)    DEFAULT 0," >> generate.sh
	echo "      OTHER_PARTY_COUNTRY_CODE	VARCHAR2(32)," >> generate.sh
	echo "		VPMN                        VARCHAR2(40)" >> generate.sh
	echo "	)" >> generate.sh
	echo "	INITRANS 4 STORAGE(FREELISTS 16)" >> generate.sh
	echo "	PARTITION BY RANGE(DAY_OF_YEAR)" >> generate.sh
	echo "	SUBPARTITION BY LIST(HOUR_OF_DAY)" >> generate.sh
	NumberOfSubpartitions=$NumberOfCDRSubpartitions 
	GenerateSubpartions
}

GenerateGPRSTableCreation()
{
	echo "DROP TABLE GPRS_CDR ; " >> generate.sh
	echo "CREATE TABLE GPRS_CDR ( " >> generate.sh
	echo "	      ID                                      NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        NETWORK_ID                              NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        RECORD_TYPE                             NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        TIME_STAMP                              DATE                    NOT NULL," >> generate.sh
	echo "        DURATION                                NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        PHONE_NUMBER                            VARCHAR2(32)    NOT NULL," >> generate.sh
	echo "        IMSI_NUMBER                             VARCHAR2(32)," >> generate.sh
	echo "        IMEI_NUMBER                             VARCHAR2(32)," >> generate.sh
	echo "        GEOGRAPHIC_POSITION                     VARCHAR2(32)," >> generate.sh
	echo "        CDR_TYPE                                NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        SERVICE_TYPE                            NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        PDP_TYPE                                VARCHAR2(32)," >> generate.sh
	echo "        SERVED_PDP_ADDRESS                      VARCHAR2(32)," >> generate.sh
	echo "        UPLOAD_DATA_VOLUME                      NUMBER(20,6)    NOT NULL," >> generate.sh
	echo "        DOWNLOAD_DATA_VOLUME            	      NUMBER(20,6)    NOT NULL," >> generate.sh
	echo "        SERVICE                                 NUMBER(20,0)    DEFAULT 2047," >> generate.sh
	echo "        QOS_REQUESTED                           NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        QOS_NEGOTIATED                          NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        VALUE                                   NUMBER(16,6)    NOT NULL," >> generate.sh
	echo "        ACCESS_POINT_NAME                       VARCHAR2(64)," >> generate.sh
	echo "        SUBSCRIBER_ID                           NUMBER(20,0)    NOT NULL," >> generate.sh
	echo "        DAY_OF_YEAR                             NUMBER(20,0)," >> generate.sh
	echo "        CAUSE_FOR_SESSION_CLOSING               NUMBER(2,0)," >> generate.sh
	echo "        SESSION_STATUS                          NUMBER(2,0)," >> generate.sh
	echo "        CHARGING_ID                             VARCHAR2(26)     NOT NULL," >> generate.sh
	echo "        ANI                                     VARCHAR2(32)," >> generate.sh
	echo "        HOUR_OF_DAY                     	      NUMBER(20,0) DEFAULT 0," >> generate.sh
	echo "	  	  VPMN       	                   		  VARCHAR2(40)" >> generate.sh
	echo "        ) " >> generate.sh
	echo "        INITRANS 4 STORAGE(FREELISTS 16) " >> generate.sh
	echo "        PARTITION BY RANGE (DAY_OF_YEAR) " >> generate.sh
	echo "        SUBPARTITION BY LIST (HOUR_OF_DAY) " >> generate.sh

	NumberOfSubpartitions=$NumberOfGPRSSubpartitions 
	GenerateSubpartions 

}

GenerateRechargeLogTableCreation()
{

echo "	DROP TABLE RECHARGE_LOG ;	    " >> generate.sh 
echo "	CREATE TABLE RECHARGE_LOG (	    " >> generate.sh 
echo "        ID						NUMBER(20) NOT NULL," >> generate.sh 
echo "        TIME_STAMP				DATE NOT NULL," >> generate.sh 
echo "        PHONE_NUMBER				VARCHAR2(32) ," >> generate.sh 
echo "        AMOUNT					NUMBER(16,6) NOT NULL," >> generate.sh 
echo "        RECHARGE_TYPE				NUMBER(16,6) NOT NULL," >> generate.sh 
echo "        STATUS					NUMBER(20) NOT NULL," >> generate.sh 
echo "        CREDIT_CARD				VARCHAR2(40) ," >> generate.sh 
echo "        PIN_NUMBER				VARCHAR2(40) ," >> generate.sh 
echo "        SERIAL_NUMBER				VARCHAR2(40) ," >> generate.sh 
echo "        NETWORK_ID				NUMBER(20) DEFAULT 999 NOT NULL," >> generate.sh 
echo "        ACCOUNT_ID				NUMBER(20) NOT NULL," >> generate.sh 
echo "        SUBSCRIBER_ID				NUMBER(20) NOT NULL," >> generate.sh 
echo "        DAY_OF_YEAR				NUMBER(20,0) DEFAULT 0," >> generate.sh 
echo "        HOUR_OF_DAY				NUMBER(20,0) DEFAULT 0" >> generate.sh 
echo "        )" >> generate.sh 
echo "        INITRANS 4 STORAGE(FREELISTS 16)" >> generate.sh 
echo "        PARTITION BY RANGE (DAY_OF_YEAR)" >> generate.sh 
echo "        SUBPARTITION BY LIST (HOUR_OF_DAY)" >> generate.sh 
NumberOfSubpartitions=$NumberOfRechargeSubpartitions 
GenerateSubpartions
}

GenerateSubpartions()
{
	echo "	SUBPARTITION TEMPLATE" >> generate.sh
	echo "	(" >> generate.sh

		for((j=0; j<$NumberOfSubpartitions; ++j))
		do
			echo -ne "\tSUBPARTITION SP_$j VALUES ('$j')" >>generate.sh
			if (( $j !=  ($NumberOfSubpartitions - 1) ))
			then
				echo -en ",\n" >>generate.sh
			else
				echo -en "\n" >>generate.sh
			fi
		done
	echo "	)" >> generate.sh
	echo "	(" >> generate.sh

	for((i=$StartDay-1; i<$EndDay; ++i))
	do
		m=$i ;
		if [ "$m" -lt "10" ]
		then
			m=00$m ;
		elif [ "$m" -lt "100" ]
		then
			m=0$m ;
		fi 

		echo -ne "\tPARTITION P$m VALUES LESS THAN ($(($i+1)))" >> generate.sh
		if [[ $i -ne $EndDay-1 ]]
		then
			echo -en ",\n" >>generate.sh
		else
			echo -en "\n" >>generate.sh
		fi
	done
	echo "	) ENABLE ROW MOVEMENT ;" >> generate.sh
}



GenerateGPRSIndexCreation()
{
	echo "" >> generate.sh
	echo "CREATE INDEX IX_GPRS_CDR_PH_NU ON GPRS_CDR(\"PHONE_NUMBER\")" >> generate.sh
	echo "INITRANS 4 STORAGE(FREELISTS 16)" >> generate.sh
	echo "LOCAL" >> generate.sh
	echo "(" >>generate.sh
	NumberOfSubpartitions=$NumberOfGPRSSubpartitions
	GenerateIndexes
}

GenerateIndexes()
{
	for((i=$StartDay-1; i<$EndDay; ++i))
	do
		m=$i ;
		if [ "$m" -lt "10" ]
		then
			m=00$m ;
		elif [ "$m" -lt "100" ]
		then
			m=0$m ;
		fi 

		echo -e "\tPARTITION P$m" >>generate.sh
		echo -e "\t(" >> generate.sh

	if [ $NumberOfSubpartitions -lt 10 ] ; then
		for((j=0; j<$NumberOfSubpartitions; ++j))
		do
			echo -ne "\t\tSUBPARTITION P"$m"_SP_"$j >> generate.sh 
			if (( $j != $NumberOfSubpartitions -1 ))
			then
				echo -en ",\n" >>generate.sh
			else
				echo -en "\n" >>generate.sh
			fi
		done

	else
		for((j=0; j<10; ++j))
		do
			echo -ne "\t\tSUBPARTITION P"$m"_SP_"$j >> generate.sh 
			if (( $j != $NumberOfSubpartitions -1 ))
			then
				echo -en ",\n" >>generate.sh
			else
				echo -en "\n" >>generate.sh
			fi
		done

		for((j=10; j<$NumberOfSubpartitions; ++j))
		do
			echo -ne "\t\tSUBPARTITION P"$m"_SP_"$j >> generate.sh 
			if (( $j != $NumberOfSubpartitions -1 ))
			then
				echo -en ",\n" >>generate.sh
			else
				echo -en "\n" >>generate.sh
			fi
		done
	fi

		if [[ $i -ne $EndDay-1 ]]
		then
			echo -en "),\n" >>generate.sh
		else
			echo -en ")\n" >>generate.sh
		fi
	done
	echo ") ;" >>generate.sh
}

GenerateCDRIndexCreation()
{
	echo "" >> generate.sh
	echo "CREATE INDEX IX_CDR_PH_NU ON CDR(\"PHONE_NUMBER\")" >> generate.sh
	echo "INITRANS 4 STORAGE(FREELISTS 16)" >> generate.sh
	echo "LOCAL" >> generate.sh
	echo "(" >>generate.sh
	NumberOfSubpartitions=$NumberOfCDRSubpartitions
	GenerateIndexes
}


GenerateRechargeLogIndexCreation()
{
	echo "" >> generate.sh
	echo "CREATE INDEX IX_RECHARGE_PH_NU ON RECHARGE_LOG(\"PHONE_NUMBER\")" >> generate.sh
	echo "INITRANS 4 STORAGE(FREELISTS 16)" >> generate.sh
	echo "LOCAL" >> generate.sh
	echo "(" >>generate.sh
	NumberOfSubpartitions=$NumberOfRechargeSubpartitions
	GenerateIndexes
}

GenerateCDRTemporaryTables()
{
	echo "" >> generate.sh
		for((j=0; j<$NumberOfCDRSubpartitions; ++j))
		do
			echo "drop table TEMP_CDR_$j ;" >> generate.sh
			echo "create table TEMP_CDR_$j as select * from cdr where 1=2 ;" >> generate.sh
			echo "create index IX_TCDR_PH_NU$j on TEMP_CDR_$j(\"PHONE_NUMBER\") ;" >> generate.sh
			echo "" >> generate.sh
		done
}

GenerateRechargeTemporaryTables()
{
	echo "" >> generate.sh
		for((j=0; j<$NumberOfRechargeSubpartitions; ++j))
		do
			echo "drop table TEMP_RECHARGE_LOG_$j ;" >> generate.sh
			echo "create table TEMP_RECHARGE_LOG_$j as select * from RECHARGE_LOG where 1=2 ;" >> generate.sh
			echo "create index IX_TRECHARGE_PH_NU$j on TEMP_RECHARGE_LOG_$j(\"PHONE_NUMBER\") ;" >> generate.sh
			echo "" >> generate.sh
		done
}

GenerateGPRSTemporaryTables()
{
	echo "" >> generate.sh
		for((j=0; j<$NumberOfGPRSSubpartitions; ++j))
		do
			echo "drop table TEMP_GPRS_CDR_$j ;" >> generate.sh
			echo "create table TEMP_GPRS_CDR_$j as select * from GPRS_CDR where 1=2 ;" >> generate.sh
			echo "create index IX_TGPRS_CDR_PH_NU$j on TEMP_GPRS_CDR_$j(\"PHONE_NUMBER\") ;" >> generate.sh
			echo "" >> generate.sh
		done
}

GenerateEnd()
{
	echo "quit ;" >>generate.sh
	echo "EOF" >>generate.sh
}

GenerateHeader
GetDayRange
GenerateCDRTableCreation
GenerateCDRIndexCreation 
GenerateCDRTemporaryTables

GenerateGPRSTableCreation
GenerateGPRSIndexCreation
GenerateGPRSTemporaryTables

GenerateRechargeLogTableCreation
GenerateRechargeLogIndexCreation
GenerateRechargeTemporaryTables

GenerateEnd
sh generate.sh
returnStatus=`echo $?`
if [ $returnStatus -ne 0 ]
then
	echo "Execution Failed"
fi
#rm generate.sh
