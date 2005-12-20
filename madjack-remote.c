/*
 *  Copyright (C) 2005 Nicholas J. Humfrey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lo/lo.h>
#include "config.h"


#define REPLY_TIMEOUT	(1000)		// Number of milliseconds to wait for a reply


// Display how to use this program
static
void usage( )
{
	printf("madjack-remote version %s\n\n", PACKAGE_VERSION);
	printf("Usage: madjack-remote [options] <command>\n");
	printf("   -u <url>    URL for remote MadJACK server\n");
	printf("   -p <port>   Port for remote MadJACK server\n\n");
	printf("Supported Commands:\n");
	printf("  play              Start deck playing\n");
	printf("  pause             Pause deck\n");
	printf("  stop              Stop Deck playback\n");
	printf("  cue               Cue deck to start of track\n");
	printf("  eject             Eject the current track from deck\n");
	printf("  load <filename>   Load <filename> into deck\n");
	printf("  state             Get deck state\n");
	printf("  position          Get playback position (in seconds)\n");
	printf("  filepath          Get path of the currently loaded file\n");
	printf("  filename          Get name (minus path/suffix) of file\n");
	printf("  ping              Check deck is still there\n");
	exit(1);
}


static
void error_handler(int num, const char *msg, const char *path)
{
    fprintf(stderr, "liblo error %d in path %s: %s\n", num, path, msg);
}

static
int state_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	printf("State: %s\n", &argv[0]->s);
    return 0;
}

static
int position_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	printf("Position: %2.2f\n", argv[0]->f);
    return 0;
}

static
int filepath_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	printf("Filepath: %s\n", &argv[0]->s);
    return 0;
}


static
int filename_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	printf("Filename: %s\n", &argv[0]->s);
    return 0;
}


static
int ping_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	printf("Pong.\n");
    return 0;
}


void wait_for_reply( lo_server serv ) {
	int size = lo_server_recv_noblock(serv, REPLY_TIMEOUT);

	// Did we get a reply ?
	if (!size) fprintf(stderr, "Error: didn't recieve a reply from MadJACK server after %d milliseconds.\n", REPLY_TIMEOUT);
}


int main(int argc, char *argv[])
{
	char *port = NULL;
	char *url = NULL;
	lo_address addr = NULL;
	lo_server serv = NULL;
	int opt;

	// Parse Switches
	while ((opt = getopt(argc, argv, "p:u:h")) != -1) {
		switch (opt) {
			case 'p':
				port = optarg;
				break;
				
			case 'u':
				url = optarg;
				break;
				
			default:
				usage( );
				break;
		}
	}
	
	// Need either a port or URL
	if (!port && !url) {
		fprintf(stderr, "Either URL or Port argument is required.\n");
		usage( );
	}
	
	// Check remaining arguments
    argc -= optind;
    argv += optind;
    if (argc<1) usage( );
    
    
	// Create address structure to send on
	if (port) 	addr = lo_address_new(NULL, port);
	else		addr = lo_address_new_from_url(url);


	// Create a server for receiving replies on
    serv = lo_server_new(NULL, error_handler);
	lo_server_add_method( serv, "/deck/state", "s", state_handler, addr);
	lo_server_add_method( serv, "/deck/position", "f", position_handler, addr);
	lo_server_add_method( serv, "/deck/filepath", "s", filepath_handler, addr);
	lo_server_add_method( serv, "/deck/filename", "s", filename_handler, addr);
	lo_server_add_method( serv, "/pong", "", ping_handler, addr);


	// Check to see what message to send
	if (strcmp( argv[0], "play") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/play", "");
	} else if (strcmp( argv[0], "pause") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/pause", "");
	} else if (strcmp( argv[0], "stop") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/stop", "");
	} else if (strcmp( argv[0], "cue") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/cue", "");
	} else if (strcmp( argv[0], "eject") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/eject", "");
	} else if (strcmp( argv[0], "load") == 0) {
		// Check for argument
		if (argc!=2) usage( );
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/load", "s", argv[1]);
	} else if (strcmp( argv[0], "state") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/get_state", "");
		wait_for_reply( serv );
	} else if (strcmp( argv[0], "position") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/get_position", "");
		wait_for_reply( serv );
	} else if (strcmp( argv[0], "filepath") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/get_filepath", "");
		wait_for_reply( serv );
	} else if (strcmp( argv[0], "filename") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/deck/get_filename", "");
		wait_for_reply( serv );
	} else if (strcmp( argv[0], "ping") == 0) {
		lo_send_from(addr, serv, LO_TT_IMMEDIATE, "/ping", "");
		wait_for_reply( serv );
	} else {
		fprintf(stderr, "Unknown command '%s'.\n", argv[0]);
		usage();
	}
	
	
    lo_address_free( addr );
    lo_server_free( serv );

    return 0;
}


