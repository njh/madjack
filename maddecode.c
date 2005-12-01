/*

	maddecode.c
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

#include <mad.h>
#include <pthread.h>
#include <jack/ringbuffer.h>

#include "madjack.h"
#include "maddecode.h"
#include "config.h"


// ------- Globals -------
pthread_t decoder_thread;		// The decoder thread
int is_decoding = 0;			// Set to 1 while thread is running


/*
 * This is the MAD input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow callback_input(void *data,
		    struct mad_stream *stream)
{
	input_file_t *input = data;
	
	// No file open ?
	if (input->file==NULL)
		return MAD_FLOW_STOP;

	// At end of file ?
	if (feof(input->file))
		return MAD_FLOW_STOP;
	
	// Any unused bytes left in buffer ?
	if(stream->next_frame) {
		unsigned int unused_bytes = input->buffer + input->buffer_used - stream->next_frame;
		memmove(input->buffer, stream->next_frame, unused_bytes);
		input->buffer_used = unused_bytes;
	}

	// Read in some bytes
	input->buffer_used += fread( input->buffer + input->buffer_used, 1, input->buffer_size - input->buffer_used, input->file);
	if (input->buffer_used==0)
		return MAD_FLOW_STOP;
	
	mad_stream_buffer(stream, input->buffer, input->buffer_used);
	
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
	unsigned int nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	
	// pcm->samplerate contains the sampling frequency
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	
	// If we have decoded a frame, then we have gone from
	//   Loading to Ready
	if (get_state()==MADJACK_STATE_LOADING) {
		set_state( MADJACK_STATE_READY );
	}
	
	// Sleep until there is room in the ring buffer
	while( jack_ringbuffer_write_space( ringbuffer[0] )
	          < (nsamples * sizeof(float) ) )
	{
	
		// Check state and possibly abort
		if (get_state() == MADJACK_STATE_LOADING || 
			get_state() == MADJACK_STATE_STOPPED ||
			get_state() == MADJACK_STATE_EMPTY ||
			get_state() == MADJACK_STATE_QUIT)
		return MAD_FLOW_STOP;

		usleep(1000);
	}


	// Put samples into the ringbuffer
	while (nsamples--) {
		jack_default_audio_sample_t sample;

		// Convert sample for left channel
		sample = mad_f_todouble(*left_ch++);
		jack_ringbuffer_write( ringbuffer[0], (char*)&sample, sizeof(sample) );
		
		// Convert sample for right channel
		if (pcm->channels == 2) sample = mad_f_todouble(*right_ch++);
		jack_ringbuffer_write( ringbuffer[1], (char*)&sample, sizeof(sample) );
	}
	
	return MAD_FLOW_CONTINUE;
}


/*
 * Check the header of frames of MPEG Audio to make sure
 * that they are what we are expecting.
 */

static
enum mad_flow callback_header(void *data,
		struct mad_header const *header)
{
	//printf("samplerate of file: %d\n", header->samplerate);
	if (jack_get_sample_rate( client ) != header->samplerate) {
		printf("Error: Sample rate of input file (%d) is different to JACK's (%d)\n",
				header->samplerate, jack_get_sample_rate( client ) );
		
		return MAD_FLOW_BREAK;
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
	//input_file_t *input = data;
	
	fprintf(stderr, "Decoding error 0x%04x (%s)\n",
		stream->error, mad_stream_errorstr(stream));
	
	/* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
	
	return MAD_FLOW_CONTINUE;
}




static
void *thread_decode_mad(void *input)
{
	struct mad_decoder decoder;
	int result;

	is_decoding = 1;

	printf("Decoder thread started.\n");

	// Initalise the MAD decoder
	mad_decoder_init(
		&decoder,			// decoder structure
		input, 				// data pointer 
		callback_input,		// input callback
		callback_header, 	// header callback
		NULL, 				// filter callback
		callback_output,	// output callback
		callback_error,		// error callback
		NULL 				// message callback
	);

	// Start new thread for this:
	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
	if (result) {
		fprintf(stderr, "Warning: mad_decoder_run returned %d\n", result);
	}

	// Shut down decoder
	mad_decoder_finish( &decoder );

	printf("Decoder thread exiting.\n");

	// If we got here while loading, we must be stopped
	if (get_state() == MADJACK_STATE_LOADING )
		set_state( MADJACK_STATE_STOPPED );

	is_decoding = 0;
	
	pthread_exit(NULL);
}



void start_decoder_thread(void *data)
{
	input_file_t *input = data;
	int result; 
	
	// Empty out the read buffer
	input->buffer_used = 0;
	
	// Empty out ringbuffers
	jack_ringbuffer_reset( ringbuffer[0] );
	jack_ringbuffer_reset( ringbuffer[1] );
	
	// Start the decoder thread
	result = pthread_create(&decoder_thread, NULL, thread_decode_mad, input);
	if (result) {
		printf("Error: return code from pthread_create() is %d\n", result);
		exit(-1);
	}

}


void finish_decoder_thread()
{

	while( is_decoding ) {
		printf("Waiting for decoder thread to finish.\n");
		sleep( 1 );
	}

}

