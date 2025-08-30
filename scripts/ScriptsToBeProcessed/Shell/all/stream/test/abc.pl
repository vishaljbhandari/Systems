#!/usr/bin/perl


sub sync_sequence_print()
{
	print 'exec sync_seq('@_[0]','@_[1]');\n';
};

sync_sequence_print('hello','hiii');
