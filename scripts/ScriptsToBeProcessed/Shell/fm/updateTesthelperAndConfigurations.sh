#!/bin/bash


fetchTesthelperConf()
{
    CONF_WATIR_HOME=`awk 'BEGIN {FS = "="} /\\$WATIR_HOME *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    HOME=`awk 'BEGIN {FS = "="} /\\$HOME *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    is_I2=`awk 'BEGIN {FS = "="} /\\$is_I2 *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    USER=`awk 'BEGIN {FS = "="} /\\$USER *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    HOST=`awk 'BEGIN {FS = "="} /\\$HOST *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    HOST_ADDRESS=`awk 'BEGIN {FS = "="} /\\$HOST_ADDRESS *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    UPLOAD_FILES=`awk 'BEGIN {FS = "="} /\\$UPLOAD_FILES *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    FILE_PATH=`awk 'BEGIN {FS = "="} /\\$FILE_PATH *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    STANDALONE=`awk 'BEGIN {FS = "="} /\\$STANDALONE *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
    DEBUG_ON=`awk 'BEGIN {FS = "="} /\\$DEBUG_ON *=/ {print $2}' atSetup.conf | sed 's/\\\\\\\/\\\\\\\\\\\\\\\/g'`
}

updateInTesthelper()
{
    echo "Updating testhelper.rb ... "
    
    sed "/\$WATIR_HOME\ *=/ c\ \$WATIR_HOME = $CONF_WATIR_HOME" $WATIR_HOME\\test_helper.rb |
    sed "/\$HOME\ *=/ c\ \$HOME = $HOME" |
    sed "/\$is_I2\ *=/ c\ \$is_I2 = $is_I2" |
    sed "/\$USER\ *=/ c\ \$USER = $USER" |
    sed "/\$HOST\ *=/ c\ \$HOST = $HOST" |
    sed "/\$HOST_ADDRESS\ *=/ c\ \$HOST_ADDRESS = $HOST_ADDRESS" |
    sed "/\$UPLOAD_FILES\ *=/ c\ \$UPLOAD_FILES = $UPLOAD_FILES" |
    sed "/\$FILE_PATH\ *=/ c\ \$FILE_PATH = $FILE_PATH" |
    sed "/\$STANDALONE\ *=/ c\ \$STANDALONE = $STANDALONE" |
    sed "/\$DEBUG_ON\ *=/ c\ \$DEBUG_ON = $DEBUG_ON" > $WATIR_HOME\\test_helper_updated.rb

    mv $WATIR_HOME\\test_helper_updated.rb $WATIR_HOME\\test_helper.rb
}

updateConfigurations()
{
    echo "Updating configurations.sh ... "
    
    server_target_dir=`awk 'BEGIN {FS = "="} /SERVER_TARGET_DIRECTORY *=/ {print $2}' bringNikiraSetup.conf`
    configuration_HOST=`echo $HOST | cut -d "\"" -f 2 | cut -d "/" -f 3 | cut -d ":" -f 1`
    configuration_USER=`echo $USER | cut -d "\"" -f 2 `
    configuration_Watir_server_home=`awk 'BEGIN {FS = "="} /WATIR_SERVER_HOME *=/ {print $2}' atSetup.conf`
    configuration_WINDOW_TIMEOUT=`awk 'BEGIN {FS = "="} /WINDOW_TIMEOUT *=/ {print $2}' atSetup.conf`
    configuration_ORAENV_ASK=`awk 'BEGIN {FS = "="} /ORAENV_ASK *=/ {print $2}' atSetup.conf`

    sed "/export\ *HOST\ *=/ c\export HOST=$configuration_HOST" $WATIR_HOME\\Scripts\\Server\\configuration.sh |
    sed "/export\ *USER\ *=/ c\export USER=$configuration_USER" |
    sed "/export\ *DBSETUP_HOME\ *=/ c\export DBSETUP_HOME=$server_target_dir/DBSetup" |
    sed "/export\ *RANGERV6_CLIENT_HOME\ *=/ c\export RANGERV6_CLIENT_HOME=$server_target_dir/Client/src" |
    sed "/export\ *RANGERV6_SERVER_HOME\ *=/ c\export RANGERV6_SERVER_HOME=$server_target_dir/Server" |
    sed "/export\ *RANGERV6_NOTIFICATIONMANAGER_HOME\ *=/ c\export RANGERV6_NOTIFICATIONMANAGER_HOME=$server_target_dir/Client/notificationmanager" |
    sed "/export\ *WATIR_SERVER_HOME\ *=/ c\export WATIR_SERVER_HOME=$configuration_Watir_server_home" |
    sed "/export\ *WINDOW_TIMEOUT\ *=/ c\export WINDOW_TIMEOUT=$configuration_WINDOW_TIMEOUT" |
    sed "/export\ *ORAENV_ASK\ *=/ c\export ORAENV_ASK=$configuration_ORAENV_ASK" >$WATIR_HOME\\Scripts\\Server\\configuration_modified.sh
    
    cd $WATIR_HOME
    configuration_DATA_HOME=`pwd`
    sed "/export\ *DATA_HOME\ *=/ c\export DATA_HOME=$configuration_DATA_HOME" $WATIR_HOME\\Scripts\\Server\\configuration_modified.sh > $WATIR_HOME\\Scripts\\Server\\configuration.sh
    rm -rf $WATIR_HOME\\Scripts\\Server\\configuration_modified.sh 
}


main()
{
    fetchTesthelperConf
    updateInTesthelper
    updateConfigurations
}


main