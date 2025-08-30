#!/usr/bin/perl
# RandomQuote.pl by Bill Weinman <http://bw.org/contact/>
# Copyright (c) 2010 The BearHeart Group, LLC
#

use strict;
use warnings;
use IO::File;

my $quotes = [];    # container for quotes
my $cgi = '';
my $header_sent = 0;

_init();
main(@ARGV);

sub main
{
    my ($filename, $dispQuan) = @_;
    $filename = "quotes.txt" unless $filename;
    $dispQuan = 1 unless $dispQuan;

    readQuotes($filename);
    while($dispQuan--) {
        displayQuote(getQuote());
    }
}

sub _init
{
    if($ENV{GATEWAY_INTERFACE}) {
        $cgi = 1;
        @ARGV = split(/:/, $ARGV[0]);
    }
}

sub readQuotes
{
    my $filename = shift or error("readQuotes: missing filename");
    my $fh = IO::File->new($filename, 'r') or error("Cannot open $filename ($!)");

    my $token = '';
    my $quote = {};
    while( my $line = $fh->getline() ) {
        if ( my $newtok = getToken($line) ) {
            $token = $newtok;
            if ($token eq 'quote') {
                $quote = newQuote();
            }
        } elsif($token) {
            chomp $line;
            $quote->{$token} .= "\n" if $quote->{$token};   # newline only in between
            $quote->{$token} .= $line;
        } else {
            error("widowed line (no token) in $filename: $line");
        }
    }
}

sub getQuote
{
    my $count = @$quotes;
    message("No quotes.") unless $count;

    my $qn = int(rand($count));
    return splice(@$quotes, $qn, 1);
}

sub displayQuote
{
    my $quote = shift or return;
    return unless $quote->{quote};

    if($cgi) {
	    message(qq{<p class="quote">$quote->{quote}</p>});
	    message(qq{<p class="byline">&mdash;$quote->{byline}</p>}) if $quote->{byline};
    } else {
        message(qq{$quote->{quote}});
        message(qq{--$quote->{byline}}) if $quote->{byline};
    }
}

sub getToken
{
    my $line = shift or return undef;
    if( $line =~ /^\$(.*)\$$/m ) {
        return $1;
    } else {
        return undef;
    } 
}

sub newQuote
{
    push( @$quotes, {} );   # empty hash for new new quote
    return $quotes->[-1];   # use this entry for our current quote 
    
}

sub header
{
    return if $header_sent;
    print("content-type: text/html\n\n");
    $header_sent = 1;
}

sub message
{
    my $m = shift or return;
    header() if $cgi;
    print("$m\n");
}

sub error
{
    my $e = shift || 'unkown error';
    my $me = $0;
    $me = ( split( m|[\\\/]|, $me ) )[-1];
    print STDERR ("$me: $e\n");
    exit 0;
}
