#! /bin/bash

PGW_RECORD_CONFIG_ID=1024
DATA_RECORD_PATH=$RANGERHOME/RangerData/DataRecord/Process
NetworkID=999

on_exit ()
{
	if [ -f $RANGERHOME/share/Ranger/duPGWParserPID ]; then
		PID=`cat $RANGERHOME/share/Ranger/duPGWParserPID`
		if [ $PID -eq $$ ]; then
			rm -f $RANGERHOME/share/Ranger/duPGWParserPID
		fi
	fi
}
trap on_exit EXIT

Status=$1
if [ -z $Status ] 
then
	echo >&2 "Usage duPGWParser.sh start/stop" && exit 1
fi

function GetAllFields
{
	FieldArray=( `$RANGERHOME/bin/extractfield.pl "$line"` )

	PhoneNumber="${FieldArray[0]}"

	if [ "${PhoneNumber:0:1}" == "+" ];then
		PhoneNumber="${FieldArray[0]}"
	else
		PhoneNumber="+${FieldArray[0]}"
	fi

	TimeStamp="${FieldArray[1]}"
	TimeStamp="${TimeStamp/T/ }"
	TimeStamp="${TimeStamp/-//}"
	TimeStamp="${TimeStamp/-//}"
	Value="${FieldArray[2]}"
	CardNo="${FieldArray[3]}"
	RspCode="${FieldArray[4]}"
	CardHolderName="${FieldArray[5]}"
	CardHolderName="${CardHolderName//_^_/ }" #Substitute the combination with space.
	ExpiryDate="${FieldArray[6]}"
	TransactionId="${FieldArray[7]}"

	StartDate="${FieldArray[8]/_^_/}"
	TranType="${FieldArray[9]/_^_/}"
	PStatus="${FieldArray[10]/_^_/}"
	PayChannel="${FieldArray[11]/_^_/}"
	FirstName="${FieldArray[12]/_^_/}"
	LastName="${FieldArray[13]/_^_/}"
	CardAcceptorID="${FieldArray[14]/_^_/}"
	IPAddr="${FieldArray[15]/_^_/}"
	DestMSISDN="${FieldArray[16]/_^_/}"
	ResponseCode="${FieldArray[17]/_^_/}"
	CardType="${FieldArray[18]/_^_/}"
	IDNumber="${FieldArray[19]/_^_/}"
	Issue="${FieldArray[20]/_^_/}"
}

function GetAccountName
{
sqlplus -s /nolog << EOF > $RANGERHOME/tmp/pgwaccountname
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 6 ;
set feedback off;
set heading off;
set serveroutput on;
declare
	subscriber_id number(40) := 4 ;
	sub_account_name  varchar2(80) := 'Default Account' ;
begin
	SELECT id, account_name INTO subscriber_id, sub_account_name FROM subscriber WHERE phone_number = '$PhoneNumber' and status in (1,2) ;
	if sub_account_name = '' or sub_account_name is null
	then
		sub_account_name := 'Default Account' ;
	end if;
	dbms_output.put_line(subscriber_id||'|'||sub_account_name) ;
exception
	when no_data_found then
		dbms_output.put_line(subscriber_id||'|'||sub_account_name) ;
end;
/
quit;
EOF

SubscriberID=`cat $RANGERHOME/tmp/pgwaccountname | tail -1 | cut -d "|" -f1`
AccountName=`cat $RANGERHOME/tmp/pgwaccountname | tail -1 | cut -d "|" -f2`
}

