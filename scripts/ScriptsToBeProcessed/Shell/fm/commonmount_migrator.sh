if [ -z "$CLIENT_CHECKOUT_PATH" ];then
	$CLIENT_CHECKOUT_PATH=../Client/src
fi

cd $CLIENT_CHECKOUT_PATH

cp -r data/ ~/CommonMountPoint/Client/
echo "$CLIENT_CHECKOUT_PATH/data Copied"
cp -r log/ ~/CommonMountPoint/Client/
echo "$CLIENT_CHECKOUT_PATH/log/ Copied"
cp -r private/ ~/CommonMountPoint/Client/
echo "$CLIENT_CHECKOUT_PATH/private/ Copied"
cp -r public/Attachments ~/CommonMountPoint/Client/public/
echo "$CLIENT_CHECKOUT_PATH/public/Attachments Copied"
cp -r public/Data ~/CommonMountPoint/Client/public/
echo "$CLIENT_CHECKOUT_PATH/public/Data Copied"
cp -r public/Imports ~/CommonMountPoint/Client/public/
echo "$CLIENT_CHECKOUT_PATH/public/Imports Copied"
cp -r report_schedule/ ~/CommonMountPoint/Client/
echo "$CLIENT_CHECKOUT_PATH/report_schedule/ Copied"
cp -r tmp/ ~/CommonMountPoint/Client/
echo "$CLIENT_CHECKOUT_PATH/tmp/ Copied"


cd ../notificationmanager/
cp -r app_logs/ ~/CommonMountPoint/
echo "$CLIENT_CHECKOUT_PATH/../notificationmanager/app_logs/ Copied"
cp -r log/* ~/CommonMountPoint/app_logs/nm_log/
echo "$CLIENT_CHECKOUT_PATH/../notificationmanager/log/ Copied"

cd ../querymanager/
cp -r LOG/* ~/CommonMountPoint/app_logs/QM_LOG/
echo "$CLIENT_CHECKOUT_PATH/../querymanager/LOG Copied"


cd $RANGERHOME
cd ..

cp -r Attachments/ ~/CommonMountPoint/
echo "$RANGERHOME/Attachments/ Copied"
cp -r FMSData/ ~/CommonMountPoint/
echo "$RANGERHOME/FMSData/ Copied"
cp -r InMemory/ ~/CommonMountPoint/
echo "$RANGERHOME/InMemory/ Copied"
cp -r LOG/ ~/CommonMountPoint/
echo "$RANGERHOME/LOG/ Copied"
cp -r QueryLogs/ ~/CommonMountPoint/
echo "$RANGERHOME/QueryLogs/ Copied"
cp -r QueryResults/ ~/CommonMountPoint/
echo "$RANGERHOME/QueryResults/ Copied"
cp -r syslog/ ~/CommonMountPoint/
echo "$RANGERHOME/syslog/ Copied"
#cp -r TABLESPACE_DATAFILES/ ~/CommonMountPoint/
#echo "$RANGERHOME/TABLESPACE_DATAFILES/ Copied"
cp -r tmp/ ~/CommonMountPoint/
echo "$RANGERHOME/tmp/ Copied"

cp sbin/ranger.conf ~/CommonMountPoint/Conf/
cp sbin/programmanager.conf* ~/CommonMountPoint/Conf/
cp sbin/rangerenv* ~/CommonMountPoint/Conf/
echo "$RANGERHOME/sbin/(ranger.conf,rangerenv & all programmanager conf files) Copied"

