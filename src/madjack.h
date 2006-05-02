/*

	madjack.h
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
	MADJACK_STATE_PLAYING,		// 0: Deck is playing
	MADJACK_STATE_PAUSED,		// 1: Deck is paused in mid-playback
	MADJACK_STATE_READY,		// 2: Deck is loaded and ready to play
	MADJACK_STATE_LOADING,		// 3: In process of loading deck
	MADJACK_STATE_STOPPED,		// 4: Track is loaded but not decoding
	MADJACK_STATE_EMPTY,		// 5: No track is loaded in deck
	MADJACK_STATE_ERROR,		// 6: Problem opening/decoding track
	MADJACK_STATE_QUIT			// 7: Time to quit
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
	float cuepoint;						// Position of cuepoint (in seconds)
	
	int bitrate;						// Bitrate of the input file (in kbps)

} input_file_t;


// ------- Globals -------
extern jack_port_t *outport[2];
extern jack_ringbuffer_t *ringbuffer[2];
extern jack_client_t *client;
extern input_file_t *input_file;
extern char * root_directory;
extern char error_string[MAX_ERRORSTR_LEN];
extern int verbose;
extern int quiet;


// ------- Prototypes -------
enum madjack_state get_state();
void set_state( enum madjack_state new_state );
const char* get_state_name( enum madjack_state state );
void error_handler( char *fmt, ... );


#endif
