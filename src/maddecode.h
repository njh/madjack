/*

	maddecode.h
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



