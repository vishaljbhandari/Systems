. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


getport()
{
port=`awk -F ':' '/port/ { print $3 }' $RANGERV6_NOTIFICATIONMANAGER_HOME/config/notification_manager.yml | sed -e 's/^ *//'`
echo $port
}

main()
{
        getport
}

main