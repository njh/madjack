/*

	QMadJACK.cpp
	
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

#include "QMadJACK.h"


QMadJACK::QMadJACK(lo_address address)
{
	Q_ASSERT( address != NULL);
	this->addr = address;
	init();
}

QMadJACK::QMadJACK(short port)
{
	Q_ASSERT( port != 0 );
	QString strport = QString::number( port );
	this->addr = lo_address_new( "localhost", (const char*)strport.toLatin1() );
	init();
}

QMadJACK::QMadJACK( const QString &string )
{
	Q_ASSERT( string != NULL);
	
	if (string.contains( QRegExp("^\\d*$") )) {
		// Port number
		this->addr = lo_address_new( "localhost", (const char*)string.toLatin1() );
	} else {
		// URL
		this->addr = lo_address_new_from_url( string.toLatin1() );
	}
	
	init();
}

QMadJACK::~QMadJACK()
{
	if (this->addr) lo_address_free( this->addr );
	if (this->serv) lo_server_free( this->serv );
}






void QMadJACK::init()
{
	// Check that the lo_address is valid
	Q_ASSERT( this->addr != NULL);
	
	// Initialise the lo_server
	this->serv = lo_server_new_with_proto(
					NULL, 
					lo_address_get_protocol(this->addr),
					QMadJACK::err_hander);
	Q_ASSERT( this->serv != NULL);	
	
	
	lo_server_add_method( serv, "/deck/state", "s", QMadJACK::state_handler, this);
	lo_server_add_method( serv, "/deck/cuepoint", "f", QMadJACK::cuepoint_handler, this);
	lo_server_add_method( serv, "/deck/duration", "f", QMadJACK::duration_handler, this);
	lo_server_add_method( serv, "/deck/position", "f", QMadJACK::position_handler, this);
	lo_server_add_method( serv, "/deck/filepath", "s", QMadJACK::filepath_handler, this);
	lo_server_add_method( serv, "/version", "ss", QMadJACK::version_handler, this);
	lo_server_add_method( serv, "/error", "s", QMadJACK::error_handler, this);
	lo_server_add_method( serv, "/pong", "", QMadJACK::pong_handler, this);


	// Initialse variables used in by reply handlers
	reply_state = QString::null;
	reply_version = QString::null;
	reply_error = QString::null;
	reply_filepath = QString::null;
	reply_duration = 0.0f;
	reply_position = 0.0f;
	reply_cuepoint = 0.0f;
	reply_pong = 0;
	
	
	// Triggers every 100 msec
	first_timer = new QTimer( this );
	first_timer->setInterval( 100 );
	connect(first_timer, SIGNAL(timeout()), this, SLOT(update_position()));
	
	// Triggers every second
	second_timer = new QTimer( this );
	second_timer->setInterval( 1000 );
	connect(second_timer, SIGNAL(timeout()), this, SLOT(update_state()));
	connect(second_timer, SIGNAL(timeout()), this, SLOT(update_duration()));
	connect(second_timer, SIGNAL(timeout()), this, SLOT(update_filepath()));

}


void QMadJACK::set_autoupdate( bool autoupdate )
{
	if (autoupdate) {
		first_timer->start();
		second_timer->start();
		
		// Force an update straight away
		update_state();
		update_position();
		update_duration();
		update_filepath();
		
	} else {
		first_timer->stop();
		second_timer->stop();
	}
}


int QMadJACK::load( const QString &filepath )
{
	QStringList desired;
	desired << "LOADING" << "READY";

	lo_message mesg = lo_message_new();
	lo_message_add_string( mesg, filepath.toLatin1() );
	int result = this->send( "/deck/load", desired, mesg );
	lo_message_free( mesg );
	
	return result;
}

int QMadJACK::play()
{
	QStringList desired( "PLAYING" );
	return this->send( "/deck/play", desired );
}

int QMadJACK::pause()
{
	QStringList desired( "PAUSED" );
	return this->send( "/deck/pause", desired );
}

int QMadJACK::stop()
{
	QStringList desired( "STOPPED" );
	return this->send( "/deck/stop", desired );
}

int QMadJACK::rewind()
{
	QStringList desired;
	desired << "LOADING" << "READY";
	return this->send( "/deck/cue", desired );
}

int QMadJACK::cue( float cuepoint )
{
	QStringList desired;
	desired << "LOADING" << "READY";
	
	lo_message mesg = lo_message_new();
	lo_message_add_float( mesg, cuepoint );
	int result = this->send( "/deck/cue", desired, mesg );
	lo_message_free( mesg );
	
	return result;
}

int QMadJACK::eject()
{
	QStringList desired( "EMPTY" );
	return this->send( "/deck/eject", desired );
}



QString QMadJACK::get_state()
{
	update_state();
	return this->reply_state;
}

void QMadJACK::update_state()
{
	this->reply_state = "UNKNOWN";
	if (this->wait_reply( "/deck/get_state" )) {
		emit stateChanged(this->reply_state);
	}
}

float QMadJACK::get_duration()
{
	update_duration();
	return this->reply_duration;
}

void QMadJACK::update_duration()
{
	this->reply_duration = 0.0f;
	if (this->wait_reply( "/deck/get_duration" )) {
		emit durationChanged(this->reply_duration);
	}
}

float QMadJACK::get_position()
{
	update_position();
	return this->reply_position;
}

void QMadJACK::update_position()
{
	this->reply_position = 0.0f;
	if (this->wait_reply( "/deck/get_position" )) {
		emit positionChanged(this->reply_position);
	}
}

QString QMadJACK::get_filepath()
{
	update_filepath();
	return this->reply_filepath;
}

void QMadJACK::update_filepath()
{
	this->reply_filepath = QString::null;
	if (this->wait_reply( "/deck/get_filepath" )) {
		emit filepathChanged(this->reply_filepath);
	}
}

QString QMadJACK::get_version()
{
	this->reply_version = QString::null;
	this->wait_reply( "/get_version" );
	return this->reply_version;
}

QString QMadJACK::get_error()
{
	this->reply_error = QString::null;
	this->wait_reply( "/get_error" );
	return this->reply_error;
}

QString QMadJACK::get_url()
{
	return lo_address_get_url(addr);
}

int QMadJACK::ping()
{
	this->reply_pong = 0;
	this->wait_reply( "/ping" );
	return this->reply_pong;
}





int QMadJACK::send_message( const char* path, lo_message mesg )
{
	int result = 0;

	if (mesg==NULL) {
		lo_message empty = lo_message_new();
		result = lo_send_message_from( addr, serv, path, empty );
		lo_message_free( empty );
	} else {
		result = lo_send_message_from( addr, serv, path, mesg );
	}
	
	if (result<1) {
		qWarning("Warning: sending message failed (%s): %s\n",
					path, lo_address_errstr(addr));
	}
	
	return result;
}


int QMadJACK::send( const QString &path,
				    QStringList &desired,
				    lo_message mesg )
{
	
	for(int i=0; i<QMADJACK_ATTEMPTS; i++) {
	
		this->send_message( path.toLatin1(), mesg );
		
		if (desired.size()) {
			QString state = this->get_state();
			
			// In error state
			if (state == "ERROR") return 0;
			
			for (int j=0; j < desired.size(); ++j) {
				if (desired.at(j) == state) return 1;
			}

		} else {
			// Success
			return 1;
		}
	}

	// Failure
	return 0;
}


int QMadJACK::wait_reply( const QString &path )
{
	int bytes = 0;
	
	// Throw away old incoming messages
	for(int i=0; i<QMADJACK_ATTEMPTS; i++) {
		lo_server_recv_noblock( this->serv, 0 );
	}
	
	// Try a few times
	for(int i=0; i<QMADJACK_ATTEMPTS; i++) {
	
		// Send Query
		int result = this->send_message( path.toLatin1() );
		if (result<1) { sleep(1); continue; }
		
		// Wait for a reply with one second
		bytes = lo_server_recv_noblock( this->serv, 1000 );
		if (bytes<1) {
			qWarning( "Timed out waiting for reply after one second." );
		} else {
			break;
		}
	}
	
	if (bytes<1) {
		qWarning( "Failed to get reply from MadJACK server after %d attempts.", QMADJACK_ATTEMPTS);
	}

	return bytes;
}







void QMadJACK::err_hander( int num, const char *msg, const char *where )
{
	qCritical( "liblo error (%d): %s %s", num, msg, where );
	
}


int QMadJACK::state_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_state = &argv[0]->s;
    return 0;
}

int QMadJACK::cuepoint_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_cuepoint = argv[0]->f;
    return 0;
}

int QMadJACK::duration_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_duration = argv[0]->f;
    return 0;
}

int QMadJACK::position_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_position = argv[0]->f;
    return 0;
}

int QMadJACK::filepath_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_filepath = &argv[0]->s;
    return 0;
}

int QMadJACK::version_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_version = &argv[0]->s;
	obj->reply_version += '/';
	obj->reply_version += &argv[1]->s;
    return 0;
}

int QMadJACK::error_handler(const char *, const char *, 
		lo_arg **argv, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_error = &argv[0]->s;
    return 0;
}

int QMadJACK::pong_handler(const char *, const char *, 
		lo_arg **, int, lo_message, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_pong++;
    return 0;
}
