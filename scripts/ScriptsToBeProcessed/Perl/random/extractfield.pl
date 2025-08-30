#! /usr/bin/perl

use strict ;

sub TranslateSpaces
{
	my $ToTranslate = $_[0] ;
	$ToTranslate =~ s/ /_^_/g ;
	return $ToTranslate ;
}

sub SplitFields
{
	my $lineData = $_[0] ;
	my @KeyValue=split ( /\|/,$lineData ) ;
	my $i ;
	my %HashValue ;

	for ($i=0 ; $i<@KeyValue ; $i++)
	{
		my @Data=split (/\=/,$KeyValue[$i]) ;
		$HashValue{ uc $Data[0] } = $Data[1] ;
	}

	my $MSISDN =  $HashValue{'MSISDN'} eq '' ? "null" : $HashValue{'MSISDN'} ;
	my $TimeStamp = $HashValue{'DATE'} eq '' ? "null" : $HashValue{'DATE'} ;
	my $Value = $HashValue{'AMOUNT'} eq '' ? 0 : $HashValue{'AMOUNT'}/100 ;
	my $CardNo = $HashValue{'PAN'}   eq '' ? "null" : $HashValue{'PAN'} ;
	my $RspCode = $HashValue{'RSPCODE'} eq '' ? "null" : $HashValue{'RSPCODE'} ;
	$RspCode =~ s/^0+$/0/gc ;

	my $CardholderName = $HashValue{'CARDHOLDERNAME'} eq '' ? "null" : $HashValue{'CARDHOLDERNAME'} ;
	my $TransactionId = $HashValue{'TRANSACTIONID'} eq '' ? "null" : $HashValue{'TRANSACTIONID'} ;

	$CardholderName = TranslateSpaces($CardholderName) ;

	my $ExpiryMonth = $HashValue{'EXPMONTH'} ;
	my $ExpiryYear = $HashValue{'EXPYEAR'} ;

	my $ExpiryDate = 'null' ;

	if ($ExpiryMonth ne '' and $ExpiryYear ne '')
	{
		$ExpiryDate = $ExpiryMonth."/".$ExpiryYear ;
	}

	my $StartMonth = $HashValue{'STARTMONTH'} ;
	my $StartYear = $HashValue{'STARTYEAR'} ;

	my $StartDate = '_^_' ;

	if ($StartMonth ne '' and $StartYear ne '')
	{
		$StartDate = "_^_".$StartMonth."/".$StartYear ;
	}

	my $TranType = "_^_".$HashValue{'TRANTYPE'} ;
	$TranType =~ s/ //g ;
	my $PStatus = "_^_".$HashValue{'STATUS'} ;
	$PStatus =~ s/ //g ;
	my $PayChannel = "_^_".$HashValue{'PAYCHANNEL'} ;
	$PayChannel =~ s/ //g ;
	my $FirstName = "_^_".$HashValue{'FIRSTNAME'} ;
	$FirstName =~ s/ //g ;
	my $LastName = "_^_".$HashValue{'LASTNAME'} ;
	$LastName =~ s/ //g ;
	my $CardAcceptorID = "_^_".$HashValue{'CARD_ACCEPTOR_ID_CODE'} ;
	$CardAcceptorID =~ s/ //g ;
	my $IPAddr = "_^_".$HashValue{'IPADDR'} ;
	$IPAddr =~ s/ //g ;
	$HashValue{'DESTMSISDN'} =~ s/ //g ;
	my $DestMSISDN = $HashValue{'DESTMSISDN'} eq '' ? "_^_" : "_^_+".$HashValue{'DESTMSISDN'} ;
	my $ResponseCode = "_^_".$HashValue{'RESPONSECODE'} ;
	$ResponseCode =~ s/ //g ;
	my $CardType = "_^_".$HashValue{'CARDTYPE'} ;
	$CardType =~ s/ //g ;
	my $IDNumber = "_^_".$HashValue{'IDNUMBER'} ;
	$IDNumber =~ s/ //g ;
	my $Issue = "_^_".$HashValue{'ISSUE'} ;
	$Issue =~ s/ //g ;

	print ( "$MSISDN $TimeStamp $Value $CardNo $RspCode $CardholderName $ExpiryDate $TransactionId $StartDate $TranType $PStatus $PayChannel $FirstName $LastName $CardAcceptorID $IPAddr $DestMSISDN $ResponseCode $CardType $IDNumber $Issue" ) ;
}

SplitFields ( $ARGV[0] ) ;


