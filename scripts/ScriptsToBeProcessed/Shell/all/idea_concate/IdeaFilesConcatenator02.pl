#!/usr/bin/perl
use File::Basename ;
use POSIX qw(strftime);
use List::Util qw[min max];
use File::stat ;
use File::Basename ;
use IO::Handle;
sub Initialize
{
$isOutPutFilePrefixSet=0;
        $file = @_[0] ;
        $test_run = @_[1];
        if (!( -f $file ))
        {
                die ( "\nConfiguration File : $file ,does not Exist! . Exiting !!! ");
        }
        open(config,$file) ;
        foreach $line(<config>)
        {
                chomp($line) ;
                if ($line =~ /^INPUTDIR/)
                {
                        $inputdir=(split(/=/,$line))[1] ;
                        if (-d $inputdir)
                        {
                                print LOGFILE "The directory " , $inputdir , " exists \n" ;
                        }
                        else
                        {
                                print LOGFILE "Warning!! The directory " , $inputdir , " does not exist \n" ;
                                exit ;
                        }
                }

                if ($line =~ /^OUTPUTDIR/)
                {
                        $outputdir=(split(/=/,$line))[1] ;
                        if (-d $outputdir)
                        {
                                print LOGFILE "The directory " , $outputdir , " exists \n" ;
                        }
                        else
                        {
                                print LOGFILE "Warning!! The directory " , $outputdir , " does not exist \n" ;
                                exit ;
                        }
                }

                if ($line =~ /^OUTPUT_FILE_SIZE/)
                {
                        $outputfilesize=(split(/=/,$line))[1] ;
                        if ($outputfilesize eq '')
                        {
                                print LOGFILE "The size of the output file size is not configured. Exiting !!!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The size of the output file size is configured to " ,$outputfilesize, "\n" ;
                        }
                }

                if ($line =~ /^WAIT_TIME/)
                {
                        $outputwaittime=(split(/=/,$line))[1] ;
                        if ($outputwaittime eq '')
                        {
                                print LOGFILE "The wait time for concatenating the file is not configured.  Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The wait time for concatenating the file is configured to " ,$outputwaittime, "\n" ;
                        }
                }

                if ($line =~ /^DELIMETER/)
                {
                        $delimeter=(split(/=/,$line))[1] ;
                        if ($delimeter eq '')
                        {
                                print LOGFILE "The delimeter has not been configured . Exiting !!!! \n" ;
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The delimiter has been configured to '" ,$delimeter, "'\n" ;
                        }
                }

                if ($line =~ /^OUTPUT_FILE_PREFIX/)
                {
                        $outputfileprefix=(split(/=/,$line))[1] ;
                        if ($outputfileprefix eq '')
                        {		
								$isOutPutFilePrefixSet=0;
                                print LOGFILE "The output file prefix has not been configured.  \n";
                                
							
                        }
                        else
                        {		 $isOutPutFilePrefixSet=1;
								 print LOGFILE "The output file prefix has been configured to '" ,$outputfileprefix , "'\n" ;
                        }
                }

                if ($line =~ /^FILE_STABILITY_SECS/)
                {
                        $filestability=(split(/=/,$line))[1] ;
                        if ($filestability eq '')
                        {
                                print LOGFILE "The file stability has not been configured . Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The file stability in seconds has been configured to '" ,$filestability , "'\n" ;
                        }
                }

                if ($line =~ /^FOLDER_COUNTS/)
                {
                        $FolderCount=(split(/=/,$line))[1] ;
                        if ($FolderCount eq '')
                        {
                                print LOGFILE "The Folder Count has not been configured . Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The Folder Count in seconds has been configured to '" , $FolderCount , "'\n" ;
                        }
                }

                if ($line =~ /^RECORD_COUNT/)
                {
                        $recordcount=(split(/=/,$line))[1] ;
                        if ($recordcount eq '')
                        {
                                print "The record count has not been configured. Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The record count has been configured to '" ,$recordcount , "'\n" ;
                        }
                }

                if ($line =~ /^RECORD_CONFIG_TYPE/)
                {
                        $recordconfigtype=(split(/=/,$line))[1] ;
                        if ($recordconfigtype eq '')
                        {
                                print "The record count has not been configured. Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The record count has been configured to '" ,$recordconfigtype , "'\n" ;
                		if($recordconfigtype==1){
					$checkPlace = 21;	# 22
				}elsif($recordconfigtype==3){
					$checkPlace = 12;	# 13
				}else{
					$checkPlace = 2;	# 3	
				} 
				print LOGFILE "Phone number will be at '" , $checkPlace , "'\n" ;
		       }
                }

        }
	$ConcatenationPrefix="consolidated_file_" . $outputfileprefix ;
	$SeqNum = 0 ;
                LOGFILE->autoflush;
}

