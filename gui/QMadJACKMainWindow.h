/*

	QMadJACKMainWindow.h
	
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

#ifndef QMADJACKMAINWINDOW_H
#define QMADJACKMAINWINDOW_H

#include <QWidget>
#include <QToolButton>
#include <QSlider>
#include <QLabel>

#include "LCDTime.h"
#include "QMadJACK.h"


class QMadJACKMainWindow : public QWidget
{
	Q_OBJECT;

	public:
		QMadJACKMainWindow( QMadJACK *madjack, QWidget *parent = 0);
		~QMadJACKMainWindow();
		
	public slots:
		void updateState( QString newState );
		void updateDuration( float newDuration );
		void updatePosition( float newPosition );
		void updateFilepath( QString newFilepath );
		void askForCuepoint();
		void askForFile();

	protected:
		virtual void keyPressEvent( QKeyEvent * event );

	private:
		void init();
		
		
		QMadJACK *madjack;
		
		QSlider *slider;
		LCDTime *time;
		QLabel *url;
		QLabel *filepath;
		QLabel *duration;
		QLabel *state;
		
		QToolButton *play;
		QToolButton *pause;
		QToolButton *stop;
		QToolButton *eject;
		QToolButton *cue;
		QToolButton *load;
		
};


#endif

