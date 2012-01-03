use 5.008;

use strict;
use warnings;

use lib "./inc";

use Test::Run::Builder;

my $builder = Test::Run::Builder->new(
    module_name         => 'File::FindLines',
    license             => 'mit',
    dist_author         => q{Shlomi Fish <shlomif@cpan.org>},
    dist_version_from   => 'lib/File/FindLines.pm',
    build_requires => {
        'Test::More' => 0,
    },
    requires => {
        'perl' => 5.006,
    },
    add_to_cleanup      => [ 'File-FindLines-*' ],
    create_makefile_pl => 'traditional',
);

$builder->create_build_script();
