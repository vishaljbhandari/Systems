#/*******************************************************************************
# *  Copyright (c) Subex Limited 2013. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from Subex Limited or in accordance with the terms and          *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************/
#!/bin/bash
#"${STATE?Need to set STATE}"
        ev="Environmental Variable"
        printf "\t%s\n" "[ERROR] Checking $ev"
	if [ ! -z $"DB_HOSTNAME" ]; then echo -e "[DB_HOSTNAME] $ev [DB_HOSTNAME] is not set"; exit 77; else exit 45; fi

