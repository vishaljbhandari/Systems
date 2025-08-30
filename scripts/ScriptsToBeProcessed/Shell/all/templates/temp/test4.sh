#/*******************************************************************************
# *  Copyright (c) Subex Limited 2013. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from Subex Limited or in accordance with the terms and          *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************/
#!/bin/bash
counter=5
#while $counter -eq 0
#do
        if [ ping -c1 $1 &>/dev/null -eq "1" ] 
	then
		echo "Try $counter, Host $1 not Found - `date`"
		counter=$((counter-1))
	else
		echo "Try $counter, Host $1 Found - `date`"
		counter=0
	fi
#done
