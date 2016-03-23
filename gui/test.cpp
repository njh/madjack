/*

	main.cpp

	QT Interface to MadJACK
	Copyright (C) 2006-2015  Nicholas J. Humfrey

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


	cout << "URL of madjack server: " << madjack.get_url() << endl;
	cout << "MadJACK server version: " << madjack.get_version() << endl;
	
	madjack.load( "/Users/njh/Music/002944.mp3" );
	madjack.set_cuepoint( 10.2 );
	madjack.cue();
	sleep(1);

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
