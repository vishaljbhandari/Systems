#!/usr/bin/perl
# main.pl
use warnings;
use strict;

require "functions.pl";

our ($glbA, $glbB);

$glbA = "Main_GlobalA";
$glbB = "Main_GlobalB";

procedureA();
procedureB();

