#!/usr/bin/perl
# db.cgi by Bill Weinman <http://bw.org/contact/>
# Copyright (c) 1995-2010 The BearHeart Group, LLC
# version of 3/2010 for Perl 5 Essential Training
#   at lynda.com
#
use strict;
use warnings;

use BW::Constants;
use BW::Common;
use BW::CGI;
use BW::Jumptable;
use BW::Config;
use BW::Include;
use BW::DB;
use BW::XML::HTML;

my $VERSION = '0.2.1';

my $g = {
    signon     => 'db.cgi version ' . $VERSION,
    copyright  => '&copy; 1995-2010 BHG LLC',
    configFile => 'testimonials/db.conf',
    tableName  => 'testimonial',
    jumptable => {
        add       => \&add,
        update    => \&update,
        delete_do => \&delete_do,
        edit_del  => \&edit_del,
        main      => \&mainPage
    }
};

init();

sub main
{
    mainPage();
    exit 0;
}

sub init
{
    $g->{cgi} = BW::CGI->new;
    checkerror( $g->{cgi} );
    $g->{vars} = $g->{cgi}->vars();

    $g->{config} = BW::Config->new( filename => $g->{configFile} );
    checkerror( $g->{config} );
    $g->{configValues} = $g->{config}->values;

    $g->{bwInclude} = BW::Include->new;
    checkerror( $g->{bwInclude} );

    $g->{html} = BW::XML::HTML->new;
    checkerror( $g->{bwInclude} );

    initDB();
    initVars();

    $g->{action} = $g->{cgi}->qv('a') or main();
    jump();
}

#
# pages
#

# print a page with header and footer
sub page
{
    my $page    = shift || 'main';
    my $i       = $g->{bwInclude};
    my $q       = $g->{configValues};
    my $htmlDir = $q->{htmlDir};

    $htmlDir .= '/' unless substr( $htmlDir, -1 ) eq '/';
    $page .= '.html' unless $page =~ /\./;

    my $headerFile = $htmlDir . $q->{header};
    my $footerFile = $htmlDir . $q->{footer};
    my $pageFile   = $htmlDir . $page;

    getHiddens();
    getMessages();
    getErrorMessages();

    # read the files
    my $h = $i->spf($headerFile) or error("page: cannot open $headerFile");
    my $f = $i->spf($footerFile) or error("page: cannot open $footerFile");
    my $p = $i->spf($pageFile) or error("page: cannot open $pageFile");

    # concatenate and print
    p( $h . $p . $f );
    exit 0;
}

#
# actions
#

sub mainPage
{
    hidden( 'a', 'add' );
    listRecs();
    var('pageTitle', 'Enter a new testimonial');
    page('main');
}

sub add
{
    my $rec = {};
    my $q   = $g->{vars};
    my $db  = $g->{db};

    $rec->{testimonial} = $q->{testimonial};
    $rec->{byline}      = $q->{byline};

    addrec($rec);
    checkerror($db);
    message("Testimonial ($rec->{byline}) added to database");
    mainPage();
}

sub update
{
    my $rec = {};
    my $q   = $g->{vars};
    my $db  = $g->{db};

    if($q->{cancel}) {
        message("Edit cancelled");
        mainPage();
    }

    $rec->{testimonial} = $q->{testimonial};
    $rec->{byline}      = $q->{byline};
    $rec->{id}          = $q->{id};

    updaterec($rec);
    checkerror($db);
    message("Testimonial ($rec->{byline}) updated in database");
    mainPage();
}

sub edit_del
{
    my $q = $g->{vars};
    if($q->{edit}) {
        edit();
    } elsif($q->{delete}) {
        delete_confirm();
    } else {
        error("edit_del: unknown submit button");
    }
}

sub edit
{
    my $q = $g->{vars};
    my $db = $g->{db};

    my $rec = getrec($q->{id});
    var('testimonial', $rec->{testimonial});
    var('byline', $rec->{byline});
    var('pageTitle', 'Edit this testimonial');

    hidden('a', 'update');
    hidden('id', $q->{id});

    page('edit');
}

sub delete_confirm
{
    my $q = $g->{vars};
    my $db = $g->{db};

    my $rec = getrec($q->{id});
    var('testimonial', $rec->{testimonial});
    var('byline', $rec->{byline});
    var('pageTitle', 'Delete this testimonial?');

    hidden('a', 'delete_do');
    hidden('id', $q->{id});

    page('delconfirm');
}

sub delete_do
{
    my $q = $g->{vars};
    my $id = $q->{id};

    if($q->{cancel}) {
        message("Delete cancelled");
        mainPage();
    }

    deleterec($id);
    message("Testimonial deleted");
    mainPage();
}

