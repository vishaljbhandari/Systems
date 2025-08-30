#!/bin/sh

rm -rf ./report-selftest
mkdir ./report-selftest

ruby -I./lib -I./test test/test/unit/ui/testreporter.rb -r html -o ./report-selftest

