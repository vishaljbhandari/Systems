use strict;
use warnings;

my $source = shift or &usage();
my $lines_per_file = shift or &usage();

open (my $FH, "<$source") or die "Could not open source file. $!";
open (my $OUT, ">00000000.log") or die "Could not open destination file. $!";

my $i = 0;

while (<$FH>) {
	print $OUT $_;
	$i++;

	if ($i % $lines_per_file == 0) {
		close($OUT);
		my $FHNEW = sprintf("%08d", $i);
		open ($OUT, ">${FHNEW}.log") or die "Could not open destination file. $!";
	}
}

sub usage() {
print <<EOF;

	PROGRAM NAME: Partition File
	
	DESCRIPTION:
	Takes a file and creates many small files out of the large file.
	
	EXAMPLE USAGE: partition_file.pl log.txt 1000
	
	PARAMETERS:
	1. Source File: File name of the source file to partition.
	2. Maximum number of lines per file: The number of lines per file.

EOF
exit;
}
