/*

	QMadJACKMainWindow.cpp
	
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

#include <QDebug>
#include <QFont>
#include <QInputDialog>
#include <QApplication>

#include "QMadJACKMainWindow.h"


QMadJACKMainWindow::QMadJACKMainWindow(  QMadJACK *in_madjack, QWidget *parent )
		: QWidget(parent)
{
	Q_ASSERT( in_madjack != NULL );
	madjack = in_madjack;
	init();
}


QMadJACKMainWindow::~QMadJACKMainWindow()
{

	delete slider;
	delete time;
	delete url;
	delete filepath;
	delete duration;
	delete state;
		
	delete play;
	delete pause;
	delete stop;
	delete eject;
	delete rewind;
	delete cue;

}





void QMadJACKMainWindow::init()
{
    this->setFixedSize(350, 200);
    this->setWindowTitle( qApp->applicationName() );


	play = new QToolButton(this);
	play->setGeometry(10, 10, 50, 50);
	play->setFont(QFont("", 9, QFont::Normal, false));
	play->setIcon(QIcon("icons/play.png"));
	play->setIconSize(QSize(32, 32));
	play->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	play->setText("Play");
	connect(play, SIGNAL(clicked()), madjack, SLOT(play()));
	
	stop = new QToolButton(this);
	stop->setGeometry(70, 10, 50, 50);
	stop->setFont(QFont("", 9, QFont::Normal, false));
	stop->setIcon(QIcon("icons/stop.png"));
	stop->setIconSize(QSize(32, 32));
	stop->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	stop->setText("Stop");
	connect(stop, SIGNAL(clicked()), madjack, SLOT(stop()));
	
	eject = new QToolButton(this);
	eject->setGeometry(70, 70, 50, 50);
	eject->setFont(QFont("", 9, QFont::Normal, false));
	eject->setIcon(QIcon("icons/eject.png"));
	eject->setIconSize(QSize(32, 32));
	eject->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	eject->setText("Eject");
	connect(eject, SIGNAL(clicked()), madjack, SLOT(eject()));
	
	pause = new QToolButton(this);
	pause->setGeometry(10, 70, 50, 50);
	pause->setFont(QFont("", 9, QFont::Normal, false));
	pause->setIcon(QIcon("icons/pause.png"));
	pause->setIconSize(QSize(32, 32));
	pause->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	pause->setText("Pause");
	connect(pause, SIGNAL(clicked()), madjack, SLOT(pause()));

	rewind = new QToolButton(this);
	rewind->setGeometry(130, 10, 50, 50);
	rewind->setFont(QFont("", 9, QFont::Normal, false));
	rewind->setIcon(QIcon("icons/rewind.png"));
	rewind->setIconSize(QSize(32, 32));
	rewind->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	rewind->setText("Rewind");
	connect(rewind, SIGNAL(clicked()), madjack, SLOT(cue()));

	cue = new QToolButton(this);
	cue->setGeometry(130, 70, 50, 50);
	cue->setFont(QFont("", 14, QFont::Bold, false));
	cue->setToolButtonStyle(Qt::ToolButtonTextOnly);
	cue->setText("Cue");
	connect(cue, SIGNAL(clicked()), this, SLOT(askForCuepoint()));




    time = new LCDTime( this );
    time->setGeometry(190, 10, 150, 50);
    time->setSegmentStyle(QLCDNumber::Filled);
	time->display(0.0f);
	connect(madjack, SIGNAL(positionChanged(float)), time, SLOT(display(float)));
	
	state = new QLabel( this );
    state->setGeometry(190, 70, 150, 50);
	state->setFont(QFont("", 24, QFont::Bold, false));
	state->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(madjack, SIGNAL(stateChanged(QString)), state, SLOT(setText(QString)));

	slider = new QSlider(Qt::Horizontal, this);
    slider->setGeometry(10, 125, 330, 25);
    slider->setTickPosition( QSlider::NoTicks );
	slider->setEnabled( false );
	connect(madjack, SIGNAL(positionChanged(float)), this, SLOT(updatePosition(float)));




	url = new QLabel( this );
    url->setGeometry(10, 150, 330, 15);
	url->setFont( QFont("Geneva", 10, QFont::Normal, false) );
	url->setAlignment(Qt::AlignLeft);
	url->setText( QString("URL: ") + madjack->get_url() );

	filepath = new QLabel( this );
    filepath->setGeometry(10, 165, 330, 15);
	filepath->setFont( QFont("Geneva", 10, QFont::Normal, false) );
	filepath->setAlignment(Qt::AlignLeft);
	connect(madjack, SIGNAL(filepathChanged(QString)), this, SLOT(updateFilepath(QString)));

	duration = new QLabel( this );
    duration->setGeometry(10, 180, 330, 15);
	duration->setFont( QFont("Geneva", 10, QFont::Normal, false) );
	duration->setAlignment(Qt::AlignLeft);
	connect(madjack, SIGNAL(durationChanged(float)), this, SLOT(updateDuration(float)));
	
}



// The filepath changed
void QMadJACKMainWindow::updateFilepath( QString newFilepath )
{
	filepath->setText( QString("Filepath: ") + newFilepath );
}


// The position changed (proxy for float->int)
void QMadJACKMainWindow::updatePosition( float newPosition )
{
	slider->setValue( (int)newPosition );
}


// The duration changed
void QMadJACKMainWindow::updateDuration( float time )
{
	int mins = (int)(time/60);
	int secs = (int)(time - (mins*60));
	int hsec = (int)((float)(time - (int)time) * 10); // *FIXME* round() ?
	
	QString str;
	str.sprintf( "%2.2d:%2.2d.%1.1d", mins, secs, hsec );
	duration->setText( QString("Duration: ") + str );
	
	slider->setMinimum( 0 );
	slider->setMaximum( (int)time );
}



// Ask for cue point
void QMadJACKMainWindow::askForCuepoint( )
{
	bool ok;
	double cuepoint = QInputDialog::getDouble(
		this,						// parent
		"Cue to position",			// window title
		"Cue to position (in seconds):",
		0.0f, 						// default value
		0.0f,						// minimum value
		madjack->get_duration(),	// Maximum value
		2,							// decimal places
		&ok,						// result (ok/cancel)
		Qt::Sheet );

	if (ok) madjack->cue( cuepoint );
}

