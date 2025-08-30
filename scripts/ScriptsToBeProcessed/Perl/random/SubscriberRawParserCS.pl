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
my @FinalSubscriberFieldValues ;
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
my $SingleTagValueValidator 	= qr/<(.*)>/is ;
my $PhoneNumberFields		= qr/PHONE_NUMBER/is ;
my $DateFields				= qr/DATE/is ;
my $DateValidator 			= qr/(\d{2}\b)\/(\d{2}\b)\/(\d{4}\b).*/is ;
# --- Freequently Used RegEx.
# --- File Handlers.
my $FILEHANDLE ;
# --- Phone Number Details.
my @PhoneNumberDetails ;
my $DIR_NO_FLAG_INDEX = 0 ;
my $PHONE_NO_INDEX    = 1 ;
my $LOWER_EXT_INDEX   = 2 ;
my $UPPER_EXT_INDEX   = 3 ;
my $DIR_NO_STATUS_INDEX = 4 ;
# --- Iterators .
my $Size ;
my $Position ;

# --- Output Position of Fields.
my $Phone_Number_Position = 19 ;
my $Dir_Num_Status_Position =20 ;
my $Dir_Num_Flag_Position = 43 ;

# --- Log Counters .
my $AcceptedRecords ;
my $RejectedRecords ;
my $ProcessedRecords ;

my $RejectedReason ;
my $FileName ;
my $stTime ;

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
			return 0;
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
	foreach my $Field (@FieldOrder)
	{
		if ($SubscriberFieldConfig{$Field} ne "Unknown")
		{
			if (exists($SubscriberFieldConfig{$SubscriberFieldConfig{$Field}}))
			{
				$Field = $SubscriberFieldConfig{$Field} ; 
				$FinalSubscriberFieldValues[$Position] = "|" ;
			}
			
			if ($SubscriberFieldConfig{$Field} ne "NotVisible")
			{
				$FinalSubscriberFieldValues[$Position] = $SubscriberFieldConfig{$Field} ;
			}
		}
		else
		{
			$FinalSubscriberFieldValues[$Position] = "|" ;
		}
		$Position ++;
		$SubscriberFieldConfig{$Field} = "" ;
	}
	return 0 ;
}

