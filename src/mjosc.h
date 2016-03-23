/*

	mjosc.h
	Multifunctional Audio Deck for the jack audio connection kit
	Copyright (C) 2005-2016  Nicholas J. Humfrey

*/


#include "madjack.h"
#include <lo/lo.h>

#ifndef _MADJACK_OSC_H_
#define _MADJACK_OSC_H_

// Prototypes
lo_server_thread init_osc( char *port );
void finish_osc( lo_server_thread st );

#endif
