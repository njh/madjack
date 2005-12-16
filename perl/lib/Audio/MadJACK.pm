package Audio::MadJACK;

################
#
# liblo: perl bindings
#
# Copyright 2005 Nicholas J. Humfrey <njh@aelius.com>
#

use Carp;

use Net::LibLO;
use strict;

use vars qw/$VERSION/;

$VERSION="0.01";


sub new {
    my $class = shift;
    my ($url) = @_;
    
    # Bless the hash into an object
    my $self = { 'method_handlers'=>[] };
    bless $self, $class;
        
    # Create new server instance
    $self->{server} = Net::LibLO::lo_server_new_with_proto( $port, $protocol );
    if (!defined $self->{server}) {
    	carp("Error creating lo_server");
    	undef $self;
    }

   	return $self;
}


#sub play {
#    my $self=shift;
#    my ($foo, $bar) = @_;
#
#}


sub DESTROY {
    my $self=shift;
    
    if (defined $self->{server}) {
    	Net::LibLO::lo_server_free( $self->{server} );
    	undef $self->{server};
    }
}


1;

__END__

=pod

=head1 NAME

Audio::MadJACK - Perl interface for liblo Lightweight OSC library

=head1 DESCRIPTION

liblo-perl is a Perl Interface for the liblo Lightweight OSC library.


=head1 SEE ALSO

L<Net::LibLO::Address>

L<Net::LibLO::Bundle>

L<Net::LibLO::Message>

L<http://plugin.org.uk/liblo/>

=head1 AUTHOR

Nicholas J. Humfrey <njh@aelius.com>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2005 Nicholas J. Humfrey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

=cut
