/*

	maddecode.h
	Multifunctional Audio Deck for the jack audio connection kit
	Copyright (C) 2005-2016  Nicholas J. Humfrey

*/



#ifndef _MADDECODE_H_
#define _MADDECODE_H_


// Constants
#define ID3v2_HEADER_LEN	(10)
#define ID3v2_FOOTER_LEN	(10)
#define ID3v1_HEADER_LEN	(3)


// Gobals
extern int is_decoding;


// Prototypes
void start_decoder_thread(void *input, float cuepoint);
void finish_decoder_thread();

#endif



