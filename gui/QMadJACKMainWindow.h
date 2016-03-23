/*

	QMadJACKMainWindow.h
	
	QT Interface to MadJACK
	Copyright (C) 2006-2015  Nicholas J. Humfrey

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

