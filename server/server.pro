######################################################################
# Automatically generated by qmake (2.01a) Sun Apr 8 19:07:53 2012
######################################################################

# Version number in <major>.<minor>.<patch> form
VERSION = 0.6.0
DEFINES += VERSION=\'\"$$VERSION\"\'
DEFINES += COMMIT_ID=\'\"$$system(git rev-parse HEAD)\"\'

TEMPLATE = app
TARGET = wahjamsrv
DEPENDPATH += ..
INCLUDEPATH += ..
QT -= gui
QT += network xml

include(../common/libcommon.pri)

# Code in common/ does not use wide characters
win32:DEFINES -= UNICODE

# Build console application
win32:CONFIG += console

unix {
    HEADERS += SignalHandler.h
    SOURCES += SignalHandler.cpp
}

# Input
HEADERS += usercon.h \
           Server.h \
           logging.h \
           JammrUserLookup.h \
           ninjamsrv.h \
           ../WDL/queue.h \
           ../WDL/heapbuf.h \
           ../WDL/string.h \
           ../WDL/ptrlist.h \
           ../WDL/lineparse.h
SOURCES += ninjamsrv.cpp \
           logging.cpp \
           usercon.cpp \
           Server.cpp \
           JammrUserLookup.cpp
