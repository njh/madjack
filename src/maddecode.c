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
pthread_t decoder_thread;   		// The decoder thread
int bitrate = 0;                    // The bitrate of the MPEG Audio (in kbps)
int is_decoding = 0;                // Set to 1 while thread is running
int decoder_thread_exists = 0;		// Set if decoder thread has been created
int terminate_decoder_thread = 0;   // Set to 1 to tell thread to stop

// Mutex to make sure we don't stop/start decoder thread simultaneously
pthread_mutex_t decoder_thread_control = PTHREAD_MUTEX_INITIALIZER;



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
	if (input->file==NULL) {
		error_handler( "File is NULL in callback_input()" );
		return MAD_FLOW_BREAK;
	}

	// At end of file ?
	if (feof(input->file)) {
		// Before we have filled the ringbuffer ?
		if (get_state()==MADJACK_STATE_LOADING) {
			// Anything in the ringbuffer ?
			if (jack_ringbuffer_read_space( ringbuffer[0] ) == 0) {
				error_handler( "Got to end of input file before putting any audio in the ringbuffer" );
				return MAD_FLOW_BREAK;
			} else {
				set_state( MADJACK_STATE_READY );
			}
		}
		return MAD_FLOW_STOP;
	}
		
	// Abort thread ?
	if (terminate_decoder_thread)
		return MAD_FLOW_STOP;
	
	// Any unused bytes left in buffer ?
	if(stream->next_frame) {
		unsigned int unused_bytes = input->buffer + input->buffer_used - stream->next_frame;
		memmove(input->buffer, stream->next_frame, unused_bytes);
		input->buffer_used = unused_bytes;
	}

	// Read in some bytes
	input->buffer_used += fread( input->buffer + input->buffer_used, 1, input->buffer_size - input->buffer_used, input->file);

	// Pass the buffer to libmad	
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

	
	// Sleep until there is room in the ring buffer
	while( jack_ringbuffer_write_space( ringbuffer[0] )
	          < (nsamples * sizeof(float) ) )
	{
		// Abort thread ?
		if (terminate_decoder_thread)
			return MAD_FLOW_STOP;

		// If ringbuffer if full and we are still in LOADING state
		// then we are ready for JACK to start emptying the ring-buffer
		if (get_state()==MADJACK_STATE_LOADING) {
			set_state( MADJACK_STATE_READY );
		}
		
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
	static int warned_vbr;
	
	// Abort thread ?
	if (terminate_decoder_thread)
		return MAD_FLOW_STOP;

	//printf("samplerate of file: %d\n", header->samplerate);
	if (jack_get_sample_rate( client ) != header->samplerate) {
		error_handler( "Sample rate of input file (%d) is different to JACK's (%d)", 
						header->samplerate, jack_get_sample_rate( client ) );
		
		return MAD_FLOW_BREAK;
	}
	
	// Check to see if bitrate has changed
	if (bitrate==0) {
		bitrate = header->bitrate;
		warned_vbr=0;
		if (verbose) printf( "Bitrate: %d kbps.\n", bitrate );
	} else if (bitrate != header->bitrate) {
		if (!warned_vbr) {
			fprintf(stderr, "Warning: Bitrate changed during decoding, VBR is not recommended.\n");
			warned_vbr=1;
		}
	}
	
	// Header looks OK
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

	switch( stream->error ) {

		case MAD_ERROR_LOSTSYNC:
		case MAD_ERROR_BADCRC:
		case MAD_ERROR_BADDATAPTR:
			if (verbose)
				fprintf(stderr, "Warning: libmad decoding error: 0x%04x (%s)\n",
					stream->error, mad_stream_errorstr(stream));
		return MAD_FLOW_CONTINUE;
		
		
		default:
			if (MAD_RECOVERABLE (stream->error)) {
				fprintf(stderr, "Warning: libmad decoding error: 0x%04x (%s)\n",
					stream->error, mad_stream_errorstr(stream));
			} else {
				error_handler("libmad decoding error: 0x%04x (%s)\n",
					stream->error, mad_stream_errorstr(stream));
			}
		break;	
	}
	
	
	// Abort decoding if error is non-recoverable
	if (MAD_RECOVERABLE (stream->error))
		 return MAD_FLOW_CONTINUE;
	else return MAD_FLOW_BREAK;

}




