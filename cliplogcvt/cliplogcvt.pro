TEMPLATE = app
CONFIG += console
CONFIG += link_pkgconfig
PKGCONFIG += ogg vorbis vorbisenc
LIBS += -lresample
QT -= gui

SOURCES = cliplogcvt.cpp
