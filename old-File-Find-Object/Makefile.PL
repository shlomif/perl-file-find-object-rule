# $Id: Makefile.PL 13 2005-12-17 21:14:31Z nanardon $

use ExtUtils::MakeMaker;

sub MY::postamble {
    <<MAKECHANGELOG;
.PHONY: ChangeLog

ChangeLog:
	cvs2cl -W 400 -I ChangeLog --accum -U ../common/username
	rm -f *.bak

MAKECHANGELOG
}

WriteMakefile(
    NAME	 => 'File::Find::Object',
    VERSION_FROM => 'Object.pm',
);
