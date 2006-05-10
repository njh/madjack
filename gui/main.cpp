/*

	main.cpp

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


#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QtGlobal>
#include <QtDebug>

#include "QMadJACKWidget.h"
#include "QMadJACK.h"


class MainWindow : public QWidget
{
	public:
		MainWindow(QMadJACK *madjack, QWidget *parent = 0);
};

MainWindow::MainWindow(QMadJACK *madjack, QWidget *parent)
		: QWidget(parent)
{
	QLabel *label = new QLabel("URL goes here");
	QMadJACKWidget *madjackw = new QMadJACKWidget(madjack);
	QPushButton *quit = new QPushButton("Quit");
	
	
	connect(quit, SIGNAL(clicked()), qApp, SLOT(quit()));
//	connect(slider, SIGNAL(valueChanged(int)),
//	lcd, SLOT(display(int)));
	
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(label);
	layout->addWidget(madjackw);
	layout->addWidget(quit);
	setLayout(layout);
}
    
    
    
    
    
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
 	QMadJACK madjack(4444);
 	
	MainWindow w( &madjack );
    w.show();
    
    qDebug( "Starting main loop" );
    
    return app.exec();
}



/*

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextStream cout(stdout, QIODevice::WriteOnly);


	cout << "URL of madjack server: " << madjack.get_url() << endl;
	cout << "MadJACK server version: " << madjack.get_version() << endl;
	
	madjack.load( "/Users/njh/Music/002944.mp3" );
	madjack.set_cuepoint( 10.2 );
	madjack.cue();
	sleep(1);

	cout << "Filename: " << madjack.get_filename() << endl;
	cout << "Filepath: " << madjack.get_filepath() << endl;
	
	cout << "Deck State: " << madjack.get_state() << endl;
	cout << "Cuepoint: " << madjack.get_cuepoint() << endl;
	cout << "Duration: " << madjack.get_duration() << endl;
	cout << "Position: " << madjack.get_position() << endl;

	if (!madjack.play()) cout << "Failed to play" << endl;
	sleep(3);
	
	if (!madjack.pause()) cout << "Failed to pause" << endl;
	sleep(1);
	if (!madjack.stop()) cout << "Failed to stop" << endl;
	sleep(1);
	if (!madjack.eject()) cout << "Failed to eject" << endl;


	cout << "Ping: " << madjack.ping() << endl;

	if (!madjack.load( "/doesnt exist" )) cout << "Failed to load" << endl;
	cout << "Deck State: " << madjack.get_state() << endl;
	cout << "Deck Error: " << madjack.get_error() << endl;
	
	return 0;
	
}
*/


