/*

	maddecode.c
	Multifunctional Audio Deck for the jack audio connection kit
	Copyright (C) 2005-2016  Nicholas J. Humfrey

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
				error_handler( "Got to end of input file before putting any audio in the ringbuffer." );
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
	input_file_t *input = data;
	static int warned_vbr;
	
	// Abort thread ?
	if (terminate_decoder_thread)
		return MAD_FLOW_STOP;

	//printf("samplerate of file: %d\n", header->samplerate);
	if (jack_get_sample_rate( client ) != header->samplerate) {
		error_handler( "Sample rate of input file (%d) is different to JACK's (%d)", 
						header->samplerate, jack_get_sample_rate( client ) );
		
		return MAD_FLOW_BREAK;
	} else {
		input->samplerate = header->samplerate;
	}
	
	
	// Check to see if bitrate has changed
	if (input->bitrate==0) {
		input->bitrate = header->bitrate;
		warned_vbr=0;
		if (verbose) printf( "Bitrate: %d bps.\n", input->bitrate );
	} else if (input->bitrate != header->bitrate) {
		if (!warned_vbr) {
			fprintf(stderr, "Warning: Bitrate changed during decoding, VBR is not recommended.\n");
			warned_vbr=1;
		}
	}
	
	// Calculate framesize ?
	if (input->framesize==0) {
		int padding = 0;
		if (header->flags&MAD_FLAG_PADDING) padding=1;
		input->framesize = 144 * header->bitrate / (header->samplerate + padding);
		if (verbose) printf( "Framesize: %d bytes.\n", input->framesize );
	}
	
	
	// Calculate duration?
	if (input->duration==0) {
		int frames = (input->end_pos - input->start_pos) / input->framesize;
		input->duration = (1152.0f * frames) / header->samplerate;
		if (verbose) printf( "Duration: %2.2f seconds.\n", input->duration );
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
	if (result && get_state() != MADJACK_STATE_ERROR) {
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
		error_handler( "Decoder thread stopped during loading." );
	}

	is_decoding = 0;
	
	pthread_exit(NULL);
}




static 
int parse_id3v2_header( FILE* file ) {
	char bytes[ID3v2_HEADER_LEN];
	unsigned int length = 0;

	// Read in (possible) ID3v2 header
	bzero( bytes, ID3v2_HEADER_LEN );
	fread( bytes, ID3v2_HEADER_LEN, 1, file);
	
	
	// Is there an ID3v2 header there ?
	if ((bytes[0] == 'I' && bytes[1] == 'D' && bytes[2] == '3') ||
	    (bytes[0] == '3' && bytes[1] == 'D' && bytes[2] == 'I')) {
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

		if (verbose) printf("Found ID3 v2.%d.%d header in file (0x%x bytes long).\n",
				ver_major, ver_minor, length);
	
	}
	
	return length;
}


static 
int parse_id3v1_header( FILE* file ) {
	char bytes[ID3v1_HEADER_LEN];

	// Read in (possible) ID3v1 header
	bzero( bytes, ID3v1_HEADER_LEN );
	fread( bytes, ID3v1_HEADER_LEN, 1, file);
	
	
	// Is there an ID3v1 header there ?
	if (bytes[0] == 'T' && bytes[1] == 'A' && bytes[2] == 'G') {
		if (verbose) printf("Found ID3 v1 header in file (0x80 bytes long).\n");
		return 0x80;
	} else {
		return 0;
	}
}


// Set the first and last byte positions of the audio
// and set the duration of the audio file
// (hunts down ID3 tags and ignores them)
static void mpeg_audio_length( input_file_t *input )
{
	FILE* file = input_file->file;

	/*
		ID3v1: Look for the marker "TAG" 128 bytes from the end of the file.
		ID3v2: Look for the marker "ID3" in the first 3 bytes of the file.
		ID3v2.4:  Look for the marker "3DI" 10 bytes from the end of the file,   
				  or 10 bytes before the beginning of an ID3v1 tag.
	*/
	
	// Seek to very start of file
	rewind( file );
	input->start_pos = parse_id3v2_header( file );
	if (input->start_pos == 0) {
		if (verbose) printf("No ID3v2 header found at start of file.\n");
	}


	// Seek to the very end of the file
	fseek( file, 0, SEEK_END);
	input->end_pos = ftell( file );
	
	// Check for an ID3v1 header at end of file
	fseek( file, input->end_pos-128, SEEK_SET);
	input->end_pos -= parse_id3v1_header( file );
	
	// Check for an ID3v2 header at end of file
	fseek( file, input->end_pos-10, SEEK_SET);
	input->end_pos -= parse_id3v2_header( file );

}


// Seek filehandle to the cuepoint
static void seek_to_cuepoint( input_file_t *input, float cuepoint )
{
	unsigned long bytes = 0;

	if (cuepoint != 0.0) {
		if (input->bitrate==0) {
			fprintf(stderr, "Warning: failed to seek to cuepoint, because bitrate is unknown.\n");
		} else if (input->framesize==0) {
			fprintf(stderr, "Warning: failed to seek to cuepoint, because frame size is unknown.\n");
		} else if (input->samplerate==0) {
			fprintf(stderr, "Warning: failed to seek to cuepoint, because sample rate is unknown.\n");
		} else if (input_file->duration < cuepoint) {
			fprintf(stderr, "Warning: failed to seek to cuepoint, because it is beyond end of file.\n" );
		} else if (cuepoint < 0.0) {
			fprintf(stderr, "Warning: failed to seek to cuepoint, because it is less than zero.\n" );
		} else {
			unsigned long frames = (cuepoint * input->samplerate) / 1152;
			bytes = frames * input->framesize;
			input->position = (1152.0f * frames) / input->samplerate;
		}
	} else {
		input->position = 0.0f;
	}

	// Perform the seek
	fseek( input->file, input->start_pos+bytes, SEEK_SET);
}


void start_decoder_thread(void *data, float cuepoint)
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

	// Empty out the read buffer
	input->buffer_used = 0;
	
	// Empty out ringbuffers
	jack_ringbuffer_reset( ringbuffer[0] );
	jack_ringbuffer_reset( ringbuffer[1] );
	
	// Get the length/start of the audio in the file (after ID3 tags)
	mpeg_audio_length( input );
	
	// Seek filehandle to the cuepoint
	seek_to_cuepoint( input, cuepoint );
	
		
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



