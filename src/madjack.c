/*

	madjack.c
	MPEG Audio Deck for the jack audio connection kit
	Copyright (C) 2005  Nicholas J. Humfrey
	
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <jack/transport.h>
#include <getopt.h>
#include <errno.h>
#include <stdarg.h>


#include "control.h"
#include "mjosc.h"
#include "madjack.h"
#include "maddecode.h"
#include "config.h"



// ------- Globals -------
jack_port_t *outport[2] = {NULL, NULL};
jack_ringbuffer_t *ringbuffer[2] = {NULL, NULL};
jack_client_t *client = NULL;

input_file_t *input_file = NULL;	// Input file info structure
int state = MADJACK_STATE_STARTING;	// State of MadJACK
int play_when_ready = 0;			// When in READY state, start playing immediately
char * root_directory = "";			// Root directory (files loaded relative to this)
int verbose = 0;					// Verbose flag (display more information)
int quiet = 0;						// Quiet flag (stay silent unless error)
float rb_duration = DEFAULT_RB_LEN;	// Duration of ring buffer (in seconds)
char error_string[MAX_ERRORSTR_LEN] = "\0";	// Last error that occurred 






// Callback called by JACK when audio is available
static
int callback_jack(jack_nframes_t nframes, void *arg)
{
    size_t to_read = sizeof (jack_default_audio_sample_t) * nframes;
	unsigned int c;
	
	for (c=0; c < 2; c++)
	{	
		char *buf = (char*)jack_port_get_buffer(outport[c], nframes);
		size_t len = 0;

		// What state are we in ?
		if (get_state() == MADJACK_STATE_PLAYING) {
		
			// Copy data from ring buffer to output buffer
			len += jack_ringbuffer_read(ringbuffer[c], buf, to_read);
			
			// Not enough samples ?
			if (len < to_read) {
				if (is_decoding) {
					// If still decoding then something has gone wrong
					error_handler( "Audio Ringbuffer underrun" );
				} else {
					// Must have reached end of file
					if (verbose) printf("Reached end of ringbuffer, playback has now stopped.\n");
					set_state( MADJACK_STATE_STOPPED );
					input_file->position = input_file->duration;
				}
			}
			
			// Increment the position in the track
			if (c==0 && get_state() == MADJACK_STATE_PLAYING) {
					input_file->position += ((float)nframes / jack_get_sample_rate( client ));
			}
		}
		
		// If we don't have enough audio, fill it up with silence
		// (this is to deal with pausing etc.)
		if (to_read > len)
			bzero( buf+len, to_read - len );
		
	}


	// Success
	return 0;
}
					


static
void shutdown_callback_jack(void *arg)
{
	fprintf(stderr, "Warning: MadJACK quitting because jackd is shutting down.\n" );

	// Shutdown if jackd stops
	set_state( MADJACK_STATE_QUIT );
}


void connect_jack_port( jack_port_t *port, const char* in )
{
	const char* out = jack_port_name( port );
	int err;
		
	if (!quiet) printf("Connecting %s to %s\n", out, in);
	
	if ((err = jack_connect(client, out, in)) != 0) {
		fprintf(stderr, "connect_jack_port(): failed to jack_connect() ports: %d\n",err);
		exit(1);
	}
}


// crude way of automatically connecting up jack ports
void autoconnect_jack_ports( jack_client_t* client )
{
	const char **all_ports;
	unsigned int ch=0;
	int i;

	// Get a list of all the jack ports
	all_ports = jack_get_ports(client, NULL, NULL, JackPortIsInput);
	if (!all_ports) {
		fprintf(stderr, "autoconnect_jack_ports(): jack_get_ports() returned NULL.");
		exit(1);
	}
	
	// Step through each port name
	for (i = 0; all_ports[i]; ++i) {
		
		// Connect the port
		connect_jack_port( outport[ch], all_ports[i] );
		
		// Found enough ports ?
		if (++ch >= 2) break;
	}
	
	free( all_ports );
}


static
void init_jack( const char* client_name, jack_options_t jack_opt ) 
{
	jack_status_t status;
	size_t ringbuffer_size = 0;
	int i;

	// Register with Jack
	if ((client = jack_client_open(client_name, jack_opt, &status)) == 0) {
		fprintf(stderr, "Failed to start jack client: 0x%x\n", status);
		exit(1);
	}
	if (!quiet) printf("JACK client registered as '%s'.\n", jack_get_client_name( client ) );


	// Create our output ports
	if (!(outport[0] = jack_port_register(client, "left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))) {
		fprintf(stderr, "Cannot register left output port.\n");
		exit(1);
	}
	
	if (!(outport[1] = jack_port_register(client, "right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))) {
		fprintf(stderr, "Cannot register left output port.\n");
		exit(1);
	}
	
	
	// Create ring buffers
	ringbuffer_size = jack_get_sample_rate( client ) * rb_duration * sizeof(float);
	if (verbose) printf("Size of the ring buffers is %2.2f seconds (%d bytes).\n", rb_duration, (int)ringbuffer_size );
	for(i=0; i<2; i++) {
		if (!(ringbuffer[i] = jack_ringbuffer_create( ringbuffer_size ))) {
			fprintf(stderr, "Cannot create ringbuffer.\n");
			exit(1);
		}
	}
	
	// Register shutdown callback
	jack_on_shutdown(client, shutdown_callback_jack, NULL );

	// Register callback
	jack_set_process_callback(client, callback_jack, NULL);
	
}


static
void finish_jack()
{
	// Leave the Jack graph
	jack_client_close(client);
	
	// Free up the ring buffers
	jack_ringbuffer_free( ringbuffer[0] );
	jack_ringbuffer_free( ringbuffer[1] );
}


static
input_file_t* init_inputfile()
{
	input_file_t* ptr;
	
	// Allocate memory for data structure
	ptr = malloc( sizeof( input_file_t ) );
	if (ptr == NULL) {
		fprintf(stderr, "Failed to allocate memory for input file record.\n");
		exit(1);
	}

	// Zero memory
	bzero( ptr, sizeof( input_file_t ) );
	
	// Allocate memory for read buffer
	ptr->buffer_size = READ_BUFFER_SIZE;
	ptr->buffer = malloc( ptr->buffer_size );
	if (ptr->buffer == NULL) {
		fprintf(stderr, "Failed to allocate memory for read buffer.\n");
		exit(1);
	}
	
	return ptr;
}


static
void finish_inputfile(input_file_t* ptr)
{
	// File still open?
	if (ptr->file) {
		fclose( ptr->file );
		ptr->file = NULL;
	}
	
	// Free filepath
	if (ptr->filepath) free( ptr->filepath );

	// Free up memory used by buffer
	if (ptr->buffer) free( ptr->buffer );
	
	// Free up main data structure memory
	free( ptr );
}


static void
termination_handler (int signum)
{
	switch(signum) {
		case SIGHUP:	fprintf(stderr, "Got hangup signal.\n"); break;
		case SIGTERM:	fprintf(stderr, "Got termination signal.\n"); break;
		case SIGINT:	fprintf(stderr, "Got interupt signal.\n"); break;
	}
	
	// Set state to Quit
	set_state( MADJACK_STATE_QUIT );
	
	signal(signum, termination_handler);
}


// Handle an error and store the error message
void error_handler( char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	
	// Set current state to error
	set_state( MADJACK_STATE_ERROR );

	// Display the error message
	fprintf( stderr, "[ERROR] " );
	vfprintf( stderr, fmt, args );
	fprintf(stderr, "\n" );
	
	// Store the error message
	vsnprintf( error_string, MAX_ERRORSTR_LEN, fmt, args );
	va_end( args );

}



const char* get_state_name( enum madjack_state state )
{
	switch( state ) {
		case MADJACK_STATE_STARTING: return "STARTING";
		case MADJACK_STATE_PLAYING: return "PLAYING";
		case MADJACK_STATE_PAUSED: return "PAUSED";
		case MADJACK_STATE_READY: return "READY";
		case MADJACK_STATE_LOADING: return "LOADING";
		case MADJACK_STATE_STOPPED: return "STOPPED";
		case MADJACK_STATE_EMPTY: return "EMPTY";
		case MADJACK_STATE_ERROR: return "ERROR";
		case MADJACK_STATE_QUIT: return "QUIT";
		default: return "UNKNOWN";
	}
}

enum madjack_state get_state()
{
	return state;
}


void set_state( enum madjack_state new_state )
{
	if (state != new_state) {
		if (verbose) {
			printf("State changing from '%s' to '%s'.    \n",
				get_state_name(state), get_state_name(new_state));
		} else if (!quiet) {
			printf("State: %s          \n",get_state_name(new_state));
		}
		state = new_state;
		
		
		if (state == MADJACK_STATE_PLAYING) {
			jack_transport_start( client );
		} else {
			jack_transport_stop( client );
		}
	}
	
	if (new_state == MADJACK_STATE_READY && play_when_ready) {
		if (verbose) printf("play_when_ready is set.\n");
		play_when_ready = 0;
		do_play();
	}
	   
}


// Display how to use this program
static
void usage()
{
	printf("%s version %s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Usage: %s [options] [<filename>]\n", PACKAGE_NAME);
	printf("   -a            Automatically connect JACK ports\n");
	printf("   -l <port>     Connect left output to this input port\n");
	printf("   -r <port>     Connect right output to this input port\n");
	printf("   -n <name>     Name for this JACK client\n");
	printf("   -j            Don't automatically start jackd\n");
	printf("   -d <dir>      Set root directory for audio files\n");
	printf("   -p <port>     Specify port to listen for OSC messages on\n");
	printf("   -R <secs>     Set duration of ringbuffer (in seconds)\n");
	printf("   -v            Enable verbose mode\n");
	printf("   -q            Enable quiet mode\n");
	printf("\n");
	exit(1);
}



int main(int argc, char *argv[])
{
	int autoconnect = 0;
	jack_options_t jack_opt = JackNullOption;
	char *client_name = DEFAULT_CLIENT_NAME;
	char *connect_left = NULL;
	char *connect_right = NULL;
	lo_server_thread osc_thread = NULL;
	char *osc_port = NULL;
	int opt;

	// Make STDOUT unbuffered
	setbuf(stdout, NULL);

	// Parse Switches
	while ((opt = getopt(argc, argv, "al:r:n:jd:p:R:vqh")) != -1) {
		switch (opt) {
			case 'a':  autoconnect = 1; break;
			case 'l':  connect_left = optarg; break;
			case 'r':  connect_right = optarg; break;
			case 'n':  client_name = optarg; break;
			case 'j':  jack_opt |= JackNoStartServer; break;
			case 'd':  root_directory = optarg; break;
			case 'p':  osc_port = optarg; break;
			case 'R':  rb_duration = atof(optarg); break;
			case 'v':  verbose = 1; break;
			case 'q':  quiet = 1; break;
			default:  usage(); break;
		}
	}
	
	// Validate parameters
	if (quiet && verbose) {
    	fprintf(stderr, "Can't be quiet and verbose at the same time.\n");
    	usage();
	}


	// Check remaining arguments
    argc -= optind;
    argv += optind;
    if (argc>1) {
    	fprintf(stderr, "%s only takes a single, optional, filename argument.\n", PACKAGE_NAME);
    	usage();
	}

	// Initialise JACK
	init_jack( client_name, jack_opt );

	// Initialse Input File Data Structure
	input_file = init_inputfile();

	// Activate JACK
	if (jack_activate(client)) {
		fprintf(stderr, "Cannot activate JACK client.\n");
		exit(1);
	}

	// Setup signal handlers
	signal(SIGTERM, termination_handler);
	signal(SIGINT, termination_handler);
	signal(SIGHUP, termination_handler);
	
	
	// Auto-connect our output ports ?
	if (autoconnect) autoconnect_jack_ports( client );
	if (connect_left) connect_jack_port( outport[0], connect_left );
	if (connect_right) connect_jack_port( outport[1], connect_right );

	// Initialise LibLO
	osc_thread = init_osc( osc_port );


	// Nothing currently loaded
	set_state( MADJACK_STATE_EMPTY );
    
	// Load an initial track ?
	if (argc) do_load( *argv );


	// Handle user keypresses (main loop)
	handle_keypresses();


	// Shut down LibLO
	if (osc_thread) finish_osc( osc_thread );

	// Wait for decoder thread to terminate
	finish_decoder_thread();
	
	// Clean up JACK
	finish_jack();
	
	
	// Clean up data structure memory
	finish_inputfile( input_file );
	

	return 0;
}

