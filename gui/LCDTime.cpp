/*

	LCDTime.cpp
	
	QT Interface to MadJACK
	Copyright (C) 2006-2015  Nicholas J. Humfrey

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
