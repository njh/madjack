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
#include <QApplication>

#include "QMadJACKMainWindow.h"


QMadJACKMainWindow::QMadJACKMainWindow(  QMadJACK *in_madjack, QWidget *parent )
		: QWidget(parent)
{
	madjack = in_madjack;

	init();
}


QMadJACKMainWindow::~QMadJACKMainWindow()
{


}





void QMadJACKMainWindow::init()
{
    this->setFixedSize(350, 200);
    this->setWindowTitle( qApp->applicationName() );

//	connect(slider, SIGNAL(valueChanged(int)), lcd, SLOT(display(int)));
	
	QFont font;
	font.setPointSize(9);
	font.setBold(false);
	font.setItalic(false);
	font.setUnderline(false);
	font.setWeight(50);
	font.setStrikeOut(false);
	
	
	play = new QToolButton(this);
	play->setGeometry(10, 10, 50, 50);
	play->setFont(font);
	play->setIcon(QIcon("icons/play.png"));
	play->setIconSize(QSize(32, 32));
	play->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	play->setText("Play");
	connect(play, SIGNAL(clicked()), madjack, SLOT(play()));
	
	stop = new QToolButton(this);
	stop->setGeometry(70, 10, 50, 50);
	stop->setFont(font);
	stop->setIcon(QIcon("icons/stop.png"));
	stop->setIconSize(QSize(32, 32));
	stop->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	stop->setText("Stop");
	connect(stop, SIGNAL(clicked()), madjack, SLOT(stop()));
	
	eject = new QToolButton(this);
	eject->setGeometry(70, 70, 50, 50);
	eject->setFont(font);
	eject->setIcon(QIcon("icons/eject.png"));
	eject->setIconSize(QSize(32, 32));
	eject->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	eject->setText("Eject");
	connect(eject, SIGNAL(clicked()), madjack, SLOT(eject()));
	
	pause = new QToolButton(this);
	pause->setGeometry(10, 70, 50, 50);
	pause->setFont(font);
	pause->setIcon(QIcon("icons/pause.png"));
	pause->setIconSize(QSize(32, 32));
	pause->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	pause->setText("Pause");
	connect(pause, SIGNAL(clicked()), madjack, SLOT(pause()));

	rewind = new QToolButton(this);
	rewind->setGeometry(130, 10, 50, 50);
	rewind->setFont(font);
	rewind->setIcon(QIcon("icons/rewind.png"));
	rewind->setIconSize(QSize(32, 32));
	rewind->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	rewind->setText("Rewind");
	connect(rewind, SIGNAL(clicked()), madjack, SLOT(rewind()));




	QFont font2;
	font2.setPointSize(14);
	font2.setBold(true);
	font2.setItalic(false);
	font2.setUnderline(false);
	font2.setWeight(80);
	font2.setStrikeOut(false);

	cue = new QToolButton(this);
	cue->setGeometry(130, 70, 50, 50);
	cue->setFont(font2);
	cue->setToolButtonStyle(Qt::ToolButtonTextOnly);
	cue->setText("Cue");



	slider = new QSlider(Qt::Horizontal, this);
    slider->setGeometry(10, 125, 330, 25);
    slider->setTickPosition( QSlider::NoTicks );
	slider->setEnabled( false );
	connect(madjack, SIGNAL(positionChanged(float)), this, SLOT(updatePosition(float)));
	
    time = new LCDTime( this );
    time->setGeometry(190, 10, 150, 50);
    time->setSegmentStyle(QLCDNumber::Filled);
	time->display(0.0f);
	connect(madjack, SIGNAL(positionChanged(float)), time, SLOT(display(float)));


	QFont font3;
	font3.setPointSize(10);
	font3.setBold(true);
	font3.setItalic(false);
	font3.setUnderline(false);
	font3.setWeight(80);
	font3.setStrikeOut(false);

	url = new QLabel( this );
    url->setGeometry(10, 150, 330, 15);
	url->setFont(font3);
	url->setAlignment(Qt::AlignLeft);
	url->setText( QString("URL: ") + madjack->get_url() );

	filepath = new QLabel( this );
    filepath->setGeometry(10, 165, 330, 15);
	filepath->setFont(font3);
	filepath->setAlignment(Qt::AlignLeft);
	connect(madjack, SIGNAL(filepathChanged(QString)), this, SLOT(updateFilepath(QString)));

	duration = new QLabel( this );
    duration->setGeometry(10, 180, 330, 15);
	duration->setFont(font3);
	duration->setAlignment(Qt::AlignLeft);
	connect(madjack, SIGNAL(durationChanged(float)), this, SLOT(updateDuration(float)));
	
	QFont font4;
	font4.setPointSize(24);
	font4.setBold(true);
	font4.setItalic(false);
	font4.setUnderline(false);
	font4.setWeight(80);
	font4.setStrikeOut(false);
	state = new QLabel( this );
    state->setGeometry(190, 70, 150, 50);
	state->setFont(font4);
	state->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(madjack, SIGNAL(stateChanged(QString)), state, SLOT(setText(QString)));
	
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
void QMadJACKMainWindow::updateDuration( float newDuration )
{
	duration->setText( QString("Duration: ") + QString::number( newDuration ) );
	slider->setMinimum( 0 );
	slider->setMaximum( (int)newDuration );
}

