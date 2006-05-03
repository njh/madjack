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


#include <QMadJACK.h>
#include <QDebug>


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
		
	this->addr = lo_address_new( "localhost", strport.toLatin1() );
	
	init();
}

QMadJACK::QMadJACK( const QString &url )
{
	Q_ASSERT( url != NULL);
	this->addr = lo_address_new_from_url( url.toLatin1() );
	init();
}


QMadJACK::~QMadJACK()
{
	qDebug( "Cleaning up." );
	
	if (this->addr) lo_address_free( this->addr );
	if (this->serv) lo_server_free( this->serv );
}




void QMadJACK::err_hander( int num, const char *msg, const char *where )
{
	qCritical( "liblo error (%d): %s %s", num, msg, where );
	
}


int QMadJACK::state_handler(const char *path, const char *types, 
		lo_arg **argv, int argc, lo_message msg, void *user_data)
{
	QMadJACK *obj = (QMadJACK*)user_data;
	obj->reply_state = &argv[0]->s;
    return 0;
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
				

	// Check that the address is valid
	qDebug( "got here with url: %s", lo_address_get_url(addr) );

}



int QMadJACK::load( const QString &filepath )
{

//	qDebug( "Loading: %s", (const char*)path.toLatin1() );


//	return this->send( '/deck/load', 'LOADING|READY|ERROR', 's', path);
	// Success
	return 0;
}


int QMadJACK::load()
{
	QStringList desired << "LOADING" << "READY";;
	return this->send( "/deck/load", desired );
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

int QMadJACK::cue()
{
	QStringList desired << "LOADING" << "READY";;
	return this->send( "/deck/cue", desired );
}

int QMadJACK::eject()
{
	QStringList desired( "EMPTY" );
	return this->send( "/deck/eject", desired );
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
			
			qDebug( "Got state: %s", (const char*)state.toLatin1() );
			
			if (state == "ERROR") {
				qWarning("Warning: MadJACK is in error state." );
				return 1;
			}
			
			for (int j=0; j < desired.size(); ++j) {
				qDebug( "Desired state: %s", (const char*)desired.at(j).toLatin1() );
				if (desired.at(j) == state) {
					qDebug( "yay!" );
					return 1;
				}
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


QString QMadJACK::get_state()
{
	this->reply_state = QString::null;
	this->wait_reply( "/deck/get_state" );
	return this->reply_state;
}


