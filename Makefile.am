AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -g -Wall @JACK_CFLAGS@ @MAD_CFLAGS@ @LIBLO_CFLAGS@
AM_LDFLAGS = -lm @JACK_LIBS@ @MAD_LIBS@ @LIBLO_LIBS@

bin_PROGRAMS = madjack
madjack_SOURCES = \
	control.c \
	control.h \
	maddecode.c \
	maddecode.h \
	madjack.c \
	madjack.h