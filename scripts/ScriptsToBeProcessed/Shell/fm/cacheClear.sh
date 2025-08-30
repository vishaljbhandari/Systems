#!/bin/bash

. $HOME/RangerRoot/sbin/rangerenv.sh
. $HOME/Watir/Scripts/configuration.sh

. $HOME/Watir/Scripts/stopProgrammanager.sh

cd $RANGERV6_CLIENT_HOME

rake log:clear
rake tmp:cache:clear

./nikiraclientctl.sh restart

cd

bash $HOME/Watir/Scripts/RunProgramManager.sh
sleep 5
