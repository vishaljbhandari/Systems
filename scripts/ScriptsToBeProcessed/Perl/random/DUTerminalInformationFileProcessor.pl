#!/usr/bin/perl
#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################


use strict ;
use warnings ;

use Time::Local ;
use Time::localtime ;

my $AcceptedRecords = 0 ;
my $IgnoredRecords = 0 ;
my $InputFileName = "" ;
my $OutputFileName = "" ;

sub LogProcessDetails
{
	my $LogFilePath = $ENV{RANGERHOME} ;
	my $LOG_FILE ;
	my $TimeStamp ;
	
	open (LOG_FILE, ">>$LogFilePath/LOG/DUTerminalInformation.log") or die ("Unable To Log Process Details\n") ;	

	my $tm = localtime;
	my ($H, $M, $S, $D, $MO, $Y) = ($tm->hour, $tm->min, $tm->sec, $tm->mday, $tm->mon, $tm->year) ;
	$Y = $Y + 1900 ;
	$MO = $MO + 1 ;
	
	print LOG_FILE "\n\n   --------------------------------------------------------------------\n" ;
	print LOG_FILE "File Name  : $InputFileName\n" ;
	print LOG_FILE "Time Stamp : " ;
	printf LOG_FILE "%04d/%02d/%02d %02d:%02d:%02d\n", $Y, $MO, $D, $H, $M, $S ;
	print LOG_FILE "   --------------------------------------------------------------------\n" ;
	print LOG_FILE "Total Records      : " . ($AcceptedRecords + $IgnoredRecords) . "\n" ;
	print LOG_FILE "Total Accepted     : $AcceptedRecords\n" ;
	print LOG_FILE "Total Ignored      : $IgnoredRecords\n" ;
	print LOG_FILE "   --------------------------------------------------------------------\n\n\n" ;
	close (LOG_FILE) ;
}

sub main
{

	my $FileName = shift ;
	my $Output = shift ;
	my ($FILE_PTR, $OUTPUT) ;

	open (FILE_PTR, $FileName) or die ("Unable To Open File [$FileName]\n") ;
	open (OUTPUT, ">$Output") or die ("Unable To Open File For Output [$Output]\n") ;

	my $Line = <FILE_PTR> ;
	while ($Line = <FILE_PTR>)
	{
		if ((my @Fields = split (/\|/, $Line)) >= 3)
		{
			$AcceptedRecords ++ ;
			print OUTPUT $Fields[0] . "," . $Fields[2] . "," . $Fields[1] . "\n" ;
		}
		else
		{
			$IgnoredRecords ++ ;
		}
	}
	LogProcessDetails ;	
	close (FILE_PTR) ;
	close (OUTPUT) ;
}

$InputFileName = $ARGV[0] ;
$OutputFileName = $ARGV[1] ;

main $InputFileName, $OutputFileName ;
