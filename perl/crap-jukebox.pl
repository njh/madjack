#!/usr/bin/perl
#
# A really crap Jukebox that uses single MadJACK instance
# crap-jukebox.pl
#
# It is excessively aggressive - designed to trash MadJACK a bit.
#
# Nicholas J. Humfrey <njh@aelius.com>
#

use Audio::MadJACK;
use Time::HiRes qw/ sleep /;
use strict;


my @MUSIC;
my @JINGLES;

push ( @MUSIC, list_mp3s( '/Users/njh/Music/Playlist_A' ) );
push ( @MUSIC, list_mp3s( '/Users/njh/Music/Playlist_B' ) );
push ( @MUSIC, list_mp3s( '/Users/njh/Music/Playlist_C' ) );
push ( @JINGLES, list_mp3s( '/Users/njh/Music/Jingles' ) );



# Create MadJACK object for talking to deck
my $madjack = new Audio::MadJACK(@ARGV);
exit(-1) unless (defined $madjack);

# Display the URL of the MadJACK deck we connected to
print "URL of madjack server: ".$madjack->get_url()."\n";

# Force hot-output
$|=1;


my $running = 1;
while( $running ) {
	
	# Choose a random Jingle
	my $jingle = random_item( \@JINGLES );
	play_file( $jingle );
	
	# Choose a random track
	my $track = random_item( \@MUSIC );
	play_file( $track );
}

print "Done.\n";




sub play_file {
	my ($filepath) = @_;

	$madjack->load( $filepath ) || warn "Failed to load";
	$madjack->play();
	
	print "Loading: ".$madjack->get_filepath();
	while( $madjack->get_state() eq 'LOADING' ) { print "."; sleep 0.3; }
	print "\n";
	
	if ($madjack->get_state() eq 'ERROR') {
		warn "Failed to load: $filepath";
		return;
	}
	
	# Start playing
	$madjack->play() || warn "Failed to start playing.";
	
	# Give song a chance to start
	sleep 1;
	
	# Check it is playing
	my $state = $madjack->get_state();
	if ( $state ne 'PLAYING' ) {
		warn "  Failed to start playing ($state)\n";
		return;
	}

	# Wait for song to end
	while ( $madjack->get_state() =~ /PLAYING|PAUSED/) {
		printf("%s [%1.1f/%1.1f]    \r", $madjack->get_state(), $madjack->get_position(), $madjack->get_duration());
		sleep 0.1;
	}	
	
	# Eject the old song
	print "  Ejecting track";
	while( $madjack->get_state() ne 'EMPTY' ) {
		# Stop and eject
		$madjack->stop() || warn "Failed to stop";
		$madjack->eject() || warn "Failed to eject";
		print ".             ";
	}
	print "\n";
	
}



sub list_mp3s {
	my ($dir) = @_;
	my @files = ();
	
	opendir(DIR, $dir) or die "Failed to open directory: $!";
	foreach my $file (readdir DIR) {
		my $path = "$dir/$file";
		
		next if ($file =~ /^\./);
		
		if (-d $path) {
			push( @files, list_mp3s( $path ) );
		} elsif (-f $path ) {
			push( @files, $path );
		} else {
			warn "Unknown file type: $path\n";
		}
	}
	closedir(DIR);

	return @files;
}


sub fisher_yates_shuffle {
    my $array = shift;
    my $i;
    for ($i = @$array; --$i; ) {
        my $j = int rand ($i+1);
        next if $i == $j;
        @$array[$i,$j] = @$array[$j,$i];
    }
}

sub random_item {
	my ($arrayref) = @_;
	my $count = scalar( @$arrayref );
	
	return $arrayref->[ int(rand($count)) ];
}

