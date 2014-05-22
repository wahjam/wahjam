TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui

# We need the include path from pkgconfig so headers are found
CONFIG += link_pkgconfig
PKGCONFIG += ogg vorbis vorbisenc portaudio-2.0

mac {
       INCLUDEPATH += /usr/local/Cellar/portmidi/217/include
}

QMAKE_CXXFLAGS += -Wno-write-strings
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
