#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#!/bin/bash

inputFile=$1

$RANGERHOME/bin/DUTerminalInformationFileProcessor.pl $inputFile $RANGERHOME/tmp/DUTerminalInformation.txt

LoadFileToTempDB ()
{
	CONNECT_TO_SQLLDR
		SILENT=(all) \
		CONTROL="$RANGERHOME/share/Ranger/DUTerminalInformationLoader.ctl" \
		DATA="$RANGERHOME/tmp/DUTerminalInformation.txt" \
		LOG="$RANGERHOME/tmp/DUTerminalInformationLoader.log" \
		BAD="$RANGERHOME/tmp/DUTerminalInformationLoader.bad" \
		DISCARD="$RANGERHOME/tmp/DUTerminalInformationLoader.dsc" ERRORS=999999
	Ret=$?
	if [ $Ret -ne 0 ]; then
		return 1
	fi
}

UpdateInsertTACInfo ()
{
	sqlplus  -s /nolog << EOF > /dev/null 2>&1
	CONNECT_TO_SQL

    whenever sqlerror exit 5;
	set heading off;
	set feedback on ;

	spool $RANGERHOME/tmp/mergeDUTerminalInformationLoader.log

	merge into du_terminal_information D  
	using 
	(
		select tac_code,manufacturer,model_number from temp_du_terminal_information
	) S 
	on 
	(
	s.tac_code = d.tac_code 
	) 
	when matched then 
		update set D.manufacturer=s.manufacturer,D.model_number=S.model_number
	when not matched then
		insert (tac_code,manufacturer,model_number) 
			values (s.tac_code,s.manufacturer,s.model_number) ;
	commit ;
	spool off 

EOF

	grep "ERROR" $RANGERHOME/tmp/merge.log >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		return 1
	else
		return 0
	fi
}

LoadFileToTempDB
UpdateInsertTACInfo

if [ $? -ne 0 ]; then
	echo "Unknown error, please check $RANGERHOME/tmp/merge.log for more information."
	exit 1
fi

rm -f $inputFile $RANGERHOME/tmp/DUTerminalInformation.txt 