function ProcessRecords
{
	for temp_file in `cut -d "|" -f1 $RANGER5_4HOME/RangerData/PGWData/$file |sort -u`
	 do
	  {
		grep $temp_file $RANGER5_4HOME/RangerData/PGWData/$file >> $RANGER5_4HOME/RangerData/PGWData/$file_"tmp"
	  }
	  done
	  mv $RANGER5_4HOME/RangerData/PGWData/$file_"tmp" $RANGER5_4HOME/RangerData/PGWData/$file

	echo "Recordtype:$PGW_RECORD_CONFIG_ID" > $RANGER5_4HOME/tmp/$file
	while read line
	do
		GetAllFields 

		# Check For Mandatory Fields

		flag=0

		if [ "$TransactionId" == "null" ]; then
			echo -n "Null Case Missing Transaction ID : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
			echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
			let RejectedCount=$RejectedCount+1
			flag=1
			continue
		fi

		CheckTransactionID
		if [ $? -eq 1 ]; then
			flag=1
			continue
		fi

		if [ "$PhoneNumber" == "+null" ]; then
			echo -n "Missing MSISDN : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
			echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
			let RejectedCount=$RejectedCount+1
			flag=1
			continue
		fi

		if [ "$RspCode" == 'null' ]; then
			echo -n "Missing RspCode : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
			echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
			let RejectedCount=$RejectedCount+1
			flag=1
			continue
		fi

		if [ "$RspCode" != '0' ]; then
			CheckForRspCode
			if [ $? -eq 1 ]; then
				flag=1
				continue
			fi
		fi

		#These fields are mandatory only if Rsp Code is 0
		if [ "$RspCode" == "0" ]; then
			if [ "$CardNo" == 'null' ]; then
				echo -n "Missing Credit Card Number : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				let RejectedCount=$RejectedCount+1
				flag=1
				continue
			fi

			if [ "$TimeStamp" == 'null' ]; then
				echo -n "Missing TimeStamp : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				let RejectedCount=$RejectedCount+1
				flag=1
				continue
			fi

			if [ "$ExpiryDate" == 'null' ]; then
				echo -n "Missing Expiry Date : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				let RejectedCount=$RejectedCount+1
				flag=1
				continue
			fi

			if [ "$CardHolderName" == 'null' ]; then
				echo -n "Missing Cardholder Name : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
				let RejectedCount=$RejectedCount+1
				flag=1
				continue
			fi
		fi

		GetAccountName

		let AcceptedCount=$AcceptedCount+1
		echo "$PhoneNumber,$TimeStamp,$CardNo,$AccountName,$Value,$StartDate,$ExpiryDate,$CardHolderName,$CardAcceptorID,$TransactionId,$TranType,$PStatus,$PayChannel,$FirstName,$LastName,$IPAddr,$RspCode,$DestMSISDN,$ResponseCode,$CardType,$IDNumber,$Issue,$SubscriberID,$NetworkID" >> $RANGER5_4HOME/tmp/$file
	done < $RANGER5_4HOME/RangerData/PGWData/$file

	if [ $AcceptedCount -gt '0' ]
	then
		cp $RANGER5_4HOME/tmp/$file $DATA_RECORD_PATH
		touch $DATA_RECORD_PATH/success/$file
	else
		> $RANGER5_4HOME/tmp/$file
	fi

	
	EndTime=`date +%H:%M:%S' '%d/%m/%Y`

	echo -e "\n        ******************************** Log File **************************\n" | tee -a $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log >/dev/null
	echo "Filename                 : $RANGER5_4HOME/RangerData/PGWData/$file" | tee -a $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log >/dev/null
	echo -n "Start Time               : $StartTime" | tee -a $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log >/dev/null
	echo  "		End Time            : $EndTime" | tee -a $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log >/dev/null
	echo "Records Processed        : $ProcessedCount" | tee -a  $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log  >/dev/null
	echo "Accepted Records         : $AcceptedCount" | tee -a  $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log  >/dev/null
	echo "Rejected Records         : $RejectedCount" | tee -a  $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log  >/dev/null
	echo -e "        ---------------------------------------------------------------------------\n\n\n" | tee -a $RANGERHOME/LOG/duPGWParser.log $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log >/dev/null

	rm $RANGER5_4HOME/RangerData/PGWData/success/$file
	mv $RANGER5_4HOME/RangerData/PGWData/$file $RANGER5_4HOME/RangerData/PGWData/Backup

	if [ $RejectedCount -eq 0 ]; then
		rm $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
	fi
}

function CheckForRspCode
{
Description=`sqlplus -s /nolog << EOF 2> /dev/null
	CONNECT_TO_SQL
	set heading off ;
	set linesize 1000 ;
	set pagesize 1000 ;
	set feedback off ;
	select description from du_rsp_codes where rsp_code = '$RspCode' ;
EOF`
Description=`echo $Description | cut -d. -f2`
Description=`echo $Description | grep -v "^$"` > /dev/null

if [ -z "$Description" ]; then
	echo -n "Invalid RspCode : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
	echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
	let RejectedCount=$RejectedCount+1
	return 1
fi
}

function GeneratePGWRFF
{
	for file in `ls $RANGER5_4HOME/RangerData/PGWData/success`
	do
		StartTime=`date +%H:%M:%S' '%d/%m/%Y`
		AcceptedCount=0
		RejectedCount=0
		ProcessedCount=`sed -n "$=" $RANGER5_4HOME/RangerData/PGWData/$file`
		ProcessRecords
	done
}

function CheckTransactionID
{
	touch $RANGERHOME/share/Ranger/maxtransactionid.dat
	MaxTransactionID=`cat $RANGERHOME/share/Ranger/maxtransactionid.dat`

	if [ -z "$MaxTransactionID" ]; then
		MaxTransactionID=0
	fi

	if [ "$TransactionId" -gt "$MaxTransactionID" ];then
		echo $TransactionId > $RANGERHOME/share/Ranger/maxtransactionid.dat
		return 0
	else
		echo -n "Invalid Transaction ID : " >> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
		echo -e "$line\n">> $RANGER5_4HOME/RangerData/PGWData/Rejected/"$file".log
		let RejectedCount=$RejectedCount+1
		return 1
	fi
}

main ()
{
	while true
	do
		GeneratePGWRFF
	done

	exit 0
}


if [ "$Status" == "stop" ]; then
	if [ -f $RANGERHOME/share/Ranger/duPGWParserPID ]; then
		kill -9 `cat $RANGERHOME/share/Ranger/duPGWParserPID`
		rm -f $RANGERHOME/share/Ranger/duPGWParserPID
		echo "duPGWParser Stopped Successfuly ..."
		exit 0 
	else
		echo "No Such Process ... "
		exit 1
	fi
elif [ "$Status" == "start" ]; then
	if [ ! -f $RANGERHOME/share/Ranger/duPGWParserPID ]; then
		echo "duPGWParser Started Successfuly ..."
		echo $$ > $RANGERHOME/share/Ranger/duPGWParserPID
		while true
		do
			main $*
		done
	else
		echo "duPGWParser Already Running ..."
		exit 2
	fi
else
	echo >&2 "Usage duPGWParser.sh <-r start/stop>" && exit 1 
	exit 3
fi

