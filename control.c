/*

	control.c
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
#include <termios.h>

#include "control.h"
#include "madjack.h"
#include "config.h"



// ------- Globals -------
struct termios saved_attributes;



static void
reset_input_mode (void)
{
	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}


static void
set_input_mode (void)
{
	struct termios tattr;
	
	// Make sure stdin is a terminal.
	if (!isatty (STDIN_FILENO))
	{
		fprintf (stderr, "Not a terminal.\n");
		exit (-1);
	}
	
	// Save the terminal attributes so we can restore them later.
	if (tcgetattr(STDIN_FILENO, &saved_attributes) == -1) {
		perror("failed to get termios settings");
		exit(-1);
	}
	atexit (reset_input_mode);
	
	// Set the funny terminal modes.
	tcgetattr (STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON|ECHO); // Clear ICANON and ECHO.
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}


// Start playing
void do_play()
{
	printf("do_play()\n");
	
	if (get_state() == MADJACK_STATE_STOPPED) {
		
	}

	set_state( MADJACK_STATE_PLAYING );
}


// Prepare 
void do_cue()
{
	printf("do_cue()\n");
	set_state( MADJACK_STATE_LOADING );

	// Open the new file
	input_file->filepath = filepath;
	input_file->file = fopen( input->filepath, "r" );
	if (input_file->file==NULL) {
		perror("Failed to open input file");
		return;
	}
	
	// Set the decoder running
	start_decoder_thread( input_file );
}

void do_pause()
{
	printf("do_pause()\n");
	set_state( MADJACK_STATE_PAUSED );

}


void do_stop()
{
	printf("do_stop()\n");
	set_state( MADJACK_STATE_STOPPED );


}

void do_eject()
{
	printf("do_eject()\n");
	set_state( MADJACK_STATE_EMPTY );


}

void do_load( char* name )
{
	printf("do_load(%s)\n", name);
	
	// Eject the previous file ?
	if (input->file) {
		fclose(input->file);
		input->file = NULL;
	}
	
	// Change state to loading
	set_state( MADJACK_STATE_LOADING );
	
	// Open the new file
	input->filepath = filepath;

	// Cue up the file	
	do_cue();
}

void do_quit()
{
	printf("do_quit()\n");
	set_state( MADJACK_STATE_QUIT );

}



void handle_keypresses()
{

	// Turn off input buffering on STDIN
	set_input_mode( );
	
	// Check for keypresses
	while (get_state() != MADJACK_STATE_QUIT) {

		// Get keypress
		int c = fgetc( stdin );
		switch(c) {
			case 'p': 
				if (get_state() == MADJACK_STATE_PLAYING) {
					do_pause();
				} else {
					do_play();
				}
			break;
			case 'l':
				printf("Enter name of file to load:");
			break;
			case 'e': do_eject(); break;
			case 's': do_stop(); break;
			case 'q': do_quit(); break;
			
			
			default:
				printf( "Unknown command '%c'.\n", (char)c );
			case 'h':
			case '?':
				printf( "help message.\n" );
			break;
		}
	}

	
}

