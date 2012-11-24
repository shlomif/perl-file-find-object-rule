package Stream::Extract;

use 5.006;

use strict;
use warnings;

use Carp;

use Class::XSAccessor
    constructor => '_dont_use_me',
    accessors => {
        _iter_coderef => '_iter_coderef',
        _filter_coderef => '_filter_coderef',
    },
    ;

=head1 NAME

Stream::Extract - find records out of a stream that match certain qualities.
(similar to grep(1).)

=head1 VERSION

Version 0.01

=cut

our $VERSION = '0.01';


=head1 SYNOPSIS

    use Stream::Extract;

    my ($re_s, $filename) = @ARGV;

    my $re = qr/$re_s/;

    open my $fh, '<', $filename
        or die "Cannot open '$filename'"

    my $finder = Stream::Extract->new(
        {
            input => { code => sub { return scalar <$fh>; }, },
            filter => sub {
                my ($self, $args) = @_;

                my $record_obj = $args->{record};

                return $record_obj->text_like($re);
            },
        }
    );

    while (my $match = $finder->next_text())
    {
        print "$match\n";
    }

    close($fh);

=head1 SUBROUTINES/METHODS

=head2 new

Initializes a new object.

=cut

sub new
{
    my $class = shift;

    my $self = {};
    bless $self, $class;

    $self->_init(@_);

    return $self;
}

sub _init
{
    my ($self, $args) = @_;

    $self->_iter_coderef($args->{input}->{code})
        or Carp::confess "No input code ref specified.";

    $self->_filter_coderef($args->{filter})
        or Carp::confess "No filter code ref specified.";

    return;
}

=head2 next

Returns a new record as an object.

=head2 next_text

Returns a new record as text.

=cut

=head1 AUTHOR

Shlomi Fish, L<http://www.shlomifish.org/>, C<< <shlomif at cpan.org> >> .

=head1 BUGS

Please report any bugs or feature requests to C<bug-stream-extract at rt.cpan.org>, or through
the web interface at L<http://rt.cpan.org/NoAuth/ReportBug.html?Queue=Stream-Extract>.  I will be notified, and then you'll
automatically be notified of progress on your bug as I make changes.

=head1 SUPPORT

You can find documentation for this module with the perldoc command.

    perldoc Stream::Extract

You can also look for information at:

=over 4

=item * RT: CPAN's request tracker (report bugs here)

L<http://rt.cpan.org/NoAuth/Bugs.html?Dist=Stream-Extract>

=item * AnnoCPAN: Annotated CPAN documentation

L<http://annocpan.org/dist/Stream-Extract>

=item * CPAN Ratings

L<http://cpanratings.perl.org/d/Stream-Extract>

=item * Search CPAN

L<http://search.cpan.org/dist/Stream-Extract/>

=back


=head1 ACKNOWLEDGEMENTS


=head1 LICENSE AND COPYRIGHT

Copyright 2012 Shlomi Fish.

This program is distributed under the MIT (X11) License:
L<http://www.opensource.org/licenses/mit-license.php>

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.


=cut

1; # End of Stream::Extract
