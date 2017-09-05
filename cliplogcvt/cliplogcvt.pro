TEMPLATE = app
CONFIG += console
CONFIG += link_pkgconfig
PKGCONFIG += ogg vorbis vorbisenc

# On Debian stretch the libresample1-dev pkgconfig file is missing the actual
# library!  Add it manually...
LIBS += -lresample

QT -= gui

SOURCES = cliplogcvt.cpp
