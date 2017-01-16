TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui

QMAKE_CXXFLAGS += -Wno-write-strings
CONFIG += link_pkgconfig
PKGCONFIG += ogg vorbis vorbisenc portaudio-2.0
SOURCES = mpb.cpp \
          netmsg.cpp \
          njclient.cpp \
          njmisc.cpp \
          UserPrivs.cpp
HEADERS = audiostream.h \
          mpb.h \
          netmsg.h \
          njclient.h \
          njmisc.h \
          UserPrivs.h \
          ConcurrentQueue.h
