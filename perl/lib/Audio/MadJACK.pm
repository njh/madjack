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

    # Bless the hash into an object
    my $self = { 
    	state => undef,
    	position => undef,
    	filename => undef,
    	filepath => undef
    };
    bless $self, $class;

    # Create address of MadJACK server
    $self->{addr} = new Net::LibLO::Address( @_ );
    if (!defined $self->{addr}) {
    	carp("Error creating Net::LibLO::Address");
    	return undef;
    }
        
    # Create new LibLO instance
    $self->{lo} = new Net::LibLO();
    if (!defined $self->{lo}) {
    	carp("Error creating Net::LibLO");
    	return undef;
    }

   	return $self;
}



sub play {
	my $self=shift;
	return $self->{lo}->send( $self->{addr}, '/deck/play', '' );
}

sub pause {
	my $self=shift;
	return $self->{lo}->send( $self->{addr}, '/deck/pause', '' );
}

sub stop {
	my $self=shift;
	return $self->{lo}->send( $self->{addr}, '/deck/stop', '' );
}

sub cue {
	my $self=shift;
	return $self->{lo}->send( $self->{addr}, '/deck/cue', '' );
}

sub eject {
	my $self=shift;
	return $self->{lo}->send( $self->{addr}, '/deck/eject', '' );
}

sub load {
	my $self=shift;
	my ($filename) = @_;
	return $self->{lo}->send( $self->{addr}, '/deck/load', 's', $filename );
}

sub _wait_reply {

}

# get_state()
# get_position()
# get_filename()
# get_filepath()
# ping()


1;

__END__

=pod

=head1 NAME

Audio::MadJACK - Perl interface for liblo Lightweight OSC library

=head1 DESCRIPTION



=head1 SEE ALSO

L<Net::LibLO>

L<http://www.ecs.soton.ac.uk/~njh/madjack/>

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
