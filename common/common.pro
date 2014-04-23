TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui

# We need the include path from pkgconfig so headers are found
CONFIG += link_pkgconfig
PKGCONFIG += ogg vorbis vorbisenc portaudio-2.0

QMAKE_CXXFLAGS += -Wno-write-strings
win32:DEFINES -= UNICODE
SOURCES = audiostream_pa.cpp \
          mpb.cpp \
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
