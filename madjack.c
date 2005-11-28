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

#include <lo/lo.h>
#include <mad.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <getopt.h>
#include "config.h"


// ------- Constants -------
#define RINGBUFFER_DURATION		(5.0)


// ------- Globals -------
jack_port_t *outport[2] = {NULL, NULL};
jack_ringbuffer_t *ringbuffer[2] = {NULL, NULL};
jack_client_t *client = NULL;

int running = 1;
int autoconnect = 0;


// Callback called by JACK when audio is available
static int proces_jack(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in;
	unsigned int i;


	/* get the audio samples, and find the peak sample */
	//in = (jack_default_audio_sample_t *) jack_port_get_buffer(input_port, nframes);
	//for (i = 0; i < nframes; i++) {
	//	const float s = fabs(in[i]);
	//	if (s > peak) {
	//		peak = s;
	//	}
	//}


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
static int usage( const char * progname )
{
	fprintf(stderr, "madjack version %s\n\n", VERSION);
	fprintf(stderr, "Usage %s [<filemame>]\n\n", progname);
	exit(1);
}



int main(int argc, char *argv[])
{
	jack_status_t status;
	size_t ringbuffer_size = 0;
	int i, opt;

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
	fprintf(stderr,"Size of ring buffers is %2.2f seconds (%d bytes).\n", RINGBUFFER_DURATION, (int)ringbuffer_size );
	for(i=0; i<2; i++) {
		if (!(ringbuffer[i] = jack_ringbuffer_create( ringbuffer_size ))) {
			fprintf(stderr, "Cannot create ringbuffer.\n");
			exit(1);
		}
	}
	
	

	// Register the peak signal callback
	jack_set_process_callback(client, proces_jack, 0);


	// Activate ourselves
	if (jack_activate(client)) {
		fprintf(stderr, "Cannot activate JACK client.\n");
		exit(1);
	}


	// Auto-connect our output ports ?
	if (autoconnect) {
		// *FIXME*
	}
	

	while (running) {

		// Check for key press
		
		// Anything else ?
		
		sleep(1);

	}
	
	
	// Free up the ring buffers
	jack_ringbuffer_free( ringbuffer[0] );
	jack_ringbuffer_free( ringbuffer[1] );
	
	
	// Leave the jack graph 
	jack_client_close(client);
	

	return 0;
}

