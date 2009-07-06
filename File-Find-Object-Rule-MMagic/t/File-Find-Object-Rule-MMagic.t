#!perl

use strict;
use warnings;

use Test::More tests => 1;

use File::Find::Object::Rule::MMagic;
# TEST
is_deeply( [ find( magic => 'image/*', maxdepth => 2, in => 't' ) ],
           [ 't/happy-baby.JPG' ] );
