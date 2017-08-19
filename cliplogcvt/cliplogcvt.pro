TEMPLATE = app
CONFIG += console
CONFIG += link_pkgconfig
PKGCONFIG += ogg vorbis vorbisenc libresample
QT -= gui

SOURCES = cliplogcvt.cpp
