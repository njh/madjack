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
#define RINGBUFFER_DURATION		(4.0)
#define READ_BUFFER_SIZE		(2048)


enum madjack_state {
	MADJACK_STATE_PLAYING,		// Deck is playing
	MADJACK_STATE_PAUSED,		// Deck is paused in mid-playback
	MADJACK_STATE_LOADING,		// In process of loading deck
	MADJACK_STATE_READY,		// Deck is loaded and ready to play
	MADJACK_STATE_STOPPED,		// Deck is stopped (mid-track or at end)
	MADJACK_STATE_EMPTY,		// No track is loaded in deck
	MADJACK_STATE_QUIT			// Time to quit
};


typedef struct {
	unsigned char* buffer;
	unsigned int buffer_size;
	unsigned int buffer_used;
	FILE* file;
	char* name;
	char* filepath;
} input_file_t;


extern jack_port_t *outport[2];
extern jack_ringbuffer_t *ringbuffer[2];
extern jack_client_t *client;
extern int state;




#endif