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


// Gobals
extern int is_decoding;


// Prototypes
void start_decoder_thread(void *input);
void finish_decoder_thread();


#ifndef mad_f_tofloat
#define mad_f_tofloat(x)	((float)  \
				 ((x) / (float) (1L << MAD_F_FRACBITS)))
#endif


#endif



