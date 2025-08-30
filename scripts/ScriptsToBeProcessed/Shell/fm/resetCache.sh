#!/bin/bash

. $WATIR_HOME/Scripts/Server/configuration.sh

ssh -l $USER $HOST rm $RANGERV6_CLIENT_HOME/tmp/nikira_cache/*
