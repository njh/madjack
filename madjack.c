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
static int callback_jack(jack_nframes_t nframes, void *arg)
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


/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer {
  unsigned char const *start;
  unsigned long length;
};


/*
 * This is the MAD input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow calljack_input(void *data,
		    struct mad_stream *stream)
{
	struct buffer *buffer = data;
	
	if (!buffer->length)
	return MAD_FLOW_STOP;
	
	mad_stream_buffer(stream, buffer->start, buffer->length);
	
	buffer->length = 0;
	
	return MAD_FLOW_CONTINUE;
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static
enum mad_flow callback_output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	
	/* pcm->samplerate contains the sampling frequency */
	
	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	
	while (nsamples--) {
		signed int sample;
		
		/* output sample(s) in 16-bit signed little-endian PCM */
		
		sample = scale(*left_ch++);
		putchar((sample >> 0) & 0xff);
		putchar((sample >> 8) & 0xff);
		
		if (nchannels == 2) {
			sample = scale(*right_ch++);
			putchar((sample >> 0) & 0xff);
			putchar((sample >> 8) & 0xff);
		}
	}
	
	return MAD_FLOW_CONTINUE;
}


/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static
enum mad_flow callback_error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct buffer *buffer = data;

  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
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
	struct mad_decoder decoder;
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
	
	
	// Initalise MAD
	mad_decoder_init(&decoder, &buffer,
		   calllback_input,
		   0 /* header */,
		   0 /* filter */,
		   calllback_output,
		   calllback_error,
		   0 /* message */);
	
	
	
	

	// Register the peak signal callback
	jack_set_process_callback(client, calllback_jack, 0);

	// Activate ourselves
	if (jack_activate(client)) {
		fprintf(stderr, "Cannot activate JACK client.\n");
		exit(1);
	}
	
	// Create decoding thread
	// ** DO IT **


	// Auto-connect our output ports ?
	if (autoconnect) {
		// *FIXME*
	}
	

	while (running) {

		// Check for key press
		
		// Anything else ?
		
		sleep(1);

	}
	
	
	// Shut down decoder
	mad_decoder_finish( &decoder );
	
	// Leave the jack graph 
	jack_client_close(client);
	
	// Free up the ring buffers
	jack_ringbuffer_free( ringbuffer[0] );
	jack_ringbuffer_free( ringbuffer[1] );
	

	return 0;
}

