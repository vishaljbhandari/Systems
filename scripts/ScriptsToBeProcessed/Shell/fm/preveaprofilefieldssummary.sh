#! /bin/bash

echo $$ > $COMMON_MOUNT_POINT/FMSData/Scheduler/PreveaProfileFieldLoaderpid
on_exit ()
{
    rm -f $COMMON_MOUNT_POINT/FMSData/Scheduler/PreveaProfileFieldLoaderpid
    exit ;
}
trap on_exit EXIT

. $RANGERHOME/sbin/rangerenv.sh

function executefunctionality()
{
sqlplus -s /nolog << EOF

CONNECT_TO_SQL

	set serveroutput on size unlimited
	whenever sqlerror exit 1 ;
        whenever oserror exit 2 ;

	ALTER INDEX IX_ACC_CRE_DET_ACCOUNT_NAME UNUSABLE ;
	ALTER TABLE ACCOUNT_CREDIT_DETAIL NOLOGGING ;
	ALTER SESSION enable parallel dml ;

	Declare
		cursor account_buff_records is
			select ACCOUNT_ID, 
				joinmanyrowsin1uniq (decode(profile_field_id, 501, profile_field_value)) profilevalueLastPaidAmount, 
				joinmanyrowsin1uniq (decode(profile_field_id, 506,profile_field_value))  profilevalueStaticCreditLimit,
				joinmanyrowsin1uniq (decode(profile_field_id, 507, profile_field_value)) profilevalueDynamicCreditLimit,
				joinmanyrowsin1uniq (decode(profile_field_id, 509, profile_field_value)) profilevalueLastPayDate, 
				joinmanyrowsin1uniq (decode(profile_field_id, 505, profile_field_value)) profilevalueOutstandingAmount, 
				joinmanyrowsin1uniq (decode(profile_field_id, 508, profile_field_value)) profilevalueUnbilledAmount, 
				joinmanyrowsin1uniq (decode(profile_field_id, 518, profile_field_value)) profilevalueExposure, 
				joinmanyrowsin1uniq (decode(profile_field_id, 517, profile_field_value)) profilevaluePercentageExposure, 
				joinmanyrowsin1uniq (decode(profile_field_id, 510, profile_field_value)) profilevalueBillCycleDay, 
				joinmanyrowsin1uniq (decode(profile_field_id, 511, profile_field_value)) profilevalueLastDueDate, 
				joinmanyrowsin1uniq (decode(profile_field_id, 512, profile_field_value)) profilevalueNoOfFullPayments, 
				joinmanyrowsin1uniq (decode(profile_field_id, 513, profile_field_value)) profilevalueNoOfPartPayments, 
				joinmanyrowsin1uniq (decode(profile_field_id, 514, profile_field_value)) profilevalueNoOfSlippages 
			from prevea.PROFILE_FIELD_VALUES pfv
			WHERE PROFILE_FIELD_ID in (501,506,507,509,505,508,518,517,510,511,512,513,514) and exists 
				( select account_id from subscriber where account_id = pfv.account_id and status = 1 and subscriber_type = 0 and product_type = 1 and account_id > 1024 )  group by account_id ;
		
	type array_account_ids 	  is table of prevea.profile_field_values.account_id%type index by binary_integer ;
	type LastPaidAmount is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
	type StaticCreditLimit is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
	type DynamicCreditLimit is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
	type LastPayDate is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type OutstandingAmount is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type UnbilledAmount is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type Exposure is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type PercentageExposure is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type BillCycleDay is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type LastDueDate is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type NoOfFullPayments is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type NoOfPartPayments is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;
        type NoOfSlippages is table of prevea.profile_field_values.profile_field_value%type index by binary_integer ;


	AccountIDs array_account_ids ;
	ProfileValueLastPaidAmount LastPaidAmount ;
	ProfileValueStaticCreditLimit StaticCreditLimit ;
	ProfileValueDynamicCreditLimit DynamicCreditLimit ;
	ProfileValueLastPayDate LastPayDate ;
	ProfileValueOutstandingAmount OutstandingAmount ;
	ProfileValueUnbilledAmount UnbilledAmount ;
	ProfileValueExposure Exposure ;
	ProfileValuePercentageExposure PercentageExposure ;
	ProfileValueBillCycleDay BillCycleDay ;
	ProfileValueLastDueDate LastDueDate ;
	ProfileValueNoOfFullPayments NoOfFullPayments ;
	ProfileValueNoOfPartPayments NoOfPartPayments ;
	ProfileValueNoOfSlippages NoOfSlippages ;

	begin
		open account_buff_records ;
		loop
			fetch account_buff_records bulk collect into AccountIDs, ProfileValueLastPaidAmount, ProfileValueStaticCreditLimit, ProfileValueDynamicCreditLimit,ProfileValueLastPayDate,ProfileValueOutstandingAmount,ProfileValueUnbilledAmount,ProfileValueExposure,ProfileValuePercentageExposure,ProfileValueBillCycleDay,ProfileValueLastDueDate,ProfileValueNoOfFullPayments,ProfileValueNoOfPartPayments,ProfileValueNoOfSlippages limit 1000 ;
				forall l_i in 1 .. AccountIDs.count
					update ACCOUNT_CREDIT_DETAIL set  
						last_paid_amount= ProfileValueLastPaidAmount(l_i),
						static_credit_limit= ProfileValueStaticCreditLimit(l_i),
						dynamic_credit_limit = ProfileValueDynamicCreditLimit(l_i),
						last_pay_date = to_date(ProfileValueLastPayDate(l_i), 'dd/mm/yyyy:hh24:mi:ss'),
						outstanding_amount = ProfileValueOutstandingAmount(l_i),
						unbilled_amount = ProfileValueUnbilledAmount(l_i),
						exposure = ProfileValueExposure(l_i),	
						percentage_exposure = ProfileValuePercentageExposure(l_i),	
						bill_cycle_day = ProfileValueBillCycleDay(l_i),
						last_due_date = to_date(ProfileValueLastDueDate(l_i), 'dd/mm/yyyy:hh24:mi:ss'),
						no_of_full_payments = ProfileValueNoOfFullPayments(l_i),
						no_of_part_payments = ProfileValueNoOfPartPayments(l_i),
						no_of_slippages = ProfileValueNoOfSlippages(l_i)
			
					where account_id = AccountIDs(l_i)
					; 
			commit ;
			exit when account_buff_records%NOTFOUND ;
    	end loop ;
	end;
/

	ALTER TABLE ACCOUNT_CREDIT_DETAIL LOGGING ;
	ALTER INDEX IX_ACC_CRE_DET_ACCOUNT_NAME REBUILD ONLINE ;

EOF
}

main ()
{
	executefunctionality
}

LOGFILE=$COMMON_MOUNT_POINT/LOG/preveaprofilesummarization.log
echo "--------------------------------------------------------------" >> $LOGFILE
echo "Start Timestamp: "`date`>> $LOGFILE
main $*	 >> $LOGFILE
echo -e "End Timestamp: "`date` >> $LOGFILE
echo "--------------------------------------------------------------" >> $LOGFILE

