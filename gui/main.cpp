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


#include <QCoreApplication>
#include <QtGlobal>
#include <QtDebug>

#include "QMadJACK.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream cout(stdout, QIODevice::WriteOnly);
	QMadJACK madjack(4444);
	
	app.setApplicationName( "QMadJack Test" );


	//qDebug("URL of madjack server: %s", madjack->get_url() );
	//qDebug("MadJACK server version: %s", madjack->get_version() );
	
	
	
	//madjack.load( "/Users/njh/Music/003524.mp3" );
	//madjack.set_cuepoint( 10.2 );
	//madjack.cue();
	
	
	
	cout << "state: " << madjack.get_state() << endl;
	madjack.play();
	
	//print "Cuepoint: ".$madjack->get_cuepoint()."\n";
	//print "Duration: ".$madjack->get_duration()."\n";
	//print "Position: ".$madjack->get_position()."\n";

	
	return 0;
	
}