sub SetOutputFileName {

        $currentTime = strftime("%Y%m%d%H%M%S", localtime);
	$ConsolidatedFileName = $ConcatenationPrefix . "_" . $currentTime  ;
	if ($PreviousFileName ne $ConsolidatedFileName )        {
                $PreviousFileName = $ConsolidatedFileName ;
        	for($loopcount=0; $loopcount < $FolderCount ; $loopcount++ ){
			$pre=sprintf("%03d", $loopcount);
			$ConsolidatedFile= $ConsolidatedFileName ;
       			open(BEG_PREFIX,  ">" , $ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFileName ) or die " BEG_PREFIX :: Cannot open the file  $ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFileName  $! ";
       			print BEG_PREFIX "BEG\n" ;
       			close(BEG_PREFIX);
		}
       		$SeqNum=0 ;
                $consolidateFileStartTime = time() ;
                $recordsWrittenInCurrentFile = 0 ;
        }
        else {
                $ConsolidatedFileName=$ConsolidatedFileName . "_" . $SeqNum ;
                $SeqNum = $SeqNum + 1 ;
                for($loopcount=0; $loopcount < $FolderCount ; $loopcount++ ){
 	               $pre=sprintf("%03d", $loopcount);
	        	$ConsolidatedFile= $ConsolidatedFileName ;
			open(BEG_PREFIX,  ">" , $ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFile ) or die " BEG_PREFIX :: Cannot open the file  $ENV{'RANGERHOME'}".'/tmp/'."$pre.$ConsolidatedFile  $! ";
                	print BEG_PREFIX "BEG\n" ;
                	close(BEG_PREFIX);
		}
                $consolidateFileStartTime = time() ;
                $recordsWrittenInCurrentFile = 0 ;
        }
}
sub AppendDataToFile {
        my ($fileName) = @_ ;
        open (FILE, $fileName) ;
	$RecordsInCurrentFile = 0 ;
        while (<FILE>)
        {               chomp($_);
                                if (($_=~ /^BEG/) ||  ($_=~ /^END/))
                                {
                                        next;
                                }
			 my @values = split(',', $_);
			 my $rem = $values[$checkPlace];
		         $rem = $rem % $FolderCount;
			 $pre=sprintf("%03d", $rem);
			 open(OUTPUTFILEPREFIX,  ">>" , $ENV{'RANGERHOME'}."/tmp/".$pre."".$ConsolidatedFile ) or die "Cannot open the file  $ENV{'RANGERHOME'}.'/tmp/'.$pre"."$ConsolidatedFile  $! ";
     			 print OUTPUTFILEPREFIX  $_."\n";
                         
		 $recordsWrittenInCurrentFile += 1 ;
		 $RecordsInCurrentFile++;
        }
        close (OUTPUTFILEPREFIX) ;
       unlink ($fileName) ;
	   return $RecordsInCurrentFile ;
}


