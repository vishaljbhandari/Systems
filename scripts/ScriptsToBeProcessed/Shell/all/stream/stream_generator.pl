#!/bin/perl
use warnings;
use strict;

# TARGET SYSTEM RELATED CONFIGURATIONS

my %CONNECTION = (
        DB_USER => '@DB_USER',
        DB_PASSWORD => '@DB_PASSWORD',
        DB_CONNECTION_STRING => '@DB_CONNECTION_STRING',
        SQL_COMMAND => '@SQL_COMMAND'
);

# STREAM RELATED CONFIGURATIONS

my %STREAM = (
        Id => 9999,
        Name => '@NAME_OF_STREAM',
        Title => '@TITLE_OF_STREAM',
        Table => '@STREAM_TABLE',
        Description => '@DESCRIPTION_OF_STREAM'
);

# SCRIPT RELATED CONFIGURATIONS
#my $RANGER=$ENV{'RANGERHOME'};
my $RANGER='~/';
my $LOGFILE=$RANGER."/LOG/".$STREAM{Name}.".log";
my $TMPFILE=$RANGER."/tmp/".$STREAM{Name}.".log";
my $SCTREAM_INSTALL_FILE=$STREAM{Name}."_install.sql";

##############

open(STFILE, ">", $SCTREAM_INSTALL_FILE) or die "File $SCTREAM_INSTALL_FILE, Open Failed" ;
STFILE->autoflush;
print STFILE "SPOOL ".$STREAM{Name}."_install.log;\n";
print STFILE "SET SERVEROUTPUT ON;\n";
print STFILE "SET FEEDBACK ON;\n";
print STFILE '@sync_sequence.sql;\n';

# sync_sequence_print(sequence_name, table_name);
sub sync_sequence_print()
{
	#print STFILE "exec sync_seq(\'"$_[0]"\',\'"$_[1]"\');\n";
	print STFILE "exec sync_seq(''$_[0]'',''$_[1]'');\n";
}


# Verify Configuration File


close(STFILE);
