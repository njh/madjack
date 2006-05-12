/*

	LCDTime.cpp
	
	QT Interface to MadJACK
	Copyright (C) 2006  Nicholas J. Humfrey
	
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

#include "LCDTime.h"

LCDTime::LCDTime(QWidget *parent)
    : QLCDNumber(parent)
{
	// MM:SS.H
	this->setNumDigits( 7 );
	this->setMode(QLCDNumber::Dec);

}


// Display time as minutes and seconds
void LCDTime::display( float time )
{
	int mins = (int)(time/60);
	int secs = (int)(time - (mins*60));
	int hsec = (int)((float)(time - (int)time) * 10); // *FIXME* round() ?
	
	QString str;
	str.sprintf( "%2.2d:%2.2d.%1.1d", mins, secs, hsec );
	
	this->QLCDNumber::display( str );
}
