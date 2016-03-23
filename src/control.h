/*

	control.h
	Multifunctional Audio Deck for the jack audio connection kit
	Copyright (C) 2005-2016  Nicholas J. Humfrey

*/


#ifndef _CONTROL_H_
#define _CONTROL_H_

void do_load( const char* name );
void do_cue( float cuepoint );
void do_play();
void do_pause();
void do_stop();
void do_eject();
void do_quit();

void handle_keypresses();


#endif
