#!/bin/bash
sleep 5
kill -SIGHUP $PPID
sleep 3
kill -SIGTERM $PPID
sleep 2
