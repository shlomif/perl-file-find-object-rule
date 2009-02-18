# This file is under the MIT X11 License:
# http://www.opensource.org/licenses/mit-license.php

use strict;
use warnings;

use File::Spec;
use lib File::Spec->catdir(File::Spec->curdir(), "inc");

use Test::Run::Builder;

my $build = Test::Run::Builder->new(
    module_name => "File::Find::Object::Rule",
    license     => 'perl',
    script_files => [ 'findrule' ],
    requires    => {
        'Test::More'      => 0,
        'Number::Compare' => 0,
        'Text::Glob'      => 0,
        'File::Find'      => 0,
        'File::Spec'      => 0,
        'Cwd'             => 0,
    },
    create_makefile_pl => 'traditional',
   );

$build->create_build_script;
