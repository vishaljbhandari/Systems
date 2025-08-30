#!/usr/bin/perl

use strict ;
use warnings ;

use Time::Local;
use Time::localtime;

# --- Data Holders.
my %SubscriberFieldConfig ;
my %SubscriberFieldConfigBackup ;
my %ServiceConfig ;
my @FieldOrder ;
my @ServiceOrder ;
# --- Directory Entries.
my $InputDir ;
my $OutputDir ;
my $RejectedDir ;
my $LOGFileName ;
my $RangerHome 				= $ENV{"RANGERHOME"} ;
# --- Precompiled Generic Regular Expressions .
my $PhoneNumberModifier 	= qr/^0|^971|^!971/is ;
my $MustCallConversion		= qr/^00|^0|^\+/is ;
my $PhoneNumberValidator 	= qr/^\+?\d+$/is ;
my $TagValueValidator 		= qr/<(.*)>(.*)<\/(.*)>/is ;
my $PhoneNumberFields		= qr/PHONE_NUMBER/is ;
my $DateFields				= qr/DATE/is ;
my $DateValidator 			= qr/(\d{2}\b)\/(\d{2}\b)\/(\d{4}\b).*/is ;
# --- Freequently Used RegEx.
# --- File Handlers.
my $FILEHANDLE ;
# --- Log Counters .
my $AcceptedFiles ;
my $IgnoredFiles ;
my $ProcessedFiles ;


sub LoadFieldList
{
    my $FileName = shift ;
    my $ConfigFile ;
    open (ConfigFile, "<$FileName") or return -1 ;
    my @KeyValue ;
    %SubscriberFieldConfig = () ;
    %SubscriberFieldConfigBackup = () ;
    %ServiceConfig = () ;
    @ServiceOrder = () ;
    @FieldOrder = () ;

    while (my $Line = <ConfigFile>)
    {
        chomp ($Line) ;
        @KeyValue = split (/=/, $Line) ;
        if ($KeyValue[0] eq "FIELD_CONFIG_FILE")
        {
            my @FieldList ;
            my $SUB_FIELDS ;
            open (SUB_FIELDS, "<$KeyValue[1]") or die ("Unable To Open Field Config File !") ;
            my $Count = 0 ;
            while (my $TagLine = <SUB_FIELDS>)
            {
                chomp ($TagLine) ;
                @FieldList = split (/=/, $TagLine) ;
                $SubscriberFieldConfig{$FieldList[0]} = $FieldList[1] ;
                if ($FieldList[1] ne "NotVisible")
                {
                    $FieldOrder[$Count] = $FieldList[0] ;
                }
                $Count++ ;
            }
    		%SubscriberFieldConfigBackup = %SubscriberFieldConfig ;
        }
        elsif ($KeyValue[0] eq "SERVICE_CONFIG")
        {
            my $SERVICE_LIST ;
            open (SERVICE_LIST, "<$KeyValue[1]") or die ("Unable To Open Service Config File !") ;
            my $Count = 0 ;
            while (my $TagLine = <SERVICE_LIST>)
            {
                chomp ($TagLine) ;
                my @FieldList = split (/=/, $TagLine) ;
                $ServiceConfig{$FieldList[0]} = $FieldList[1] ;
                $ServiceOrder[$Count++] = $FieldList[0] ;
            }
        }
	}
    return 0 ;

}

sub LoadConfig
{
	my $FileName = shift ;
	my $ConfigFile ;
	open (ConfigFile, "<$FileName") or return -1 ;

	my @KeyValue ;
	while (my $Line = <ConfigFile>)
	{
		chomp ($Line) ;
		@KeyValue = split (/=/, $Line) ;
		
		if ($KeyValue[0] eq "INPUT_DIR")
		{
			$InputDir = $KeyValue[1] ;
			if (! -d $InputDir or ! -d ($InputDir . "/success"))
			{
				die ("Invalid Input [" . $KeyValue[1] . "] Directory !\n") ;
			}
		}
		elsif ($KeyValue[0] eq "OUTPUT_DIR")
		{
			$OutputDir = $KeyValue[1] ;
			if (! -d $OutputDir or ! -d ($OutputDir . "/success"))
			{
				die ("Invalid Output [" . $KeyValue[1] . "] Directory !\n") ;
			}
		}
		elsif ($KeyValue[0] eq "LOG_FILE_NAME")
		{
			$LOGFileName = $KeyValue[1] ;
		}
	}
	return 0 ;
}

