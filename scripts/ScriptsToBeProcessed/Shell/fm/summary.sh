#! /bin/bash


if [ $# -eq 4 ]
then
	Parameter1=$2
	Parameter2=$3
	SubPartitionLevelSummary="Y"
else
	Parameter1=0
	Parameter2=0
	SubPartitionLevelSummary="N"
fi 

SummaryDate=$1

$SQL_COMMAND -s /nolog << EOF

CONNECT_TO_SQL
set feedback off;
set serveroutput off;

declare

begin
	
	CDRSummary.Initialize('$SummaryDate', '$Parameter1', '$Parameter2', '$SubPartitionLevelSummary') ;

	if ('$SubPartitionLevelSummary' = 'Y') then
		CDRSummary.GenerateSummaryWithPartition ;	
	else
		CDRSummary.GenerateSummary ;	
	end if ;

		CDRSummary.CleanUpOldRecords ;
		CDRSummary.UpdateSummaryInfo ;	
	commit ;

exception
	when others then
		dbms_output.put_line (substr ('Error : ' || sqlcode || '-' || sqlerrm, 1, 250)) ; 
end;
/
 
EOF
 
if [ $? -ne 5 ]; then
        exit 0
else
        exit 1
fi

