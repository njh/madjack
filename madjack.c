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
#include <pthread.h>
#include <termios.h>

#include <lo/lo.h>
#include <mad.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <getopt.h>


#include "control.h"
#include "madjack.h"
#include "maddecode.h"
#include "config.h"



// ------- Globals -------
jack_port_t *outport[2] = {NULL, NULL};
jack_ringbuffer_t *ringbuffer[2] = {NULL, NULL};
jack_client_t *client = NULL;

int state = MADJACK_STATE_EMPTY;





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
		if (state == MADJACK_STATE_PLAYING) {
		
			// Copy data from ring buffer to output buffer
			len += jack_ringbuffer_read(ringbuffer[c], buf, to_read);
			
			if (len < to_read) {
				// *FIXME:* are we still decoding ?
				fprintf(stderr, "ringbuffer underrun.\n");
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






/* Connect the chosen port to ours */
//static void connect_port(jack_client_t *client, char *port_name)
//{
//	jack_port_t *port;
//
//	// Get the port we are connecting to
//	port = jack_port_by_name(client, port_name);
//	if (port == NULL) {
///		fprintf(stderr, "Can't find port '%s'\n", port_name);
//		exit(1);
///	}

//	// Connect the port to our input port
//	fprintf(stderr,"Connecting '%s' to '%s'...\n", jack_port_name(port), jack_port_name(input_port));
//	if (jack_connect(client, jack_port_name(port), jack_port_name(input_port))) {
//		fprintf(stderr, "Cannot connect port '%s' to '%s'\n", jack_port_name(port), jack_port_name(input_port));
//		exit(1);
//	}
//}


/* Display how to use this program */
static
int usage( const char * progname )
{
	fprintf(stderr, "madjack version %s\n\n", VERSION);
	fprintf(stderr, "Usage %s [<filemame>]\n\n", progname);
	exit(1);
}




void load_inputfile(input_file_t* input, char* filepath )
{
	int result;
	
	// Eject the previous file ?
	if (input->file) {
		fclose(input->file);
		input->file = NULL;
	}
	
	// Open the new file
	input->filepath = filepath;
	input->file = fopen( filepath, "r" );
	if (input->file==NULL) {
		perror("Failed to open input file");
		return;
	}
	
	// Set the decoder running
	result = pthread_create(&decoder_thread, NULL, thread_decode_mad, input);
	if (result) {
		printf("Error: return code from pthread_create() is %d\n", result);
		exit(-1);
	}

}


void init_jack() 
{
	jack_status_t status;
	size_t ringbuffer_size = 0;
	int i;

	// Register with Jack
	if ((client = jack_client_open("madjack", JackNullOption, &status)) == 0) {
		fprintf(stderr, "Failed to start jack client: %d\n", status);
		exit(1);
	}
	fprintf(stderr,"Registering as '%s'.\n", jack_get_client_name( client ) );


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
	ringbuffer_size = jack_get_sample_rate( client ) * RINGBUFFER_DURATION * sizeof(float);
	fprintf(stderr,"Size of the ring buffers is %2.2f seconds (%d bytes).\n", RINGBUFFER_DURATION, (int)ringbuffer_size );
	for(i=0; i<2; i++) {
		if (!(ringbuffer[i] = jack_ringbuffer_create( ringbuffer_size ))) {
			fprintf(stderr, "Cannot create ringbuffer.\n");
			exit(1);
		}
	}


	// Register callback
	jack_set_process_callback(client, callback_jack, NULL);
	
}


void finish_jack()
{
	// Leave the Jack graph
	jack_client_close(client);
	
	// Free up the ring buffers
	jack_ringbuffer_free( ringbuffer[0] );
	jack_ringbuffer_free( ringbuffer[1] );
}


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


void finish_inputfile(input_file_t* ptr)
{
	// File still open?
	if (ptr->file) {
		fclose( ptr->file );
		ptr->file = NULL;
	}

	// Free up memory used by buffer
	if (ptr->buffer) free( ptr->buffer );
	
	// Free up main data structure memory
	free( ptr );
}

int main(int argc, char *argv[])
{
	int autoconnect = 0;
	input_file_t *input_file;
	int opt;

	while ((opt = getopt(argc, argv, "ahv")) != -1) {
		switch (opt) {
			case 'a':
				// Autoconnect our output ports
				autoconnect = 1;
				break;
		
			default:
				// Show usage/version information
				usage( argv[0] );
				break;
		}
	}



	// Initialise JACK
	init_jack();

	// Initialse Input File Data Structure
	input_file = init_inputfile();
	
	
	// Activate JACK
	if (jack_activate(client)) {
		fprintf(stderr, "Cannot activate JACK client.\n");
		exit(1);
	}
	
	// Auto-connect our output ports ?
	if (autoconnect) {
		// *FIXME*
	}


	// Load a file (and start decoding)
	load_inputfile( input_file, "003548.mp3" );



	// Handle user keypresses
	handle_keypresses();

	
	// Wait for decoder thread to terminate
	printf("Waiting for decoder thread to finish.\n");
	pthread_join( decoder_thread, NULL);
	
	
	// Clean up JACK
	finish_jack();
	
	
	// Clean up data structure memory
	finish_inputfile( input_file );
	

	return 0;
}