static
void *thread_decode_mad(void *input)
{
	struct mad_decoder *decoder = NULL;
	int result;

	is_decoding = 1;

	if (verbose) printf("Decoder thread started.\n");
	
	// Allocate memory for mad_decoder
	decoder = malloc(sizeof( struct mad_decoder ));
	if (!decoder) {
		fprintf(stderr, "Error: failed to allocate memory for mad_decoder structure.\n");
		exit(-1);
	}

	// Initalise the MAD decoder
	mad_decoder_init(
		decoder,			// decoder structure
		input, 				// data pointer 
		callback_input,		// input callback
		callback_header, 	// header callback
		NULL, 				// filter callback
		callback_output,	// output callback
		callback_error,		// error callback
		NULL 				// message callback
	);

	// Start new thread for this:
	result = mad_decoder_run(decoder, MAD_DECODER_MODE_SYNC);
	if (result) {
		fprintf(stderr, "Warning: mad_decoder_run returned %d\n", result);
	}

	// Shut down decoder
	mad_decoder_finish( decoder );
	free( decoder );
	
	if (verbose) printf("Decoder thread exiting.\n");

	// If we got here while loading (and weren't told to stop), 
	// then something went wrong
	if (get_state() == MADJACK_STATE_LOADING &&
	    terminate_decoder_thread == 0)
	{
		set_state( MADJACK_STATE_ERROR );
	}

	is_decoding = 0;
	
	pthread_exit(NULL);
}



// Seek to the start of the MPEG audio in the file
static void seek_start_mpeg_audio( FILE* file )
{
	char bytes[ID3v2_HEADER_LEN];

	// Seek to very start of file
	fseek( file, 0, SEEK_SET);
	
	
	// Read in (possible) ID3v2 header
	bzero( bytes, ID3v2_HEADER_LEN );
	fread( bytes, ID3v2_HEADER_LEN, 1, file);
	
	
	// Is there an ID3v2 header there ?
	if (bytes[0] == 'I' && bytes[1] == 'D' && bytes[2] == '3') {
		unsigned int length = 0;
		int ver_major = bytes[3];
		int ver_minor = bytes[4];
	
		// Calculate the length of the extended header
		length = (length << 7) | (bytes[6] & 0x7f);
		length = (length << 7) | (bytes[7] & 0x7f);
		length = (length << 7) | (bytes[8] & 0x7f);
		length = (length << 7) | (bytes[9] & 0x7f);
		
		// Add on the header length
		length += ID3v2_HEADER_LEN;
		
		// Check flag to see if footer is present
		if (bytes[6] & 0x10)
			length += ID3v2_FOOTER_LEN;
		
		if (verbose) printf("Found ID3 v2.%d.%d header at start of file (0x%x bytes).\n",
				ver_major, ver_minor, length);

		// Seek to end of ID3 headers
		fseek( file, length, SEEK_SET);
		
	} else {
		if (verbose) printf("No ID3v2 header found at start of file.\n");

		// Just seek back to the start
		fseek( file, 0, SEEK_SET);
	}

}




void start_decoder_thread(void *data)
{
	input_file_t *input = data;
	int result;
	
	// Stop the previous thread
	finish_decoder_thread();



	// Don't allow another control thread to 
	// start or stop the decoder thread
	pthread_mutex_lock( &decoder_thread_control );
	
	// Sanity check
	if (decoder_thread_exists) {
		fprintf(stderr, "Bad bad bad: decoder thread already exists while trying to start thread.\n");
		exit(-1);
	}

	// Go to Loading state
	set_state( MADJACK_STATE_LOADING );
	
	// Signal the thread to run
	terminate_decoder_thread = 0;

	// Reset the position in track
	position = 0.0;
	
	// Empty out the read buffer
	input->buffer_used = 0;
	
	// Reset the bitrate
	bitrate = 0;
	
	// Empty out ringbuffers
	jack_ringbuffer_reset( ringbuffer[0] );
	jack_ringbuffer_reset( ringbuffer[1] );
	
	// Seek to start of file (after ID3 tag)
	seek_start_mpeg_audio( input_file->file );

		
	// Start the decoder thread
	result = pthread_create(&decoder_thread, NULL, thread_decode_mad, input);
	if (result) {
		fprintf(stderr, "Error: return code from pthread_create() is %d\n", result);
		exit(-1);
	} else {
		// A thread has been created that will later need disposed of
		decoder_thread_exists = 1;
	}

	pthread_mutex_unlock( &decoder_thread_control );
}


void finish_decoder_thread()
{
	// Don't allow another control thread to 
	// start or stop the decoder thread
	pthread_mutex_lock( &decoder_thread_control );

	if (decoder_thread_exists) {
		int result;
	
		// Signal the thread to terminate
		terminate_decoder_thread = 1;

		if (verbose && is_decoding)
			printf("Waiting for decoder thread to finish.\n");
		
		// pthread_join waits for thread to terminate 
		// and then releases the thread's resources
		result = pthread_join( decoder_thread, NULL );
		if (result) {
			fprintf(stderr, "Warning: pthread_join() failed: %s\n", strerror(result));
		} else {
			decoder_thread_exists = 0;
		}
	}
	
	// Sanity Check
	if (is_decoding) {
		fprintf(stderr, "Bad bad bad: decoder thread still decoding after it was shutdown.\n");
		exit(-1);
	}

	pthread_mutex_unlock( &decoder_thread_control );
}