sub ConsolidateFiles {
        opendir(DIR, $inputdir) ;
        @OldFiles = (sort {-M $b <=> -M $a} <$inputdir/*>) ;
        closedir(DIR) ;
        $curtime = time() ;
        $EndTimeConsolidatedFile =  strftime("%Y/%m/%d %T", localtime(time())) ;
        if (($#OldFiles == -1) && ((time() - $consolidateFileStartTime )>$outputwaittime ))
        {

                if ($recordsWrittenInCurrentFile > 0)
			{
             		for($loopcount=0; $loopcount < $FolderCount ; $loopcount++ ){
             			$pre=sprintf("%03d", $loopcount);
				open(END_PREFIX,  ">>" , $ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFile ) or die "END_PREFIX :: Cannot open the file  $ConsolidatedFile  $! ";
				print END_PREFIX "END" ;
				close(END_PREFIX);
				rename($ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFile, $outputdir."/".$pre."/".$ConsolidatedFileName)  or die "Cannot open $outputdir $!";
				print LOGFILE "File  $pre.$ConsolidatedFileName  moved to $outputdir / $pre  with $recordsWrittenInCurrentFile ended at $EndTimeConsolidatedFile \n " ;
			}
			LOGFILE->autoflush;
			SetOutputFileName () ;

			}
        }


        foreach $file(@OldFiles)
        {
                if( -f $file)
                {
                        $filemtime = stat ($file)->mtime ;
                        if (($curtime - $filemtime) > $filestability)
                        {
                                my $timeSinceFileOpened = (time() - $consolidateFileStartTime) ;
                                         if (($recordsWrittenInCurrentFile == 0) ||
                                        ($recordsWrittenInCurrentFile > $recordcount ) ||
                                        ($timeSinceFileOpened > $outputwaittime))
                                {
                                        if ($recordsWrittenInCurrentFile > 0)
                                        {
                                           for($loopcount=0; $loopcount < $FolderCount ; $loopcount++ ){
 		                               	$pre=sprintf("%03d", $loopcount);

                                           	open(END_PREFIX,  ">>" , $ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFile ) or die "END_PREFIX :: Cannot open the file $pre$ConsolidatedFile  $! ";
                                           	print END_PREFIX "END" ;
                                           	close(END_PREFIX);
                                           	rename($ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFile, $outputdir."/".$pre."/".$ConsolidatedFileName)  or die "Cannot open $outputdir $!";
                                           	print LOGFILE "File $pre$ConsolidatedFileName moved to $outputdir with $recordsWrittenInCurrentFile ended at $EndTimeConsolidatedFile\n" ;
                                            }
                                        }
                                          SetOutputFileName () ;
                                       print LOGFILE "Consolidating Data to file : $ConsolidatedFile - Started at [" . strftime("%Y/%m/%d %T", localtime(time())) . "]\n " ;
                        		}
                              for($loopcount=0; $loopcount < $FolderCount ; $loopcount++ ){
                               $pre=sprintf("%03d", $loopcount);		
                                $consolidatedFileSize = -s $pre.$ConsolidatedFile ;
                                if ($consolidatedFileSize >= $outputfilesize )
                                {
                                        if ($recordsWrittenInCurrentFile > 0)
                                        {
                                                rename($ENV{'RANGERHOME'}."/tmp/".$pre.$ConsolidatedFile, $outputdir."/".$pre."/".$ConsolidatedFileName)  or die "Cannot open $outputdir $!";
                                                print LOGFILE "File $pre$ConsolidatedFileName moved to $outputdir with $recordsWrittenInCurrentFile \n" ;
                                        }
                                        SetOutputFileName () ;
                                        print LOGFILE "Consolidating Data to file : $pre$ConsolidatedFile \n " ;
                                }
			      }
                                $fileaccesstime = strftime("%Y%m%d%H%M%S", localtime($filemtime));
                                $filenameonly = fileparse($file);
                                chomp($filenameonly);
	                              my $Cnt = AppendDataToFile($file);
								print "Count for $file [$Cnt]";                            
							print LOGFILE "Consolidated --- [$filenameonly|$ConsolidatedFileName] - With Records = [$Cnt], Record Count :$recordsWrittenInCurrentFile \n " if ($Cnt > 1);

						LOGFILE->autoflush; 
						}
                }
        }
}

sub main
{
        $Today = strftime("%Y%m%d", localtime) ;#FORMAT : 20131004 <yyyymmdd>
        my $logfile = $ENV{'RANGERHOME'}."/LOG/Ideafilesconcatenator".$Today.'.log' ;   #PER DAY LOG FILE
        print "\nLOGFILE = ".$logfile."\n" ;
                open(LOGFILE, ">>",$ENV{'RANGERHOME'}."/LOG/Ideafilesconcatenator".$Today.'.log') or die "log opening faile $!" ;
        $StartTime = `date "+%D %H:%M:%S"`;
        print LOGFILE "################################################################################## \n";
        print LOGFILE "Opening log file $logfile \n " ;
        print LOGFILE "Start Time       :: $StartTime \n";
                LOGFILE->autoflush;
                Initialize (@ARGV) ;
       if($isOutPutFilePrefixSet eq  0)
		{
			print "isOutPutFilePrefixSet : $isOutPutFilePrefixSet";
			print LOGFILE "OUTPUT_FILE_PREFIX not Set\n";
		}
		 my $recordsWrittenInCurrentFile = 0 ;
        if ($test_run eq "999")
        {
                                    ConsolidateFiles ;
        }
        else
        {
                while (1)
                {
                        ConsolidateFiles ;
                                                sleep 10 ;
                }
        }
        $ENDTime = `date "+%D %H:%M:%S"`;
        print LOGFILE "\nEnd Time         :: $ENDTime\n";
        print LOGFILE "################################################################################## \n";
        close(LOGFILE);
}
main();
