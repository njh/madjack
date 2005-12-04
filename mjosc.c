/*

	mjosc.c
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

#include "control.h"
#include "madjack.h"
#include "mjosc.h"
#include "config.h"


static
void error_handler(int num, const char *msg, const char *path)
{
    fprintf(stderr, "LibLO server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

static
int play_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	do_play();
    return 0;
}

static
int pause_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	do_pause();
    return 0;
}

static
int stop_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	do_stop();
    return 0;
}

static
int cue_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	do_cue();
    return 0;
}

static
int eject_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	do_eject();
    return 0;
}

static
int load_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	// Double check arguments
	if (argc!=1 || types[0] != LO_STRING) {
		fprintf(stderr, "Error: was expecting single string argument to /deck/load\n");
		return -1;
	}

	// Load the requested track
	do_load( &argv[0]->s );
    return 0;
}

static
int state_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	
	// Send back reply
	lo_send( src, "/deck/state", "s", get_state_name( get_state() ) );

    return 0;
}

static
int position_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	
	// Send back reply
	lo_send( src, "/deck/position", "f", position );

    return 0;
}

static
int ping_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	if (verbose) printf( "Got ping from: %s\n", lo_address_get_url(src));
	
	// Send back reply
	lo_send( src, "/pong", "" );

    return 0;
}

static
int wildcard_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	if (verbose) {
		fprintf(stderr, "Warning: unhandled OSC message: '%s' with args '%s'.\n", path, types);
	}

    return -1;
}



lo_server_thread init_osc( char *port )
{
	lo_server_thread st;
	
	// Create new server
	st = lo_server_thread_new( port, error_handler );

	// Add the methods
	lo_server_thread_add_method( st, "/deck/play", "", play_handler, NULL);
	lo_server_thread_add_method( st, "/deck/pause", "", pause_handler, NULL);
	lo_server_thread_add_method( st, "/deck/stop", "", stop_handler, NULL);
	lo_server_thread_add_method( st, "/deck/cue", "", cue_handler, NULL);
	lo_server_thread_add_method( st, "/deck/eject", "", eject_handler, NULL);
	lo_server_thread_add_method( st, "/deck/load", "s", load_handler, NULL);
	lo_server_thread_add_method( st, "/deck/get_state", "", state_handler, NULL);
	lo_server_thread_add_method( st, "/deck/get_position", "", position_handler, NULL);
	lo_server_thread_add_method( st, "/ping", "", ping_handler, NULL);

    // add method that will match any path and args
    lo_server_thread_add_method(st, NULL, NULL, wildcard_handler, NULL);

	// Start the thread
	lo_server_thread_start(st);

	if (verbose) printf( "OSC server thread started: %s\n",
		lo_server_thread_get_url( st ) );
	lo_server_thread_pp( st );
	
	return st;
}


void finish_osc( lo_server_thread st )
{
	if (verbose) printf( "Stopping OSC server thread.\n");

	lo_server_thread_stop( st );
	lo_server_thread_free( st );
	
}



