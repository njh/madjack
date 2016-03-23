/*

	QMadJACK.h
	
	QT Interface to MadJACK
	Copyright (C) 2006-2015  Nicholas J. Humfrey

*/

#ifndef QMADJACK_H
#define QMADJACK_H

#include <QObject>
#include <QString>
#include <QTimer>
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
		
		
		void set_autoupdate( bool autoupdate );
		
		float get_duration();
		float get_position();
		
		QString get_filepath();
		QString get_version();
		
		QString get_state();
		QString get_error();
		QString get_url();

		
	public slots:
		int load( const QString &path );
		int play();
		int pause();
		int stop();
		int cue( float cuepoint = 0.0f );
		int eject();
		int ping();

	signals:
		void stateChanged(QString newState);
		void filepathChanged(QString newFilepath);
		void durationChanged(float newDuration);
		void positionChanged(float newPosition);
        
        
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

		static int duration_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int position_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int filepath_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int version_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int error_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

		static int pong_handler(const char *path, const char *types, 
			lo_arg **argv, int argc, lo_message msg, void *user_data);

	private slots:
		void update_state();	// called by timer
		void update_position();	// called by timer
		void update_duration();	// called by timer
		void update_filepath();	// called by timer
		
		
	// Private variables
	private:
		QTimer		*first_timer;		// Triggers every 100 msec
		QTimer		*second_timer;		// Triggers every second
	
		lo_address	addr;
		lo_server	serv;
		
		QString		reply_state;
		QString		reply_version;
		QString		reply_error;
		QString		reply_filepath;
		float		reply_duration;
		float		reply_position;
		int			reply_pong;
		
};


#endif
