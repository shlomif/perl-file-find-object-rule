#!/usr/bin/perl

use strict;
use warnings;

use Test::More tests => 1;

use Stream::Extract;

{
    my @lines = ( "Ini\n", "Mini\n", "Foobar\n", "Moo\n", );

    my $finder = Stream::Extract->new(
        {
            input => {
                code => sub { return shift(@lines); }
            },
            filter => sub {
                my ( $self, $args ) = @_;

                my $record_obj = $args->{record};

                return $record_obj->text_like(qr/foob.r/);
            },
        }
    );

    # TEST
    ok( $finder, "Finder was initialized." );
}
