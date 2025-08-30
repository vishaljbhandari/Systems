#!/usr/bin/perl
# testimonials.pl by Bill Weinman <http://bw.org/contact/>
# Copyright (c) 2009 The BearHeart Group, LLC
# created 2009-12-24
#
use strict;
use warnings;

use BW::Constants;
use BW::Common;
use BW::DB;
use BW::Config;

my $g = {
    configFile => '/home/billw/web/perl.bw.org/examples/twotrees/testimonials/db.conf',
};

main(@ARGV);

sub main
{
    init();
    testimonials(shift);
}

sub testimonials
{
    my $n = shift || 1;
    my $db = $g->{db};

    # get a list of ids
    my $ids = $db->sql_select_column('SELECT id FROM testimonial');
    checkerror($db);
    my $count = scalar @$ids;

    if ( $n > ( int( $count / 4 ) ) ) {
        error(
            "There are $count records in the database. For good randomness, you cannot display more than " .
            int( $count / 4 ) .
            " at a time"
        );
    }

    # select n random record ids
    my @dispIds;
    while( $n-- ) {
        my $id = $ids->[int(rand(@$ids))];
        while(grep { $id == $_ } @dispIds) {    # try again if we already have that one
            $id = $ids->[int(rand(@$ids))];
        }
        push( @dispIds, $id );
    }

    # display the records
    foreach my $id ( @dispIds ) {
        my $rec = getrec($id);
        checkerror();
        error("record $id not found.") unless $rec;
        dispRec($rec);
    }

}

sub dispRec
{
    my $rec = shift or error("dispRec: missing rec");
    my $testimonial = $rec->{testimonial} || '';
    my $byline = $rec->{byline} || '';
    p(qq{<div class="testimonial">\n});
    p(qq{<p class="testimonial">$testimonial</p>\n});
    p(qq{<p class="byline">&mdash;$byline</p>\n});
    p(qq{</div>\n});
}

# getrec ( id ) returns hashref
sub getrec
{
    my $id = shift or error("getrec: missing id");
    my $db = $g->{db};
    my $rc = $db->sql_select('SELECT * FROM testimonial WHERE id = ?', $id);
    checkerror();
    error("getrec: testimonial id $id not found") unless $rc and scalar @$rc;
    return $rc->[0];
}

sub init
{
    $g->{config} = BW::Config->new( filename => $g->{configFile} );
    checkerror( $g->{config} );
    $g->{configValues} = $g->{config}->values;

    $g->{me} = BW::Common::basename($0);

    error("missing db config value") unless $g->{configValues}{db};
    $g->{db} = BW::DB->new( dbengine => 'SQLite', dbname => $g->{configValues}{db} );
    checkerror( $g->{db} );
}

sub p
{
    print "content-type: text/html\n\n" unless $g->{headerFlag};
    $g->{headerFlag} = TRUE;
    print @_;
}

sub message
{
    my $m = shift or return;
    p "$m\n";
}

sub error
{
    my $e = shift || 'unkown error';
    p "$g->{me} error: $e\n";
    exit 0;
}

sub checkerror
{
    my $o = shift     or return;
    my $e = $o->error or return;
    error($e);
}

