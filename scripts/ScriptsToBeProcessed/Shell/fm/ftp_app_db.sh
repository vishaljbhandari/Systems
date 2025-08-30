#! /bin/bash
. /nikira/.bashrc

   echo " " >> $RANGERHOME/LOG/app_db_server.log
   echo "Starting the SCP of the Attachment from APP server to DB server"   >> $RANGERHOME/LOG/app_db_server.log
   cd $NIKIRACLIENT/public/Attachments/
   scp -r Alarm/ nikira@fmsprod1:$NIKIRACLIENT/public/Attachments/

   rm -rf $NIKIRACLIENT/public/Attachments/Alarm/*
