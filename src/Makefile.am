bin_PROGRAMS = madjack madjack-remote

madjack_CFLAGS = -g -Wall @JACK_CFLAGS@ @MAD_CFLAGS@ @LIBLO_CFLAGS@
madjack_LDFLAGS = -lm @JACK_LIBS@ @MAD_LIBS@ @LIBLO_LIBS@
madjack_SOURCES = \
	control.c \
	control.h \
	maddecode.c \
	maddecode.h \
	mjosc.c \
	mjosc.h \
	madjack.c \
	madjack.h

madjack_remote_CFLAGS = -g -Wall @LIBLO_CFLAGS@
madjack_remote_LDFLAGS = @LIBLO_LIBS@
madjack_remote_SOURCES = madjack-remote.c
