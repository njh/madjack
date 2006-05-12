
QTTARGET=qmadjack

QTSOURCES = \
	main.cpp \
	LCDTime.cpp \
	QMadJACK.cpp \
	QMadJACKMainWindow.cpp

QTHEADERS = \
	LCDTime.h \
	QMadJACK.h \
	QMadJACKMainWindow.h

EXTRA_DIST = $(QTTARGET).pro $(QTSOURCES) $(QTHEADERS)



# Glue to make automake call qmake

all-local: $(QTTARGET)
	
clean-local: 
	@make -f $(QTTARGET).mak clean
	rm -f $(QTTARGET).mak

$(QTTARGET).mak: Makefile.am $(QTTARGET).pro
	$(ac_qmake) -makefile -o $(QTTARGET).mak \
		TARGET="$(QTTARGET)" \
		SOURCES="$(QTSOURCES)" \
		HEADERS="$(QTHEADERS)" \
		$(QTTARGET).pro
	
$(QTTARGET): $(QTTARGET).mak $(QTSOURCES) $(QTHEADERS)
	@make -f $(QTTARGET).mak