sub ProcessServicesTag
{
	my $Code = shift ;
	my $Line = "" ;
	if ($Code == 337)
	{
		my $Index = 1 ;
		$Line = <FILEHANDLE> ;
		$Line = <FILEHANDLE> ;
		if (my ($StartTag, $Value, $EndTag) = ($Line =~ /$TagValueValidator/))
		{
			if ($StartTag =~ /\bSN_DESCRIPTION\b/ and $Value =~ /.*Mine international.*/i)
			{
				while (!(($Line = <FILEHANDLE>) =~ /.*\/SERVICES_LIST.*/))
				{
					if (my ($StartTag, $Value, $EndTag) = ($Line =~ /$TagValueValidator/))
					{
						if ($StartTag =~ /\bIMC_DESTINATION_NUMBER\b/)
						{
							$Value =~ s/$MustCallConversion/+/ ;
							$SubscriberFieldConfig{"MCN$Index"} = $Value ;
							$Index ++ ;
						}
					}
				}
			}
		}
		return ;
	}
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

sub WriteOutput
{
	my $Output="|" ;
        for(my $Iterator = 0 ; $Iterator < $Position ; $Iterator++ )
       	{
                $Output =  $Output.$FinalSubscriberFieldValues[$Iterator] ;
               	if ( $FinalSubscriberFieldValues[$Iterator] ne "|")
               	{
                        $Output =  $Output."|";
       	        }
       	}
        $Output = $Output . "\n" ;
	print FILEHANDLE $Output ;
	return 0;		
}

sub WriteLog
{		
	my $enTime = time ;
	my $diffTime = $enTime - $stTime ; 
	my $LOGFILE ;
	if (open (LOGFILE, ">>$LOGFileName"))
	{
		
		if ($ProcessedRecords > 0)
		{
			print LOGFILE "$RejectedReason\n" ;
			my $tm = localtime;
			my ($H, $M, $S, $D, $MO, $Y) = ($tm->hour, $tm->min, $tm->sec, $tm->mday, $tm->mon, $tm->year) ;
			$Y = $Y + 1900 ;
			printf LOGFILE "TIBCO Subscriber Data Parser @ %04d/%02d/%02d %02d:%02d:%02d\n", $Y,$MO,$D,$H,$M,$S ;
			print LOGFILE "--------------------------------------------------\n" ;
			print LOGFILE "FileName : $FileName\n";
			print LOGFILE "Total Records Processed    : $ProcessedRecords\n" ;
			print LOGFILE "Total Records Accepted     : $AcceptedRecords\n" ;
			print LOGFILE "Total Records Rejected     : $RejectedRecords\n" ;
			print LOGFILE "--------------------------------------------------\n" ;
			print LOGFILE "Total Time Taken         : $diffTime\n" ;
			print LOGFILE "--------------------------------------------------\n\n" ;
			print LOGFILE "              ********************\n\n" ;
		}
		close (LOGFILE) ;
	}
	return 0 ;
}

sub WriteIgnoredLog
{		
	my $enTime = time ;
	my $diffTime = $enTime - $stTime ; 
	my $LOGFILE ;
	if (open (LOGFILE, ">>$LOGFileName"))
	{
			print LOGFILE "$RejectedReason\n" ;
			my $tm = localtime;
			my ($H, $M, $S, $D, $MO, $Y) = ($tm->hour, $tm->min, $tm->sec, $tm->mday, $tm->mon, $tm->year) ;
			$Y = $Y + 1900 ;
			printf LOGFILE "TIBCO Subscriber Data Parser @ %04d/%02d/%02d %02d:%02d:%02d\n", $Y,$MO,$D,$H,$M,$S ;
			print LOGFILE "--------------------------------------------------\n\n" ;
			print LOGFILE "Ignored FileName : $FileName\n\n";
			print LOGFILE "--------------------------------------------------\n" ;
			print LOGFILE "Total Time Taken         : $diffTime\n" ;
			print LOGFILE "--------------------------------------------------\n\n" ;
			print LOGFILE "              ********************\n\n" ;
			
			close (LOGFILE) ;
	}
	return 0 ;
}

sub ProcessFile
{
	my $File = shift ;
	my $Index = 1 ;
	$AcceptedRecords = $RejectedRecords = $ProcessedRecords = $Position = 0;
	$Size = -1 ;
	$RejectedReason = "";
	$stTime = time ;

	open (FILEHANDLE, "<$InputDir/$File") or return 1 ;
	$SubscriberFieldConfig{"OPT_FIELD_5"} = $File;   #-----Added by Du-Development Team (Bilal Ghazi)	
	while (my $Line = <FILEHANDLE>)
	{
		if (my ($StartTag, $Value, $EndTag) = ($Line =~ /$TagValueValidator/))
		{
			if (exists($SubscriberFieldConfig{$StartTag}))
			{
				if ($StartTag =~ /$DateFields/)
				{
					$Value = FormatDate $Value ;
				}
				elsif ($StartTag =~ /BILL_CYCLE/)
				{
					$Value =~ s#([A-Za-z]+)([\d]+)#$2# ;
				}
				$SubscriberFieldConfig{$StartTag} = $Value ;
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
		elsif ( my ($SingleTag) = ($Line =~ /$SingleTagValueValidator/))
		{
			if ($SingleTag =~ /DIRECTORY_NUMBERS_LIST/)
			{
			  $Size++;
			  while($SingleTag ne "/DIRECTORY_NUMBERS_LIST")
				{
					 $Line = <FILEHANDLE> ;
					 if ( ($StartTag, $Value, $EndTag) = ($Line =~ /$TagValueValidator/) )
					 {
						if ($StartTag =~ /$PhoneNumberFields/)
		                                {
                		                        $Value = UNFConversion $Value ;
							$PhoneNumberDetails[$Size][$PHONE_NO_INDEX] = $Value ;
                                		}
						elsif ($StartTag =~ /DIRNUM_STATUS/)
						{
							$Value = substr($Value, 0, 1) ;
							$PhoneNumberDetails[$Size][$DIR_NO_STATUS_INDEX] = $Value ;
						}
						elsif ($StartTag =~ /MAIN_DIRNUM_FLAG/)
						{
							$PhoneNumberDetails[$Size][$DIR_NO_FLAG_INDEX] = $Value ;
						}
						elsif ($StartTag =~ /LOWER_EXT/)
						{
							if ($Value =~ /^[\d+]?\d+$/ && $Value =~/^[^\+]/ )
							{
								$PhoneNumberDetails[$Size][$LOWER_EXT_INDEX] = $Value ;
							}
							else
							{
								$PhoneNumberDetails[$Size][$LOWER_EXT_INDEX] = -1 ;
							}
				
						}
						elsif ($StartTag =~ /UPPER_EXT/)
						{
							if ($Value =~ /^[\d+]?\d+$/ && $Value =~/^[^\+]/ )
							{
								$PhoneNumberDetails[$Size][$UPPER_EXT_INDEX] = $Value ;
							}
							else
							{
								$PhoneNumberDetails[$Size][$UPPER_EXT_INDEX] = -1 ;
							}
						}
					  }
					 elsif (($SingleTag) = ($Line =~ /$SingleTagValueValidator/))
					 {
						if ($SingleTag =~ /LOWER_EXT\//)
						{
							$PhoneNumberDetails[$Size][$LOWER_EXT_INDEX] = -999 ;
						}
						elsif ($SingleTag =~ /UPPER_EXT\//)
						{
							$PhoneNumberDetails[$Size][$UPPER_EXT_INDEX] = -999 ;
						}
						
				         }
				}
			}
		}
		
	}

	if ($SubscriberFieldConfig{"CONTRACT_ID_PUB"} eq "Unknown")
	{
		return 1 ;
	}

	my $ServiceString = "" ;
	foreach my $Fil (@ServiceOrder)
	{
		$ServiceConfig{$Fil} =~ s/\bactive\b/0/i ;
		$ServiceConfig{$Fil} =~ s/\bdeactive\b/1/i ;
		$ServiceConfig{$Fil} =~ s/\bInactive\b/1/i ;
		$ServiceConfig{$Fil} =~ s/\bSuspended\b/1/i ;
		$ServiceConfig{$Fil} =~ s/\bOn-hold\b/1/i ;
		$ServiceString = $ServiceString . $Fil . " " . $ServiceConfig{$Fil} . " " ;
	}
	chop ($ServiceString) ;
	$SubscriberFieldConfig{"SERVICE"} = $ServiceString ;

	if ($SubscriberFieldConfig{"DOCUMENT_TYPE"} =~ /passport/i)
	{
		$SubscriberFieldConfig{"PASSPORT_NUMBER"} = $SubscriberFieldConfig{"DOCUMENT_NUMBER"} ;
	}
	elsif ($SubscriberFieldConfig{"DOCUMENT_TYPE"} =~ /[company regestration no\.|license no\.]/)
	{
		$SubscriberFieldConfig{"COMPANY_REGESTRATION_NUMBER"} = $SubscriberFieldConfig{"DOCUMENT_NUMBER"} ;
	}
	
	ProcessFields ;

	close (FILEHANDLE) ;
	open (FILEHANDLE, ">$OutputDir/$File") ;
	my $Padded ;
	my $PadLength ;
	
	for(my $Iter = 0; $Iter <= $Size ; $Iter++ )
	{
		$FinalSubscriberFieldValues[$Phone_Number_Position -1 ]   = $PhoneNumberDetails[$Iter][$PHONE_NO_INDEX];
		$FinalSubscriberFieldValues[$Dir_Num_Status_Position -1 ] = $PhoneNumberDetails[$Iter][$DIR_NO_STATUS_INDEX];
		$FinalSubscriberFieldValues[$Dir_Num_Flag_Position -1 ]   = $PhoneNumberDetails[$Iter][$DIR_NO_FLAG_INDEX];
	
	if ( $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] != -999 && $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX] != -999 &&
	     $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] != -1 && $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX] != -1 && 
	     $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] <= $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX])
	{
		$PadLength = length $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] ;
		for (my $Iter2 = $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] ;$Iter2 <= $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX] ;$Iter2++)
		  {     
			$Padded = sprintf("%0${PadLength}d", $Iter2);
			$FinalSubscriberFieldValues[$Phone_Number_Position -1 ]   = $PhoneNumberDetails[$Iter][$PHONE_NO_INDEX].$Padded;
			WriteOutput ;
			$AcceptedRecords++;
	  	  }
	}		
	elsif ( $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] == -999 && $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX] == -999 )
	{
		WriteOutput ;	
		$AcceptedRecords++;
	}
	else
	{	
		if ( $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] == -1 )
		{
			$RejectedReason = $RejectedReason . "Invalid Lower Extension for PhoneNumber :$FinalSubscriberFieldValues[$Phone_Number_Position -1 ] " . "\n";
		}
		elsif ( $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX] == -1 )
		{
			$RejectedReason = $RejectedReason . "Invalid Upper Extension for PhoneNumber :$FinalSubscriberFieldValues[$Phone_Number_Position -1 ] " . "\n";
		}
		elsif ( $PhoneNumberDetails[$Iter][$LOWER_EXT_INDEX] > $PhoneNumberDetails[$Iter][$UPPER_EXT_INDEX] )
		{
			$RejectedReason = $RejectedReason . "Lower Extension Value is greater than Upper Extension Value for PhoneNumber :$FinalSubscriberFieldValues[$Phone_Number_Position -1 ] " . "\n";
		}
		$RejectedRecords++ ;
	
	}

	}


	close (FILEHANDLE) ;
	open (FILEHANDLE, ">$OutputDir/success/$File") ;
	close (FILEHANDLE) ;
	$ProcessedRecords = $AcceptedRecords + $RejectedRecords ;
	WriteLog ;
		
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
		opendir (DIRHANDLER, "$DirName/success") or die "Unable To Open Directory '" . $DirName . "] !" ;
		foreach my $File (sort {$a cmp $b} readdir(DIRHANDLER))
		{
			$FileName= $DirName . "/$File" ;
			%SubscriberFieldConfig = %SubscriberFieldConfigBackup ;
			my $TestFile = $DirName . "/$File" ;
			if (-s $TestFile and -f $TestFile and -r $TestFile and -T $TestFile)
			{
				if (ProcessFile ($File) == 0) 
				{
					if ( $AcceptedRecords == 0 )
					{
						unlink "$OutputDir/$File" ;
						unlink "$OutputDir/success/$File" ;
					}
				}
				else
				{
					WriteIgnoredLog ;
				}
				unlink "$DirName/$File" ;
				unlink "$DirName/success/$File" ;
			}
		}
		closedir (DIRHANDLER) ;
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
