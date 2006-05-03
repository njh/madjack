/*

	QMadJACK.h
	
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

#ifndef QMADJACK_H
#define QMADJACK_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <lo/lo.h>


#define QMADJACK_ATTEMPTS	(5)


class QMadJACK : public QObject
{

    Q_OBJECT
	
	public:
		QMadJACK( lo_address addr );
		QMadJACK( short port );
		QMadJACK( const QString &url );
		~QMadJACK();
		
		int load( const QString &path );
		int play();
		int pause();
		int stop();
		int cue();
		int eject();
		int ping();
		
		int set_cuepoint( float cuepoint );
		float get_cuepoint();
		float get_duration();
		float get_position();
		
		QString get_filename();
		QString get_filepath();
		QString get_version();
		
		QString get_state();
		QString get_error();
		QString get_url();


	private:
		void init();
		
		int send_message( const char* path, lo_message mesg=NULL );
		
		int send(  const QString &path,
				   QStringList &desired,
				   lo_message mesg=NULL
				);
				
		int wait_reply( const QString &path );
		
		
		// Callbacks
		static void err_hander( int num, const char *msg, const char *where );
		
		static int state_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int cuepoint_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int duration_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int position_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int filename_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int filepath_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int version_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int error_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int pong_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		
	// Private variables
	private:
		lo_address	addr;
		lo_server	serv;
		
		QString		reply_state;
		QString		reply_version;
		QString		reply_error;
		QString		reply_filename;
		QString		reply_filepath;
		float		reply_duration;
		float		reply_position;
		float		reply_cuepoint;
		int			reply_pong;
		
};


#endif