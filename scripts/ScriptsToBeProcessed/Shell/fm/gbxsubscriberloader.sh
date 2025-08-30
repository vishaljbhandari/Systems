#! /bin/bash

if [ $# -ne 3 ]
then
	echo "Usage: $0 <Input Path> <Starting Account ID> <Starting Subscriber ID>"
	exit 1
fi

INPUT_PATH=$1

if [ ! -d $INPUT_PATH/success ]
then
	echo "Please provide a touch file in $INPUT_PATH/success directory"
	exit 2
fi

echo $2"|"$3"|" > acc_id_sub_id_info
for i in `ls -rt $INPUT_PATH/success`
do
>subscriber.txt
>account.txt
>account_credit_detail.txt
>subscriber_group.txt
>errfile

START_ACCOUNT_ID=`cut -d"|" -f1 acc_id_sub_id_info`
START_SUBSCRIBER_ID=`cut -d"|" -f2 acc_id_sub_id_info`
PREV_ACCOUNT_NAME=`cut -d"|" -f3 acc_id_sub_id_info`

sort $INPUT_PATH/$i | ./gbxsubscriberdatagen.awk start_account_id=$START_ACCOUNT_ID start_subscriber_id=$START_SUBSCRIBER_ID prev_account_name=$PREV_ACCOUNT_NAME

sqlldr $DB_USER/$DB_PASSWORD control="account.ctl" rows=1000 log=account.log.$START_ACCOUNT_ID data=account.txt
sqlldr $DB_USER/$DB_PASSWORD control="account_credit_detail.ctl" rows=1000 log=account_credit_detail.log.$START_ACCOUNT_ID data=account_credit_detail.txt
sqlldr $DB_USER/$DB_PASSWORD control="subscriber.ctl" rows=1000 log=subscriber.log.$START_SUBSCRIBER_ID data=subscriber.txt

rm -f $INPUT_PATH/$i $INPUT_PATH/success/$i

sqlplus $DB_USER/$DB_PASSWORD << EOF
update subscriber set MODIFIED_DATE = sysdate where MODIFIED_DATE is null;
commit;

EOF
done
