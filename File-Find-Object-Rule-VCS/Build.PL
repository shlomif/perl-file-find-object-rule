# This file is under the MIT X11 License:
# http://www.opensource.org/licenses/mit-license.php

use strict;
use warnings;

use File::Spec;
use lib File::Spec->catdir(File::Spec->curdir(), "inc");

use Test::Run::Builder;

my $build = Test::Run::Builder->new(
    module_name => "File::Find::Object::Rule::VCS",
    license     => 'perl',
    dist_author => 'Shlomi Fish <shlomif@cpan.org>',
    requires =>
    {
        'strict' => 0,
        'warnings'           => 0,
        'File::Find::Object::Rule' => 0,
    },
    test_requires    => {
        'File::Spec' => 0.82,
        'Test::More' => 0.47,
    },
    create_makefile_pl => 'traditional',
    meta_merge =>
    {
        resources =>
        {
            repository => "http://svn.berlios.de/svnroot/repos/web-cpan/File-Find-Object/",
            homepage => "http://www.shlomifish.org/open-source/projects/File-Find-Object/",
        },
        keywords =>
        [
            "bzr",
            "cvs",
            "file-find-object-rule",
            "file-find-rule",
            "find",
            "git",
            "mercurial",
            "mmagic",
            "object",
            "revision control",
            "subversion",
            "types",
            "vcs",
            "version control",
        ],
    },
);

$build->create_build_script;

