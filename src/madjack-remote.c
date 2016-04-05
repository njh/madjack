/*

	Multifunctional Audio Deck for the jack audio connection kit
	Copyright (C) 2005-2016  Nicholas J. Humfrey

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
	printf("  cue [<cuepoint>]  Cue deck (option cuepoint in seconds)\n");
	printf("  eject             Eject the current track from deck\n");
	printf("  load <filepath>   Load <filepath> into deck\n");
	exit(1);
}



int main(int argc, char *argv[])
{
	char *port = NULL;
	char *url = NULL;
	lo_address addr = NULL;
	int result = -1;
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


	// Check to see what message to send
	if (strcmp( argv[0], "play") == 0) {
		result = lo_send(addr, "/deck/play", "");
	} else if (strcmp( argv[0], "pause") == 0) {
		result = lo_send(addr, "/deck/pause", "");
	} else if (strcmp( argv[0], "stop") == 0) {
		result = lo_send(addr, "/deck/stop", "");
	} else if (strcmp( argv[0], "cue") == 0) {
		// Check for argument
		if (argc>=2) {
			float cuepoint = atof( argv[1] );
			result = lo_send(addr, "/deck/cue", "f", cuepoint);
		} else {
			result = lo_send(addr, "/deck/cue", "");
		}
	} else if (strcmp( argv[0], "eject") == 0) {
		result = lo_send(addr, "/deck/eject", "");
	} else if (strcmp( argv[0], "load") == 0) {
		// Check for argument
		if (argc!=2) usage( );
		result = lo_send(addr, "/deck/load", "s", argv[1]);
	} else {
		fprintf(stderr, "Unknown command '%s'.\n", argv[0]);
		usage();
	}

  lo_address_free( addr );
	
	if (result<1) {
		fprintf(stderr, "Error: failed to send OSC message (error=%d).\n", result);
		return -2;
	} else {
		// Success
		return 0;
	}
}


