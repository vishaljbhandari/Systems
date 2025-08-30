#!/bin/bash

. $WATIR_HOME/Scripts/Server/configuration.sh

ssh -l $USER $HOST $WATIR_SERVER_HOME/BringNikiraSetup/bringNikiraSetup.sh
