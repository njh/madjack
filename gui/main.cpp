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
#include <QtGlobal>
#include <QMenuBar>
#include <QAction>
#include <QtDebug>

#include "QMadJACKMainWindow.h"
#include "QMadJACK.h"




QMenuBar* setupMenuBar( QWidget *window )
{
	// Create menubar
	QMenuBar *menubar = new QMenuBar( window );
	QMenu *helpMenu = menubar->addMenu("&Help");
	
	QAction *aboutQtAct = new QAction("About &Qt", window);
	aboutQtAct->setStatusTip("Show the Qt library's About box");
	QObject::connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	helpMenu->addAction(aboutQtAct);
	
	return menubar;
}
    
    
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
    app.setApplicationName( "QMadJACK" );
    
    // Check the commandline arguments
    if (app.arguments().size() < 2) {
    	qFatal("Missing command line argument.");
    }
    
    // Create QMadJACK object
	QMadJACK madjack( app.arguments().at(1) );
	qDebug( "url: %s", (const char*)madjack.get_url().toLatin1() );
	
	// Create main window
	QMadJACKMainWindow window( &madjack );
	setupMenuBar( &window );
	window.show();
	
	// Start updating automatically
	madjack.set_autoupdate( true );
	
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


