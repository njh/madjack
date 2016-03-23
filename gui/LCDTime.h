/*

	LCDTime.h
	
	QT Interface to MadJACK
	Copyright (C) 2006-2015  Nicholas J. Humfrey

*/

#ifndef LCDTIME_H
#define LCDTIME_H

#include <QLCDNumber>



class LCDTime : public QLCDNumber
{
    Q_OBJECT

	public:
		LCDTime(QWidget *parent = 0);

	public slots:
		virtual void display( float time );


};

#endif
