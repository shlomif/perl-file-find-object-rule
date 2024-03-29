#!/usr/bin/perl

use strict;
use warnings;

use Test::More tests => 1;

use File::TreeCreate ();

use File::Path qw( mkpath rmtree );

{
    my $tree = {
        'name' => "traverse-1/",
        'subs' => [
            {
                'name'     => "b.doc",
                'contents' => "This file was spotted in the wild.",
            },
            {
                'name' => "a/",
            },
            {
                'name' => "foo/",
                'subs' => [
                    {
                        'name' => "yet/",
                    },
                ],
            },
        ],
    };

    my $t = File::TreeCreate->new();
    mkpath("./t/sample-data");
    $t->create_tree( "./t/sample-data/", $tree );

    open my $lff_fh,
        "./minifind " . $t->get_path("./t/sample-data/traverse-1") . "|"
        or die "Cannot execute minifind";

    my @results = <$lff_fh>;
    chomp(@results);

    close($lff_fh);

    # TEST
    is_deeply(
        \@results,
        [
            (
                map { $t->get_path("t/sample-data/traverse-1/$_") } (
                    "", qw(
                        a
                        b.doc
                        foo
                        foo/yet
                    )
                )
            ),
        ],
        "Checking for regular, lexicographically sorted order",
    );

    rmtree( $t->get_path("./t/sample-data/traverse-1") );
}