#
# page support
#

sub listRecs
{
    my $db           = $g->{db};
    my $conf         = $g->{configValues};
    my $q            = $g->{vars};
    my $sql_limit    = $conf->{sql_limit} || 25;
    my $page_results = FALSE;
    my $offset       = 0;
    my $pagehead     = '';
    my $tableName    = $g->{tableName};

    my $htmlDir = $conf->{htmlDir};
    $htmlDir .= '/' unless substr( $htmlDir, -1 ) eq '/';
    my $reclineFn = $htmlDir . $conf->{recline};
    my $recline   = BW::Common::readfile($reclineFn);
    error("cannot read $reclineFn ($!)") unless $recline;

    my $count = $db->sql_select_value("SELECT COUNT(*) FROM $tableName");
    checkerror($db);

    if ($count) {
        message( "There are " . BW::Common::comma($count) . " records in the database. Add some more!" );
    } else {
        message("There are no records in the database. Add some!");
    }

    # paging logic
    if ( $count > $sql_limit ) {
        $page_results = TRUE;
        my $pageno = $q->{pageno} || 0;              # pageno is zero-based
        my $numpages = int( $count / $sql_limit );
        $numpages++ if $count % $sql_limit;

        if ( defined $q->{nextpage} ) {
            $pageno++ if $pageno < ( $numpages - 1 );
        } elsif ( defined $q->{prevpage} ) {
            $pageno-- if $pageno > 0;
        } elsif ( defined $q->{pagejump} ) {
            my $p = $q->{pagejump};
            $pageno = $p if ( $p > 0 and $p < $numpages );
        }

        $pagehead = pagehead( $pageno, $numpages );
        $offset = $pageno * $sql_limit;
    }

    my $recs = $db->sql_select("
        SELECT id, SUBSTR(testimonial, 1, 128) as testimonial, byline
            FROM $tableName
            ORDER BY byline
            LIMIT $offset, $sql_limit
    ");
    checkerror($db);

    my $a = '';
    foreach my $rec (@$recs) {
        var( 'id',          $rec->{id} );
        var( 'byline',      $rec->{byline} );
        var( 'testimonial', $rec->{testimonial} );

        $a .= $g->{bwInclude}->sps($recline);
    }

    # add the paging header, top and bottom
    if( $page_results ) {
        $a = $pagehead . $a . $pagehead;
    }

    var( 'id',          '' );
    var( 'byline',      '' );
    var( 'testimonial', '' );
    var( 'CONTENT', $a );
}

sub pagehead
{
    my ( $pageno, $numpages ) = @_;
    my $html     = $g->{html};
    my $linkback = $g->{cgi}->linkback;

    my $prevlink = qq(<span class="n">&lt;&lt;</span>);
    my $nextlink = qq(<span class="n">&gt;&gt;</span>);

    if($pageno > 0) {
        $prevlink = $html->link( $linkback . "?pageno=$pageno&prevpage=1", "&lt;&lt;" );
    }

    if ( $pageno < ( $numpages - 1 ) ) {
        $nextlink = $html->link( $linkback . "?pageno=$pageno&nextpage=1", "&gt;&gt;" );
    }

    my @plinks = ();
    foreach my $n ( 1 .. $numpages ) {
        my $p = $n - 1;
        if($p == $pageno) {
            push( @plinks, qq(<span class="n">$n</span>) );
        } else {
            push( @plinks, $html->link( $linkback . "?pagejump=$p", $n ) );
        }
    }
    my $pagebar = join('', @plinks);

    var( 'prevlink', $prevlink );
    var( 'nextlink', $nextlink );
    var( 'pagebar', $pagebar );
    return $g->{bwInclude}->sps( getHTMLFile('nextprev') );
}

#
# db routines
#

sub addrec
{
    my $rec = shift or error("add: missing rec");
    my $db = $g->{db};
    my $cgi = $g->{cgi};
    my $tableName = $g->{tableName};

    $rec->{testimonial} = $cgi->html_encode($rec->{testimonial}) if $rec->{testimonial};
    $rec->{byline} = $cgi->html_encode($rec->{byline}) if $rec->{testimonial};

    $db->insert( $g->{tableName}, $rec );
    checkerror($db);
}

# getrec ( id ) returns hashref
sub getrec
{
    my $id = shift or error("getrec: missing id");
    my $db = $g->{db};
    my $tableName = $g->{tableName};
    my $rc = $db->sql_select("SELECT * FROM $tableName WHERE id = ?", $id);
    checkerror();
    error("getrec: testimonial id $id not found") unless $rc and scalar @$rc;
    return $rc->[0];
}

# updaterec ( rec ) updates rec in database
sub updaterec
{
    my $rec = shift or error("updaterec: missing rec");
    my $id = $rec->{id} or error("updaterec: missing rec->id");
    my $db = $g->{db};
    my $cgi = $g->{cgi};
    my $tableName = $g->{tableName};

    $rec->{testimonial} = $cgi->html_encode($rec->{testimonial}) if $rec->{testimonial};
    $rec->{byline} = $cgi->html_encode($rec->{byline}) if $rec->{testimonial};

    $db->sql_do("
        UPDATE $tableName
            SET testimonial = ?, byline = ?
        WHERE id = ?
    ", $rec->{testimonial}, $rec->{byline}, $id) or
        error("updaterec: could not update record $id");
    checkerror();
}

# deleterec ( id ) deletes rec from database
sub deleterec
{
    my $id = shift or error("deleterec: missing id");
    my $db = $g->{db};
    my $tableName = $g->{tableName};

    $db->sql_do("DELETE FROM $tableName WHERE id = ?", $id) or
        error("deleterec: could not delete record $id");
    checkerror();
}

#
# utilities
#

sub initDB
{
    my $tableName = $g->{tableName};
    my $dbfile = $g->{configValues}{db};

    if( -e $dbfile ) {
        error("database file ($dbfile) is not writable") unless -w $dbfile;
    }

    my $db = BW::DB->new( dbengine => 'SQLite', dbname => $dbfile );
    checkerror( $db );

    # set the global varaible
    $g->{db} = $db;

    unless( $db->table_exists($tableName) ) {
        my $rc = $db->sql_do(qq{
            CREATE TABLE $tableName (
                id integer PRIMARY KEY,
                testimonial TEXT,
                byline varchar(255)
            );
        });
        checkerror($db);
    }
}

sub initVars
{
    var( 'SELF', $g->{cgi}->linkback );
    var( 'CONTENT', '' );
    var( 'buttons', '' );
    var( 'hiddens', '' );
    var( 'testimonial', '' );
    var( 'byline', '' );
}

sub hidden
{
    my ( $k, $v ) = @_;
    push @{$g->{hiddens}}, { name => $k, value => $v };
}

sub getHTMLFile
{
    my $f = shift or error("getFileName: no filename");
    my $q = $g->{configValues};
    my $fn = $g->{configValues}{$f} or error("$f not found in config file");

    my $htmlDir = $q->{htmlDir};
    $htmlDir .= '/' unless substr( $htmlDir, -1 ) eq '/';
    my $rc = BW::Common::readfile( $htmlDir . $fn );
    error("getHTMLFile: cannot read $fn ($!)") unless $rc;
    return $rc;
}

sub getHiddens
{
    my $a = '';
    my $html = $g->{html};
    my $i = $g->{bwInclude};
    foreach my $h ( @{$g->{hiddens}} ) {
        $a .= $html->hidden( $h->{name}, $h->{value} ) . "\n";
    }
    $i->var( 'hiddens', $a );
}

sub getMessages
{
    my $i = $g->{bwInclude};

    if ( $g->{messages} ) {
        $i->var( 'MESSAGES', join( '', map { qq[<p class="message">$_</p>\n] } @{ $g->{messages} } ) );
        return scalar @{ $g->{messages} };
    } else {
        $i->var( 'MESSAGES', '' );
        return 0;
    }
}

sub getErrorMessages
{
    my $i = $g->{bwInclude};

    if ( $g->{errorMessages} ) {
        $i->var( 'ERRORS', join( '', map { qq[<p class="error">$_</p>\n] } @{ $g->{errorMessages} } ) );
        return scalar @{ $g->{errorMessages} };
    } else {
        $i->var( 'ERRORS', '' );
        return 0;
    }
}

sub jump
{
    my $jt = BW::Jumptable->new( jumptable => $g->{jumptable} ) or
        error("cannot initialize jump table");
    $jt->jump( $g->{action} );
    checkerror($jt);
}

sub p
{
    $g->{cgi}->p(@_);
    $g->{have_headers} = TRUE;
}

# shortcut for bwInclude->var
sub var
{
    return $g->{bwInclude}->var( @_ );
}

sub message
{
    my $m = shift or return;
    push( @{$g->{messages}}, $m );
}

sub errorMessage
{
    my $m = shift or return;
    push( @{$g->{errorMessages}}, $m );
    # debug($m) unless $g->{have_headers};
    error($m) unless $g->{have_headers};
}

sub debug
{
    $g->{cgi}->debug(@_);
}

sub error
{
    my $m = shift || 'no message';
    p(qq{ <p style="color:red"> $m </p> });
    exit;
}

sub checkerror
{
    my $o = shift     or return;
    my $e = $o->error or return;
    errorMessage($e);
}

