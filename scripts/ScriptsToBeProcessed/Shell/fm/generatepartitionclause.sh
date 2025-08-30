#!/bin/bash

#############################################################################
#   Copyright (c) SubexAzure Limited 2011. All rights reserved.				#
#   The copyright to the computer program(s) herein is the property of		#
#   SubexAzure Limited. The program(s) may be used and/or copied with the	#
#   written permission from SubexAzure Limited or in accordance with the	#
#   terms and conditions stipulated in the agreement/contract under which	#
#   the program(s) have been supplied.										#
#############################################################################

# Script for generating table/index partition template
# Eg: ./generatepartitionclause.sh oracle table_partition_clause day_of_year range 1-366 hour_of_day list 1,2,3
# Eg: ./generatepartitionclause.sh db2 index_partition_clause 367 3

# To do
# Validation of partition values : wrong range/number of partitions etc
# Support for non-numeric partition keys
# Convert fully to awk script & optimization

LogFile=generatepartitionclause.log

declare -a PartitionValues
declare -a SubpartitionValues

ValidateParameters()
{
	check=`echo $* | awk '
		($1 == "ORACLE" || $1 == "DB2") &&
		((($2 == "TABLE_PARTITION_CLAUSE") &&
		(NF == 5 || NF == 8) &&
		($3 ~ /^\w+$/) &&
		((($4 == "RANGE" || $4 == "LIST") && ($5 ~ /^[0-9]+-[0-9]+$/ || $5 ~ /^[0-9]+(,[0-9]+)+$/)) ||
			($4 == "HASH" && $5 ~ /^[0-9]+$/)) &&
		((NF == 5) ||
			((($4 == "LIST" || $4 == "RANGE") && $6 ~ /^\w+$/) && 
				(($7 == "LIST" && ($8 ~ /^[0-9]+-[0-9]+$/ || $8 ~ /^[0-9]+(,[0-9]+)+$/)) ||
					($7 == "HASH" && $8 ~ /^[0-9]+$/))))) ||
		(($2 == "INDEX_PARTITION_CLAUSE") &&
		(NF == 3 || NF == 4) &&
		($3 ~ /^[0-9]+$/ && $4 ~ /^[0-9]*$/)))'`
	echo $check>>$LogFile
	
	if [ $# -eq 0 -o "$check" != "$*" ]
	then
		echo "Invalid Parameters" >> $LogFile
		exit 1
	fi

	Database=$1
	RequestType=$2

	if [ "$RequestType" == "TABLE_PARTITION_CLAUSE" ]
	then
		PartitionKey=$3
		PartitionType=$4
		SubpartitionKey=$6
		SubpartitionType=$7

		if [ "$PartitionKey" == "DAY_OF_YEAR" ]
		then
		   ListDayOfYear="TRUE" 
		fi

		list=`echo $5 | awk '/-/ { split($1, list,"-"); for( i=list[1]; i<= list[2]; i++ ) printf i " " } 
					/,/ { n = split($1, list, ","); for( i=1; i<=n; i++ ) printf list[i] " " } !/[-,]/'`
		PartitionValues=($list)
		
		list=`echo $8 | awk '/-/ { split($1, list,"-"); for( i=list[1]; i<= list[2]; i++ ) printf i " " } 
					/,/ { n = split($1, list, ","); for( i=1; i<=n; i++ ) printf list[i] " " } !/[-,]/'`
		SubpartitionValues=($list)
	else
		PartitionCount=$3
		SubpartitionCount=$4
	fi
}

GenerateTableCodeForOracle()
{
	echo "PARTITION BY $PartitionType ($PartitionKey)"

	if [ "$SubpartitionKey" != "" ]
	then
		echo "SUBPARTITION BY $SubpartitionType ($SubpartitionKey)"
		echo "SUBPARTITION TEMPLATE"
		echo "("

		if [ "$SubpartitionType" == "LIST" ]
		then
			ListSize=${#SubpartitionValues[*]}
			for((i=0; i<$ListSize-1; ++i))
			do
				value=${SubpartitionValues[$i]}
				printf "\tSUBPARTITION SP_$value VALUES($value),\n"
			done
			value=${SubpartitionValues[$ListSize-1]}
			printf "\tSUBPARTITION SP_$value VALUES($value)\n"
		else
			ListSize=${SubpartitionValues[0]}
			for((i=1; i<$ListSize; ++i))
			do
				printf "\tSUBPARTITION SP_%d,\n" $i
			done
			printf "\tSUBPARTITION SP_%d\n" ${ListSize} 
		fi

		echo ")"
	fi

	echo "("

	if [ "$PartitionType" == "RANGE" ]
	then
		ListSize=${#PartitionValues[*]}
		for((i=0; i<$ListSize-1; ++i))
		do
			value=${PartitionValues[$i]}
			printf "\tPARTITION P%03d VALUES LESS THAN ($(($value + 1))),\n" $value
		done
		value=${PartitionValues[$ListSize-1]}
		printf "\tPARTITION P%03d VALUES LESS THAN ($(($value + 1)))\n" $value
	elif [ "$PartitionType" == "LIST" ]
	then                       
		if [ "$PartitionKey" == "DAY_OF_YEAR" ]
		then
			ListSize=${#PartitionValues[*]}
			for((i=1; i<$ListSize; ++i))
			do
				value=${PartitionValues[$i]}
				printf "\tPARTITION P%03d VALUES($value" $value
				j=$(($value+$ListSize-1))
				while(($j<367))
				do
	            	printf ",$j"	
					j=$(($j+$ListSize-1))
				done
				if [ $i == $(($ListSize-1)) ]
				then
					printf ")\n"
				else
					printf "),\n"
				fi
			done
		else
			ListSize=${#PartitionValues[*]}
			for((i=0; i<$ListSize-1; ++i))
			do
				value=${PartitionValues[$i]}
				printf "\tPARTITION P%03d VALUES ($value),\n" $(($value + 1))
			done
			value=${PartitionValues[$ListSize-1]}
			printf "\tPARTITION P%03d VALUES ($value)\n" $(($value + 1))
		fi
	else
		ListSize=${PartitionValues[0]}
		for((i=1; i<$ListSize; ++i))
		do
			printf "\tPARTITION P%03d,\n" $i
		done
		printf "\tPARTITION P%03d\n" ${ListSize} 
	fi

	echo ")"
}

GenerateTableCodeForDB2()
{
	ListSize=${#PartitionValues[*]}

	if [ "$SubpartitionType" == "LIST" ]
	then
		echo "PARTITION BY RANGE ($PartitionKey, $SubpartitionKey)"
		echo "("

		SublistSize=${#SubpartitionValues[*]}

		for ((i=0; i<$ListSize; ++i))
		do
			PartVal=${PartitionValues[$i]}
			for ((j=0; j<$SublistSize; ++j))
			do
				SubpartVal=${SubpartitionValues[$j]}
				
				if [ $i -eq $(($ListSize - 1)) -a $j -eq $(($SublistSize - 1)) ]
				then
					printf "\tPARTITION P%03d_SP_%d STARTING ($PartVal, $SubpartVal) ENDING ($PartVal, $SubpartVal)\n" $PartVal $SubpartVal
				else
					printf "\tPARTITION P%03d_SP_%d STARTING ($PartVal, $SubpartVal) ENDING ($PartVal, $SubpartVal),\n" $PartVal $SubpartVal
				fi
			done
		done

		echo ")"
	elif [ "$PartitionType" != "HASH" ]
	then
		echo "PARTITION BY RANGE ($PartitionKey)"
		echo "("

		for ((i=0; i<$ListSize-1; ++i))
		do
			PartVal=${PartitionValues[$i]}
			printf "\tPARTITION P%03d STARTING ($PartVal) ENDING ($PartVal),\n" $PartVal
		done
		PartVal=${PartitionValues[$ListSize-1]}
		printf "\tPARTITION P%03d STARTING ($PartVal) ENDING ($PartVal)\n" $PartVal

		echo ")"
	fi
}

GenerateIndexCodeForOracle()
{
	echo "("

	if [ "$SubpartitionCount" != "" ]
	then
		for((i=1; i<=$PartitionCount; ++i))
		do
			printf "\tPARTITION P%03d\n\t(\n" $i

			for((j=0; j<$SubpartitionCount-1; ++j))
			do
				printf "\t\tSUBPARTITION P%03d_SP_%d,\n" $i $j
			done
			printf "\t\tSUBPARTITION P%03d_SP_%d\n" $i $(($SubpartitionCount - 1))

			if [ "$i" == "$PartitionCount" ]
			then
				printf "\t)\n"
			else
				printf "\t),\n"
			fi
		done
	else
		for((i=1; i<$PartitionCount; ++i))
		do
			printf "\tPARTITION P%03d,\n" $i
		done
		printf "\tPARTITION P%03d\n" $PartitionCount
	fi

	echo ")"
}

GenerateIndexCodeForDB2()
{
	echo PARTITIONED
}

main()
{
	echo "Request received ($*)" >> $LogFile
	Params=`echo $* | awk '{ print toupper($0) }'`

	ValidateParameters $Params

	if [ $RequestType == "TABLE_PARTITION_CLAUSE" ]
	then
		if [ "$Database" == "ORACLE" ]
		then
			GenerateTableCodeForOracle
		else
			GenerateTableCodeForDB2
		fi
	else
		if [ "$Database" == "ORACLE" ]
		then
			GenerateIndexCodeForOracle
		else
			GenerateIndexCodeForDB2
		fi
	fi
}

main $*
