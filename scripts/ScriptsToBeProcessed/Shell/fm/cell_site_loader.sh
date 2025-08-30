# *******************************************************************************/
# *     Copyright (c) Subex Systems Limited 2001. All rights reserved.             *
# *     The copyright to the computer program(s) herein is the property of Subex   *
# *     Systems Limited. The program(s) may be used and/or copied with the written *
# *     permission from Subex Systems Limited or in accordance with the terms and  *
# *     conditions stipulated in the agreement/contract under which the program(s) *
# *     have been supplied.                                                        *
# *******************************************************************************/

#! /bin/bash

. rangerenv.sh  

mkdir -p $HOME/Loader_Location

LOG_LOCATION=$RANGERHOME/LOG/cellsite_loader.log
cd $HOME/Loader_Location

ls cellsite_loader.dat > /dev/null 2>&1
Ret=$?
if [ "$Ret" != "0" ]; then
        echo " WARNING : No cellsite_loader.dat file present `date` " >> $RANGERHOME/LOG/cellsite_loader.log
        echo " WARNING : No cellsite_loader.dat file present " 
else 
        echo "Backing Cell Site Data ..." 
        sqlplus -s /nolog << EOF > /dev/null
		CONNECT_TO_SQL
        whenever sqlerror exit 5 ;
        set heading off ;
        spool $RANGERHOME/LOG/cellsite.log;
        drop table TRASH_CELL_SITE_GEO_POSITIONS;
        create table TRASH_CELL_SITE_GEO_POSITIONS as (select * from CELL_SITE_GEO_POSITIONS);
        delete from CELL_SITE_GEO_POSITIONS;
        commit;
        spool off;
        quit;
EOF
        echo "Loading Cell Site Data ..."
		CONNECT_TO_SQLLDR
        silent=header,feedback \
        control=$RANGERHOME/share/Ranger/cellsite_loader.ctl \
        data=$HOME/Loader_Location/cellsite_loader.dat \
        log=$RANGERHOME/LOG/cellsite_loader.log \
        bad=$RANGERHOME/LOG/cellsite_loader.bad \
        discard=$RANGERHOME/LOG/cellsite_loader.dsc parallel=YES ERRORS=999999
        echo " Loading of the data complete"
        echo " Log Files cellsite_loader.log and cellsite_loader.bad"
        cd $HOME/Loader_Location/
        rm cellsite_loader.dat
fi
        echo " Log File are located at $RANGERHOME/LOG/cellsite_loader.log" 
        echo " Log File are located at $RANGERHOME/LOG/cellsite.log" 

cd - >/dev/null 2>&1
