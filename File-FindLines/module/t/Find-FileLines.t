#!/usr/bin/perl

use strict;
use warnings;

use Test::More tests => 1;

{
    my $finder = File::FindLines->new(
        {
            input => { code => sub { return shift(@lines); } },
            filter => sub {
                my ($self, $record_obj) = @_;
                return $record_obj->text_like(qr/foob.r/);
            },
        }
    );

    # TEST
    ok ($finder, "Finder was initialized.");
}