sub FormatDate
{
	my $InputDate = shift ;
	if (my @Flds = ($InputDate =~ /$DateValidator/))
	{
		$InputDate = "$Flds[1]/$Flds[0]/$Flds[2]" ;
	}
	return $InputDate ;
}

sub ProcessFields
{
	my $Record = "|" ;
	foreach my $Field (@FieldOrder)
	{
		if ($SubscriberFieldConfig{$Field} ne "Unknown")
		{
			if (exists($SubscriberFieldConfig{$SubscriberFieldConfig{$Field}}))
			{
				$Field = $SubscriberFieldConfig{$Field} ; 
			}
			
			if ($SubscriberFieldConfig{$Field} ne "NotVisible")
			{
				$Record = $Record . $SubscriberFieldConfig{$Field} ;
			}
		}
		$Record = $Record . "|" ;
		$SubscriberFieldConfig{$Field} = "" ;
	}
	$Record = $Record . "\n" ;
	return $Record ;
}

sub ProcessServicesTag
{
	my $Code = shift ;
	my $Line = "" ;
	$Line = <FILEHANDLE> ;
	$Line = <FILEHANDLE> ;
	$Line = <FILEHANDLE> ;
	chomp ($Line) ;
	if (my ($StartTag, $Value, $EndTag) = ($Line =~ /$TagValueValidator/))
	{
		if (exists($ServiceConfig{$Code}))
		{
			$ServiceConfig{$Code} = $Value ;
		}
	}
}

sub UNFConversion
{

	my $PhoneNumber = shift ;
	if ($PhoneNumber =~ m/$PhoneNumberValidator/) 
	{
		if ($PhoneNumber =~ /^\+/)
		{
			return $PhoneNumber ;
		}
		$PhoneNumber =~ s/$PhoneNumberModifier/+971/ ;
	}
	return $PhoneNumber ;
}

sub PopulateService
{

	my $LandlineService = 4096 ;
	my $BroadBandService = 8192 ;
	my $HomePlusService = 16384 ;
	my $TVService = 32768 ;

	$SubscriberFieldConfig{"SERVICE"} = "$LandlineService" ;
	if ( $SubscriberFieldConfig{"RATE_PLAN"}  eq "IICSA" ) 
	{
		$SubscriberFieldConfig{"SERVICE"} = "$BroadBandService" ;
	}
	elsif( $SubscriberFieldConfig{"RATE_PLAN"}  eq "IVCSA")
	{
		$SubscriberFieldConfig{"SERVICE"} = "$LandlineService" ;
	}
	elsif ( $SubscriberFieldConfig{"RATE_PLAN"}  eq "IBCDB")
	{
		$SubscriberFieldConfig{"SERVICE"} = "$HomePlusService" ;
	}
	elsif ( $SubscriberFieldConfig{"RATE_PLAN"}  eq "ITCSA")
	{
		$SubscriberFieldConfig{"SERVICE"} = "$TVService" ;
	}
}


sub PopulateQOS
{
	$SubscriberFieldConfig{"OPT_FIELD_6"} = "" ;
}

sub PopulateLineType
{
	$SubscriberFieldConfig{"OPT_FIELD_7"} = "";   
}

