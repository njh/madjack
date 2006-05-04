TEMPLATE = app
TARGET = qmadjack
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += qt debug warn_on link_pkgconfig
PKGCONFIG += liblo

# Input
HEADERS += QMadJACKWidget.h QMadJACK.h
SOURCES += main.cpp QMadJACKWidget.cpp QMadJACK.cpp

