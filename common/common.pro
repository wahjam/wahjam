TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui
QMAKE_CXXFLAGS += -Wno-write-strings
win32:DEFINES -= UNICODE
SOURCES = audiostream_pa.cpp \
          mpb.cpp \
          netmsg.cpp \
          njclient.cpp \
          njmisc.cpp
HEADERS = audiostream.h \
          mpb.h \
          netmsg.h \
          njclient.h \
          njmisc.h
