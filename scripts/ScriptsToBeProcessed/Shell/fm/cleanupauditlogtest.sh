#! /bin/sh

. shUnit

function TestAuditLogCleanup
{
	shuAssert "Setup" "scriptlauncher ./Tests/testcleanupAuditLogsetup.sh"
	shuAssert "Running script" "scriptlauncher ./cleanupAuditLog.sh"
	shuAssert "Verification" "scriptlauncher ./Tests/testcleanupAuditLogV.sh"
} > /dev/null 2>&1

function InitFunction
{
	shuRegTest TestAuditLogCleanup 
}

shuStart InitFunction

