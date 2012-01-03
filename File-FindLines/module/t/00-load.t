#!perl -T

use Test::More tests => 4;

BEGIN {
    use_ok( 'File::FindLines' ) || print "Bail out!\n";
    use_ok( 'File::FindLines::Result' ) || print "Bail out!\n";
    use_ok( 'File::FindLines::Result::Context' ) || print "Bail out!\n";
    use_ok( 'File::FindLines::Result::Record' ) || print "Bail out!\n";
}

diag( "Testing File::FindLines $File::FindLines::VERSION, Perl $], $^X" );
