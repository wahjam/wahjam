TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui

# For third-party library headers from Homebrew
mac {
       INCLUDEPATH += /usr/local/include
}

QMAKE_CXXFLAGS += -Wno-write-strings
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
