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
	printf("  status            Get deck status\n");
	printf("  position          Get playback position (in seocnds)\n");
	printf("  ping              Check deck is still there\n");
	exit(1);
}



int main(int argc, char *argv[])
{
	char *port = NULL;
	char *url = NULL;
	lo_address addr = NULL;
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
    
    
	// Create address structure
	if (port) {
		addr = lo_address_new(NULL, port);
	} else {
		addr = lo_address_new_from_url(url);
	}


	// Check to see what message to send
	if (strcmp( argv[0], "play") == 0) {
		lo_send(addr, "/deck/play", "");
	} else if (strcmp( argv[0], "pause") == 0) {
		lo_send(addr, "/deck/pause", "");
	} else if (strcmp( argv[0], "stop") == 0) {
		lo_send(addr, "/deck/stop", "");
	} else if (strcmp( argv[0], "cue") == 0) {
		lo_send(addr, "/deck/cue", "");
	} else if (strcmp( argv[0], "eject") == 0) {
		lo_send(addr, "/deck/eject", "");
	} else if (strcmp( argv[0], "load") == 0) {
		// Check for argument
		if (argc!=2) usage( );
		lo_send(addr, "/deck/load", "s", argv[1]);
	} else if (strcmp( argv[0], "status") == 0) {
		lo_send(addr, "/deck/get_status", "");
	} else if (strcmp( argv[0], "position") == 0) {
		lo_send(addr, "/deck/get_position", "");
	} else if (strcmp( argv[0], "ping") == 0) {
		lo_send(addr, "/ping", "");
	} else {
		fprintf(stderr, "Unknown command '%s'.\n", argv[0]);
		usage();
	}
	
	
    lo_address_free( addr );

    return 0;
}

