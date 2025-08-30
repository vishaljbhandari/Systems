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
use File::Basename ;

my $InputFileName = "" ;
my $OutputFileName = "" ;
my $RangerHome = $ENV{RANGERHOME} ;
my $Ranger5_4Home = $ENV{RANGER5_4HOME} ;

sub main
{
	my $FileName = shift ;
	my $Output = shift ;
	my $FieldPositions ;
	my ($FILE_PTR, $OUTPUT) ;
	my $AcceptedRecords = 0 ;
	my $RejectedRecords = 0 ;
	my $ProcessedRecords = 0 ;
	my $BaseName = basename($FileName) ;

	open (FILE_PTR, $FileName) or die ("Unable To Open File [$FileName]\n") ;
	open (OUTPUT, ">$Output") or die ("Unable To Open File For Output [$Output]\n") ;
	open (REJECT, ">$Ranger5_4Home/RangerData/RejectedTerminalUsageData/$BaseName.bad") or die ("Unable To Open File For Writing Rejected Data.\n") ;
	open (LOGFILE, ">>$RangerHome/LOG/DUTerminalUsage.log") or die ("Unable To Open Log File.\n") ;

	print REJECT "\n\n   --------------------------------------------------------------------\n" ;
	print REJECT "   Records Rejected since Length of Phone Number less than 8 :\n" ;
	print REJECT "   --------------------------------------------------------------------\n\n" ;

	if ($FileName =~ /_GSM$/)
	{
		$FieldPositions="GSM" ;
	}
	else
	{
		$FieldPositions="GPRS" ;
	}

	while (my $Line = <FILE_PTR>)
	{
		my @Fields = split (/,/, $Line) ;
		if (($FieldPositions eq "GSM") and (length($Fields[21]) >= 8))
		{
			print OUTPUT $Fields[7] . "," . $Fields[8] . "," . $Fields[9] . "," . $Fields[21] . "\n" ;
			$AcceptedRecords ++ ;
		}
		elsif (($FieldPositions eq "GPRS") and (length($Fields[5]) >= 8))
		{
			print OUTPUT $Fields[3] . "," . $Fields[17] . "," . $Fields[16] . "," . $Fields[5] . "\n" ;
			$AcceptedRecords ++ ;
		}
		else
		{
			$RejectedRecords ++ ;
			print REJECT $Line . "\n" ;
		}
	}

	my $tm = localtime;
	my ($H, $M, $S, $D, $MO, $Y) = ($tm->hour, $tm->min, $tm->sec, $tm->mday, $tm->mon, $tm->year) ;
	$Y = $Y + 1900 ;
	$MO ++ ;

	print LOGFILE "\n\n   --------------------------------------------------------------------\n" ;
	print LOGFILE "File Name  : $InputFileName\n" ;
	print LOGFILE "Time Stamp : " ;
	printf LOGFILE "%04d/%02d/%02d %02d:%02d:%02d\n", $Y, $MO, $D, $H, $M, $S ;
	print LOGFILE "   --------------------------------------------------------------------\n" ;
	print LOGFILE "Total Records      : " . ($AcceptedRecords + $RejectedRecords) . "\n" ;
	print LOGFILE "Total Accepted     : $AcceptedRecords\n" ;
	print LOGFILE "Total Rejected     : $RejectedRecords\n" ;
	print LOGFILE "   --------------------------------------------------------------------\n\n\n" ;

	print REJECT "   --------------------------------------------------------------------\n" ;
	print REJECT "File Name  : $InputFileName\n" ;
	print REJECT "Time Stamp : " ;
	printf REJECT "%04d/%02d/%02d %02d:%02d:%02d\n", $Y, $MO, $D, $H, $M, $S ;
	print REJECT "   --------------------------------------------------------------------\n" ;
	print REJECT "Total Records      : " . ($AcceptedRecords + $RejectedRecords) . "\n" ;
	print REJECT "Total Accepted     : $AcceptedRecords\n" ;
	print REJECT "Total Rejected     : $RejectedRecords\n" ;
	print REJECT "   --------------------------------------------------------------------\n\n\n" ;

	unlink "$RangerHome/RangerData/RejectedTerminalUsageData/$BaseName.bad" if $RejectedRecords == 0 ; 
}

END
{
	close (FILE_PTR) ;
	close (OUTPUT) ;
	close (REJECT) ;
	close (LOGFILE) ;
}

$InputFileName = $ARGV[0] ;
$OutputFileName = $ARGV[1] ;

main $InputFileName, $OutputFileName ;
