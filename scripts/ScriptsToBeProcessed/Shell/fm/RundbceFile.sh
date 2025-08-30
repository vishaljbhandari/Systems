#!/bin/bash
. $WATIR_HOME/Scripts/Server/configuration.sh

ssh -l $USER $HOST Watir/Scripts/runDBCEfile.sh $*

