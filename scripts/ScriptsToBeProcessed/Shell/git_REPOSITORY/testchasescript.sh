#! /bin/bash

. shUnit

SampleData="DIR/FILE.data"

function testCaseName
{
    shuAssert "output" "command" >/dev/null 2>&1
    shuAssert "result" "execution of any script" >/dev/null 2>&1
}

function InitFunction
{
	shuRegTest testCaseName

}

shuStart InitFunction

