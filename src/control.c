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
#include <ctype.h>
#include <errno.h>

#include "control.h"
#include "madjack.h"
#include "maddecode.h"
#include "config.h"



// ------- Globals -------
struct termios saved_attributes;



static void
reset_input_mode (void)
{
	
	// Make sure stdin is a terminal
	if (!isatty (STDIN_FILENO)) return;

	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}


static void
set_input_mode (void)
{
	struct termios tattr;
	
	// Make sure stdin is a terminal
	if (!isatty(STDIN_FILENO)) return;
	
	// Save the terminal attributes so we can restore them later
	if (tcgetattr(STDIN_FILENO, &saved_attributes) == -1) {
		perror("failed to get termios settings");
		exit(-1);
	}
	atexit (reset_input_mode);
	
	// Set the funky terminal modes.
	tcgetattr (STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON|ECHO); // Clear ICANON and ECHO.
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

// Read in the name of a file from STDIN
static
char* read_filepath()
{
	char *filepath = malloc(MAX_FILENAME_LEN);
	int i;
	
	reset_input_mode();

	printf("Enter name of file to load: ");
	fgets( filepath, MAX_FILENAME_LEN-1, stdin );
	
	// Remove carrage return from end of filepath
	for(i=0; i<MAX_FILENAME_LEN; i++) {
		if (filepath[i]==10 || filepath[i]==13) filepath[i]=0;
	}

	set_input_mode( );
	
	return filepath;
}

// Read a cuepoint from STDIN
float read_cuepoint()
{
	float cue;

	reset_input_mode();

	printf("Enter new cue point: ");
	fscanf( stdin, "%f", &cue );

	set_input_mode( );

	return cue;
}



// Concatinate a filename on the end of the root path
static
char* build_fullpath( const char* root, const char* name )
{
	int len = 0;
	char* filepath;
	
	// No root directory specified?
	if (root==NULL) {
		return strdup( name );
	}
	
	// Calculate length of full filepath
	len = strlen(root) + strlen(name) + 2;
	filepath = malloc( len );
	if (!filepath) {
		perror("failed to allocate memory for filepath");
		exit(1);
	}
	
	// Copy root directory to start of filepath
	strcpy( filepath, root );
	
	// Remove surplus trailing slash(es)
	len = strlen(filepath);
	while( len > 1 && filepath[len-1] == '/' ) {
		filepath[--len] = '\0';
	}
	
	// Append a slash
	if (len) filepath[len++] = '/';
	
	// Append the filename
	strcpy( filepath+len, name );
	
	return filepath;
}


// Start playing
void do_play()
{
	if (verbose) printf("-> do_play()\n");
	
	if (get_state() == MADJACK_STATE_PAUSED ||
	    get_state() == MADJACK_STATE_READY)
	{
		set_state( MADJACK_STATE_PLAYING );
	}
	else if (get_state() == MADJACK_STATE_LOADING)
	{
		play_when_ready = 1;
	}
	else if (get_state() != MADJACK_STATE_PLAYING)
	{
		fprintf(stderr, "Warning: Can't change from %s to state PLAYING.\n", get_state_name(get_state()) );
	}
}


// Prepare deck to go into 'READY' state
void do_cue(float cuepoint)
{
	if (verbose) printf("-> do_cue(%f)\n", cuepoint);

	// Stop first
	if (get_state() == MADJACK_STATE_PLAYING ||
	    get_state() == MADJACK_STATE_PAUSED)
	{
		do_stop();
	}
	
	// Had cue-point changed?
	if (get_state() == MADJACK_STATE_READY &&
	    input_file->position != cuepoint)
	{
		if (verbose) printf("Stopping because cuepoint changed.\n");
		do_stop();
	}
	
	// Start the new thread
	if (get_state() == MADJACK_STATE_LOADING ||
	    get_state() == MADJACK_STATE_STOPPED )
	{

		// Set the decoder running
		start_decoder_thread( input_file, cuepoint );
		
	}
	else if (get_state() != MADJACK_STATE_READY)
	{
		fprintf(stderr, "Warning: Can't change from %s to state READY.\n", get_state_name(get_state()) );
	}

}


// Pause Deck (if playing)
void do_pause()
{
	if (verbose) printf("-> do_pause()\n");
	
	if (get_state() == MADJACK_STATE_PLAYING )
	{
		set_state( MADJACK_STATE_PAUSED );
	}
	else if (get_state() != MADJACK_STATE_PAUSED)
	{
		fprintf(stderr, "Warning: Can't change from %s to state PAUSED.\n", get_state_name(get_state()) );
	}
}




// Stop deck (and close down decoder)
void do_stop()
{
	if (verbose) printf("-> do_stop()\n");

	if (get_state() == MADJACK_STATE_PLAYING ||
	    get_state() == MADJACK_STATE_PAUSED ||
	    get_state() == MADJACK_STATE_READY || 
	    get_state() == MADJACK_STATE_LOADING )
	{
		// Store our new state
		set_state( MADJACK_STATE_STOPPED );
		
		// Stop decoder thread
		finish_decoder_thread();
	}
	else if (get_state() != MADJACK_STATE_STOPPED)
	{
		fprintf(stderr, "Warning: Can't change from %s to state STOPPED.\n", get_state_name(get_state()) );
	}

}


// Eject track from Deck
void do_eject()
{
	if (verbose) printf("-> do_eject()\n");

	// Stop first
	if (get_state() == MADJACK_STATE_PLAYING ||
	    get_state() == MADJACK_STATE_PAUSED ||
	    get_state() == MADJACK_STATE_READY)
	{
		do_stop();
	}
	
	if (get_state() == MADJACK_STATE_STOPPED ||
	    get_state() == MADJACK_STATE_ERROR)
	{

		// Ensure decoder thread is terminated
		finish_decoder_thread();

		// Close the input file
		if (input_file->file) {
			fclose(input_file->file);
			input_file->file = NULL;
			free(input_file->filepath);
			input_file->filepath = NULL;
		}
		
		// Reset positions
		input_file->position = 0.0;
		input_file->duration = 0.0;
		input_file->bitrate = 0;
		input_file->samplerate = 0;
		input_file->framesize = 0;

		// Deck is now empty
		set_state( MADJACK_STATE_EMPTY );
	}
	else if (get_state() != MADJACK_STATE_EMPTY)
	{
		fprintf(stderr, "Warning: Can't change from %s to state EMPTY.\n", get_state_name(get_state()) );
	}
	
}



// Load Track into Deck
void do_load( const char* filepath )
{
	if (verbose) printf("-> do_load(%s)\n", filepath);
	
	// Can only load if Deck is empty
	if (get_state() != MADJACK_STATE_EMPTY )
	{
		do_eject();
	}
	
	
	// Check it really is empty
	if (get_state() == MADJACK_STATE_EMPTY )
	{
		char* fullpath;
	
		set_state( MADJACK_STATE_LOADING );
		
		// Pre-pend the root directory path
		fullpath = build_fullpath( root_directory, filepath );
		if (!quiet) printf("Loading: %s\n", fullpath);
		
		// Open the new file
		input_file->file = fopen( fullpath, "r" );
		if (input_file->file==NULL) {
			error_handler( "%s: %s", strerror( errno ), fullpath);
			free( fullpath );
			return;
		}
		
		// Copy string
		input_file->filepath = strdup( filepath );
		free( fullpath );

		// Cue up the new file	
		do_cue(0.0f);
	}
	else if (get_state() != MADJACK_STATE_EMPTY)
	{
		fprintf(stderr, "Warning: Can't change from %s to state LOADING.\n", get_state_name(get_state()) );
	}
	
}


// Quit MadJack
void do_quit()
{
	if (verbose) printf("-> do_quit()\n");
	set_state( MADJACK_STATE_QUIT );
}


void display_keyhelp()
{
	printf( "%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION );
	printf( "  h: This Screen\n" );
	printf( "  p: Play/Pause Deck\n" );
	printf( "  l: Load a Track\n" );
	printf( "  e: Eject current track\n" );
	printf( "  s: Stop Deck\n" );
	printf( "  c: Cue to start of track\n" );
	printf( "  C: Cue to specified time\n" );
	printf( "  q: Quit MadJack\n" );
	printf( "\n" );	
}


static
void read_keypress()
{
	// Get keypress
	int c = fgetc( stdin );

	// Ignore EOF (e.g. when stdin is /dev/null)
	if (c == EOF) return;

	switch(c) {
	
		// Pause/Play
		case 'p': 
			if (get_state() == MADJACK_STATE_PLAYING) {
				do_pause();
			} else {
				do_play();
			}
		break;
		
		// Load
		case 'l': {
			char* filepath = read_filepath();
			do_load( filepath );
			free( filepath );
			break;
		}

		case 'e': do_eject(); break;
		case 's': do_stop(); break;
		case 'q': do_quit(); break;
		case 'c': do_cue(0.0f); break;

		case 'C': {
			float cuepoint = read_cuepoint();
			do_cue( cuepoint );
			break;
		}
		
		default:
			printf( "Unknown command '%c'.\n", (char)c );
		case 'h':
		case '?':
			display_keyhelp();
		break;
		
		// Ignore return and enter
		case 13:
		case 10:
		break;
	}
}


void handle_keypresses()
{
	struct timeval timeout;
	fd_set readfds;
	int retval = -1;

	// Make STDOUT unbuffered (if it is a terminal)
	if (isatty(STDOUT_FILENO))
		setbuf(stdout, NULL);

	// Turn off input buffering on STDIN
	set_input_mode( );
	
	// Check for keypresses
	while (get_state() != MADJACK_STATE_QUIT) {

		// Display position
		if (!quiet && isatty(STDOUT_FILENO)) {
			printf("[%1.1f/%1.1f]         \r", input_file->position, input_file->duration);
		}

		// Set timeout to 1/10 second
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;

		// Watch socket to see when it has input.
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		retval = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

		// Check return value 
		if (retval < 0) {
			// Something went wrong
			perror("select()");
			break;
			
		} else if (retval > 0) {
		
			read_keypress();

		}
	}

	// Restore the input mode
	reset_input_mode();
}