sub ProcessFile
{
	my $File = shift ;
	my $Index = 1 ;

	open (FILEHANDLE, "<$InputDir/$File") or return 1 ;
	$SubscriberFieldConfig{"OPT_FIELD_5"} = $File;   


	while (my $Line = <FILEHANDLE>)
	{
		if (my ($StartTag, $Value, $EndTag) = ($Line =~ /$TagValueValidator/))
		{
			if (exists($SubscriberFieldConfig{$StartTag}))
			{
				if ($StartTag =~ /$PhoneNumberFields/)
				{
					$Value = UNFConversion $Value ;	
				}

				elsif ($StartTag =~ /$DateFields/)
				{
					$Value = FormatDate $Value ;
				}

				elsif ($StartTag =~ /CO_STATUS/)
				{
					$Value = substr($Value, 0, 1) ;
				}

				elsif ($StartTag =~ /BILL_CYCLE/)
				{
					$Value =~ s#([A-Za-z]+)([\d]+)#$2# ;
				}

				if ($StartTag ne 'PHONE_NUMBER' or $SubscriberFieldConfig{"MAIN_DIRNUM_FLAG"} eq "Y")
				{
					$SubscriberFieldConfig{$StartTag} = $Value ;
				}

				if ($StartTag eq 'PHONE_NUMBER' and $SubscriberFieldConfig{"MAIN_DIRNUM_FLAG"} eq "Y")
				{
					$SubscriberFieldConfig{"MAIN_DIRNUM_FLAG"} = "N"
				}
			}
			elsif ($StartTag =~ /SN_CODE/)
			{
				ProcessServicesTag $Value ;
			}
			elsif ($StartTag =~ /PAYMENT_RESPONSIBLE_FLAG/)
			{
				if ($Value eq "Y")
				{
					$SubscriberFieldConfig{"OPT_FIELD_3"} = $SubscriberFieldConfig{"ACCOUNT_ID_PUB"} ;
				}
			}
		}
	}

	if ($SubscriberFieldConfig{"CONTRACT_ID_PUB"} eq "Unknown")
	{
		return 1 ;
	}

	PopulateService ;
	PopulateQOS ;
	PopulateLineType ;

	if ($SubscriberFieldConfig{"DOCUMENT_TYPE"} =~ /passport/i)
	{
		$SubscriberFieldConfig{"PASSPORT_NUMBER"} = $SubscriberFieldConfig{"DOCUMENT_NUMBER"} ;
	}
	elsif ($SubscriberFieldConfig{"DOCUMENT_TYPE"} =~ /[company regestration no\.|license no\.]/)
	{
		$SubscriberFieldConfig{"COMPANY_REGESTRATION_NUMBER"} = $SubscriberFieldConfig{"DOCUMENT_NUMBER"} ;
	}

	my $Output = ProcessFields ;
	
	close (FILEHANDLE) ;
	open (FILEHANDLE, ">$OutputDir/$File") ;
	print FILEHANDLE $Output ;
	close (FILEHANDLE) ;
	open (FILEHANDLE, ">$OutputDir/success/$File") ;
	close (FILEHANDLE) ;
	return 0 ;
}

sub ProcessDir
{
	my $ConfigFile = shift ;
	my $DirName = $InputDir ;
	my $DIRHANDLER ;
	
	while (1)
	{
		LoadFieldList ($ConfigFile) ;
		my $stime = time ;
		opendir (DIRHANDLER, "$DirName/success") or die "Unable To Open Directory '" . $DirName . "] !" ;
		$AcceptedFiles = $IgnoredFiles = $ProcessedFiles = 0 ;
		foreach my $File (sort {$a cmp $b} readdir(DIRHANDLER))
		{
			%SubscriberFieldConfig = %SubscriberFieldConfigBackup ;
			my $TestFile = $DirName . "/$File" ;
			if (-s $TestFile and -f $TestFile and -r $TestFile and -T $TestFile)
			{
				$ProcessedFiles++ ;
				if (ProcessFile ($File) == 0)
				{
					$AcceptedFiles++ ;
				}
				else
				{
					$IgnoredFiles++ ;
				}
				unlink "$DirName/$File" ;
				unlink "$DirName/success/$File" ;
			}
		}
		closedir (DIRHANDLER) ;
		my $etime = time ;
		my $diff = $etime - $stime ;

		my $LOGFILE ;
		if (open (LOGFILE, ">>$LOGFileName"))
		{
			if ($ProcessedFiles > 0)
			{
				my $tm = localtime;
				my ($H, $M, $S, $D, $MO, $Y) = ($tm->hour, $tm->min, $tm->sec, $tm->mday, $tm->mon, $tm->year) ;
				$Y = $Y + 1900 ;
				print LOGFILE "              ********************\n" ;
				printf LOGFILE "TIBCO Subscriber Data Parser @ %04d/%02d/%02d %02d:%02d:%02d\n", $Y,$MO,$D,$H,$M,$S ;
				print LOGFILE "--------------------------------------------------\n" ;
				print LOGFILE "Total Files Processed    : $ProcessedFiles\n" ;
				print LOGFILE "Total Files Accepted     : $AcceptedFiles\n" ;
				print LOGFILE "Total Files Ignored      : $IgnoredFiles\n" ;
				print LOGFILE "--------------------------------------------------\n" ;
				print LOGFILE "Total Time Taken         : $diff\n" ;
				print LOGFILE "--------------------------------------------------\n\n\n" ;
			}
			close (LOGFILE) ;
		}
		sleep (30) ;
	}
}

sub main
{
	my $ConfigFile = shift ;
	LoadConfig ($ConfigFile) ;
	ProcessDir ($ConfigFile) ;
}

main $ARGV[0] ;
