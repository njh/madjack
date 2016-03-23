/*

	madjack.h
	Multifunctional Audio Deck for the jack audio connection kit
	Copyright (C) 2005-2016  Nicholas J. Humfrey

*/

#include <jack/jack.h>
#include <jack/ringbuffer.h>


#ifndef _MADJACK_H_
#define _MADJACK_H_


// ------- Constants -------
#define DEFAULT_RB_LEN			(2.0)
#define READ_BUFFER_SIZE		(2048)
#define DEFAULT_CLIENT_NAME		"madjack"
#define MAX_FILENAME_LEN		(255)
#define MAX_ERRORSTR_LEN		(255)


enum madjack_state {
	MADJACK_STATE_STARTING=-1,	// -1: Initial state
	MADJACK_STATE_PLAYING,		//  0: Deck is playing
	MADJACK_STATE_PAUSED,		//  1: Deck is paused in mid-playback
	MADJACK_STATE_READY,		//  2: Deck is loaded and ready to play
	MADJACK_STATE_LOADING,		//  3: In process of loading deck
	MADJACK_STATE_STOPPED,		//  4: Track is loaded but not decoding
	MADJACK_STATE_EMPTY,		//  5: No track is loaded in deck
	MADJACK_STATE_ERROR,		//  6: Problem opening/decoding track
	MADJACK_STATE_QUIT			//  7: Time to quit
};


typedef struct input_file_struct {
	unsigned char* buffer;			// MPEG Audio Read buffer
	unsigned int buffer_size;		// Total length of read buffer
	unsigned int buffer_used;		// Amount of buffer currently used
	
	FILE* file;
	char* filepath;						// Path to the audio file
	char filename[MAX_FILENAME_LEN];	// Filename without the path
	unsigned long start_pos;			// First byte of MPEG audio
	unsigned long end_pos;				// Last byte of MPEG audio
	
	float position;						// Current postion of playback (in seconds)
	float duration;						// Total duration of file (in seconds)
	
	int bitrate;						// Bitrate of the input file (in kbps)
	int samplerate;						// Sample rate of the input file (in Hz)
	int framesize;						// Length of a frame of audio (in bytes)

} input_file_t;


// ------- Globals -------
extern jack_port_t *outport[2];
extern jack_ringbuffer_t *ringbuffer[2];
extern jack_client_t *client;
extern input_file_t *input_file;
extern char * root_directory;
extern char error_string[MAX_ERRORSTR_LEN];
extern int play_when_ready;
extern int verbose;
extern int quiet;


// ------- Prototypes -------
enum madjack_state get_state();
void set_state( enum madjack_state new_state );
const char* get_state_name( enum madjack_state state );
void error_handler( char *fmt, ... );


#endif
