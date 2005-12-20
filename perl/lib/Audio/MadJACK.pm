package Audio::MadJACK;

################
#
# MadJACK: perl control interface
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
    
    croak( "Missing MadJACK server port or URL" ) if (scalar(@_)<1);

    # Bless the hash into an object
    my $self = { 
    	pong => 0,
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
    
    # Add reply handlers
    $self->{lo}->add_method( '/deck/state', 's', \&_state_handler, $self );
    $self->{lo}->add_method( '/deck/position', 'd', \&_position_handler, $self );
    $self->{lo}->add_method( '/deck/filename', 's', \&_filename_handler, $self );
    $self->{lo}->add_method( '/deck/filepath', 's', \&_filepath_handler, $self );
    $self->{lo}->add_method( '/pong', '', \&_pong_handler, $self );
    
    # Check MadJACK server is there
    if (!$self->ping()) {
    	carp("MadJACK server is not responding");
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

sub get_state {
	my $self=shift;
	$self->{state} = undef;
	$self->_wait_reply( '/deck/get_state' );
	return $self->{state};
}

sub _state_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{state}=$_[0];
}

sub get_position {
	my $self=shift;
	$self->{postion} = undef;
	$self->_wait_reply( '/deck/get_position' );
	return $self->{position};
}

sub _position_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{position}=$_[0];
}

sub get_filename {
	my $self=shift;
	$self->{filename} = undef;
	$self->_wait_reply( '/deck/get_filename' );
	return $self->{filename};
}

sub _filename_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{filename}=$_[0];
}

sub get_filepath {
	my $self=shift;
	$self->{filepath} = undef;
	$self->_wait_reply( '/deck/get_filepath' );
	return $self->{filepath};
}

sub _filepath_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{filepath}=$_[0];
}

sub ping {
	my $self=shift;
	$self->{pong} = 0;
	$self->_wait_reply( '/ping' );
	return $self->{pong};
}

sub _pong_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{pong}++;
}

sub get_url {
	my $self=shift;
	return $self->{addr}->get_url();
}

sub _wait_reply {
	my $self=shift;
	my ($path) = @_;
	my $attempts = 3;
	my $bytes = 0;

	# Try a few times
	for(1..$attempts) {
	
		# Send Query
		$self->{lo}->send( $self->{addr}, $path, '' );

		# Wait for reply within 0.25 seconds
		$bytes = $self->{lo}->recv_noblock( 250 );
		if ($bytes<1) {
			warn "Timed out waiting for reply after 0.25 seconds\n";
		} else { last; }
	}
	
	# Failed to get reply ?
	if ($bytes<1) {
		warn "Failed to get reply from MadJACK server after $attempts attempts.\n";
	}
	
	return $bytes;
}


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
