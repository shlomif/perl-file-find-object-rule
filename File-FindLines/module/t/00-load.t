#!perl -T

use Test::More tests => 4;

BEGIN {
    use_ok( 'Stream::Extract' ) || print "Bail out!\n";
    use_ok( 'Stream::Extract::Result' ) || print "Bail out!\n";
    use_ok( 'Stream::Extract::Result::Context' ) || print "Bail out!\n";
    use_ok( 'Stream::Extract::Result::Match' ) || print "Bail out!\n";
}

diag( "Testing Stream::Extract $Stream::Extract::VERSION, Perl $], $^X" );
